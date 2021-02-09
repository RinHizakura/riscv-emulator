#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"

/* Many type conversion are appied for expected result. To know the detail, you
 * should check out the International Standard of C:
 * http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1548.pdf
 *
 * For example, belows are some C standard you may want to know:
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
 *
 * The important rule of gcc implementation we will use here:
 * 1. The result of, or the signal raised by, converting an integer to a signed
 * integer type when the value cannot be represented in an object of that type
 * (C90 6.2.1.2, C99 and C11 6.3.1.3): For conversion to a type of width N, the
 * value is reduced modulo 2^N to be within range of the type; no signal is
 * raised.
 *
 * 2. The results of some bitwise operations on signed integers (C90 6.3, C99
 * and C11 6.5): Bitwise operators act on the representation of the value
 * including both the sign and value bits, where the sign bit is considered
 * immediately above the highest-value value bit. Signed ‘>>’ acts on negative
 * numbers by sign extension. As an extension to the C language, GCC does not
 * use the latitude given in C99 and C11 only to treat certain aspects of signed
 * ‘<<’ as undefined. However, -fsanitize=shift (and -fsanitize=undefined) will
 * diagnose such cases. They are also diagnosed where constant expressions are
 * required.
 *
 * On the other words, use compiler other than gcc may result error!
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
    cpu->instr.imm = (int32_t)(instr & 0xfff00000) >> 20;
}

static void S_decode(riscv_cpu *cpu)
{
    uint32_t instr = cpu->instr.instr;
    cpu->instr.rs1 = ((instr >> 15) & 0x1f);
    cpu->instr.rs2 = ((instr >> 20) & 0x1f);
    /* note that we have to convert the right statement of "bitwise or" to
     * int32_t to avoid non-expected promotion. Same case in B type and J type
     * decoding */
    cpu->instr.imm = (((int32_t)(instr & 0xfe000000) >> 20) |
                      (int32_t)((instr >> 7) & 0x1f));
}

static void B_decode(riscv_cpu *cpu)
{
    uint32_t instr = cpu->instr.instr;
    cpu->instr.rs1 = ((instr >> 15) & 0x1f);
    cpu->instr.rs2 = ((instr >> 20) & 0x1f);
    // imm[12|10:5|4:1|11] = inst[31|30:25|11:8|7]
    cpu->instr.imm = ((int32_t)(instr & 0x80000000) >> 19)  // 12
                     | (int32_t)((instr & 0x80) << 4)       // 11
                     | (int32_t)((instr >> 20) & 0x7e0)     // 10:5
                     | (int32_t)((instr >> 7) & 0x1e);      // 4:1
}

static void U_decode(riscv_cpu *cpu)
{
    uint32_t instr = cpu->instr.instr;
    cpu->instr.rd = (instr >> 7) & 0x1f;
    cpu->instr.imm = (int32_t)(instr & 0xfffff000);
}

static void J_decode(riscv_cpu *cpu)
{
    uint32_t instr = cpu->instr.instr;
    cpu->instr.rd = (instr >> 7) & 0x1f;

    // imm[20|10:1|11|19:12] = inst[31|30:21|20|19:12]
    cpu->instr.imm = ((int32_t)(instr & 0x80000000) >> 11)  // 20
                     | (int32_t)((instr & 0xff000))         // 19:12
                     | (int32_t)((instr >> 9) & 0x800)      // 11
                     | (int32_t)((instr >> 20) & 0x7fe);    // 10:1
}

static void instr_lb(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    uint64_t value = read_bus(&cpu->bus, addr, 8);
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
    uint64_t value = read_bus(&cpu->bus, addr, 8);
    cpu->xreg[cpu->instr.rd] = value;
}

static void instr_lhu(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    uint64_t value = read_bus(&cpu->bus, addr, 16);
    cpu->xreg[cpu->instr.rd] = value;
}

