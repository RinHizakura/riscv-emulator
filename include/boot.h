#ifndef RISCV_BOOT
#define RISCV_BOOT

#include <stdbool.h>
#include <stdint.h>

#include "exception.h"

typedef struct {
    uint8_t *boot_mem;
    size_t boot_mem_size;
} riscv_boot;

bool init_boot(riscv_boot *boot, uint64_t entry_addr);
uint64_t read_boot(riscv_boot *boot,
                   uint64_t addr,
                   uint64_t size,
                   riscv_exception *exc);

void free_boot(riscv_boot *boot);

#endif
