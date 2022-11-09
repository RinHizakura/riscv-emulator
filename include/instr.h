#ifndef RISCV_INSTR
#define RISCV_INSTR

#include <stdint.h>

typedef struct CPU riscv_cpu;

/* FIXME: we are able to consider space complexity here:
 *  1. save more space by bit field
 *  2. we don't need that many funct* for each bits length */
typedef struct {
    uint32_t instr;
    uint8_t opcode;
    uint8_t rd;
    uint8_t rs1;
    uint8_t rs2;
    uint64_t imm;
    uint8_t funct2;
    uint8_t funct3;
    uint8_t funct4;
    uint8_t funct6;
    uint8_t funct7;

    void (*exec_func)(riscv_cpu *cpu);
} riscv_instr;

void R_decode(riscv_instr *instr);
void I_decode(riscv_instr *instr);
void P_decode(riscv_instr *instr);
void S_decode(riscv_instr *instr);
void B_decode(riscv_instr *instr);
void U_decode(riscv_instr *instr);
void J_decode(riscv_instr *instr);
void Cx_decode(riscv_instr *instr);
void Cxx_decode(riscv_instr *instr);
void CIW_decode(riscv_instr *instr);
void CL_decode(riscv_instr *instr);
void CS_decode(riscv_instr *instr);
void CI_decode(riscv_instr *instr);
void CSS_decode(riscv_instr *instr);
void CJ_decode(riscv_instr *instr);
void CB_decode(riscv_instr *instr);
void CA_decode(riscv_instr *instr);
void CR_decode(riscv_instr *instr);

#endif
