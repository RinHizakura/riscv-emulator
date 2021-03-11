#ifndef RISCV_EMU
#define RISCV_EMU

#include "cpu.h"
#include "memory.h"

typedef struct Emu {
    riscv_cpu cpu;
} riscv_emu;

bool init_emu(riscv_emu *emu, const char *filename, bool is_elf);
void start_emu(riscv_emu *emu);
void close_emu(riscv_emu *emu);

#endif
