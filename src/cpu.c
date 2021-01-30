#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"

#define asr(value, amount) (value < 0 ? ~(~value >> amount) : value >> amount)

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

static void R_decode(riscv_cpu *cpu)
{
    uint32_t instr = cpu->instr.instr;
    cpu->instr.rd = (instr >> 7) & 0x1f;
    cpu->instr.rs1 = ((instr >> 15) & 0x1f);
    cpu->instr.rs2 = ((instr >> 20) & 0x1f);
}

static void I_decode(riscv_cpu *cpu)
{
    uint32_t instr = cpu->instr.instr;
    cpu->instr.rd = (instr >> 7) & 0x1f;
    cpu->instr.rs1 = ((instr >> 15) & 0x1f);
    cpu->instr.imm = asr((int32_t)(instr & 0xfff00000), 20);
}

static void U_decode(riscv_cpu *cpu)
{
    uint32_t instr = cpu->instr.instr;
    cpu->instr.rd = (instr >> 7) & 0x1f;
    cpu->instr.imm = asr((int32_t)(instr & 0xfffff000), 12);
}

static void instr_lb(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    uint64_t value = read_bus(&cpu->bus, addr, (1 << 3));
    cpu->xreg[cpu->instr.rd] = ((int8_t)(value));
}

static void instr_lh(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    uint64_t value = read_bus(&cpu->bus, addr, 16);
    cpu->xreg[cpu->instr.rd] = ((int16_t)(value));
}

static void instr_lw(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    uint64_t value = read_bus(&cpu->bus, addr, 32);
    cpu->xreg[cpu->instr.rd] = ((int32_t)(value));
}

static void instr_ld(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    uint64_t value = read_bus(&cpu->bus, addr, 64);
    cpu->xreg[cpu->instr.rd] = value;
}

static void instr_lbu(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    uint64_t value = read_bus(&cpu->bus, addr, 16);
    cpu->xreg[cpu->instr.rd] = value;
}

static void instr_lhu(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    uint64_t value = read_bus(&cpu->bus, addr, 32);
    cpu->xreg[cpu->instr.rd] = value;
}

static void instr_lwu(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    uint64_t value = read_bus(&cpu->bus, addr, 64);
    cpu->xreg[cpu->instr.rd] = value;
}

static void instr_addi(riscv_cpu *cpu)
{
    cpu->xreg[cpu->instr.rd] = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
}

static void instr_slli(riscv_cpu *cpu)
{
    // shift amount is the lower 6 bits of immediate
    uint32_t shamt = (cpu->instr.imm & 0x3f);
    cpu->xreg[cpu->instr.rd] = cpu->xreg[cpu->instr.rs1] << shamt;
}

static void instr_slti(riscv_cpu *cpu)
{
    cpu->xreg[cpu->instr.rd] =
        ((int64_t) cpu->xreg[cpu->instr.rs1] < (int64_t) cpu->instr.imm) ? 1
                                                                         : 0;
}

static void instr_sltiu(riscv_cpu *cpu)
{
    cpu->xreg[cpu->instr.rd] =
        (cpu->xreg[cpu->instr.rs1] < cpu->instr.imm) ? 1 : 0;
}

static void instr_xori(riscv_cpu *cpu)
{
    cpu->xreg[cpu->instr.rd] = cpu->xreg[cpu->instr.rs1] ^ cpu->instr.imm;
}

static void instr_srli(riscv_cpu *cpu)
{
    // shift amount is the lower 6 bits of immediate
    uint32_t shamt = (cpu->instr.imm & 0x3f);
    cpu->xreg[cpu->instr.rd] = cpu->xreg[cpu->instr.rs1] >> shamt;
}

static void instr_srai(riscv_cpu *cpu)
{
    // shift amount is the lower 6 bits of immediate
    uint32_t shamt = (cpu->instr.imm & 0x3f);
    cpu->xreg[cpu->instr.rd] = asr((int64_t)(cpu->xreg[cpu->instr.rs1]), shamt);
}

static void instr_ori(riscv_cpu *cpu)
{
    cpu->xreg[cpu->instr.rd] = cpu->xreg[cpu->instr.rs1] | cpu->instr.imm;
}

static void instr_andi(riscv_cpu *cpu)
{
    cpu->xreg[cpu->instr.rd] = cpu->xreg[cpu->instr.rs1] & cpu->instr.imm;
}

