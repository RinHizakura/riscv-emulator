#ifndef RISCV_EMU
#define RISCV_EMU

#include "common.h"

typedef struct Emu riscv_emu;

riscv_emu *create_emu(const char *filename, const char *rfs_name);
void run_emu(riscv_emu *emu);
int test_emu(riscv_emu *emu);
int take_signature_emu(riscv_emu *emu, char *signature_out_file);
void free_emu(riscv_emu *emu);

#endif
