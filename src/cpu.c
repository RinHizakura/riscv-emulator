#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"

#define asr(value, amount) (value < 0 ? ~(~value >> amount) : value >> amount)

bool init_cpu(riscv_cpu *cpu, const char *filename)
{
    if (!init_bus(&cpu->bus, filename))
        return false;
    memset(&cpu->xreg[0], 0, sizeof(uint64_t) * 32);
    cpu->pc = DRAM_BASE;
    cpu->xreg[2] = DRAM_BASE + DRAM_SIZE;
    return true;
}

uint32_t fetch(riscv_cpu *cpu)
{
    return read_bus(&cpu->bus, cpu->pc, 32);
}

/* Note of some C standard:
 * 1. The result of E1 >> E2 is E1 right-shifted E2 bit positions.
 * If E1 has an unsigned type the value of the result is the integral
 * part of the quotient of E1 / 2E2.  If E1 has a signed type and a negative
 * value, the resulting value is implementation-defined.
 *
 * 2. A computation involving unsigned operands can never overflow,
 * because a result that cannot be represented by the resulting unsigned
 * integer type is reduced modulo the number that is one greater than the
 * largest value that can be represented by the resulting type
 *
 * 3. the new type is signed and the value cannot be represented in it;
 * either the result is implementation-defined or an implementation-defined
 * signal is raised.
 *
 * Implementation-defined of gcc:
 * https://gcc.gnu.org/onlinedocs/gcc/Integers-implementation.html
 */


static bool inst_ld(riscv_cpu *cpu, uint32_t inst)
{
    uint8_t rd = (inst >> 7) & 0x1f;
    uint8_t rs1 = ((inst >> 15) & 0x1f);
    uint8_t funct3 = (inst >> 12) & 0x7;

    uint64_t imm = asr((int32_t)(inst & 0xfff00000), 20);
    uint64_t addr = cpu->xreg[rs1] + imm;

    switch (funct3) {
    // lb
    case 0: {
        uint64_t value = read_bus(&cpu->bus, addr, (1 << 3));
        cpu->xreg[rd] = ((int8_t)(value));
    } break;
    // lh
    case 1: {
        uint64_t value = read_bus(&cpu->bus, addr, (1 << 4));
        cpu->xreg[rd] = ((int16_t)(value));
    } break;
    // lw
    case 2: {
        uint64_t value = read_bus(&cpu->bus, addr, (1 << 5));
        cpu->xreg[rd] = ((int32_t)(value));
    } break;
    // ld
    case 3: {
        uint64_t value = read_bus(&cpu->bus, addr, (1 << 6));
        cpu->xreg[rd] = value;
    } break;
    // lbu
    case 4: {
        uint64_t value = read_bus(&cpu->bus, addr, (1 << 3));
        cpu->xreg[rd] = value;
    } break;
    // lhu
    case 5: {
        uint64_t value = read_bus(&cpu->bus, addr, (1 << 4));
        cpu->xreg[rd] = value;
    } break;
    // lwu
    case 6: {
        uint64_t value = read_bus(&cpu->bus, addr, (1 << 5));
        cpu->xreg[rd] = value;
    } break;
    default:
        LOG_ERROR("Unknown funct3 0x%x for opcode 0x3\n", funct3);
        return false;
    }

    return true;
}

