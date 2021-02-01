#ifndef RISCV_CSR
#define RISCV_CSR

#include <stdint.h>

#define CSR_SIZE 4096
// Supervisor status register.
#define SSTATUS 0x100
// Supervisor interrupt-enable register.
#define SIE 0x104
// Supervisor interrupt pending.
#define SIP 0x144

// Machine status register
#define MSTATUS 0x300
// Machine interrupt delefation register.
#define MIDELEG 0x303
// Machine interrupt-enable register.
#define MIE 0x304
// Machine interrupt pending.
#define MIP 0x344


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
#define SSTATUS_SD 0x300000000UL


typedef struct {
    uint64_t csr_reg[CSR_SIZE];
} riscv_csr;


uint64_t read_csr(riscv_csr *csr, uint16_t addr);
void write_csr(riscv_csr *csr, uint16_t addr, uint64_t value);

#endif
