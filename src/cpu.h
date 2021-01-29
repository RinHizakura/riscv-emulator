#ifndef RISCV_CPU
#define RISCV_CPU

#include "bus.h"

typedef struct Cpu {
    riscv_bus bus;
    uint64_t xreg[32];
    uint64_t pc;
} riscv_cpu;

bool init_cpu(riscv_cpu *cpu, const char *filename);
uint32_t fetch(riscv_cpu *cpu);
bool exec(riscv_cpu *cpu, uint32_t inst);
void dump_reg(riscv_cpu *cpu);
void free_cpu(riscv_cpu *cpu);
#endif
