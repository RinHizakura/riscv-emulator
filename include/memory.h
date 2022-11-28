#ifndef RISCV_MEM
#define RISCV_MEM

#include <stdbool.h>
#include <stdint.h>

#include "elf_parser.h"
#include "exception.h"

typedef struct {
    elf_t elf;
    uint8_t *mem;
    uint64_t sig_start;
    uint64_t sig_end;
    uint64_t tohost_addr;
} riscv_mem;

uint64_t get_entry_addr();
bool init_mem(riscv_mem *mem, const char *filename);
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
