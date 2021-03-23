#include <assert.h>
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
    uint64_t value = read_bus(&cpu->bus, addr, 8, &cpu->exc);
    if (cpu->exc.exception != NoException) {
        assert(value == (uint64_t) -1);
        return;
    }
    cpu->xreg[cpu->instr.rd] = ((int8_t)(value));
}

static void instr_lh(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    uint64_t value = read_bus(&cpu->bus, addr, 16, &cpu->exc);
    if (cpu->exc.exception != NoException) {
        assert(value == (uint64_t) -1);
        return;
    }
    cpu->xreg[cpu->instr.rd] = ((int16_t)(value));
}

static void instr_lw(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    uint64_t value = read_bus(&cpu->bus, addr, 32, &cpu->exc);
    if (cpu->exc.exception != NoException) {
        assert(value == (uint64_t) -1);
        return;
    }
    cpu->xreg[cpu->instr.rd] = ((int32_t)(value));
}

static void instr_ld(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    uint64_t value = read_bus(&cpu->bus, addr, 64, &cpu->exc);
    if (cpu->exc.exception != NoException) {
        assert(value == (uint64_t) -1);
        return;
    }
    cpu->xreg[cpu->instr.rd] = value;
}

static void instr_lbu(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    uint64_t value = read_bus(&cpu->bus, addr, 8, &cpu->exc);
    if (cpu->exc.exception != NoException) {
        assert(value == (uint64_t) -1);
        return;
    }
    cpu->xreg[cpu->instr.rd] = value;
}

static void instr_lhu(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    uint64_t value = read_bus(&cpu->bus, addr, 16, &cpu->exc);
    if (cpu->exc.exception != NoException) {
        assert(value == (uint64_t) -1);
        return;
    }
    cpu->xreg[cpu->instr.rd] = value;
}

static void instr_lwu(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    uint64_t value = read_bus(&cpu->bus, addr, 32, &cpu->exc);
    if (cpu->exc.exception != NoException) {
        assert(value == (uint64_t) -1);
        return;
    }
    cpu->xreg[cpu->instr.rd] = value;
}

