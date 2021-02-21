#ifndef RISCV_BUS
#define RISCV_BUS

#include "clint.h"
#include "exception.h"
#include "memmap.h"
#include "memory.h"

typedef struct {
    riscv_mem memory;
    riscv_clint clint;
} riscv_bus;

bool init_bus(riscv_bus *bus, const char *filename);
uint64_t read_bus(riscv_bus *bus,
                  uint64_t addr,
                  uint8_t size,
                  riscv_exception *exc);
bool write_bus(riscv_bus *bus,
               uint64_t addr,
               uint8_t size,
               uint64_t value,
               riscv_exception *exc);
void free_bus(riscv_bus *bus);
#endif
