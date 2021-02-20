#ifndef RISCV_CPU
#define RISCV_CPU

#include "bus.h"
#include "csr.h"

typedef struct {
    enum { USER = 0x0, SUPERVISOR = 0x1, MACHINE = 0x3 } mode;
} riscv_mode;

/* the term 'trap' refer to the transfer of control to a trap handler caused by
 * either an exception or an interrupt */
typedef enum trap Trap;
enum trap {
    /* The trap is visible to, and handled by, software running inside the
     * execution environment */
    Trap_Contained,
    /* The trap is a synchronous exception that is an explicit call to the
     * execution environment requesting an action on behalf of software
     * inside the execution environment */
    Trap_Requested,
    /* The trap is handled transparently by the execution environment and
     * execution resumes normally after the trap is handled */
    Trap_Invisible,
    /* The trap represents a fatal failure and causes the execution
     * environment to terminate execution */
    Trap_Fatal,
};

/* the term 'exception' refer to an unusual condition occurring at run time
associated with an instruction in the current RISC-V hart */
typedef struct {
    enum {
        InstructionAddressMisaligned = 0,
        InstructionAccessFault = 1,
        IllegalInstruction = 2,
        Breakpoint = 3,
        LoadAddressMisaligned = 4,
        LoadAccessFault = 5,
        StoreAMOAddressMisaligned = 6,
        StoreAMOAccessFault = 7,
        EnvironmentCallFromUMode = 8,
        EnvironmentCallFromSMode = 9,
        EnvironmentCallFromMMode = 11,
        InstructionPageFault = 12,
        LoadPageFault = 13,
        StoreAMOPageFault = 15,
        // extra number to present no exception for error checking
        NoException = 99,
    } exception;
} riscv_exception;

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

#define INIT_RISCV_INSTR_LIST(_type, _instr)  \
    static riscv_instr_desc _instr##_list = { \
        {_type}, sizeof(_instr) / sizeof(_instr[0]), _instr}

bool init_cpu(riscv_cpu *cpu, const char *filename);
bool fetch(riscv_cpu *cpu);
bool decode(riscv_cpu *cpu);
bool exec(riscv_cpu *cpu);
Trap take_trap(riscv_cpu *cpu);
void dump_reg(riscv_cpu *cpu);
void dump_csr(riscv_cpu *cpu);
void free_cpu(riscv_cpu *cpu);
#endif
