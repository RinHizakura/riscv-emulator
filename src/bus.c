#include "bus.h"

bool init_bus(riscv_bus *bus, const char *filename)
{
    if (!init_mem(&bus->memory, filename))
        return false;
    return true;
}

uint64_t read_bus(riscv_bus *bus, uint64_t addr, uint8_t size)
{
    if (addr >= DRAM_BASE) {
        return read_mem(&bus->memory, addr, size);
    }
    LOG_ERROR("Invalid read memory address 0x%lx < 0x%lx\n", addr, DRAM_BASE);
    return -1;
}

void write_bus(riscv_bus *bus, uint64_t addr, uint8_t size, uint64_t value)
{
    if (addr >= DRAM_BASE) {
        write_mem(&bus->memory, addr, size, value);
        return;
    }
    LOG_ERROR("Invalid write memory address 0x%ld < 0x%ld\n", addr, DRAM_BASE);
}

void free_bus(riscv_bus *bus)
{
    free_memory(&bus->memory);
}