static void instr_add(riscv_cpu *cpu)
{
    cpu->xreg[cpu->instr.rd] =
        cpu->xreg[cpu->instr.rs1] + cpu->xreg[cpu->instr.rs2];
}

static void instr_mul(riscv_cpu *cpu)
{
    cpu->xreg[cpu->instr.rd] =
        cpu->xreg[cpu->instr.rs1] * cpu->xreg[cpu->instr.rs2];
}

static void instr_sub(riscv_cpu *cpu)
{
    cpu->xreg[cpu->instr.rd] =
        cpu->xreg[cpu->instr.rs1] - cpu->xreg[cpu->instr.rs2];
}

static void instr_sll(riscv_cpu *cpu)
{
    cpu->xreg[cpu->instr.rd] = cpu->xreg[cpu->instr.rs1]
                               << cpu->xreg[cpu->instr.rs2];
}

static void instr_slt(riscv_cpu *cpu)
{
    cpu->xreg[cpu->instr.rd] = ((int64_t) cpu->xreg[cpu->instr.rs1] <
                                (int64_t) cpu->xreg[cpu->instr.rs2])
                                   ? 1
                                   : 0;
}

static void instr_sltu(riscv_cpu *cpu)
{
    cpu->xreg[cpu->instr.rd] =
        (cpu->xreg[cpu->instr.rs1] < cpu->xreg[cpu->instr.rs2]) ? 1 : 0;
}

static void instr_xor(riscv_cpu *cpu)
{
    cpu->xreg[cpu->instr.rd] =
        cpu->xreg[cpu->instr.rs1] ^ cpu->xreg[cpu->instr.rs2];
}

static void instr_sra(riscv_cpu *cpu)
{
    // shift amount is the low 6 bits of rs2
    uint32_t shamt = (cpu->xreg[cpu->instr.rs2] & 0x3f);
    cpu->xreg[cpu->instr.rd] = asr((int64_t) cpu->xreg[cpu->instr.rs1], shamt);
}

static void instr_or(riscv_cpu *cpu)
{
    cpu->xreg[cpu->instr.rd] =
        cpu->xreg[cpu->instr.rs1] | cpu->xreg[cpu->instr.rs2];
}

static void instr_and(riscv_cpu *cpu)
{
    cpu->xreg[cpu->instr.rd] =
        cpu->xreg[cpu->instr.rs1] & cpu->xreg[cpu->instr.rs2];
}

