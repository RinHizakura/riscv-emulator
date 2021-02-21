#ifndef RISCV_CLINT
#define RISCV_CLINT

/* CLINT is responsible for maintaining memory mapped control and status
 * registers which are associated with the software and timer interrupts */

/* We can see the qemu implementation of clint:
 * - https://github.com/qemu/qemu/blob/master/hw/intc/sifive_clint.c
 * - https://github.com/qemu/qemu/blob/master/include/hw/intc/sifive_clint.h
 */

#include <stdbool.h>
#include <stdint.h>

#include "exception.h"
#include "memmap.h"

#define CLINT_MSIP CLINT_BASE + 0x0
#define CLINT_MTIMECMP CLINT_BASE + 0x4000
#define CLINT_MTIME CLINT_BASE + 0XBFF8

typedef struct {
    /* note that msip has only 32 bits, but we define it as uint64_t for the
     * consistent clint read and write logic */
    uint64_t msip;
    uint64_t mtimecmp;
    uint64_t mtime;
} riscv_clint;

uint64_t read_clint(riscv_clint *clint,
                    uint64_t addr,
                    uint8_t size,
                    riscv_exception *exc);
bool write_clint(riscv_clint *clint,
                 uint64_t addr,
                 uint8_t size,
                 uint64_t value,
                 riscv_exception *exc);

#endif
