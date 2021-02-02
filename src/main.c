#include <stdlib.h>

#include "emu.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        LOG_ERROR("Binary is required for memory!\n");
        return -1;
    }

    riscv_emu *emu = malloc(sizeof(riscv_emu));

    if (!init_emu(emu, argv[1])) {
        /* FIXME: should properly cleanup first */
        exit(1);
    }

    uint64_t start_pc = emu->cpu.pc;
    while (emu->cpu.pc < start_pc + emu->cpu.bus.memory.code_size &&
           emu->cpu.pc >= DRAM_BASE) {
        fetch(&emu->cpu);
        emu->cpu.pc += 4;
        bool ret = decode(&emu->cpu);
        if (ret == false)
            break;
        exec(&emu->cpu);
#ifdef DEBUG
        dump_reg(&emu->cpu);
#endif
    }

    dump_reg(&emu->cpu);

    close_emu(emu);
    return 0;
}
