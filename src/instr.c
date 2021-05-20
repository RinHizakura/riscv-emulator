#include "instr.h"

void R_decode(riscv_instr *instr)
{
    instr->rd = (instr->instr >> 7) & 0x1f;
    instr->rs1 = ((instr->instr >> 15) & 0x1f);
    instr->rs2 = ((instr->instr >> 20) & 0x1f);
    instr->funct3 = (instr->instr >> 12) & 0x7;
    instr->funct7 = (instr->instr >> 25) & 0x7f;
}

void I_decode(riscv_instr *instr)
{
    instr->rd = (instr->instr >> 7) & 0x1f;
    instr->rs1 = ((instr->instr >> 15) & 0x1f);
    instr->imm = (int32_t)(instr->instr & 0xfff00000) >> 20;
    instr->funct3 = (instr->instr >> 12) & 0x7;
    instr->funct7 = (instr->instr >> 25) & 0x7f;
}

/* This function is used for priviledge instruction. It is
 * similar to I-type decoding and the only different is:
 * 1. the sign extension of right shift on fied imm
 * 2. the parsing of field rs2
 */
void P_decode(riscv_instr *instr)
{
    instr->rd = (instr->instr >> 7) & 0x1f;
    instr->rs1 = ((instr->instr >> 15) & 0x1f);
    instr->rs2 = ((instr->instr >> 20) & 0x1f);
    instr->imm = (instr->instr & 0xfff00000) >> 20;
    instr->funct3 = (instr->instr >> 12) & 0x7;
    instr->funct7 = (instr->instr >> 25) & 0x7f;
}

void S_decode(riscv_instr *instr)
{
    instr->rs1 = ((instr->instr >> 15) & 0x1f);
    instr->rs2 = ((instr->instr >> 20) & 0x1f);
    /* note that we have to convert the right statement of "bitwise or" to
     * int32_t to avoid non-expected promotion. Same case in B type and J type
     * decoding */
    instr->imm = (((int32_t)(instr->instr & 0xfe000000) >> 20) |
                  (int32_t)((instr->instr >> 7) & 0x1f));
    instr->funct3 = (instr->instr >> 12) & 0x7;
}

void B_decode(riscv_instr *instr)
{
    instr->rs1 = ((instr->instr >> 15) & 0x1f);
    instr->rs2 = ((instr->instr >> 20) & 0x1f);
    // imm[12|10:5|4:1|11] = inst[31|30:25|11:8|7]
    instr->imm = ((int32_t)(instr->instr & 0x80000000) >> 19)  // 12
                 | (int32_t)((instr->instr & 0x80) << 4)       // 11
                 | (int32_t)((instr->instr >> 20) & 0x7e0)     // 10:5
                 | (int32_t)((instr->instr >> 7) & 0x1e);      // 4:1
    instr->funct3 = (instr->instr >> 12) & 0x7;
}

void U_decode(riscv_instr *instr)
{
    instr->rd = (instr->instr >> 7) & 0x1f;
    instr->imm = (int32_t)(instr->instr & 0xfffff000);
}

void J_decode(riscv_instr *instr)
{
    instr->rd = (instr->instr >> 7) & 0x1f;

    // imm[20|10:1|11|19:12] = inst[31|30:21|20|19:12]
    instr->imm = ((int32_t)(instr->instr & 0x80000000) >> 11)  // 20
                 | (int32_t)((instr->instr & 0xff000))         // 19:12
                 | (int32_t)((instr->instr >> 9) & 0x800)      // 11
                 | (int32_t)((instr->instr >> 20) & 0x7fe);    // 10:1
}

/* For C extension instruction decoding:
 * 1. Same opcode can relate to different type of instructions, so only funct*
 * is calculated first in Cx_decode.
 * 2. Since some immediate has distinct format on different instrutions,
 * decoding value will be delayed for actual instrution.
 */
void Cx_decode(riscv_instr *instr)
{
    instr->instr &= 0xffff;
    instr->funct3 = (instr->instr >> 13) & 0x7;
}

void Cxx_decode(riscv_instr *instr)
{
    instr->instr &= 0xffff;
    instr->funct6 = (instr->instr >> 10) & 0x3f;
    instr->funct2 = (instr->instr >> 5) & 0x3;
}

void CIW_decode(riscv_instr *instr)
{
    instr->instr &= 0xffff;
    instr->rd = ((instr->instr >> 2) & 0x7) + 8;
}

void CL_decode(riscv_instr *instr)
{
    instr->instr &= 0xffff;
    instr->rd = ((instr->instr >> 2) & 0x7) + 8;
    instr->rs1 = ((instr->instr >> 7) & 0x7) + 8;
}

void CS_decode(riscv_instr *instr)
{
    instr->instr &= 0xffff;
    instr->rs2 = ((instr->instr >> 2) & 0x7) + 8;
    instr->rs1 = ((instr->instr >> 7) & 0x7) + 8;
}

void CI_decode(riscv_instr *instr)
{
    instr->instr &= 0xffff;
    // note that 'rd' could also be used as 'rs1'
    instr->rd = (instr->instr >> 7) & 0x1f;
}

void CSS_decode(riscv_instr *instr)
{
    instr->instr &= 0xffff;
    instr->rs2 = (instr->instr >> 2) & 0x1f;
}

void CJ_decode(riscv_instr *instr)
{
    instr->instr &= 0xffff;
    // offset[11|4|9:8|10|6|7|3:1|5] = inst[12|11|10:9|8|7|6|5:3|2]
    instr->imm = ((instr->instr >> 1) & 0x800) | ((instr->instr << 2) & 0x400) |
                 ((instr->instr >> 1) & 0x300) | ((instr->instr << 1) & 0x80) |
                 ((instr->instr >> 1) & 0x40) | ((instr->instr << 3) & 0x20) |
                 ((instr->instr >> 7) & 0x10) | ((instr->instr >> 2) & 0xe);
}

void CB_decode(riscv_instr *instr)
{
    instr->instr &= 0xffff;
    instr->rs1 = ((instr->instr >> 7) & 0x7) + 8;
    instr->rd = instr->rs1;
}

void CA_decode(riscv_instr *instr)
{
    instr->instr &= 0xffff;
    instr->rd = ((instr->instr >> 7) & 0x7) + 8;
    instr->rs2 = ((instr->instr >> 2) & 0x7) + 8;
}

void CR_decode(riscv_instr *instr)
{
    instr->instr &= 0xffff;
    instr->funct4 = (instr->instr >> 12) & 0xf;
    instr->rs1 = (instr->instr >> 7) & 0x1f;
    instr->rd = instr->rs1;
    instr->rs2 = (instr->instr >> 2) & 0x1f;
}