static void instr_fence(riscv_cpu *cpu)
{
    /* Since our emulator only execute instruction on a single thread.
     * So nothing will do for fence instruction */
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

static void instr_srl(riscv_cpu *cpu)
{
    uint32_t shamt = (cpu->xreg[cpu->instr.rs2] & 0x3f);
    cpu->xreg[cpu->instr.rd] = cpu->xreg[cpu->instr.rs1] >> shamt;
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

static void instr_remu(riscv_cpu *cpu)
{
    uint64_t dividend = cpu->xreg[cpu->instr.rs1];
    uint64_t divisor = cpu->xreg[cpu->instr.rs2];

    if (divisor == 0) {
        /* TODO: set DZ (Divide by Zero) in the FCSR */

        // the quotient of division by zero has all bits set
        cpu->xreg[cpu->instr.rd] = dividend;
    } else {
        cpu->xreg[cpu->instr.rd] = dividend % divisor;
    }
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
    write_bus(&cpu->bus, addr, 8, cpu->xreg[cpu->instr.rs2], &cpu->exc);
}

static void instr_sh(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    write_bus(&cpu->bus, addr, 16, cpu->xreg[cpu->instr.rs2], &cpu->exc);
}

static void instr_sw(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    write_bus(&cpu->bus, addr, 32, cpu->xreg[cpu->instr.rs2], &cpu->exc);
}

static void instr_sd(riscv_cpu *cpu)
{
    uint64_t addr = cpu->xreg[cpu->instr.rs1] + cpu->instr.imm;
    write_bus(&cpu->bus, addr, 64, cpu->xreg[cpu->instr.rs2], &cpu->exc);
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

static void instr_divuw(riscv_cpu *cpu)
{
    uint32_t dividend = cpu->xreg[cpu->instr.rs1];
    uint32_t divisor = cpu->xreg[cpu->instr.rs2];

    if (divisor == 0) {
        /* TODO: set DZ (Divide by Zero) in the FCSR */

        // the quotient of division by zero has all bits set
        cpu->xreg[cpu->instr.rd] = -1;
    } else {
        cpu->xreg[cpu->instr.rd] = (int32_t)(dividend / divisor);
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

static void instr_sret(riscv_cpu *cpu)
{
    cpu->pc = read_csr(&cpu->csr, SEPC);
    /* When an SRET instruction is executed to return from the trap handler, the
     * privilege level is set to user mode if the SPP bit is 0, or supervisor
     * mode if the SPP bit is 1; SPP is then set to 0 */
    uint64_t sstatus = read_csr(&cpu->csr, SSTATUS);
    cpu->mode.mode = (sstatus & SSTATUS_SPP) >> 8;

    /* When an SRET instruction is executed, SIE is set to SPIE */
    /* Since SPIE is at bit 5 and SIE is at bit 1, so we right shift 4 bit for
     * correct value */
    write_csr(&cpu->csr, SSTATUS,
              (sstatus & ~SSTATUS_SIE) | ((sstatus & SSTATUS_SPIE) >> 4));
    /* SPIE is set to 1 */
    set_csr_bits(&cpu->csr, SSTATUS, SSTATUS_SPIE);
    /* SPP is set to 0 */
    clear_csr_bits(&cpu->csr, SSTATUS, SSTATUS_SPP);
}

static void instr_mret(riscv_cpu *cpu)
{
    cpu->pc = read_csr(&cpu->csr, MEPC);

    uint64_t mstatus = read_csr(&cpu->csr, MSTATUS);
    cpu->mode.mode = (mstatus & MSTATUS_MPP) >> 11;

    write_csr(&cpu->csr, MSTATUS,
              (mstatus & ~MSTATUS_MIE) | ((mstatus & MSTATUS_MPIE) >> 4));
    set_csr_bits(&cpu->csr, MSTATUS, MSTATUS_MPIE);
    clear_csr_bits(&cpu->csr, MSTATUS, MSTATUS_MPP);
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
    uint64_t tmp =
        read_bus(&cpu->bus, cpu->xreg[cpu->instr.rs1], 32, &cpu->exc);
    if (cpu->exc.exception != NoException) {
        assert(tmp == (uint64_t) -1);
        return;
    }
    if (!write_bus(&cpu->bus, cpu->xreg[cpu->instr.rs1], 32,
                   tmp + cpu->xreg[cpu->instr.rs2], &cpu->exc))
        return;
    cpu->xreg[cpu->instr.rd] = tmp;
}

static void instr_amoswapw(riscv_cpu *cpu)
{
    uint64_t tmp =
        read_bus(&cpu->bus, cpu->xreg[cpu->instr.rs1], 32, &cpu->exc);
    if (cpu->exc.exception != NoException) {
        assert(tmp == (uint64_t) -1);
        return;
    }
    if (!write_bus(&cpu->bus, cpu->xreg[cpu->instr.rs1], 32,
                   cpu->xreg[cpu->instr.rs2], &cpu->exc))
        return;
    cpu->xreg[cpu->instr.rd] = tmp;
}

static void instr_amoaddd(riscv_cpu *cpu)
{
    uint64_t tmp =
        read_bus(&cpu->bus, cpu->xreg[cpu->instr.rs1], 64, &cpu->exc);
    if (cpu->exc.exception != NoException) {
        assert(tmp == (uint64_t) -1);
        return;
    }
    if (!write_bus(&cpu->bus, cpu->xreg[cpu->instr.rs1], 64,
                   tmp + cpu->xreg[cpu->instr.rs2], &cpu->exc))
        return;
    cpu->xreg[cpu->instr.rd] = tmp;
}

static void instr_amoswapd(riscv_cpu *cpu)
{
    uint64_t tmp =
        read_bus(&cpu->bus, cpu->xreg[cpu->instr.rs1], 64, &cpu->exc);
    if (cpu->exc.exception != NoException) {
        assert(tmp == (uint64_t) -1);
        return;
    }
    if (!write_bus(&cpu->bus, cpu->xreg[cpu->instr.rs1], 64,
                   cpu->xreg[cpu->instr.rs2], &cpu->exc))
        return;
    cpu->xreg[cpu->instr.rd] = tmp;
}

/* clang-format off */
#define INIT_RISCV_INSTR_LIST(_type, _instr)  \
    static riscv_instr_desc _instr##_list = { \
        {_type}, sizeof(_instr) / sizeof(_instr[0]), _instr}

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
    [0x10] = {NULL, instr_srai, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC7_S, instr_srli_srai_type);

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

static riscv_instr_entry instr_srl_divu_sra_type[] = {
    [0x00] = {NULL, instr_srl, NULL},
    [0x01] = {NULL, instr_divu, NULL},
    [0x20] = {NULL, instr_sra, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC7, instr_srl_divu_sra_type);

static riscv_instr_entry instr_or_type[] = {
    [0x00] = {NULL, instr_or, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC7, instr_or_type);

static riscv_instr_entry instr_and_remu_type[] = {
    [0x00] = {NULL, instr_and, NULL},
    [0x01] = {NULL, instr_remu, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC7, instr_and_remu_type);

static riscv_instr_entry instr_reg_type[] = {
    [0x0] = {NULL, NULL, &instr_add_mul_sub_type_list},
    [0x1] = {NULL, NULL, &instr_sll_type_list},
    [0x2] = {NULL, NULL, &instr_slt_type_list},
    [0x3] = {NULL, NULL, &instr_sltu_type_list},
    [0x4] = {NULL, NULL, &instr_xor_type_list},
    [0x5] = {NULL, NULL, &instr_srl_divu_sra_type_list},
    [0x6] = {NULL, NULL, &instr_or_type_list},
    [0x7] = {NULL, NULL, &instr_and_remu_type_list}
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

static riscv_instr_entry instr_srlw_divuw_sraw_type[] = {
    [0x00] = {NULL, instr_srlw, NULL},
    [0x01] = {NULL, instr_divuw, NULL},
    [0x20] = {NULL, instr_sraw, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC7, instr_srlw_divuw_sraw_type);

static riscv_instr_entry instr_remuw_type[] = {
    [0x01] =  {NULL, instr_remuw, NULL}
};
INIT_RISCV_INSTR_LIST(FUNC7, instr_remuw_type);

static riscv_instr_entry instr_regw_type[] = {
    [0x0] = {NULL, NULL, &instr_addw_subw_type_list},
    [0x1] = {NULL, NULL, &instr_sllw_type_list},
    [0x5] = {NULL, NULL, &instr_srlw_divuw_sraw_type_list},
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

static riscv_instr_entry instr_sret_mret_type[] = {
    [0x08] = {NULL, instr_sret, NULL},
    [0x18] = {NULL, instr_mret, NULL},
};
INIT_RISCV_INSTR_LIST(FUNC7, instr_sret_mret_type);

static riscv_instr_entry instr_ret_type[] = {
     [0x2] = {NULL, NULL, &instr_sret_mret_type_list},
};
INIT_RISCV_INSTR_LIST(RS2, instr_ret_type);

static riscv_instr_entry instr_csr_type[] = {
     [0x0] = {NULL, NULL, &instr_ret_type_list},
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
    [0x0f] = {I_decode, instr_fence, NULL},
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
    case FUNC7_S:
        index = cpu->instr.funct7 >> 1;
        break;
    case FUNC7:
        index = cpu->instr.funct7;
        break;
    case RS2:
        index = cpu->instr.rs2;
        break;
    default:
        LOG_ERROR("Invalid index type\n");
        /* we don't change the variable of exception number here, since
         * this should only happens when our emulator's implementation error*/
        return false;
    }

    if (index >= instr_desc->size) {
        LOG_ERROR(
            "Not implemented or invalid instruction:\n"
            "opcode = 0x%x funct3 = 0x%x funct7 = 0x%x at pc %lx\n",
            cpu->instr.opcode, cpu->instr.funct3, cpu->instr.funct7, cpu->pc);
        cpu->exc.exception = IllegalInstruction;
        return false;
    }

    riscv_instr_entry entry = instr_desc->instr_list[index];

    if (entry.decode_func)
        entry.decode_func(cpu);

    if (entry.decode_func == NULL && entry.exec_func == NULL &&
        entry.next == NULL) {
        LOG_ERROR(
            "@ Not implemented or invalid instruction:\n"
            "opcode = 0x%x funct3 = 0x%x funct7 = 0x%x at pc %lx\n",
            cpu->instr.opcode, cpu->instr.funct3, cpu->instr.funct7, cpu->pc);
        cpu->exc.exception = IllegalInstruction;
        return false;
    }

    if (entry.next != NULL)
        return __decode(cpu, entry.next);
    else
        cpu->exec_func = entry.exec_func;

    return true;
}

static inline riscv_virtq_desc load_desc(riscv_cpu *cpu, uint64_t addr)
{
    uint64_t desc_addr = read_bus(&cpu->bus, addr, 64, &cpu->exc);
    uint64_t tmp = read_bus(&cpu->bus, addr + 8, 64, &cpu->exc);
    return (riscv_virtq_desc){.addr = desc_addr,
                              .len = tmp & 0xffffffff,
                              .flags = (tmp >> 32) & 0xffff,
                              .next = (tmp >> 48) & 0xffff};
}

static bool access_disk(riscv_cpu *cpu)
{
    riscv_virtio_blk *virtio_blk = &cpu->bus.virtio_blk;

    assert(virtio_blk->queue_sel == 0);

    /* the interrupt was asserted because the device has used a buffer
     * in at least one of the active virtual queues. */
    virtio_blk->isr |= 0x1;

    uint64_t desc = virtio_blk->vq[0].desc;
    uint64_t avail = virtio_blk->vq[0].avail;
    uint64_t used = virtio_blk->vq[0].used;

    /* idx field indicates where the driver would put the next descriptor entry
     * in the ring (modulo the queue size). This starts at 0, and increases. */
    int idx = read_bus(&cpu->bus, avail + 2, 16, &cpu->exc);
    int desc_offset = read_bus(&cpu->bus, avail + 4 + idx / VIRTQUEUE_MAX_SIZE,
                               16, &cpu->exc);

    /* MUST use a single 8-type descriptor containing type, reserved and sector,
     * followed by descriptors for data, then finally a separate 1-byte
     * descriptor for status. */
    riscv_virtq_desc desc0 =
        load_desc(cpu, desc + sizeof(riscv_virtq_desc) * desc_offset);

    riscv_virtq_desc desc1 =
        load_desc(cpu, desc + sizeof(riscv_virtq_desc) * desc0.next);

    riscv_virtq_desc desc2 =
        load_desc(cpu, desc + sizeof(riscv_virtq_desc) * desc1.next);

    assert(desc0.flags & VRING_DESC_F_NEXT);

    // take value of type and sector in field of struct virtio_blk_req
    int blk_req_type = read_bus(&cpu->bus, desc0.addr, 32, &cpu->exc);
    int blk_req_sector = read_bus(&cpu->bus, desc0.addr + 8, 64, &cpu->exc);
}

bool init_cpu(riscv_cpu *cpu,
              const char *filename,
              const char *rfs_name,
              bool is_elf)
{
    if (!init_bus(&cpu->bus, filename, rfs_name, is_elf))
        return false;

    if (!init_csr(&cpu->csr))
        return false;

    cpu->mode.mode = MACHINE;
    cpu->exc.exception = NoException;
    cpu->irq.irq = NoInterrupt;

    memset(&cpu->instr, 0, sizeof(riscv_instr));
    memset(&cpu->xreg[0], 0, sizeof(uint64_t) * 32);

    cpu->pc = DRAM_BASE;
    cpu->xreg[2] = DRAM_BASE + DRAM_SIZE;
    cpu->exec_func = NULL;
    return true;
}

bool fetch(riscv_cpu *cpu)
{
    uint32_t instr = read_bus(&cpu->bus, cpu->pc, 32, &cpu->exc);
    if (cpu->exc.exception != NoException) {
        assert(instr == (uint32_t) -1);
        return false;
    }
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

    return true;
}

bool decode(riscv_cpu *cpu)
{
    return __decode(cpu, &opcode_type_list);
}

bool exec(riscv_cpu *cpu)
{
    cpu->exec_func(cpu);

    // Emulate register x0 to 0
    cpu->xreg[0] = 0;

    if (cpu->exc.exception != NoException)
        return false;

    /* FIXME: If all of our implementation are right, we don't actually need to
     * clean up the structure below. But for easily debugging purpose, we'll
     * reset all of the instruction-relatd structure now. */
    memset(&cpu->instr, 0, sizeof(riscv_instr));
    cpu->exec_func = NULL;

    return true;
}

Trap exception_take_trap(riscv_cpu *cpu)
{
    uint64_t exc_pc = cpu->pc - 4;
    riscv_mode prev_mode = cpu->mode;

    uint8_t cause = cpu->exc.exception;

    /* certain exceptions and interrupts can be processed directly by a lower
     * privilege level within medeleg or mideleg register */

    /* For medeleg, the index of the bit position equal to the value returned in
     * the mcause register */
    uint64_t medeleg = read_csr(&cpu->csr, MEDELEG);
    if ((cpu->mode.mode <= SUPERVISOR) && (((medeleg >> cause) & 1) != 0)) {
        cpu->mode.mode = SUPERVISOR;

        /* The last two bits of stvec indicate the vector mode, which make
         * different for pc setting when interrupt. For (synchronous) exception,
         * no matter what the mode is, always set pc to BASE by stvec when
         * exception. */
        cpu->pc = read_csr(&cpu->csr, STVEC) & ~0x3;
        /* The low bit of sepc(sepc[0]) is always zero. When a trap is taken
         * into S-mode, sepc is written with the virtual address of the
         * instruction that was interrupted or that encountered the exception.
         */
        write_csr(&cpu->csr, SEPC, exc_pc & ~0x1);
        /* When a trap is taken into S-mode, scause is written with a code
         * indicating the event that caused the trap. */
        write_csr(&cpu->csr, SCAUSE, cause);

        /* FIXME: write stval with exception-specific information to assist
         * software in handling the trap.*/
        write_csr(&cpu->csr, STVAL, 0);

        /* When a trap is taken into supervisor mode, SPIE is set to SIE */
        uint64_t sstatus = read_csr(&cpu->csr, SSTATUS);
        write_csr(&cpu->csr, SSTATUS,
                  (sstatus & ~SSTATUS_SPIE) | ((sstatus & SSTATUS_SIE) << 4));
        /* SIE is set to 0 */
        clear_csr_bits(&cpu->csr, SSTATUS, SSTATUS_SIE);
        /* When a trap is taken, SPP is set to 0 if the trap originated from
         * user mode, or 1 otherwise. */
        sstatus = read_csr(&cpu->csr, SSTATUS);
        write_csr(&cpu->csr, SSTATUS,
                  (sstatus & ~SSTATUS_SPP) | prev_mode.mode << 8);
    } else {
        /* Handle in machine mode. You can see that the process is similar to
         * handle in supervisor mode. */
        cpu->mode.mode = MACHINE;
        cpu->pc = read_csr(&cpu->csr, MTVEC) & ~0x3;
        write_csr(&cpu->csr, MEPC, exc_pc & ~0x1);
        write_csr(&cpu->csr, MCAUSE, cause);
        /* FIXME: write mtval with exception-specific information to assist
         * software in handling the trap.*/
        write_csr(&cpu->csr, MTVAL, 0);
        uint64_t mstatus = read_csr(&cpu->csr, MSTATUS);
        write_csr(&cpu->csr, MSTATUS,
                  (mstatus & ~MSTATUS_MPIE) | ((mstatus & MSTATUS_MIE) << 4));
        clear_csr_bits(&cpu->csr, MSTATUS, MSTATUS_MIE);
        mstatus = read_csr(&cpu->csr, MSTATUS);
        write_csr(&cpu->csr, MSTATUS,
                  (mstatus & ~MSTATUS_MPP) | prev_mode.mode << 11);
    }

    // https://github.com/d0iasm/rvemu/blob/master/src/exception.rs
    switch (cpu->exc.exception) {
    case InstructionAddressMisaligned:
    case InstructionAccessFault:
        return Trap_Fatal;
    case IllegalInstruction:
        return Trap_Invisible;
    case Breakpoint:
        return Trap_Requested;
    case LoadAddressMisaligned:
    case LoadAccessFault:
    case StoreAMOAddressMisaligned:
    case StoreAMOAccessFault:
        return Trap_Fatal;
    case EnvironmentCallFromUMode:
    case EnvironmentCallFromSMode:
    case EnvironmentCallFromMMode:
        return Trap_Requested;
    case InstructionPageFault:
    case LoadPageFault:
    case StoreAMOPageFault:
        return Trap_Invisible;
    default:
        LOG_ERROR("Not defined exception!");
        return Trap_Fatal;
    }
}

void interrput_take_trap(riscv_cpu *cpu)
{
    uint64_t irq_pc = cpu->pc;
    riscv_mode prev_mode = cpu->mode;

    uint8_t cause = cpu->exc.exception;

    uint64_t mideleg = read_csr(&cpu->csr, MIDELEG);

    if ((cpu->mode.mode <= SUPERVISOR) && (((mideleg >> cause) & 1) != 0)) {
        cpu->mode.mode = SUPERVISOR;

        uint64_t stvec = read_csr(&cpu->csr, STVEC);
        if (stvec & 0x1)
            cpu->pc = (stvec & ~0x3) + 4 * cause;
        else
            cpu->pc = stvec & ~0x3;

        write_csr(&cpu->csr, SEPC, irq_pc & ~0x1);
        /* The Interrupt bit in the scause register is set if the trap was
         * caused by an interrupt */
        write_csr(&cpu->csr, SCAUSE, 1UL << 63 | cause);

        /* FIXME: write stval with exception-specific information to assist
         * software in handling the trap.*/
        write_csr(&cpu->csr, STVAL, 0);

        uint64_t sstatus = read_csr(&cpu->csr, SSTATUS);
        write_csr(&cpu->csr, SSTATUS,
                  (sstatus & ~SSTATUS_SPIE) | ((sstatus & SSTATUS_SIE) << 4));
        clear_csr_bits(&cpu->csr, SSTATUS, SSTATUS_SIE);
        sstatus = read_csr(&cpu->csr, SSTATUS);
        write_csr(&cpu->csr, SSTATUS,
                  (sstatus & ~SSTATUS_SPP) | prev_mode.mode << 8);
    } else {
        cpu->mode.mode = MACHINE;

        uint64_t mtvec = read_csr(&cpu->csr, MTVEC);
        if (mtvec & 0x1)
            cpu->pc = (mtvec & ~0x3) + 4 * cause;
        else
            cpu->pc = mtvec & ~0x3;

        write_csr(&cpu->csr, MEPC, irq_pc & ~0x1);
        write_csr(&cpu->csr, MCAUSE, 1UL << 63 | cause);
        /* FIXME: write stval with exception-specific information to assist
         * software in handling the trap.*/
        write_csr(&cpu->csr, MTVAL, 0);
        uint64_t mstatus = read_csr(&cpu->csr, MSTATUS);
        write_csr(&cpu->csr, MSTATUS,
                  (mstatus & ~MSTATUS_MPIE) | ((mstatus & MSTATUS_MIE) << 4));
        clear_csr_bits(&cpu->csr, MSTATUS, MSTATUS_MIE);
        mstatus = read_csr(&cpu->csr, MSTATUS);
        write_csr(&cpu->csr, MSTATUS,
                  (mstatus & ~MSTATUS_MPP) | prev_mode.mode << 11);
    }
}

bool check_pending_irq(riscv_cpu *cpu)
{
    /* TODO: concern USER mode of every step in pending interrupt handling for
     * widely usage */

    /* FIXME:
     * 1. Priority of interrupt should be considered
     * 2. The interactive between PLIC and interrput are not fully implement, I
     * should dig in more to perform better emulation.
     */
    int irq = 0;
    if (uart_is_interrupt(&cpu->bus.uart)) {
        irq = UART0_IRQ;
    }

    if (!irq) {
        // pending the external interrput bit
        set_csr_bits(&cpu->csr, MIP, MIP_SEIP);
    }

    /* FIXME: I am coufusing for the MIDELEG mechanism. According to the
     * document:
     *
     * > When a hart is executing in privilege mode x, interrupts are globally
     * > enabled when x IE=1 and globally disabled when x IE=0. Interrupts for
     * > lower-privilege modes, w<x, are always globally disabled regardless of
     * > the setting of the lower-privilege mode’s global w IE bit. Interrupts
     * for > higher-privilege modes, y>x, are always globally enabled regardless
     * of the > setting of the higher-privilege mode’s global y IE bit.
     *
     * But the document also say that:
     * > By default, all traps at any privilege level are handled in machine
     * mode... > To increase performance, implementations can provide individual
     * read/write bits > within medeleg and mideleg to indicate that certain
     * exceptions and interrupts > should be processed directly by a lower
     * privilege level.
     *
     * Then what will happen when bit in MIDELEG is not set and in the
     * SUPERVISOR mode? Does CPU take the interrupt but not handling the causing
     * trap? On the other hand, it also say that:
     *
     * > If bit i in MIDELEG is set, however, interrupts are considered to be
     * globally > enabled if the hart’s current privilege mode equals the
     * delegated privilege mode > (S or U) and that mode’s interrupt enable bit
     * (SIE or UIE in mstatus) is set, > or if the current privilege mode is
     * less than the delegated privilege mode.
     *
     * Does this mean that when bit i in MIDELEG is set and SIE is not set, the
     * MACHINE level interrput should not be taken ? I should make sure I
     * cosider the delegated priviledge mode by MIDELEG correctly */
    uint64_t pending = read_csr(&cpu->csr, MIE) & read_csr(&cpu->csr, MIP);

    if (cpu->mode.mode == MACHINE) {
        if (!check_csr_bit(&cpu->csr, MSTATUS, MSTATUS_MIE)) {
            cpu->irq.irq = NoInterrupt;
            return false;
        }
    }

    if (!(pending & MIP_MEIP)) {
        clear_csr_bits(&cpu->csr, MIP, MIP_MEIP);
        cpu->irq.irq = MachineExternalInterrupt;
        return true;
    }
    if (!(pending & MIP_MSIP)) {
        clear_csr_bits(&cpu->csr, MIP, MIP_MSIP);
        cpu->irq.irq = MachineSoftwareInterrupt;
        return true;
    }
    if (!(pending & MIP_MTIP)) {
        clear_csr_bits(&cpu->csr, MIP, MIP_MTIP);
        cpu->irq.irq = MachineTimerInterrupt;
        return true;
    }

    if (cpu->mode.mode == SUPERVISOR) {
        if (!check_csr_bit(&cpu->csr, SSTATUS, SSTATUS_SIE)) {
            cpu->irq.irq = NoInterrupt;
            return false;
        }
    }

    if (!(pending & MIP_SEIP)) {
        /* perform an interrupt claim and atomically clear
         * the corresponding pending bit */
        write_bus(&cpu->bus, PLIC_CLAIM_1, 32, irq, &cpu->exc);
        assert(cpu->exc.exception == NoException);

        clear_csr_bits(&cpu->csr, MIP, MIP_SEIP);
        cpu->irq.irq = SupervisorExternalInterrupt;
        return true;
    }
    if (!(pending & MIP_SSIP)) {
        clear_csr_bits(&cpu->csr, MIP, MIP_SSIP);
        cpu->irq.irq = SupervisorSoftwareInterrupt;
        return true;
    }
    if (!(pending & MIP_STIP)) {
        clear_csr_bits(&cpu->csr, MIP, MIP_STIP);
        cpu->irq.irq = SupervisorTimerInterrupt;
        return true;
    }

    return false;
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
