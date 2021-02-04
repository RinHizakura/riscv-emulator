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
    DECLARE_CSR_ENTRY(tmp_csr_entry);

    csr->size = sizeof(tmp_csr_entry) / sizeof(riscv_csr_entry);
    csr->list = malloc(sizeof(tmp_csr_entry));
    if (csr->list == NULL) {
        LOG_ERROR("Error when allocating space through malloc for csr map.\n");
        return false;
    }
    memcpy(csr->list, tmp_csr_entry, sizeof(tmp_csr_entry));
    return true;
}

uint64_t read_csr(riscv_csr *csr, uint16_t addr)
{
    if (addr >= csr->size) {
        LOG_ERROR("Not implemented or invalid CSR register 0x%x\n", addr);
        return -1;
    }

    riscv_csr_entry *csr_reg = &csr->list[addr];

    if (csr_reg->read_mask == ALL_INVALID) {
        LOG_ERROR("Not implemented or invalid CSR register 0x%x\n", addr);
        return -1;
    }

    switch (addr) {
    case SIE:
    case SIP: {
        riscv_csr_entry *mideleg = &csr->list[MIDELEG];
        return csr_reg->value & csr_reg->read_mask & mideleg->value;
    }
    default:
        return csr_reg->value & csr_reg->read_mask;
    }
}

void write_csr(riscv_csr *csr, uint16_t addr, uint64_t value)
{
    if (addr >= csr->size) {
        LOG_ERROR("Not implemented or invalid CSR register 0x%x\n", addr);
        return;
    }

    riscv_csr_entry *csr_reg = &csr->list[addr];

    if (csr_reg->write_mask == ALL_INVALID) {
        LOG_ERROR("Not implemented or invalid CSR register 0x%x\n", addr);
        return;
    }

    switch (addr) {
    case MSTATUS:
    case SSTATUS: {
        riscv_csr_entry *mstatus = &csr->list[MSTATUS];
        riscv_csr_entry *sstatus = &csr->list[SSTATUS];
        uint64_t mask = csr_reg->write_mask;
        sstatus->value = (value & sstatus->write_mask);
        mstatus->value = (mstatus->value & ~mask) | (value & mask);
    } break;
    case MIE: {
        riscv_csr_entry *sie = &csr->list[SIE];
        riscv_csr_entry *mideleg = &csr->list[MIDELEG];
        csr_reg->value = value & csr_reg->write_mask;
        sie->value = value & (sie->write_mask | mideleg->value);
    } break;
    case SIE: {
        riscv_csr_entry *mie = &csr->list[MIE];
        riscv_csr_entry *mideleg = &csr->list[MIDELEG];
        uint64_t mask = csr_reg->write_mask | mideleg->value;
        csr_reg->value = value & mask;
        mie->value = (mie->value & ~mask) | (value & mask);
    } break;
    case MIP: {
        riscv_csr_entry *sip = &csr->list[SIP];
        riscv_csr_entry *mideleg = &csr->list[MIDELEG];
        csr_reg->value = value & csr_reg->write_mask;
        sip->value = value & (sip->write_mask | mideleg->value);
    } break;
    case SIP: {
        riscv_csr_entry *mip = &csr->list[MIP];
        riscv_csr_entry *mideleg = &csr->list[MIDELEG];
        uint64_t mask = csr_reg->write_mask | mideleg->value;
        csr_reg->value = value & mask;
        mip->value = (mip->value & ~mask) | (value & mask);
    } break;
    default:
        csr_reg->value = value & csr_reg->write_mask;
    }
}

void free_csr(riscv_csr *csr)
{
    free(csr->list);
}
