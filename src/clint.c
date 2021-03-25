#include "clint.h"
#include "exception.h"

uint64_t read_clint(riscv_clint *clint,
                    uint64_t addr,
                    uint8_t size,
                    riscv_exception *exc)
{
    if (size == 32) {
        if (addr & 0x3)
            goto read_clint_fail;

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
        else
            goto read_clint_fail;
    } else if (size == 64) {
        if (addr & 0x7)
            goto read_clint_fail;

        if (addr == CLINT_MTIMECMP)
            return clint->mtimecmp;
        else if (addr == CLINT_MTIME)
            return clint->mtime;
        else
            goto read_clint_fail;
    } else {
        goto read_clint_fail;
    }

read_clint_fail:
    LOG_ERROR("read clint addr %lx for size %d failed\n", addr, size);
    exc->exception = LoadAccessFault;
    return -1;
}

bool write_clint(riscv_clint *clint,
                 uint64_t addr,
                 uint8_t size,
                 uint64_t value,
                 riscv_exception *exc)
{
    if (size == 32) {
        if (addr & 0x3)
            goto write_clint_fail;

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
            goto write_clint_fail;
        }
    } else if (size == 64) {
        if (addr & 0x7)
            goto write_clint_fail;

        if (addr == CLINT_MTIMECMP)
            clint->mtimecmp = value;
        else if (addr == CLINT_MTIME)
            clint->mtime = value;
        else
            goto write_clint_fail;
    } else {
        goto write_clint_fail;
    }

    return true;

write_clint_fail:
    LOG_ERROR("write clint addr %lx for size %d failed\n", addr, size);
    exc->exception = StoreAMOAccessFault;
    return false;
}
