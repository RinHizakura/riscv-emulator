#include <assert.h>
#include <stdlib.h>

#include "elf.h"
#include "emu.h"

bool init_emu(riscv_emu *emu, const char *filename, const char *rfs_name)
{
    if (!init_cpu(&emu->cpu, filename, rfs_name))
        return false;

    return true;
}

void run_emu(riscv_emu *emu)
{
    while (tick_cpu(&emu->cpu))
        ;
}

void test_emu(riscv_emu *emu)
{
    while (tick_cpu(&emu->cpu)) {
        /* If a riscv-tests program is done, it will write non-zero value to
         * a certain address. We can poll it in every tick to terminate the
         * emulator. */
        riscv_mem *mem = &emu->cpu.bus.memory;
        uint64_t tohost_addr = mem->tohost_addr;
        assert(tohost_addr > DRAM_BASE);
        if (read_mem(mem, tohost_addr, 8, &emu->cpu.exc) != 0)
            break;
    }
}

void free_emu(riscv_emu *emu)
{
    free_cpu(&emu->cpu);
}
