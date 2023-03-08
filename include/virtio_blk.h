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

/* The design base on 4.2.4 Legacy interface, and 4.2.2 MMIO Device Register
 * Layout could be supported as future work */
#define VIRTIO_MMIO_MAGIC_VALUE 0x0
#define VIRTIO_MMIO_VERSION 0x4
#define VIRTIO_MMIO_DEVICE_ID 0x8
#define VIRTIO_MMIO_VENDOR_ID 0xc
#define VIRTIO_MMIO_DEVICE_FEATURES 0x10
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x14
#define VIRTIO_MMIO_DRIVER_FEATURES 0x20
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x24
#define VIRTIO_MMIO_GUEST_PAGE_SIZE 0x28
#define VIRTIO_MMIO_QUEUE_SEL 0x30
#define VIRTIO_MMIO_QUEUE_NUM_MAX 0x34
#define VIRTIO_MMIO_QUEUE_NUM 0x38
#define VIRTIO_MMIO_QUEUE_ALIGN 0x3c
#define VIRTIO_MMIO_QUEUE_PFN 0x40
#define VIRTIO_MMIO_QUEUE_NOTIFY 0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS 0x60
#define VIRTIO_MMIO_INTERRUPT_ACK 0x064
#define VIRTIO_MMIO_STATUS 0x070
#define VIRTIO_MMIO_CONFIG 0x100

#define VIRT_MAGIC 0x74726976
#define VIRT_VERSION_LEGACY 1
#define VIRT_VENDOR 0x554D4551
#define VIRT_BLK_DEV 0x02

#define VIRTQUEUE_MAX_SIZE 1024
#define VIRTQUEUE_ALIGN 4096

#define DISK_DELAY 500
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
    uint64_t clock;
    uint64_t notify_clock;

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
bool virtio_is_interrupted(riscv_virtio_blk *virtio_blk);
void tick_virtio_blk(riscv_virtio_blk *virtio_blk);
void free_virtio_blk(riscv_virtio_blk *virtio_blk);

#endif
