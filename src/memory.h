#ifndef RISCV_MEM
#define RISCV_MEM

#include <stdbool.h>
#include <stdint.h>

typedef struct memory {
    uint8_t *mem;
    uint64_t code_size;
} riscv_mem;

bool init_mem(riscv_mem *mem, const char *filename);
void free_memory(riscv_mem *mem);
#endif
