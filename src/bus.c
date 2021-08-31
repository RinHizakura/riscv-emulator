#include <string.h>

#include "bus.h"

bool init_bus(riscv_bus *bus, const char *filename, const char *rfs_name)
{
    if (!init_mem(&bus->memory, filename))
        return false;

    /* since the initialize of CLINT / PLIC is simple, we don't put it to
     * another function */
    memset(&bus->clint, 0, sizeof(riscv_clint));
    memset(&bus->plic, 0, sizeof(riscv_plic));

    if (!init_uart(&bus->uart))
        return false;

    if (!init_virtio_blk(&bus->virtio_blk, rfs_name))
        return false;

    if (!init_boot(&bus->boot, get_entry_addr()))
        return false;

    return true;
}

uint64_t read_bus(riscv_bus *bus,
                  uint64_t addr,
                  uint8_t size,
                  riscv_exception *exc)
{
    if (addr >= CLINT_BASE && addr < CLINT_END)
        return read_clint(&bus->clint, addr, size, exc);

    if (addr >= PLIC_BASE && addr < PLIC_END)
        return read_plic(&bus->plic, addr, size, exc);

    if (addr >= UART_BASE && addr < UART_END)
        return read_uart(&bus->uart, addr, size, exc);

    if (addr >= VIRTIO_BASE && addr < VIRTIO_END)
        return read_virtio_blk(&bus->virtio_blk, addr, size, exc);

    if (addr >= DRAM_BASE && addr < DRAM_END)
        return read_mem(&bus->memory, addr, size, exc);

    if ((addr >= BOOT_ROM_BASE) &&
        (addr < (BOOT_ROM_BASE + bus->boot.boot_mem_size)))
        return read_boot(&bus->boot, addr, size, exc);

    LOG_ERROR("Invalid read memory address 0x%lx\n", addr);
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

    if (addr >= PLIC_BASE && addr < PLIC_END)
        return write_plic(&bus->plic, addr, size, value, exc);

    if (addr >= UART_BASE && addr < UART_END)
        return write_uart(&bus->uart, addr, size, value, exc);

    if (addr >= VIRTIO_BASE && addr < VIRTIO_END)
        return write_virtio_blk(&bus->virtio_blk, addr, size, value, exc);

    if (addr >= DRAM_BASE && addr < DRAM_END)
        return write_mem(&bus->memory, addr, size, value, exc);

    LOG_ERROR("Invalid write memory address 0x%ld\n", addr);
    exc->exception = StoreAMOAccessFault;
    return false;
}

void tick_bus(riscv_bus *bus, riscv_csr *csr)
{
    tick_clint(&bus->clint, csr);
}

void free_bus(riscv_bus *bus)
{
    free_memory(&bus->memory);
    free_uart(&bus->uart);
    free_virtio_blk(&bus->virtio_blk);
    free_boot(&bus->boot);
}