static void instr_lwu(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    uint64_t value = read_bus(&cpu->bus, addr, 32);
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
    cpu->xreg[cpu->instr.rd] = (int64_t)(cpu->xreg[cpu->instr.rs1]) >> shamt;
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
    cpu->xreg[cpu->instr.rd] = (int64_t) cpu->xreg[cpu->instr.rs1] >> shamt;
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

static void instr_auipc(riscv_cpu *cpu)
{
    cpu->xreg[cpu->instr.rd] = cpu->pc + cpu->instr.imm - 4;
}

static void instr_addiw(riscv_cpu *cpu)
{
    cpu->xreg[cpu->instr.rd] = (int32_t)(
        ((uint32_t) cpu->xreg[cpu->instr.rs1] + (uint32_t) cpu->instr.imm));
}

static void instr_slliw(riscv_cpu *cpu)
{
    uint32_t shamt = (cpu->instr.imm & 0x1f);
    cpu->xreg[cpu->instr.rd] =
        (int32_t)(((uint32_t) cpu->xreg[cpu->instr.rs1] << shamt));
}

static void instr_srliw(riscv_cpu *cpu)
{
    uint32_t shamt = (cpu->instr.imm & 0x1f);
    cpu->xreg[cpu->instr.rd] =
        (int32_t)((uint32_t) cpu->xreg[cpu->instr.rs1] >> shamt);
}

static void instr_sraiw(riscv_cpu *cpu)
{
    uint32_t shamt = (cpu->instr.imm & 0x1f);
    cpu->xreg[cpu->instr.rd] =
        (int32_t)((uint32_t) cpu->xreg[cpu->instr.rs1]) >> shamt;
}

static void instr_sb(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    write_bus(&cpu->bus, addr, 8, cpu->xreg[cpu->instr.rs2]);
}

static void instr_sh(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    write_bus(&cpu->bus, addr, 16, cpu->xreg[cpu->instr.rs2]);
}

static void instr_sw(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    write_bus(&cpu->bus, addr, 32, cpu->xreg[cpu->instr.rs2]);
}

static void instr_sd(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    write_bus(&cpu->bus, addr, 64, cpu->xreg[cpu->instr.rs2]);
}

static void instr_lui(riscv_cpu *cpu)
{
    cpu->xreg[cpu->instr.rd] = cpu->instr.imm;
}

static void instr_addw(riscv_cpu *cpu)
{
    cpu->xreg[cpu->instr.rd] = (int32_t)((uint32_t) cpu->xreg[cpu->instr.rs1] +
                                         (uint32_t) cpu->xreg[cpu->instr.rs2]);
}

static void instr_subw(riscv_cpu *cpu)
{
    cpu->xreg[cpu->instr.rd] = (int32_t)((uint32_t) cpu->xreg[cpu->instr.rs1] -
                                         (uint32_t) cpu->xreg[cpu->instr.rs2]);
}

static void instr_sllw(riscv_cpu *cpu)
{
    uint32_t shamt = (cpu->xreg[cpu->instr.rs2] & 0x1f);
    cpu->xreg[cpu->instr.rd] =
        (int32_t)((uint32_t) cpu->xreg[cpu->instr.rs1] << shamt);
}

static void instr_srlw(riscv_cpu *cpu)
{
    uint32_t shamt = (cpu->xreg[cpu->instr.rs2] & 0x1f);
    cpu->xreg[cpu->instr.rd] =
        (int32_t)((uint32_t) cpu->xreg[cpu->instr.rs1] >> shamt);
}

static void instr_divu(riscv_cpu *cpu)
{
    uint64_t dividend = cpu->xreg[cpu->instr.rs1];
    uint64_t divisor = cpu->xreg[cpu->instr.rs2];

    if (divisor == 0) {
        /* TODO: set DZ (Divide by Zero) in the FCSR */

        // the quotient of division by zero has all bits set
        cpu->xreg[cpu->instr.rd] = -1;
    } else {
        cpu->xreg[cpu->instr.rd] = dividend / divisor;
    }
}

static void instr_sraw(riscv_cpu *cpu)
{
    uint32_t shamt = (cpu->xreg[cpu->instr.rs2] & 0x1f);
    cpu->xreg[cpu->instr.rd] =
        (int32_t)((uint32_t) cpu->xreg[cpu->instr.rs1]) >> shamt;
}

static void instr_remuw(riscv_cpu *cpu)
{
    uint32_t dividend = cpu->xreg[cpu->instr.rs1];
    uint32_t divisor = cpu->xreg[cpu->instr.rs2];

    if (divisor == 0) {
        // the remainder of division by zero equals the dividend
        cpu->xreg[cpu->instr.rd] = dividend;
    } else {
        cpu->xreg[cpu->instr.rd] = (int32_t)(dividend % divisor);
    }
}

static void instr_beq(riscv_cpu *cpu)
{
    if (cpu->xreg[cpu->instr.rs1] == cpu->xreg[cpu->instr.rs2])
        cpu->pc = cpu->pc + cpu->instr.imm - 4;
}

static void instr_bne(riscv_cpu *cpu)
{
    if (cpu->xreg[cpu->instr.rs1] != cpu->xreg[cpu->instr.rs2])
        cpu->pc = cpu->pc + cpu->instr.imm - 4;
}

static void instr_blt(riscv_cpu *cpu)
{
    if ((int64_t) cpu->xreg[cpu->instr.rs1] <
        (int64_t) cpu->xreg[cpu->instr.rs2])
        cpu->pc = cpu->pc + cpu->instr.imm - 4;
}

static void instr_bge(riscv_cpu *cpu)
{
    if ((int64_t) cpu->xreg[cpu->instr.rs1] >=
        (int64_t) cpu->xreg[cpu->instr.rs2])
        cpu->pc = cpu->pc + cpu->instr.imm - 4;
}

static void instr_bltu(riscv_cpu *cpu)
{
    if (cpu->xreg[cpu->instr.rs1] < cpu->xreg[cpu->instr.rs2])
        cpu->pc = cpu->pc + cpu->instr.imm - 4;
}

static void instr_bgeu(riscv_cpu *cpu)
{
    if (cpu->xreg[cpu->instr.rs1] >= cpu->xreg[cpu->instr.rs2])
        cpu->pc = cpu->pc + cpu->instr.imm - 4;
}

static void instr_jalr(riscv_cpu *cpu)
{
    // we don't need to add 4 because the pc already moved on.
    uint64_t prev_pc = cpu->pc;

    // note that we have to set the least-significant bit of the result to zero
    cpu->pc = (cpu->xreg[cpu->instr.rs1] + cpu->instr.imm) & ~1;
    cpu->xreg[cpu->instr.rd] = prev_pc;
}

static void instr_jal(riscv_cpu *cpu)
{
    cpu->xreg[cpu->instr.rd] = cpu->pc;
    cpu->pc = cpu->pc + cpu->instr.imm - 4;
}

static void instr_csrrw(riscv_cpu *cpu)
{
    uint64_t tmp = read_csr(&cpu->csr, cpu->instr.imm);
    write_csr(&cpu->csr, cpu->instr.imm, cpu->xreg[cpu->instr.rs1]);
    cpu->xreg[cpu->instr.rd] = tmp;
}

static void instr_csrrs(riscv_cpu *cpu)
{
    uint64_t tmp = read_csr(&cpu->csr, cpu->instr.imm);
    write_csr(&cpu->csr, cpu->instr.imm, tmp | cpu->xreg[cpu->instr.rs1]);
    cpu->xreg[cpu->instr.rd] = tmp;
}

static void instr_csrrc(riscv_cpu *cpu)
{
    uint64_t tmp = read_csr(&cpu->csr, cpu->instr.imm);
    write_csr(&cpu->csr, cpu->instr.imm, tmp & (~cpu->xreg[cpu->instr.rs1]));
    cpu->xreg[cpu->instr.rd] = tmp;
}

static void instr_csrrwi(riscv_cpu *cpu)
{
    uint64_t zimm = cpu->instr.rs1;
    cpu->xreg[cpu->instr.rd] = read_csr(&cpu->csr, cpu->instr.imm);
    write_csr(&cpu->csr, cpu->instr.imm, zimm);
}

static void instr_csrrsi(riscv_cpu *cpu)
{
    uint64_t zimm = cpu->instr.rs1;
    uint64_t tmp = read_csr(&cpu->csr, cpu->instr.imm);
    write_csr(&cpu->csr, cpu->instr.imm, tmp | zimm);
    cpu->xreg[cpu->instr.rd] = tmp;
}

static void instr_csrrci(riscv_cpu *cpu)
{
    uint64_t zimm = cpu->instr.rs1;
    uint64_t tmp = read_csr(&cpu->csr, cpu->instr.imm);
    write_csr(&cpu->csr, cpu->instr.imm, tmp & (~zimm));
    cpu->xreg[cpu->instr.rd] = tmp;
}

/* TODO: the lock acquire and realease are not implemented now */
static void instr_amoaddw(riscv_cpu *cpu)
{
    uint64_t tmp = read_bus(&cpu->bus, cpu->xreg[cpu->instr.rs1], 32);
    write_bus(&cpu->bus, cpu->xreg[cpu->instr.rs1], 32,
              tmp + cpu->xreg[cpu->instr.rs2]);
    cpu->xreg[cpu->instr.rd] = tmp;
}

static void instr_amoswapw(riscv_cpu *cpu)
{
    uint64_t tmp = read_bus(&cpu->bus, cpu->xreg[cpu->instr.rs1], 32);
    write_bus(&cpu->bus, cpu->xreg[cpu->instr.rs1], 32,
              cpu->xreg[cpu->instr.rs2]);
    cpu->xreg[cpu->instr.rd] = tmp;
}

static void instr_amoaddd(riscv_cpu *cpu)
{
    uint64_t tmp = read_bus(&cpu->bus, cpu->xreg[cpu->instr.rs1], 64);
    write_bus(&cpu->bus, cpu->xreg[cpu->instr.rs1], 64,
              tmp + cpu->xreg[cpu->instr.rs2]);
    cpu->xreg[cpu->instr.rd] = tmp;
}

static void instr_amoswapd(riscv_cpu *cpu)
{
    uint64_t tmp = read_bus(&cpu->bus, cpu->xreg[cpu->instr.rs1], 64);
    write_bus(&cpu->bus, cpu->xreg[cpu->instr.rs1], 64,
              cpu->xreg[cpu->instr.rs2]);
    cpu->xreg[cpu->instr.rd] = tmp;
}

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

static riscv_instr_entry instr_srliw_sraiw_type[] = {
    [0x00] = {NULL, instr_srliw, NULL},
    [0x20] = {NULL, instr_sraiw, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC7, instr_srliw_sraiw_type);

static riscv_instr_entry instr_immw_type[] = {
    [0x0] = {NULL, instr_addiw, NULL},
    [0x1] = {NULL, instr_slliw, NULL},
    [0x5] = {NULL, NULL, &instr_srliw_sraiw_type_list}
};
INIT_RISCV_INSTR_LIST(FUNC3, instr_immw_type);

static riscv_instr_entry instr_store_type[] = {
   [0x0] = {NULL, instr_sb, NULL},
   [0x1] = {NULL, instr_sh, NULL},
   [0x2] = {NULL, instr_sw, NULL},
   [0x3] = {NULL, instr_sd, NULL},
};
INIT_RISCV_INSTR_LIST(FUNC3, instr_store_type);

static riscv_instr_entry instr_addw_subw_type[] = {
    [0x00] = {NULL, instr_addw, NULL},
    [0x20] = {NULL, instr_subw, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC7, instr_addw_subw_type);

static riscv_instr_entry instr_sllw_type[] = {
    [0x00] = {NULL, instr_sllw, NULL},
};
INIT_RISCV_INSTR_LIST(FUNC7, instr_sllw_type);

static riscv_instr_entry instr_srlw_divu_sraw_type[] = {
    [0x00] = {NULL, instr_srlw, NULL},
    [0x01] = {NULL, instr_divu, NULL},
    [0x20] = {NULL, instr_sraw, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC7, instr_srlw_divu_sraw_type);

static riscv_instr_entry instr_remuw_type[] = {
    [0x01] =  {NULL, instr_remuw, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC7, instr_remuw_type);

static riscv_instr_entry instr_regw_type[] = {
    [0x0] = {NULL, NULL, &instr_addw_subw_type_list},
    [0x1] = {NULL, NULL, &instr_sllw_type_list},
    [0x5] = {NULL, NULL, &instr_srlw_divu_sraw_type_list},
    [0x7] = {NULL, NULL, &instr_remuw_type_list},
};
INIT_RISCV_INSTR_LIST(FUNC3, instr_regw_type);

static riscv_instr_entry instr_branch_type[] = {
    [0x0] = {NULL, instr_beq, NULL},
    [0x1] = {NULL, instr_bne, NULL},
    [0x4] = {NULL, instr_blt, NULL},
    [0x5] = {NULL, instr_bge, NULL},
    [0x6] = {NULL, instr_bltu, NULL},
    [0x7] = {NULL, instr_bgeu, NULL},
};
INIT_RISCV_INSTR_LIST(FUNC3, instr_branch_type);

static riscv_instr_entry instr_csr_type[] = {
     [0x1] = {NULL, instr_csrrw, NULL},
     [0x2] = {NULL, instr_csrrs, NULL},
     [0x3] = {NULL, instr_csrrc, NULL},
     [0x5] = {NULL, instr_csrrwi, NULL},
     [0x6] = {NULL, instr_csrrsi, NULL},
     [0x7] = {NULL, instr_csrrci, NULL},
};
INIT_RISCV_INSTR_LIST(FUNC3, instr_csr_type);

static riscv_instr_entry instr_amoaddw_amoswapw_type[] = {
    [0x00] = {NULL, instr_amoaddw, NULL}, 
    [0x01] = {NULL, instr_amoswapw, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC5, instr_amoaddw_amoswapw_type);

static riscv_instr_entry instr_amoaddd_amoswapd_type[] = {
    [0x00] = {NULL, instr_amoaddd, NULL}, 
    [0x01] = {NULL, instr_amoswapd, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC5, instr_amoaddd_amoswapd_type);


static riscv_instr_entry instr_atomic_type[] = {
    [0x2] = {NULL, NULL, &instr_amoaddw_amoswapw_type_list},
    [0x3] = {NULL, NULL, &instr_amoaddd_amoswapd_type_list},
};
INIT_RISCV_INSTR_LIST(FUNC3, instr_atomic_type);

static riscv_instr_entry opcode_type[] = {
    [0x03] = {I_decode, NULL, &instr_load_type_list},
    [0x13] = {I_decode, NULL, &instr_imm_type_list},
    [0x17] = {U_decode, instr_auipc, NULL},
    [0x1b] = {I_decode, NULL, &instr_immw_type_list},
    [0x23] = {S_decode, NULL, &instr_store_type_list},
    [0x2f] = {R_decode, NULL, &instr_atomic_type_list},
    [0x33] = {R_decode, NULL, &instr_reg_type_list},
    [0x37] = {U_decode, instr_lui, NULL},
    [0x3b] = {R_decode, NULL, &instr_regw_type_list},
    [0x63] = {B_decode, NULL, &instr_branch_type_list},
    [0x67] = {I_decode, instr_jalr, NULL},
    [0x6f] = {J_decode, instr_jal, NULL},
    [0x73] = {I_decode, NULL, &instr_csr_type_list},
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
    case FUNC5:
        index = (cpu->instr.funct7 & 0b1111100) >> 2;
        break;
    case FUNC7:
        index = cpu->instr.funct7;
        break;
    default:
        LOG_ERROR("Invalid index type\n");
        return false;
    }

    if (index >= instr_desc->size) {
        LOG_ERROR(
            "Not implemented or invalid instruction:\n"
            "opcode = 0x%x funct3 = 0x%x funct7 = 0x%x \n",
            cpu->instr.opcode, cpu->instr.funct3, cpu->instr.funct7);
        return false;
    }

    riscv_instr_entry entry = instr_desc->instr_list[index];

    if (entry.decode_func)
        entry.decode_func(cpu);

    if (entry.decode_func == NULL && entry.exec_func == NULL &&
        entry.next == NULL) {
        LOG_ERROR(
            "@ Not implemented or invalid instruction:\n"
            "opcode = 0x%x funct3 = 0x%x funct7 = 0x%x \n",
            cpu->instr.opcode, cpu->instr.funct3, cpu->instr.funct7);
        return false;
    }

    if (entry.next != NULL)
        return __decode(cpu, entry.next);
    else
        cpu->exec_func = entry.exec_func;

    return true;
}

bool init_cpu(riscv_cpu *cpu, const char *filename)
{
    if (!init_bus(&cpu->bus, filename))
        return false;

    if (!init_csr(&cpu->csr))
        return false;

    cpu->mode.mode = MACHINE;
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

    LOG_DEBUG(
        "[DEBUG] instr: 0x%x\n"
        "opcode = 0x%x funct3 = 0x%x funct7 = 0x%x \n",
        cpu->instr.instr, cpu->instr.opcode, cpu->instr.funct3,
        cpu->instr.funct7);
}

bool decode(riscv_cpu *cpu)
{
    return __decode(cpu, &opcode_type_list);
}

void exec(riscv_cpu *cpu)
{
    cpu->exec_func(cpu);

    // Emulate register x0 to 0
    cpu->xreg[0] = 0;

    /* FIXME: If all of our implementation are right, we don't actually need to
     * clean up the structure below. But for easily debugging purpose, we'll
     * reset all of the instruction-relatd structure now. */
    memset(&cpu->instr, 0, sizeof(riscv_instr));
    cpu->exec_func = NULL;
}

void dump_reg(riscv_cpu *cpu)
{
    static char *abi_name[] = {
        "z",  "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
        "a1", "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
        "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

    printf("pc = 0x%lx\n", cpu->pc);
    for (size_t i = 0; i < 32; i++) {
        printf("x%-2ld(%-3s) = 0x%-8lx, ", i, abi_name[i], cpu->xreg[i]);
        if (!((i + 1) & 3))
            printf("\n");
    }
    printf("\n");
}

void dump_csr(riscv_cpu *cpu)
{
    printf("%-10s = 0x%-8lx, ", "MSTATUS", read_csr(&cpu->csr, MSTATUS));
    printf("%-10s = 0x%-8lx, ", "MTVEC", read_csr(&cpu->csr, MTVEC));
    printf("%-10s = 0x%-8lx, ", "MEPC", read_csr(&cpu->csr, MEPC));
    printf("%-10s = 0x%-8lx\n", "MCAUSE", read_csr(&cpu->csr, MCAUSE));

    printf("%-10s = 0x%-8lx, ", "SSTATUS", read_csr(&cpu->csr, SSTATUS));
    printf("%-10s = 0x%-8lx, ", "STVEC", read_csr(&cpu->csr, STVEC));
    printf("%-10s = 0x%-8lx, ", "SEPC", read_csr(&cpu->csr, SEPC));
    printf("%-10s = 0x%-8lx\n", "SCAUSE", read_csr(&cpu->csr, SCAUSE));
}

void free_cpu(riscv_cpu *cpu)
{
    free_bus(&cpu->bus);
    free_csr(&cpu->csr);
}
