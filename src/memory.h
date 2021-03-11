#ifndef RISCV_MEM
#define RISCV_MEM

#include <stdbool.h>
#include <stdint.h>

#include "elf.h"
#include "memmap.h"

typedef struct {
    riscv_elf elf;
    uint8_t *mem;
    uint64_t code_size;
} riscv_mem;

bool init_mem(riscv_mem *mem, const char *filename, bool is_elf);
uint64_t read_mem(riscv_mem *mem,
                  uint64_t addr,
                  uint64_t size,
                  riscv_exception *exc);
bool write_mem(riscv_mem *mem,
               uint64_t addr,
               uint8_t size,
               uint64_t value,
               riscv_exception *exc);
void free_memory(riscv_mem *mem);
#endif
