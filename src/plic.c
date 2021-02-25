#include "plic.h"

uint64_t read_plic(riscv_plic *plic,
                   uint64_t addr,
                   uint8_t size,
                   riscv_exception *exc)
{
    // support only word-aligned and word size access now
    if ((size != 32) || ((addr & ~0x3) != addr)) {
        exc->exception = LoadAccessFault;
        return -1;
    }

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
            break;
        }
    }

    exc->exception = LoadAccessFault;
    return -1;
}

bool write_plic(riscv_plic *plic,
                uint64_t addr,
                uint8_t size,
                uint64_t value,
                riscv_exception *exc)
{
    // TODO
    return false;
}
