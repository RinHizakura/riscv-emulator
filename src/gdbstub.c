#include "emu.h"
#include "emu_private.h"

static size_t gdbstub_read_reg(void *args, int regno)
{
    riscv_emu *emu = (riscv_emu *) args;

    if (regno > 32)
        return -1;

    if (regno == 32)
        return emu->cpu.pc;

    return emu->cpu.xreg[regno];
}

static void gdbstub_write_reg(void *args, int regno, size_t data)
{
    riscv_emu *emu = (riscv_emu *) args;

    if (regno > 32)
        return;

    if (regno == 32)
        emu->cpu.pc = data;
    else
        emu->cpu.xreg[regno] = data;
}

static void gdbstub_read_mem(void *args, size_t addr, size_t len, void *val)
{
    riscv_emu *emu = (riscv_emu *) args;

    for (size_t i = 0; i < len; i++)
        *((uint8_t *) val + i) =
            read_bus(&emu->cpu.bus, addr + i, 8, &emu->cpu.exc);
}

static void gdbstub_write_mem(void *args, size_t addr, size_t len, void *val)
{
    riscv_emu *emu = (riscv_emu *) args;

    for (size_t i = 0; i < len; i++)
        write_bus(&emu->cpu.bus, addr + i, 8, *((uint8_t *) val + i),
                  &emu->cpu.exc);
}

static inline bool is_interrupted(riscv_emu *emu)
{
    return __atomic_load_n(&emu->is_interrupted, __ATOMIC_RELAXED);
}

static gdb_action_t gdbstub_conti(void *args)
{
    riscv_emu *emu = (riscv_emu *) args;
    while ((!(emu->bp.is_set && (emu->cpu.pc == emu->bp.addr))) &&
           !is_interrupted(emu)) {
        if (!step_cpu(&emu->cpu))
            return ACT_SHUTDOWN;
    }

    /* Clear the interrupt if it's pending */
    __atomic_store_n(&emu->is_interrupted, false, __ATOMIC_RELAXED);

    return ACT_RESUME;
}

static gdb_action_t gdbstub_stepi(void *args)
{
    riscv_emu *emu = (riscv_emu *) args;
    if (!step_cpu(&emu->cpu))
        return ACT_SHUTDOWN;
    return ACT_RESUME;
}

static bool gdbstub_set_bp(void *args, size_t addr, bp_type_t type)
{
    riscv_emu *emu = (riscv_emu *) args;

    if (type != BP_SOFTWARE || emu->bp.is_set)
        return false;

    emu->bp.is_set = true;
    emu->bp.addr = addr;
    return true;
}

static bool gdbstub_del_bp(void *args, size_t addr, bp_type_t type)
{
    riscv_emu *emu = (riscv_emu *) args;

    // It's fine when there's no matching breakpoint, just doing nothing
    if (type != BP_SOFTWARE || !emu->bp.is_set || emu->bp.addr != addr)
        return true;

    emu->bp.is_set = false;
    emu->bp.addr = 0;
    return true;
}

static void gdbstub_on_interrupt(void *args)
{
    riscv_emu *emu = (riscv_emu *) args;
    /* Notify the emulator to break out the for loop in cont method */
    __atomic_store_n(&emu->is_interrupted, true, __ATOMIC_RELAXED);
}

const struct target_ops gdbstub_ops = {
    .read_reg = gdbstub_read_reg,
    .write_reg = gdbstub_write_reg,
    .read_mem = gdbstub_read_mem,
    .write_mem = gdbstub_write_mem,
    .cont = gdbstub_conti,
    .stepi = gdbstub_stepi,
    .set_bp = gdbstub_set_bp,
    .del_bp = gdbstub_del_bp,
    .on_interrupt = gdbstub_on_interrupt,
};
