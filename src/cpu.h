#ifndef RISCV_CPU
#define RISCV_CPU

#include "bus.h"
#include "csr.h"
#include "exception.h"
#include "irq.h"

typedef struct {
    enum { USER = 0x0, SUPERVISOR = 0x1, MACHINE = 0x3 } mode;
} riscv_mode;

/* FIXME: we are able to consider space complexity here */
typedef struct {
    uint32_t instr;
    uint8_t opcode;
    uint8_t rd;
    uint8_t rs1;
    uint8_t rs2;
    uint64_t imm;
    uint8_t funct3;
    uint8_t funct7;
} riscv_instr;

typedef struct CPU riscv_cpu;
typedef struct CPU {
    riscv_mode mode;
    riscv_exception exc;
    riscv_irq irq;
    riscv_instr instr;
    riscv_bus bus;
    riscv_csr csr;

    uint64_t xreg[32];
    uint64_t pc;

    void (*exec_func)(riscv_cpu *cpu);
} riscv_cpu;

typedef struct {
    enum {
        OPCODE,
        FUNC3,
        FUNC5,
        FUNC7,
        RS2,
    } type;
} riscv_instr_desc_type;

typedef struct {
    riscv_instr_desc_type type;
    uint64_t size;
    struct INSTR_ENTRY *instr_list;
} riscv_instr_desc;

typedef struct INSTR_ENTRY {
    void (*decode_func)(riscv_cpu *cpu);
    void (*exec_func)(riscv_cpu *cpu);
    riscv_instr_desc *next;
} riscv_instr_entry;

bool init_cpu(riscv_cpu *cpu, const char *filename, bool is_elf);
bool fetch(riscv_cpu *cpu);
bool decode(riscv_cpu *cpu);
bool exec(riscv_cpu *cpu);
bool check_pending_irq(riscv_cpu *cpu);
Trap exception_take_trap(riscv_cpu *cpu);
void interrput_take_trap(riscv_cpu *cpu);
void dump_reg(riscv_cpu *cpu);
void dump_csr(riscv_cpu *cpu);
void free_cpu(riscv_cpu *cpu);
#endif
