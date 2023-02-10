#ifndef RISCV_EMU_PRIVATE
#define RISCV_EMU_PRIVATE

#include "cpu.h"
#include "mini-gdbstub/include/gdbstub.h"

/* FIXME: */
struct breakpoint {
    bool is_set;
    uint64_t addr;
};

struct Emu {
    riscv_cpu cpu;
    gdbstub_t gdbstub;
    struct breakpoint bp;
};

#endif
