#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
bool init_cpu(riscv_cpu *cpu, const char *filename)
{
    if (!init_mem(&cpu->memory, filename))
        return false;
    memset(&cpu->xreg[0], 0, sizeof(uint64_t) * 32);
    cpu->pc = 0;
    cpu->xreg[2] = DRAM_BASE + DRAM_SIZE;
    return true;
}

uint32_t fetch(riscv_cpu *cpu)
{
    uint64_t index = cpu->pc;
    return (uint32_t) cpu->memory.mem[index] |
           (uint32_t)(cpu->memory.mem[index + 1] << 8) |
           (uint32_t)(cpu->memory.mem[index + 2] << 16) |
           (uint32_t)(cpu->memory.mem[index + 3] << 24);
}

void exec(riscv_cpu *cpu, uint32_t inst)
{
    /* The result of E1 >> E2 is E1 right-shifted E2 bit positions.
     * If E1 has an unsigned type the value of the result is the integral
     * part of the quotient of E1 / 2E2.
     */
    uint8_t opcode = inst & 0x7f;
    uint8_t rd = (inst >> 7) & 0x1f;
    uint8_t rs1 = ((inst >> 15) & 0x1f);
    uint8_t rs2 = ((inst >> 20) & 0x1f);
    /* If E1 has a signed type and a negative value, the resulting
     * value is implementation-defined.*/
    uint64_t imm = asr_i64((int) (inst & 0xfff00000), 20);

    /* Note: A computation involving unsigned operands can never overflow,
     * because a result that cannot be represented by the resulting unsigned
     * integer type is reduced modulo the number that is one greater than the
     * largest value that can be represented by the resulting type */
    switch (opcode) {
    // addi
    case 0x13:
        cpu->xreg[rd] = cpu->xreg[rd] + imm;
        break;
    // add
    case 0x33:
        cpu->xreg[rd] = cpu->xreg[rs1] + cpu->xreg[rs2];
        break;
    default:
        LOG_ERROR("Not implemented or invalid opcode 0x%x\n", opcode);
        exit(1);
    }
}

void dump_reg(riscv_cpu *cpu)
{
    for (size_t i = 0; i < 32; i++) {
        printf("x%ld = 0x%lx, ", i, cpu->xreg[i]);
        if (!((i + 1) & 3))
            printf("\n");
    }
    printf("\n");
}

void free_cpu(riscv_cpu *cpu)
{
    free_memory(&cpu->memory);
}
