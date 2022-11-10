#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "macros.h"
#include "memmap.h"

#define ALIGN_UP(n, m) ((((n) + (m) -1) / (m)) * (m))

static void reset_virtio_blk(riscv_virtio_blk *virtio_blk)
{
    /* FIXME: Can't find the content of reset sequence in document...... */
    virtio_blk->id = 0;
    virtio_blk->queue_sel = 0;
    virtio_blk->guest_features[0] = 0;
    virtio_blk->guest_features[1] = 0;
    virtio_blk->status = 0;
    virtio_blk->isr = 0;

    virtio_blk->vq[0].desc = 0;
    virtio_blk->vq[0].avail = 0;
    virtio_blk->vq[0].used = 0;
}

static void virtqueue_update(riscv_virtio_blk *virtio_blk)
{
    virtio_blk->vq[0].desc = virtio_blk->queue_pfn
                             << virtio_blk->guest_page_shift;
    virtio_blk->vq[0].avail = virtio_blk->vq[0].desc +
                              virtio_blk->vq[0].num * sizeof(riscv_virtq_desc);
    virtio_blk->vq[0].used =
        ALIGN_UP(virtio_blk->vq[0].avail +
                     offsetof(riscv_virtq_avail, ring[virtio_blk->vq[0].num]),
                 virtio_blk->vq[0].align);
}

static inline riscv_virtq_desc load_desc(riscv_cpu *cpu, uint64_t addr)
{
    uint64_t desc_addr = read_bus(&cpu->bus, addr, 64, &cpu->exc);
    uint64_t tmp = read_bus(&cpu->bus, addr + 8, 64, &cpu->exc);
    return (riscv_virtq_desc){.addr = desc_addr,
                              .len = tmp & 0xffffffff,
                              .flags = (tmp >> 32) & 0xffff,
                              .next = (tmp >> 48) & 0xffff};
}

// FIXME: error of read / write bus should be handled
static void access_disk(riscv_virtio_blk *virtio_blk)
{
    riscv_cpu *cpu = container_of(
        container_of(virtio_blk, riscv_bus, virtio_blk), riscv_cpu, bus);

    assert(virtio_blk->queue_sel == 0);

    uint64_t desc = virtio_blk->vq[0].desc;
    uint64_t avail = virtio_blk->vq[0].avail;
    uint64_t used = virtio_blk->vq[0].used;

    uint64_t queue_size = virtio_blk->vq[0].num;

    /* (for avail) idx field indicates where the driver would put the next
     * descriptor entry in the ring (modulo the queue size). This starts at 0,
     * and increases. */
    int idx = read_bus(&cpu->bus, avail + offsetof(riscv_virtq_avail, idx), 16,
                       &cpu->exc);
    int desc_offset =
        read_bus(&cpu->bus, avail + 4 + (idx % queue_size) * sizeof(uint16_t),
                 16, &cpu->exc);

    /* MUST use a single 8-type descriptor containing type, reserved and sector,
     * followed by descriptors for data, then finally a separate 1-byte
     * descriptor for status. */
    riscv_virtq_desc desc0 =
        load_desc(cpu, desc + sizeof(riscv_virtq_desc) * desc_offset);

    riscv_virtq_desc desc1 =
        load_desc(cpu, desc + sizeof(riscv_virtq_desc) * desc0.next);

    riscv_virtq_desc desc2 =
        load_desc(cpu, desc + sizeof(riscv_virtq_desc) * desc1.next);

    assert(desc0.flags & VIRTQ_DESC_F_NEXT);
    assert(desc1.flags & VIRTQ_DESC_F_NEXT);
    assert(!(desc2.flags & VIRTQ_DESC_F_NEXT));

    // the desc address should map to memory and we can then use memcpy directly
    assert(desc1.addr >= DRAM_BASE && desc1.addr < DRAM_END);
    assert((desc1.addr + desc1.len) >= DRAM_BASE &&
           (desc1.addr + desc1.len) < DRAM_END);

    // take value of type and sector in field of struct virtio_blk_req
    int blk_req_type = read_bus(&cpu->bus, desc0.addr, 32, &cpu->exc);
    int blk_req_sector = read_bus(&cpu->bus, desc0.addr + 8, 64, &cpu->exc);

    // write device
    if (blk_req_type == VIRTIO_BLK_T_OUT) {
        assert(!(desc1.flags & VIRTQ_DESC_F_WRITE));

        memcpy(cpu->bus.virtio_blk.rfsimg + (blk_req_sector * SECTOR_SIZE),
               cpu->bus.memory.mem + (desc1.addr - DRAM_BASE), desc1.len);
    }
    // read device
    else {
        assert(desc1.flags & VIRTQ_DESC_F_WRITE);

        memcpy(cpu->bus.memory.mem + (desc1.addr - DRAM_BASE),
               cpu->bus.virtio_blk.rfsimg + (blk_req_sector * SECTOR_SIZE),
               desc1.len);
    }

    assert(desc2.flags & VIRTQ_DESC_F_WRITE);

    /* The final status byte is written by the device: VIRTIO_BLK_S_OK for
     * success */
    write_bus(&cpu->bus, desc2.addr, 8, VIRTIO_BLK_S_OK, &cpu->exc);

    /* (for used) idx field indicates where the device would put the next
     * descriptor entry in the ring (modulo the queue size). This starts at 0,
     * and increases */

    write_bus(&cpu->bus, used + 4 + ((virtio_blk->id % queue_size) * 8), 16,
              desc_offset, &cpu->exc);
    virtio_blk->id++;
    write_bus(&cpu->bus, used + 2, 16, virtio_blk->id, &cpu->exc);
}

