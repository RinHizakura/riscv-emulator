#include <stdlib.h>

#include "virtio.h"

bool init_virtio(riscv_virtio *virtio, const char *rfs_name)
{
    if (rfs_name[0] == '\0') {
        virtio->rfsimg = NULL;
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

    virtio->rfsimg = malloc(sz);
    size_t read_size = fread(virtio->rfsimg, sizeof(uint8_t), sz, fp);

    if (read_size != sz) {
        LOG_ERROR("Error when reading binary through fread.\n");
        fclose(fp);
        return false;
    }
    fclose(fp);

    return true;
}

uint64_t read_virtio(riscv_virtio *virtio,
                     uint64_t addr,
                     uint64_t size,
                     riscv_exception *exc)
{
}

bool write_virtio(riscv_virtio *virtio,
                  uint64_t addr,
                  uint8_t size,
                  uint64_t value,
                  riscv_exception *exc)
{
}

void free_virtio(riscv_virtio *virtio)
{
    free(virtio->rfsimg);
}
