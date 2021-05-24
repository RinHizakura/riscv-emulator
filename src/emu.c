#include <stdlib.h>

#include "elf.h"
#include "emu.h"

bool init_emu(riscv_emu *emu, const char *filename, const char *rfs_name)
{
    if (!init_cpu(&emu->cpu, filename, rfs_name))
        return false;

    while (1) {
        if (check_pending_irq(&emu->cpu)) {
            // if any interrupt is pending
            interrput_take_trap(&emu->cpu);
#ifdef ICACHE_CONFIG
            // flush cache when jumping in trap handler
            invalid_icache(&emu->cpu.icache);
#endif
        }

        bool ret = true;
        bool is_cache = false;

        ret = fetch(&emu->cpu, &is_cache);
#ifdef ICACHE_CONFIG
        if (is_cache)
            goto exec_stage;
#endif
        if (!ret)
            goto get_trap;

        ret = decode(&emu->cpu);
        if (!ret)
            goto get_trap;

    exec_stage:
        ret = exec(&emu->cpu);
    get_trap:
        if (!ret) {
            uint64_t next_pc = emu->cpu.pc;
            Trap trap = exception_take_trap(&emu->cpu);
            if (trap == Trap_Fatal) {
                LOG_ERROR("Trap %x happen before pc %lx\n", trap, next_pc);
                break;
            }
            // reset exception flag if recovery from trap
            emu->cpu.exc.exception = NoException;
#ifdef ICACHE_CONFIG
            // flush cache when jumping in trap handler
            invalid_icache(&emu->cpu.icache);
#endif
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
