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

void free_emu(riscv_emu *emu)
{
    free_cpu(&emu->cpu);
}
