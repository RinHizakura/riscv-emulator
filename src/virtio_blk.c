#include <stdlib.h>
#include <string.h>

#include "virtio_blk.h"

bool init_virtio_blk(riscv_virtio_blk *virtio_blk, const char *rfs_name)
{
    memset(&virtio_blk->reg, 0, VIRTIO_REG_CNT * sizeof(uint32_t));
    memset(&virtio_blk->config, 0, 8);

    // handwire value
    virtio_blk->reg[VIRTIO_MMIO_MAGIC_VALUE] = VIRT_MAGIC;
    virtio_blk->reg[VIRTIO_MMIO_VERSION] = VIRT_VERSION_LEGACY;
    virtio_blk->reg[VIRTIO_MMIO_DEVICE_ID] = VIRT_BLK_DEV;
    virtio_blk->reg[VIRTIO_MMIO_VENDOR_ID] = VIRT_VENDOR;

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