bool init_virtio_blk(riscv_virtio_blk *virtio_blk, const char *rfs_name)
{
    memset(virtio_blk, 0, sizeof(riscv_virtio_blk));
    // notify is set to -1 for no event happen
    virtio_blk->queue_notify = 0xFFFFFFFF;
    // default the align of virtqueue to 4096
    virtio_blk->vq[0].align = VIRTQUEUE_ALIGN;

    virtio_blk->config[1] = 0x20;
    virtio_blk->config[2] = 0x03;

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
        return virtio_blk->host_features[virtio_blk->host_features_sel];
    case VIRTIO_MMIO_QUEUE_NUM_MAX:
        return VIRTQUEUE_MAX_SIZE;
    case VIRTIO_MMIO_QUEUE_PFN:
        assert(virtio_blk->queue_sel == 0);
        return virtio_blk->queue_pfn;
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
        virtio_blk->config[index] = (value >> (index * 8)) & 0xFF;
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
        virtio_blk->guest_features[virtio_blk->guest_features_sel] = value;
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
        virtio_blk->queue_pfn = value;
        virtqueue_update(virtio_blk);
        break;
    case VIRTIO_MMIO_QUEUE_NOTIFY:
        assert(value == 0);
        virtio_blk->queue_notify = value;
        virtio_blk->notify_clock = virtio_blk->clock;
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

        if (virtio_blk->status & 0x4)
            virtqueue_update(virtio_blk);
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
    return (virtio_blk->isr & 0x1) == 1;
}

void tick_virtio_blk(riscv_virtio_blk *virtio_blk)
{
    if (virtio_blk->queue_notify != 0xFFFFFFFF &&
        virtio_blk->clock == virtio_blk->notify_clock + DISK_DELAY) {
        /* the interrupt was asserted because the device has used a buffer
         * in at least one of the active virtual queues. */
        virtio_blk->isr |= 0x1;
        access_disk(virtio_blk);
        virtio_blk->queue_notify = 0xFFFFFFFF;
    }
    virtio_blk->clock++;
}

void free_virtio_blk(riscv_virtio_blk *virtio_blk)
{
    free(virtio_blk->rfsimg);
}
