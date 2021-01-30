#ifndef RISCV_MEM
#define RISCV_MEM

/// Default memory size (1GiB)
#define DRAM_SIZE (1024 * 1024 * 1024)

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t *mem;
    uint64_t code_size;
} riscv_mem;

bool init_mem(riscv_mem *mem, const char *filename);
uint64_t read_mem(riscv_mem *mem, uint64_t addr, uint64_t size);
void write_mem(riscv_mem *mem, uint64_t addr, uint64_t value, uint8_t size);
void free_memory(riscv_mem *mem);
#endif
