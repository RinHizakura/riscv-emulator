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
        exit(1);
    }

    while (emu->cpu.pc < emu->cpu.memory.code_size) {
        uint32_t inst = fetch(&emu->cpu);
        emu->cpu.pc += 4;
        exec(&emu->cpu, inst);
    }

    dump_reg(&emu->cpu);

    close_emu(emu);
    return 0;
}
