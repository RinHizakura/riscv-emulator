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

#define SECTOR_SIZE 512

#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1

#define VIRTQ_DESC_F_NEXT 1
#define VIRTQ_DESC_F_WRITE 2

#define VIRTIO_BLK_S_OK 0

typedef struct VRingAvail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[];
} riscv_virtq_avail;

typedef struct {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} riscv_virtq_desc;

typedef struct {
    uint32_t num;
    uint32_t align;

    uint64_t desc;
    uint64_t avail;
    uint64_t used;
} riscv_virtq;

typedef struct {
    uint64_t id;

    riscv_virtq vq[1];
    uint16_t queue_sel;

    uint32_t host_features[2];
    uint32_t guest_features[2];
    uint32_t host_features_sel;
    uint32_t guest_features_sel;
    uint32_t guest_page_shift;
    uint32_t queue_pfn;

    uint32_t queue_notify;
    uint8_t isr;
    uint8_t status;

    uint8_t config[8];

    uint8_t *rfsimg;
} riscv_virtio_blk;

bool init_virtio_blk(riscv_virtio_blk *virtio_blk, const char *rfs_name);
uint64_t read_virtio_blk(riscv_virtio_blk *virtio_blk,
                         uint64_t addr,
                         uint8_t size,
                         riscv_exception *exc);
bool write_virtio_blk(riscv_virtio_blk *virtio_blk,
                      uint64_t addr,
                      uint8_t size,
                      uint64_t value,
                      riscv_exception *exc);
bool virtio_is_interrupt(riscv_virtio_blk *virtio_blk);
void free_virtio_blk(riscv_virtio_blk *virtio_blk);

#endif
