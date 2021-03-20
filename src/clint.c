#include "clint.h"
#include "exception.h"

uint64_t read_clint(riscv_clint *clint,
                    uint64_t addr,
                    uint8_t size,
                    riscv_exception *exc)
{
    // support only word-aligned and word size access now
    if (size != 32 || !(addr & 0x3)) {
        exc->exception = StoreAMOAccessFault;
        return false;
    }

    if (addr == CLINT_MSIP)
        return clint->msip;
    else if (addr == CLINT_MTIMECMP)
        return clint->mtimecmp & 0xFFFFFFFF;
    else if (addr == CLINT_MTIMECMP + 4)
        return (clint->mtimecmp >> 32) & 0xFFFFFFFF;
    else if (addr == CLINT_MTIME)
        return clint->mtime & 0xFFFFFFFF;
    else if (addr == CLINT_MTIME + 4)
        return (clint->mtime >> 32) & 0xFFFFFFFF;

    exc->exception = LoadAccessFault;
    return -1;
}

bool write_clint(riscv_clint *clint,
                 uint64_t addr,
                 uint8_t size,
                 uint64_t value,
                 riscv_exception *exc)
{
    // support only word-aligned and word size access now
    if (size != 32 || !(addr & 0x3)) {
        exc->exception = StoreAMOAccessFault;
        return false;
    }

    value &= 0xFFFFFFFF;

    if (addr == CLINT_MSIP) {
        clint->msip = value;
    } else if (addr == CLINT_MTIMECMP) {
        uint64_t timecmp_hi = clint->mtimecmp >> 32;
        clint->mtimecmp = timecmp_hi << 32 | value;
    } else if (addr == CLINT_MTIMECMP + 4) {
        uint64_t timecmp_lo = clint->mtimecmp & 0xFFFFFFFF;
        clint->mtimecmp = timecmp_lo | value << 32;
    } else if (addr == CLINT_MTIME) {
        uint64_t time_hi = clint->mtime >> 32;
        clint->mtime = time_hi << 32 | value;
    } else if (addr == CLINT_MTIME + 4) {
        uint64_t time_lo = clint->mtime & 0xFFFFFFFF;
        clint->mtime = time_lo | value << 32;
    } else {
        exc->exception = StoreAMOAccessFault;
        return false;
    }

    return true;
}
