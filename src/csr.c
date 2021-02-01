#include "csr.h"

/* 1.The supervisor should only view CSR state that should be visible to a
 * supervisor-level operating system. In particular, there is no information
 * about the existence (or non-existence) of higher privilege levels (hypervisor
 * or machine) visible in the CSRs accessible by the supervisor.
 *
 * 2. Many supervisor CSRs are a subset of the equivalent machine-mode CSR, and
 * the machine- mode chapter should be read first to help understand the
 * supervisor-level CSR descriptions.
 */
uint64_t read_csr(riscv_csr *csr, uint16_t addr)
{
#ifdef DEBUG
    /* For debug mode, we will use a non effective way to access the
     * registers that we have implemented only. So we can know where's
     * the error if it happens. */

    /* TODO */
#else

    switch (addr) {
    /* The sstatus register is a subset of the mstatus register. In a
     * straightforward implementation, reading or writing any field in
     * sstatus is equivalent to reading or writing the homonymous field in
     * mstatus.*/
    case SSTATUS: {
        uint64_t mask = SSTATUS_SIE | SSTATUS_SPIE | SSTATUS_SPP | SSTATUS_FS |
                        SSTATUS_XS | SSTATUS_SUM | SSTATUS_MXR | SSTATUS_UXL;

        return csr->csr_reg[MSTATUS] & mask;
    }
    /* 1. The sip and sie registers are subsets of the mip and mie
     * registers. Reading any field, or writing any writable field, of
     * sip/sie effects a read or write of the homonymous field of mip/mie
     * 2. If an interrupt is delegated to privilege mode x by setting a bit
     * in the mideleg register, it becomes visible in the x ip register and
     * is maskable using the x ie register. Otherwise, the corresponding
     * bits in xip and xie appear to be hardwired to zero. */
    case SIE:
        return csr->csr_reg[MIE] & csr->csr_reg[MIDELEG];
    case SIP:
        return csr->csr_reg[MIP] & csr->csr_reg[MIDELEG];
    default:
        return csr->csr_reg[addr];
    }
#endif
}
