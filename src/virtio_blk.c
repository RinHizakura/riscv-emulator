#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "memmap.h"
#include "virtio_blk.h"

bool init_virtio_blk(riscv_virtio_blk *virtio_blk, const char *rfs_name)
{
    memset(virtio_blk, 0, sizeof(riscv_virtio_blk));

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
                         uint64_t size,
                         riscv_exception *exc)
{
    uint32_t offset = addr - VIRTIO_BASE;

    // support only byte size access for VIRTIO_MMIO_CONFIG
    if (offset >= VIRTIO_MMIO_CONFIG) {
        int index = offset - VIRTIO_MMIO_CONFIG;
        if (index >= 8)
            goto read_virtio_fail;
        return virtio_blk->config[index];
    }

    // support only word-aligned and word size access for other registers
    if ((size != 32) || !(addr & 0x3))
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
        assert(virtio_blk->device_features_sel < 2);
        return virtio_blk->device_features[virtio_blk->device_features_sel];
    case VIRTIO_MMIO_QUEUE_NUM_MAX:
        return VIRTQUEUE_MAX_SIZE;
    case VIRTIO_MMIO_QUEUE_PFN:
        return virtio_blk->queue_pfn;
    case VIRTIO_MMIO_QUEUE_READY:
        return virtio_blk->queue_ready;
    case VIRTIO_MMIO_INTERRUPT_STATUS:
        return virtio_blk->interrupt_status;
    case VIRTIO_MMIO_STATUS:
        return virtio_blk->status;
    case VIRTIO_MMIO_CONFIG_GENERATION:
        return virtio_blk->config_generation;
    default:
        goto read_virtio_fail;
    }

read_virtio_fail:
    exc->exception = LoadAccessFault;
    return -1;
}

bool write_virtio_blk(riscv_virtio_blk *virtio_blk,
                      uint64_t addr,
                      uint8_t size,
                      uint64_t value,
                      riscv_exception *exc)
{
}

void free_virtio_blk(riscv_virtio_blk *virtio_blk)
{
    free(virtio_blk->rfsimg);
}
