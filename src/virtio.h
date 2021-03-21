#ifndef RISCV_VIRTIO
#define RISCV_VIRTIO

/* VIRTIO is an abstraction layer over devices in a paravirtualized hypervisor.
 *
 * The implementation of VIRTIO is according to document:
 * - https://docs.oasis-open.org/virtio/virtio/v1.1/virtio-v1.1.pdf */

/* We can see the qemu implementation of virtio
 * - https://github.com/qemu/qemu/blob/master/hw/virtio/virtio-mmio.c
 * -
 * https://github.com/qemu/qemu/blob/master/include/standard-headers/linux/virtio_mmio.h
 */


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

#endif
