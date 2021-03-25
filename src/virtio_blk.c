#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "memmap.h"
#include "virtio_blk.h"

#define ALIGN_UP(n, m) ((((n) + (m) -1) / (m)) * (m))


static void reset_virtio_blk(riscv_virtio_blk *virtio_blk)
{
    /* FIXME: Can't find the content of reset sequence in document...... */
    virtio_blk->queue_sel = 0;
    virtio_blk->guest_features = 0;
    virtio_blk->status = 0;
    virtio_blk->isr = 0;

    virtio_blk->vq[0].desc = 0;
    virtio_blk->vq[0].avail = 0;
    virtio_blk->vq[0].used = 0;
}

bool init_virtio_blk(riscv_virtio_blk *virtio_blk, const char *rfs_name)
{
    memset(virtio_blk, 0, sizeof(riscv_virtio_blk));
    // notify is set to -1 for no event happen
    virtio_blk->queue_notify = 0xFFFFFFFF;
    // default the align of virtqueue to 4096
    virtio_blk->vq[0].align = VIRTQUEUE_ALIGN;

    if (rfs_name[0] == '\0') {
        virtio_blk->rfsimg = NULL;
        return true;
    }

    FILE *fp = fopen(rfs_name, "rb");
    if (!fp) {
        LOG_ERROR("Invalid root filesystem path.\n");
        return false;
    }

    fseek(fp, 0, SEEK_END);
    size_t sz = ftell(fp) * sizeof(uint8_t);
    rewind(fp);

    virtio_blk->rfsimg = malloc(sz);
    size_t read_size = fread(virtio_blk->rfsimg, sizeof(uint8_t), sz, fp);

    if (read_size != sz) {
        LOG_ERROR("Error when reading binary through fread.\n");
        fclose(fp);
        return false;
    }
    fclose(fp);

    return true;
}

uint64_t read_virtio_blk(riscv_virtio_blk *virtio_blk,
                         uint64_t addr,
                         uint8_t size,
                         riscv_exception *exc)
{
    uint64_t offset = addr - VIRTIO_BASE;

    // support only byte size access for VIRTIO_MMIO_CONFIG
    if (offset >= VIRTIO_MMIO_CONFIG) {
        int index = offset - VIRTIO_MMIO_CONFIG;
        if (index >= 8)
            goto read_virtio_fail;
        return virtio_blk->config[index];
    }

    // support only word-aligned and word size access for other registers
    if ((size != 32) || (addr & 0x3))
        goto read_virtio_fail;

    switch (offset) {
    case VIRTIO_MMIO_MAGIC_VALUE:
        return VIRT_MAGIC;
    case VIRTIO_MMIO_VERSION:
        return VIRT_VERSION_LEGACY;
    case VIRTIO_MMIO_DEVICE_ID:
        return VIRT_BLK_DEV;
    case VIRTIO_MMIO_VENDOR_ID:
        return VIRT_VENDOR;
    case VIRTIO_MMIO_DEVICE_FEATURES:
        return virtio_blk->host_features_sel ? 0 : virtio_blk->host_features;
    case VIRTIO_MMIO_QUEUE_NUM_MAX:
        return VIRTQUEUE_MAX_SIZE;
    case VIRTIO_MMIO_QUEUE_PFN:
        assert(virtio_blk->queue_sel == 0);
        return virtio_blk->vq[0].desc >> virtio_blk->guest_page_shift;
    case VIRTIO_MMIO_INTERRUPT_STATUS:
        return virtio_blk->isr;
    case VIRTIO_MMIO_STATUS:
        return virtio_blk->status;
    default:
        goto read_virtio_fail;
    }

read_virtio_fail:
    LOG_ERROR("read virtio addr %lx for size %d failed\n", addr, size);
    exc->exception = LoadAccessFault;
    return -1;
}

