#include <stdlib.h>

#include "elf.h"
#include "emu.h"

bool init_emu(riscv_emu *emu, const char *filename, const char *rfs_name)
{
    if (!init_cpu(&emu->cpu, filename, rfs_name))
        return false;

    while (tick(&emu->cpu)) {
#ifdef DEBUG
        dump_reg(&emu->cpu);
        dump_csr(&emu->cpu);
#endif
    }

    return true;
}

void free_emu(riscv_emu *emu)
{
    free_cpu(&emu->cpu);
}
