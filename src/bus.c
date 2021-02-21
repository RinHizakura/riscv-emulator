#include <string.h>

#include "bus.h"

bool init_bus(riscv_bus *bus, const char *filename)
{
    if (!init_mem(&bus->memory, filename))
        return false;

    // since the initialize of CLINT is simple, we don't put it to another
    // function
    memset(&bus->clint, 0, sizeof(riscv_clint));
    return true;
}

uint64_t read_bus(riscv_bus *bus,
                  uint64_t addr,
                  uint8_t size,
                  riscv_exception *exc)
{
    if (addr >= CLINT_BASE && addr < CLINT_END)
        return read_clint(&bus->clint, addr, size, exc);

    if (addr >= DRAM_BASE)
        return read_mem(&bus->memory, addr, size, exc);

    LOG_ERROR("Invalid read memory address 0x%lx < 0x%lx\n", addr, DRAM_BASE);
    exc->exception = LoadAccessFault;
    return -1;
}


bool write_bus(riscv_bus *bus,
               uint64_t addr,
               uint8_t size,
               uint64_t value,
               riscv_exception *exc)
{
    if (addr >= CLINT_BASE && addr < CLINT_END)
        return write_clint(&bus->clint, addr, size, value, exc);

    if (addr >= DRAM_BASE)
        return write_mem(&bus->memory, addr, size, value, exc);

    LOG_ERROR("Invalid write memory address 0x%ld < 0x%ld\n", addr, DRAM_BASE);
    exc->exception = StoreAMOAccessFault;
    return false;
}

void free_bus(riscv_bus *bus)
{
    free_memory(&bus->memory);
}