bool write_virtio_blk(riscv_virtio_blk *virtio_blk,
                      uint64_t addr,
                      uint8_t size,
                      uint64_t value,
                      riscv_exception *exc)
{
    uint64_t offset = addr - VIRTIO_BASE;

    // support only byte size access for VIRTIO_MMIO_CONFIG
    if (offset >= VIRTIO_MMIO_CONFIG) {
        int index = offset - VIRTIO_MMIO_CONFIG;
        if (index >= 8)
            goto write_virtio_fail;
        virtio_blk->config[index] = value & 0xFF;
        return true;
    }

    value &= 0xFFFFFFFF;

    // support only word-aligned and word size access for other registers
    if ((size != 32) || (addr & 0x3))
        goto write_virtio_fail;

    switch (offset) {
    case VIRTIO_MMIO_DEVICE_FEATURES_SEL:
        virtio_blk->host_features_sel = value ? 1 : 0;
        break;
    case VIRTIO_MMIO_DRIVER_FEATURES:
        if (virtio_blk->guest_features_sel)
            LOG_ERROR(
                "Attempt to write guest features with guest_features_sel > 0 "
                "in legacy mode\n");
        else
            virtio_blk->guest_features = value;
        break;
    case VIRTIO_MMIO_DRIVER_FEATURES_SEL:
        virtio_blk->guest_features_sel = value ? 1 : 0;
        break;
    case VIRTIO_MMIO_GUEST_PAGE_SIZE:
        assert(value != 0);
        virtio_blk->guest_page_shift = __builtin_ctz(value);
        break;
    case VIRTIO_MMIO_QUEUE_SEL:
        if (value != 0) {
            LOG_ERROR("Support only on virtio queue now \n");
            goto write_virtio_fail;
        }
        break;
    case VIRTIO_MMIO_QUEUE_NUM:
        assert(virtio_blk->queue_sel == 0);
        virtio_blk->vq[0].num = value;
        break;
    case VIRTIO_MMIO_QUEUE_ALIGN:
        assert(virtio_blk->queue_sel == 0);
        virtio_blk->vq[0].align = value;
        break;
    case VIRTIO_MMIO_QUEUE_PFN:
        assert(virtio_blk->queue_sel == 0);
        virtio_blk->vq[0].desc = value << virtio_blk->guest_page_shift;
        virtio_blk->vq[0].avail =
            virtio_blk->vq[0].desc +
            virtio_blk->vq[0].num * sizeof(riscv_virtq_desc);
        virtio_blk->vq[0].used = ALIGN_UP(
            virtio_blk->vq[0].avail +
                offsetof(riscv_virtq_avail, ring[virtio_blk->vq[0].num]),
            virtio_blk->vq[0].align);
        break;
    case VIRTIO_MMIO_QUEUE_NOTIFY:
        assert(value == 0);
        virtio_blk->queue_notify = value;
        break;
    case VIRTIO_MMIO_INTERRUPT_ACK:
        /* clear bits by given bitmask to represent that the events causing
         * the interrupt have been handled */
        virtio_blk->isr &= ~(value & 0xff);
        break;
    case VIRTIO_MMIO_STATUS:
        virtio_blk->status = value & 0xff;

        // Writing zero (0x0) to this register triggers a device reset.
        if (virtio_blk->status == 0)
            reset_virtio_blk(virtio_blk);
        /* FIXME: we may have to do something for the indicating driver
         * progress? */
        break;
    default:
        goto write_virtio_fail;
    }

    return true;

write_virtio_fail:
    LOG_ERROR("write virtio addr %lx for size %d failed\n", addr, size);
    exc->exception = StoreAMOAccessFault;
    return false;
}

bool virtio_is_interrupt(riscv_virtio_blk *virtio_blk)
{
    if (virtio_blk->queue_notify != 0xFFFFFFFF) {
        virtio_blk->queue_notify = 0xFFFFFFFF;
        return true;
    }
    return false;
}

void free_virtio_blk(riscv_virtio_blk *virtio_blk)
{
    free(virtio_blk->rfsimg);
}
