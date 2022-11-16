#include <stdlib.h>
#include <string.h>

#include "csr.h"

/* 1.The supervisor should only view CSR state that should be visible to a
 * supervisor-level operating system. In particular, there is no information
 * about the existence (or non-existence) of higher privilege levels (hypervisor
 * or machine) visible in the CSRs accessible by the supervisor.
 *
 * 2. Many supervisor CSRs are a subset of the equivalent machine-mode CSR, and
 * the machine- mode chapter should be read first to help understand the
 * supervisor-level CSR descriptions.
 *
 * 3. The sstatus register is a subset of the mstatus register. In a
 * straightforward implementation, reading or writing any field in
 * sstatus is equivalent to reading or writing the homonymous field in
 * mstatus.
 *
 * 4. The sip and sie registers are subsets of the mip and mie
 * registers. Reading any field, or writing any writable field, of
 * sip/sie effects a read or write of the homonymous field of mip/mie
 *
 * 5. If an interrupt is delegated to privilege mode x by setting a bit
 * in the mideleg register, it becomes visible in the x ip register and
 * is maskable using the x ie register. Otherwise, the corresponding
 * bits in xip and xie appear to be hardwired to zero.
 */

bool init_csr(riscv_csr *csr)
{
    memset(&csr->reg, 0, sizeof(uint64_t) * CSR_CAPACITY);

    uint64_t misa_val = (2UL << 62) |  // XLEN = 64
                        (1 << 20) |    // User mode implemented
                        (1 << 18) |    // Supervisor mode implemented
                        (1 << 12) |    // Integer Multiply/Divide extension
                        (1 << 8) |     // RV32I/64I/128I base ISA
                        (1 << 2) |     // Compressed extension)
                        1;             // Atomic extension
    write_csr(csr, MISA, misa_val);
    return true;
}

uint64_t read_csr(riscv_csr *csr, uint16_t addr)
{
    if (addr >= CSR_CAPACITY) {
        LOG_ERROR("Invalid CSR addr 0x%x\n", addr);
        return -1;
    }

    switch (addr) {
    case SSTATUS:
        return (csr->reg[MSTATUS] | SSTATUS_UXL_64BIT) & SSTATUS_VISIBLE;
    case SIE:
        return (csr->reg[MIE] & csr->reg[MIDELEG]);
    case SIP:
        return (csr->reg[MIP] & csr->reg[MIDELEG]);
    default:
        return csr->reg[addr];
    }
}

void write_csr(riscv_csr *csr, uint16_t addr, uint64_t value)
{
    if (addr >= CSR_CAPACITY) {
        LOG_ERROR("Invalid CSR addr 0x%x\n", addr);
        return;
    }

    switch (addr) {
    case SSTATUS: {
        uint64_t *mstatus = &csr->reg[MSTATUS];
        *mstatus = (*mstatus & ~SSTATUS_WRITABLE) | (value & SSTATUS_WRITABLE);
        break;
    }
    case SIE: {
        uint64_t *mie = &csr->reg[MIE];
        uint64_t mask = csr->reg[MIDELEG];
        *mie = (*mie & ~mask) | (value & mask);
        break;
    }
    case SIP: {
        uint64_t *mip = &csr->reg[MIP];
        uint64_t mask = csr->reg[MIDELEG] & SIP_WRITABLE;
        *mip = (*mip & ~mask) | (value & mask);
        break;
    }
    case MIDELEG: {
        uint64_t *mideleg = &csr->reg[MIDELEG];
        *mideleg = (*mideleg & ~MIDELEG_WRITABLE) | (value & MIDELEG_WRITABLE);
        break;
    }
    case MSTATUS: {
        uint64_t *mstatus = &csr->reg[MSTATUS];
        *mstatus = (*mstatus & ~MSTATUS_WRITABLE) | (value & MSTATUS_WRITABLE);
        break;
    }
    // read only CSR
    case MHARTID:
    case TIME:
        break;
    default:
        csr->reg[addr] = value;
    }
}

void tick_csr(riscv_csr *csr)
{
    csr->reg[TIME]++;
}
