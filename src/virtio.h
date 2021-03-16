#ifndef RISCV_VIRTIO
#define RISCV_VIRTIO

#include <stdbool.h>
#include <stdint.h>

#include "exception.h"

typedef struct {
    uint8_t *rfsimg;
} riscv_virtio;

bool init_virtio(riscv_virtio *virtio, const char *rfs_name);
uint64_t read_virtio(riscv_virtio *virtio,
                     uint64_t addr,
                     uint64_t size,
                     riscv_exception *exc);
bool write_virtio(riscv_virtio *virtio,
                  uint64_t addr,
                  uint8_t size,
                  uint64_t value,
                  riscv_exception *exc);
void free_virtio(riscv_virtio *virtio);

#endif
