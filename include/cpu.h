#ifndef RISCV_CPU
#define RISCV_CPU

#include "bus.h"
#include "csr.h"
#include "exception.h"
#include "icache.h"
#include "irq.h"
#include "pte.h"

typedef enum access Access;
enum access { Access_Instr, Access_Load, Access_Store };

typedef struct {
    enum { USER = 0x0, SUPERVISOR = 0x1, MACHINE = 0x3 } mode;
} riscv_mode;

/* These union is used for valid type punning on GCC. Note that it
 * could be incorrect on other compiler.
 *
 * See https://gcc.gnu.org/bugs/#nonbugs for more information */
typedef union {
    double f;
    uint64_t u;
} float32_reg_t;

typedef union {
    double f;
    uint64_t u;
} float64_reg_t;

typedef struct CPU {
    riscv_mode mode;
    riscv_exception exc;
    riscv_irq irq;
    riscv_instr instr;
    riscv_bus bus;
    riscv_csr csr;
#ifdef ICACHE_CONFIG
    riscv_icache icache;
#endif

    uint64_t xreg[32];
    float64_reg_t freg[32];
    uint64_t pc;
    // FIXME: we should maintain a reservation set but not a single u64
    uint64_t reservation;

    bool debug_mode;
} riscv_cpu;

/* the *_S type means a special form of index to map the instruction. You can
 * take a look at function __decode for more detail */
typedef struct {
    enum {
        OPCODE,
        FUNC2_S,
        FUNC3,
        FUNC4_S,
        FUNC5,
        FUNC6_S,
        FUNC7,
        FUNC7_S,
        RS2,
        WIDTH,
    } type;
} riscv_instr_desc_type;

typedef struct {
    riscv_instr_desc_type type;
    uint64_t size;
    struct INSTR_ENTRY *instr_list;
} riscv_instr_desc;

typedef struct INSTR_ENTRY {
    void (*decode_func)(riscv_instr *instr);
    void (*exec_func)(riscv_cpu *cpu);
    riscv_instr_desc *next;
    char *entry_name;
} riscv_instr_entry;

bool init_cpu(riscv_cpu *cpu, const char *filename, const char *rfs_name);
void cpu_set_debug_mode(riscv_cpu *cpu, bool debug_mode);
uint64_t read_cpu(riscv_cpu *cpu, uint64_t addr, uint8_t size);
bool write_cpu(riscv_cpu *cpu, uint64_t addr, uint8_t size, uint64_t value);
bool step_cpu(riscv_cpu *cpu);
void free_cpu(riscv_cpu *cpu);
#endif
