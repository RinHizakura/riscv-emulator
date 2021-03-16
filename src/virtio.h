#ifndef RISCV_VIRTIO
#define RISCV_VIRTIO

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t *rfsimg;
} riscv_virtio;

bool init_virtio(riscv_virtio *virtio, const char *rfs_name);
void free_virtio(riscv_virtio *virtio);

#endif
