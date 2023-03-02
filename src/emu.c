#include <assert.h>
#include <stdlib.h>

#include "emu.h"
#include "emu_private.h"

riscv_emu *create_emu(const char *filename, const char *rfs_name)
{
    riscv_emu *emu = calloc(1, sizeof(riscv_emu));
    if (!emu)
        return NULL;

    if (!init_cpu(&emu->cpu, filename, rfs_name)) {
        free_emu(emu);
        return NULL;
    }

    return emu;
}

void run_emu(riscv_emu *emu)
{
    while (step_cpu(&emu->cpu))
        ;
}

extern struct target_ops gdbstub_ops;
void run_emu_debug(riscv_emu *emu)
{
    emu->bp.is_set = false;
    emu->is_interrupted = false;
    if (!gdbstub_init(&emu->gdbstub, &gdbstub_ops,
                      (arch_info_t){
                          .reg_num = 33,
                          .reg_byte = 8,
                          .target_desc = TARGET_RV64,
                      },
                      GDBSTUB_COMM))
        return;

    gdbstub_run(&emu->gdbstub, (void *) emu);

    gdbstub_close(&emu->gdbstub);
}

int test_emu(riscv_emu *emu)
{
    while (step_cpu(&emu->cpu)) {
        /* If a riscv-tests program is done, it will write non-zero value to
         * a certain address. We can poll it in every step to terminate the
         * emulator. */
        riscv_mem *mem = &emu->cpu.bus.memory;
        uint64_t tohost_addr = mem->tohost_addr;
        assert(tohost_addr > DRAM_BASE);
        if (read_mem(mem, tohost_addr, 8, &emu->cpu.exc) != 0)
            break;
    }

    return emu->cpu.xreg[10];
}

int take_signature_emu(riscv_emu *emu, char *signature_out_file)
{
    // FIXME: Refactor to prettier code
    FILE *f = fopen(signature_out_file, "w");
    if (!f) {
        ERROR("Failed to open file %s for compilance test.\n",
              signature_out_file);
        return -1;
    }

    uint64_t begin = emu->cpu.bus.memory.sig_start;
    uint64_t end = emu->cpu.bus.memory.sig_end;

    for (uint64_t i = begin; i < end; i += 4) {
        uint32_t value =
            read_mem(&emu->cpu.bus.memory, i, 32, &emu->cpu.exc) & 0xffffffff;
        fprintf(f, "%08x\n", value);
    }
    fclose(f);
    return 0;
}

void free_emu(riscv_emu *emu)
{
    if (emu == NULL)
        return;

    free_cpu(&emu->cpu);
    free(emu);
}
