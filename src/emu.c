#include "emu.h"
#include "stdlib.h"

bool init_emu(riscv_emu *emu, const char *filename)
{
    if (!init_cpu(&emu->cpu, filename))
        return false;

    return true;
}

void close_emu(riscv_emu *emu)
{
    free_cpu(&emu->cpu);
    free(emu);
}
