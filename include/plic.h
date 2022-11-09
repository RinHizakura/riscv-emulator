#ifndef RISCV_PLIC
#define RISCV_PLIC

/* The PLIC multiplexes various device interrupts onto the external interrupt
 * lines of Hart contexts, with hardware support for interrupt priorities.  It's
 * the global interrupt controller in a RISC-V system. */

/* We can see the qemu implementation of plic:
 * - https://github.com/qemu/qemu/blob/master/hw/intc/sifive_plic.c
 * - https://github.com/qemu/qemu/blob/master/include/hw/intc/sifive_plic.h
 */

#include <stdbool.h>
#include <stdint.h>

#include "csr.h"
#include "exception.h"
#include "memmap.h"

#define PLIC_PRIORITY PLIC_BASE
// 4096 bytes / 4 bytes per source represent 1024 sources
#define PLIC_PRIORITY_END (PLIC_PRIORITY + 0x1000)

#define PLIC_PENDING (PLIC_BASE + 0x1000)
// 128 bytes / 1 bit per source represent 1024 sources
#define PLIC_PENDING_END (PLIC_PENDING + 0x80)

#define PLIC_ENABLE (PLIC_BASE + 0x2000)
/*  PLIC has 15872 Interrupt Enable blocks for the contexts. The context is
 * referred to the specific privilege mode in the specific Hart of specific
 * RISC-V processor instance. We will use two context now in the emulator */
// 256 bytes / 1 bit per source represent 1024 sources in 2 contexts
#define PLIC_ENABLE_END (PLIC_ENABLE + 0x100)

#define PLIC_THRESHOLD_0 (PLIC_BASE + 0x200000)
#define PLIC_CLAIM_0 (PLIC_BASE + 0x200004)
#define PLIC_THRESHOLD_1 (PLIC_BASE + 0x201000)
#define PLIC_CLAIM_1 (PLIC_BASE + 0x201004)

typedef struct {
    uint32_t priority[1024];
    uint32_t pending[32];
    uint32_t enable[32 * 2];
    uint32_t threshold[2];
    uint32_t claim[2];

    bool update_irq;
} riscv_plic;

uint64_t read_plic(riscv_plic *plic,
                   uint64_t addr,
                   uint8_t size,
                   riscv_exception *exc);

bool write_plic(riscv_plic *plic,
                uint64_t addr,
                uint8_t size,
                uint64_t value,
                riscv_exception *exc);
void tick_plic(riscv_plic *plic,
               riscv_csr *csr,
               bool is_uart_irq,
               bool is_virtio_irq);
#endif
