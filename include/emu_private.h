#ifndef RISCV_EMU_PRIVATE
#define RISCV_EMU_PRIVATE

#include "cpu.h"
#include "mini-gdbstub/include/gdbstub.h"

/* FIXME: Reimplement this to enable setting more breakpoint */
struct breakpoint {
    bool is_set;
    uint64_t addr;
};

struct Emu {
    riscv_cpu cpu;

    /* members for debug purpose */
    gdbstub_t gdbstub;
    struct breakpoint bp;
    bool is_interrupted;
};

#endif
