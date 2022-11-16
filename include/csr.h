#ifndef RISCV_CSR
#define RISCV_CSR

#include <stdbool.h>
#include <stdint.h>

#define CSR_CAPACITY 0x1000

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
// Counter-Enable register.
#define SCOUNTEREN 0x106
/// Scratch register for supervisor trap handlers.
#define SSCRATCH 0x140
/// Supervisor exception program counter.
#define SEPC 0x141
/// Supervisor trap cause.
#define SCAUSE 0x142
/// Supervisor bad address or instruction.
#define STVAL 0x143
// Supervisor interrupt pending.
#define SIP 0x144
// Supervisor address translation and protection.
#define SATP 0x180

// Machine status register.
#define MSTATUS 0x300
/// Machine exception delefation register.
#define MEDELEG 0x302
// Machine interrupt delefation register.
#define MIDELEG 0x303
// Machine interrupt-enable register.
#define MIE 0x304
// Machine trap-handler base address.
#define MTVEC 0x305
// Machine ISA Register.
#define MISA 0x301
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

// Physical memory protection configuration.
#define PMPCFG0 0x3a0
// Physical memory protection address register.
#define PMPADDR0 0x3b0
#define PMPADDR1 0x3b1
#define PMPADDR2 0x3b2
#define PMPADDR3 0x3b3

// Cycle counter for RDCYCLE instruction.
#define CYCLE 0xc00
/* FIXME:
 * 1. timer should be simulated for this register
 * 2. should we invalid write for this register? */
// Timer for RDTIME instruction.
#define TIME 0xc01

// SSTATUS fields
#define SSTATUS_UIE 0x1UL
#define SSTATUS_SIE 0x2UL
#define SSTATUS_UPIE 0x10UL
#define SSTATUS_SPIE 0x20UL
#define SSTATUS_SPP 0x100UL
#define SSTATUS_FS 0x6000UL
#define SSTATUS_XS 0x18000UL
#define SSTATUS_SUM 0x40000UL
#define SSTATUS_MXR 0x80000UL
#define SSTATUS_UXL 0x300000000UL
#define SSTATUS_UXL_64BIT 0x200000000UL
#define SSTATUS_VISIBLE                                                   \
    (SSTATUS_SIE | SSTATUS_SPIE | SSTATUS_SPP | SSTATUS_FS | SSTATUS_XS | \
     SSTATUS_SUM | SSTATUS_MXR | SSTATUS_UXL)
#define SSTATUS_WRITABLE \
    (SSTATUS_SIE | SSTATUS_SPIE | SSTATUS_SPP | SSTATUS_SUM | SSTATUS_MXR)


// MSTATUS fields
#define MSTATUS_MIE 0x8UL
#define MSTATUS_MPIE 0x80UL
#define MSTATUS_MPP 0x1800UL
#define MSTATUS_MPRV 0x20000UL
#define MSTATUS_WRITABLE \
    (MSTATUS_MIE | MSTATUS_MPIE | MSTATUS_MPP | SSTATUS_WRITABLE)

// MIP fields
#define MIP_SSIP 0x2UL
#define MIP_MSIP 0x8UL
#define MIP_STIP 0x20UL
#define MIP_MTIP 0x80UL
#define MIP_SEIP 0x200UL
#define MIP_MEIP 0x800UL

// SIP fields
#define SIP_USIP 0x1UL
#define SIP_SSIP 0x2UL
#define SIP_UTIP 0x10UL
#define SIP_STIP 0x20UL
#define SIP_UEIP 0x100UL
#define SIP_SEIP 0x200UL
// All bits besides SSIP, USIP, and UEIP in the sip register are read-only
#define SIP_WRITABLE (SIP_SSIP | SIP_USIP | SIP_UEIP)

// SATP fields
#define SATP_PPN 0xfffffffffffUL

// mask for csr delegable interrupt:
// - https://github.com/qemu/qemu/blob/master/target/riscv/csr.c
#define MIDELEG_WRITABLE (MIP_SSIP | MIP_STIP | MIP_SEIP)

// mask for naive usage
#define ALL_VALID 0xffffffffffffffffUL
#define ALL_INVALID 0x0

/* macro for setting and clearing csr bits */
#define set_csr_bits(csr, reg, mask) \
    write_csr(csr, reg, read_csr(csr, reg) | mask)

#define clear_csr_bits(csr, reg, mask) \
    write_csr(csr, reg, read_csr(csr, reg) & ~mask)

/* macro for checking csr bit(single bit only) */
#define check_csr_bit(csr, reg, mask) (!!((read_csr(csr, reg) & mask)))

typedef struct {
    uint64_t reg[CSR_CAPACITY];
} riscv_csr;

bool init_csr(riscv_csr *csr);
uint64_t read_csr(riscv_csr *csr, uint16_t addr);
void write_csr(riscv_csr *csr, uint16_t addr, uint64_t value);
void tick_csr(riscv_csr *csr);

#endif
