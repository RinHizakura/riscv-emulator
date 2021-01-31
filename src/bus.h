#ifndef RISCV_BUS
#define RISCV_BUS

#include "memory.h"

/// The address which DRAM starts
#define DRAM_BASE 0x80000000UL

typedef struct {
    riscv_mem memory;
} riscv_bus;

bool init_bus(riscv_bus *bus, const char *filename);
uint64_t read_bus(riscv_bus *bus, uint64_t addr, uint8_t size);
void write_bus(riscv_bus *bus, uint64_t addr, uint8_t size, uint64_t value);
void free_bus(riscv_bus *bus);
#endif