static void instr_auipc() {}
/* clang-format off */
static riscv_instr_entry instr_load_type[] = {
    [0x0] = {NULL, instr_lb, NULL},  
    [0x1] = {NULL, instr_lh, NULL},
    [0x2] = {NULL, instr_lw, NULL},  
    [0x3] = {NULL, instr_ld, NULL},
    [0x4] = {NULL, instr_lbu, NULL}, 
    [0x5] = {NULL, instr_lhu, NULL},
    [0x6] = {NULL, instr_lwu, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC3, instr_load_type);

static riscv_instr_entry instr_srli_srai_type[] = {
    [0x0] =  {NULL, instr_srli, NULL},
    [0x20] = {NULL, instr_srai, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC7, instr_srli_srai_type);

static riscv_instr_entry instr_imm_type[] = {
    [0x0] = {NULL, instr_addi, NULL},
    [0x1] = {NULL, instr_slli, NULL},
    [0x2] = {NULL, instr_slti, NULL},
    [0x3] = {NULL, instr_sltiu, NULL},
    [0x4] = {NULL, instr_xori, NULL},
    [0x5] = {NULL, NULL, &instr_srli_srai_type_list},
    [0x6] = {NULL, instr_ori, NULL},
    [0x7] = {NULL, instr_andi, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC3, instr_imm_type);

static riscv_instr_entry instr_add_mul_sub_type[] = {
    [0x00] = {NULL, instr_add, NULL},
    [0x01] = {NULL, instr_mul, NULL},
    [0x20] = {NULL, instr_sub, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC7, instr_add_mul_sub_type);

static riscv_instr_entry instr_sll_type[] = {
    [0x00] = {NULL, instr_sll, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC7, instr_sll_type);

static riscv_instr_entry instr_slt_type[] = {
    [0x00] = {NULL, instr_slt, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC7, instr_slt_type);

static riscv_instr_entry instr_sltu_type[] = {
    [0x00] = {NULL, instr_sltu, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC7, instr_sltu_type);

static riscv_instr_entry instr_xor_type[] = {
    [0x00] = {NULL, instr_xor, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC7, instr_xor_type);

static riscv_instr_entry instr_sra_type[] = {
    [0x20] = {NULL, instr_sra, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC7, instr_sra_type);

static riscv_instr_entry instr_or_type[] = {
    [0x00] = {NULL, instr_or, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC7, instr_or_type);

static riscv_instr_entry instr_and_type[] = {
    [0x00] = {NULL, instr_and, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC7, instr_and_type);

static riscv_instr_entry instr_reg_type[] = {
    [0x0] = {NULL, NULL, &instr_add_mul_sub_type_list},
    [0x1] = {NULL, NULL, &instr_sll_type_list},
    [0x2] = {NULL, NULL, &instr_slt_type_list},
    [0x3] = {NULL, NULL, &instr_sltu_type_list},
    [0x4] = {NULL, NULL, &instr_xor_type_list},
    [0x5] = {NULL, NULL, &instr_sra_type_list},
    [0x6] = {NULL, NULL, &instr_or_type_list},
    [0x7] = {NULL, NULL, &instr_and_type_list}
};
INIT_RISCV_INSTR_LIST(FUNC3, instr_reg_type);

static riscv_instr_entry opcode_type[] = {
    [0x03] = {I_decode, NULL, &instr_load_type_list},
    [0x13] = {I_decode, NULL, &instr_imm_type_list},
    [0x17] = {U_decode, instr_auipc, NULL},
    [0x33] = {R_decode, NULL, &instr_reg_type_list},
};
INIT_RISCV_INSTR_LIST(OPCODE, opcode_type);

/* clang-format on */

static bool __decode(riscv_cpu *cpu, riscv_instr_desc *instr_desc)
{
    uint8_t index;

    switch (instr_desc->type.type) {
    case OPCODE:
        index = cpu->instr.opcode;
        break;
    case FUNC3:
        index = cpu->instr.funct3;
        break;
    case FUNC7:
        index = cpu->instr.funct3;
        break;
    default:
        LOG_ERROR("Invalid index type\n");
        return false;
    }

    if (index > instr_desc->size) {
        LOG_ERROR("Not implemented or invalid instruction 0x%x\n",
                  cpu->instr.instr);
        return false;
    }

    riscv_instr_entry entry = instr_desc->instr_list[index];

    if (entry.decode_func)
        entry.decode_func(cpu);

    if (entry.decode_func == NULL && entry.exec_func == NULL &&
        entry.next == NULL) {
        LOG_ERROR("Not implemented or invalid instruction 0x%x\n",
                  cpu->instr.instr);
        return false;
    }

    if (entry.next != NULL)
        __decode(cpu, entry.next);
    else
        cpu->exec_func = entry.exec_func;

    return true;
}

bool init_cpu(riscv_cpu *cpu, const char *filename)
{
    if (!init_bus(&cpu->bus, filename))
        return false;
    memset(&cpu->instr, 0, sizeof(riscv_instr));
    memset(&cpu->xreg[0], 0, sizeof(uint64_t) * 32);
    cpu->pc = DRAM_BASE;
    cpu->xreg[2] = DRAM_BASE + DRAM_SIZE;
    cpu->exec_func = NULL;
    return true;
}

void fetch(riscv_cpu *cpu)
{
    uint32_t instr = read_bus(&cpu->bus, cpu->pc, 32);
    // opcode for indexing the table will be decoded first
    cpu->instr.instr = instr;
    cpu->instr.opcode = instr & 0x7f;
    cpu->instr.funct3 = (instr >> 12) & 0x7;
    cpu->instr.funct7 = (instr >> 25) & 0x7f;
}

bool decode(riscv_cpu *cpu)
{
    return __decode(cpu, &opcode_type_list);
}

void exec(riscv_cpu *cpu)
{
    // Emulate register x0 to 0
    cpu->xreg[0] = 0;
    cpu->exec_func(cpu);

    /* FIXME: If all of our implementation are right, we don't actually need to
     * clean up the structure below. But for easily debugging purpose, we'll
     * reset all of the instruction-relatd structure now. */
    memset(&cpu->instr, 0, sizeof(riscv_instr));
    cpu->exec_func = NULL;
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
