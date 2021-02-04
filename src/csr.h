#ifndef RISCV_CSR
#define RISCV_CSR

#include <stdbool.h>
#include <stdint.h>

// Supervisor status register.
#define SSTATUS 0x100
// Supervisor exception delegation register.
#define SEDELEG 0x102
/// Supervisor interrupt delegation register.
#define SIDELEG 0x103
// Supervisor interrupt-enable register.
#define SIE 0x104
// Supervisor trap handler base address.
#define STVEC 0x105
/// Scratch register for supervisor trap handlers.
#define SSCRATCH 0x140
/// Supervisor exception program counter.
#define SEPC 0x141
/// Supervisor trap cause.
#define SCAUSE 0x142
/// Supervisor bad address or instruction.
#define STVALCsr 0x143
// Supervisor interrupt pending.
#define SIP 0x144
// Supervisor address translation and protection.
#define SATP 0x180

// Machine status register
#define MSTATUS 0x300
/// Machine exception delefation register.
#define MEDELEG 0x302
// Machine interrupt delefation register.
#define MIDELEG 0x303
// Machine interrupt-enable register.
#define MIE 0x304
// Machine trap-handler base address.
#define MTVEC 0x305
// Machine counter enable.
#define MCOUNTEREN 0x306
// Scratch register for machine trap handlers.
#define MSCRATCH 0x340
// Machine exception program counter.
#define MEPC 0x341
// Machine trap cause.
#define MCAUSE 0x342
/// Machine bad address or instruction.
#define MTVAL 0x343
// Machine interrupt pending.
#define MIP 0x344
// Hardware thread ID.
#define MHARTID 0xf14

// SSTATUS fields
#define SSTATUS_UIE 0x1
#define SSTATUS_SIE 0x2
#define SSTATUS_UPIE 0x10
#define SSTATUS_SPIE 0x20
#define SSTATUS_SPP 0x100
#define SSTATUS_FS 0x6000
#define SSTATUS_XS 0x18000
#define SSTATUS_SUM 0x40000
#define SSTATUS_MXR 0x80000
#define SSTATUS_UXL 0x300000000UL
#define SSTATUS_VISIBLE                                                  \
    SSTATUS_SIE | SSTATUS_SPIE | SSTATUS_SPP | SSTATUS_FS | SSTATUS_XS | \
        SSTATUS_SUM | SSTATUS_MXR | SSTATUS_UXL

// SIP fields
#define SIP_USIP 0x1
#define SIP_SSIP 0x2
#define SIP_UTIP 0x10
#define SIP_STIP 0x20
#define SIP_UEIP 0x100
#define SIP_SEIP 0x200
// All bits besides SSIP, USIP, and UEIP in the sip register are read-only
#define SIP_WRITABLE SIP_SSIP | SIP_USIP | SIP_UEIP

// mask for naive usage
#define ALL_VALID 0xffffffffffffffffUL
#define ALL_INVALID 0x0

/* FIXME: we don't perfectly deal with the read write permission now! */
/* clang-format off */
#define DECLARE_CSR_ENTRY(_name)                                        \
    riscv_csr_entry  _name [] = {                                       \
         [SSTATUS] =   {SSTATUS_VISIBLE, SSTATUS_VISIBLE, 0},           \
         [SEDELEG] =   {ALL_VALID,       ALL_VALID,       0},           \
         [SIE]     =   {ALL_VALID,       ALL_VALID,       0},           \
         [STVEC] =     {ALL_VALID,       ALL_VALID,       0},           \
         [SSCRATCH]    {ALL_VALID,       ALL_VALID,       0},           \
         [SEPC] =      {ALL_VALID,       ALL_VALID,       0},           \
         [SCAUSE] =    {ALL_VALID,       ALL_VALID,       0},           \
         [STVALCsr] =  {ALL_VALID,       ALL_VALID,       0},           \
         [SIP] =       {ALL_VALID,       SIP_WRITABLE,    0},           \
         [SATP] =      {ALL_VALID,       ALL_VALID,       0},           \
         [MSTATUS]=    {ALL_VALID,       ALL_VALID,       0},           \
         [MEDELEG]=    {ALL_VALID,       ALL_VALID,       0},           \
         [MIDELEG] =   {ALL_VALID,       ALL_VALID,       0},           \
         [MIE] =       {ALL_VALID,       ALL_VALID,       0},           \
         [MTVEC] =     {ALL_VALID,       ALL_VALID,       0},           \
         [MCOUNTEREN]= {ALL_VALID,       ALL_VALID,       0},           \
         [MSCRATCH]=   {ALL_VALID,       ALL_VALID,       0},           \
         [MEPC] =      {ALL_VALID,       ALL_VALID,       0},           \
         [MCAUSE] =    {ALL_VALID,       ALL_VALID,       0},           \
         [MTVAL] =     {ALL_VALID,       ALL_VALID,       0},           \
         [MIP] =       {ALL_VALID,       ALL_VALID,       0},           \
         [MHARTID] =   {ALL_VALID,       ALL_VALID,       0},           \
    }
/* clang-format on */

typedef struct {
    uint64_t read_mask;
    uint64_t write_mask;

    uint64_t value;
} riscv_csr_entry;

typedef struct {
    uint64_t size;
    riscv_csr_entry *list;
} riscv_csr;

bool init_csr(riscv_csr *csr);
uint64_t read_csr(riscv_csr *csr, uint16_t addr);
void write_csr(riscv_csr *csr, uint16_t addr, uint64_t value);
void free_csr(riscv_csr *csr);

#endif