static bool inst_imm(riscv_cpu *cpu, uint32_t inst)
{
    uint8_t rd = (inst >> 7) & 0x1f;
    uint8_t rs1 = ((inst >> 15) & 0x1f);
    uint8_t funct3 = (inst >> 12) & 0x7;
    uint8_t funct7 = (inst >> 25) & 0x7f;

    uint64_t imm = asr((int32_t)(inst & 0xfff00000), 20);
    // shift amount is the lower 6 bits of immediate
    uint32_t shamt = (imm & 0x3f);

    switch (funct3) {
    // addi
    case 0x0:
        cpu->xreg[rd] = cpu->xreg[rs1] + imm;
        break;
    // slli
    case 0x1:
        cpu->xreg[rd] = cpu->xreg[rs1] << shamt;
        break;
    // slti
    case 0x2:
        cpu->xreg[rd] = ((int64_t) cpu->xreg[rs1] < (int64_t) imm) ? 1 : 0;
        break;
    // sltiu
    case 0x3:
        cpu->xreg[rd] = (cpu->xreg[rs1] < imm) ? 1 : 0;
        break;
    // xori
    case 0x4:
        cpu->xreg[rd] = cpu->xreg[rs1] ^ imm;
        break;
    // sr..
    case 0x5: {
        switch (funct7) {
            // srli:
        case 0x0:
            cpu->xreg[rd] = cpu->xreg[rs1] >> shamt;
            break;
        // srai
        case 0x20:
            cpu->xreg[rd] = asr((int64_t)(cpu->xreg[rs1]), shamt);
            break;

        default:
            LOG_ERROR("Unknown funct7 0x%x for opcode 0x13\n", funct7);
            return false;
        }
    } break;
    // ori
    case 0x6:
        cpu->xreg[rd] = cpu->xreg[rs1] | imm;
        break;
    // andi
    case 0x7:
        cpu->xreg[rd] = cpu->xreg[rs1] & imm;
        break;
    default:
        LOG_ERROR("Unknown funct3 0x%x for opcode 0x13\n", funct3);
        return false;
    }

    return true;
}

static bool inst_reg(riscv_cpu *cpu, uint32_t inst)
{
    uint8_t rd = (inst >> 7) & 0x1f;
    uint8_t rs1 = ((inst >> 15) & 0x1f);
    uint8_t rs2 = ((inst >> 20) & 0x1f);
    uint8_t funct3 = (inst >> 12) & 0x7;
    uint8_t funct7 = (inst >> 25) & 0x7f;

    // shift amount is the low 6 bits of rs2
    uint32_t shamt = (cpu->xreg[rs2] & 0x3f);
    uint16_t funct = (uint16_t) funct3 << 8 | funct7;

    switch (funct) {
    // add
    case 0x0000:
        cpu->xreg[rd] = cpu->xreg[rs1] + cpu->xreg[rs2];
        break;
    // mul
    case 0x0001:
        cpu->xreg[rd] = cpu->xreg[rs1] * cpu->xreg[rs2];
        break;
    // sub
    case 0x0020:
        cpu->xreg[rd] = cpu->xreg[rs1] - cpu->xreg[rs2];
        break;
    // sll
    case 0x0100:
        cpu->xreg[rd] = cpu->xreg[rs1] << cpu->xreg[rs2];
        break;
    // slt
    case 0x0200:
        cpu->xreg[rd] =
            ((int64_t) cpu->xreg[rs1] < (int64_t) cpu->xreg[rs2]) ? 1 : 0;
        break;
    // sltu
    case 0x0300:
        cpu->xreg[rd] = (cpu->xreg[rs1] < cpu->xreg[rs2]) ? 1 : 0;
        break;
    // xor
    case 0x0400:
        cpu->xreg[rd] = cpu->xreg[rs1] ^ cpu->xreg[rs2];
        break;
    // sra
    case 0x0520:
        cpu->xreg[rd] = asr((int64_t) cpu->xreg[rs1], shamt);
        break;
    // or
    case 0x0600:
        cpu->xreg[rd] = cpu->xreg[rs1] | cpu->xreg[rs2];
        break;
    // and
    case 0x0700:
        cpu->xreg[rd] = cpu->xreg[rs1] & cpu->xreg[rs2];
        break;
    default:
        LOG_ERROR("Unknown funct3 0x%x / funct7 0x%x for opcode 0x33\n", funct3,
                  funct7);
        return false;
    }
    return true;
}
bool exec(riscv_cpu *cpu, uint32_t inst)
{
    uint8_t opcode = inst & 0x7f;

    switch (opcode) {
    // load
    case 0x3:
        return inst_ld(cpu, inst);
    // imm
    case 0x13:
        return inst_imm(cpu, inst);
    // reg
    case 0x33:
        return inst_reg(cpu, inst);
    default:
        LOG_ERROR("Not implemented or invalid opcode 0x%x\n", opcode);
        return false;
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
    free_bus(&cpu->bus);
}
