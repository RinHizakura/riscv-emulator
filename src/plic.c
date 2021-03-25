#include "plic.h"

uint64_t read_plic(riscv_plic *plic,
                   uint64_t addr,
                   uint8_t size,
                   riscv_exception *exc)
{
    // support only word-aligned and word size access now
    if ((size != 32) || (addr & 0x3))
        goto read_plic_fail;

    if (addr >= PLIC_PRIORITY && addr < PLIC_PRIORITY_END) {
        return plic->priority[(addr - PLIC_PRIORITY) / 4];
    }

    else if (addr >= PLIC_PENDING && addr < PLIC_PENDING_END) {
        return plic->pending[(addr - PLIC_PENDING) / 4];
    }

    else if (addr >= PLIC_ENABLE && addr < PLIC_ENABLE_END) {
        return plic->enable[(addr - PLIC_ENABLE) / 4];
    } else {
        switch (addr) {
        case PLIC_THRESHOLD_0:
            return plic->threshold[0];
        case PLIC_CLAIM_0:
            return plic->claim[0];
        case PLIC_THRESHOLD_1:
            return plic->threshold[1];
        case PLIC_CLAIM_1:
            return plic->claim[1];
        default:
            goto read_plic_fail;
        }
    }

read_plic_fail:
    LOG_ERROR("read plic addr %lx for size %d failed\n", addr, size);
    exc->exception = LoadAccessFault;
    return -1;
}

bool write_plic(riscv_plic *plic,
                uint64_t addr,
                uint8_t size,
                uint64_t value,
                riscv_exception *exc)
{
    // support only word-aligned and word size access now
    if ((size != 32) || (addr & 0x3))
        goto write_plic_fail;

    value &= 0xffffffff;

    if (addr >= PLIC_PRIORITY && addr < PLIC_PRIORITY_END) {
        plic->priority[(addr - PLIC_PRIORITY) / 4] = value;
    }

    else if (addr >= PLIC_PENDING && addr < PLIC_PENDING_END) {
        plic->pending[(addr - PLIC_PENDING) / 4] = value;
    }

    else if (addr >= PLIC_ENABLE && addr < PLIC_ENABLE_END) {
        plic->enable[(addr - PLIC_ENABLE) / 4] = value;
    } else {
        switch (addr) {
        case PLIC_THRESHOLD_0:
            plic->threshold[0] = value;
            break;
        case PLIC_CLAIM_0:
            plic->claim[0] = value;
            break;
        case PLIC_THRESHOLD_1:
            plic->threshold[1] = value;
            break;
        case PLIC_CLAIM_1:
            plic->claim[1] = value;
            break;
        default:
            goto write_plic_fail;
        }
    }
    return true;

write_plic_fail:
    LOG_ERROR("wrtie plic addr %lx for size %d failed\n", addr, size);
    exc->exception = StoreAMOAccessFault;
    return false;
}
