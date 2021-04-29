#include <stdlib.h>

#include "elf.h"
#include "emu.h"

bool init_emu(riscv_emu *emu,
              const char *filename,
              const char *rfs_name,
              bool is_elf)
{
    if (!init_cpu(&emu->cpu, filename, rfs_name, is_elf))
        return false;

    while (1) {
        if (check_pending_irq(&emu->cpu)) {
            // if any interrupt is pending
            interrput_take_trap(&emu->cpu);
        }

        bool ret = true;
        ret = fetch(&emu->cpu);
        if (!ret)
            goto get_trap;

        ret = decode(&emu->cpu);
        if (!ret)
            goto get_trap;

        ret = exec(&emu->cpu);
    get_trap:
        if (!ret) {
            Trap trap = exception_take_trap(&emu->cpu);
            if (trap == Trap_Fatal) {
                LOG_ERROR("Trap %x happen when pc %lx\n", trap, emu->cpu.pc);
                break;
            }
            // reset exception flag if recovery from trap
            emu->cpu.exc.exception = NoException;
        }
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
