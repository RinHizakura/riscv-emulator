#ifndef RISCV_VIRTIO_BLK
#define RISCV_VIRTIO_BLK

/* VIRTIO is an abstraction layer over devices in a paravirtualized hypervisor.
 *
 * The implementation of VIRTIO is according to document:
 * - https://docs.oasis-open.org/virtio/virtio/v1.1/virtio-v1.1.pdf */

/* We can see the qemu implementation of virtio
 * - https://github.com/qemu/qemu/blob/master/hw/virtio/virtio-mmio.c
 * -
 * https://github.com/qemu/qemu/blob/master/include/standard-headers/linux/virtio_mmio.h
 */

#include <stdbool.h>
#include <stdint.h>

#include "exception.h"
#include "virtio.h"

typedef struct {
    uint32_t num;
    uint32_t align;

    uint64_t desc;
    uint64_t avail;
    uint64_t used;
} riscv_virtio_queue;

typedef struct {
    riscv_virtio_queue vq[1];
    uint16_t queue_sel;

    uint64_t host_features;
    uint64_t guest_features;
    uint32_t host_features_sel;
    uint32_t guest_features_sel;
    uint32_t guest_page_shift;

    uint32_t queue_notify;
    uint8_t isr;
    uint8_t status;

    uint8_t config[8];

    uint8_t *rfsimg;
} riscv_virtio_blk;

bool init_virtio_blk(riscv_virtio_blk *virtio_blk, const char *rfs_name);
uint64_t read_virtio_blk(riscv_virtio_blk *virtio_blk,
                         uint64_t addr,
                         uint64_t size,
                         riscv_exception *exc);
bool write_virtio_blk(riscv_virtio_blk *virtio_blk,
                      uint64_t addr,
                      uint8_t size,
                      uint64_t value,
                      riscv_exception *exc);
void free_virtio_blk(riscv_virtio_blk *virtio_blk);

#endif
