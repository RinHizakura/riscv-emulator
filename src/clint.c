#include "clint.h"
#include "exception.h"

uint64_t read_clint(riscv_clint *clint,
                    uint64_t addr,
                    uint8_t size,
                    riscv_exception *exc)
{
    uint64_t reg_value;
    uint8_t offset;

    if (addr >= CLINT_MSIP && addr < CLINT_MSIP + 4) {
        // remember that we use uint64_t to record 32 bits msip
        reg_value = clint->msip & 0xffffffff;
        offset = addr - CLINT_MSIP;
    } else if (addr >= CLINT_MTIMECMP && addr < CLINT_MTIMECMP + 8) {
        reg_value = clint->mtimecmp;
        offset = addr - CLINT_MTIMECMP;
    } else if (addr >= CLINT_MTIME && addr < CLINT_MTIME + 8) {
        reg_value = clint->mtime;
        offset = addr - CLINT_MTIME;
    } else {
        exc->exception = LoadAccessFault;
        return -1;
    }

    switch (size) {
    case 8:
    case 16:
    case 32:
        return (reg_value >> (offset << 3)) & ((1 << size) - 1);
    case 64:
        return reg_value;
    default:
        exc->exception = LoadAccessFault;
        LOG_ERROR("Invalid clint size!\n");
        return -1;
    }
}

bool write_clint(riscv_clint *clint,
                 uint64_t addr,
                 uint8_t size,
                 uint64_t value,
                 riscv_exception *exc)
{
    uint64_t *reg;
    uint8_t offset;

    if (addr >= CLINT_MSIP && addr < CLINT_MSIP + 4) {
        reg = &clint->msip;
        offset = addr - CLINT_MSIP;
    } else if (addr >= CLINT_MTIMECMP && addr < CLINT_MTIMECMP + 8) {
        reg = &clint->mtimecmp;
        offset = addr - CLINT_MTIMECMP;
    } else if (addr >= CLINT_MTIME && addr < CLINT_MTIME + 8) {
        reg = &clint->mtime;
        offset = addr - CLINT_MTIME;
    } else {
        exc->exception = StoreAMOAccessFault;
        return false;
    }

    switch (size) {
    case 8:
    case 16:
    case 32:
        // clear the target byte
        *reg &= ~(((1 << size) - 1) << (offset << 3));
        // set the new value to the target byte
        *reg |= (value & ((1 << size) - 1)) << (offset << 3);
        break;
    case 64:
        *reg = value;
        break;
    default:
        exc->exception = LoadAccessFault;
        LOG_ERROR("Invalid clint size!\n");
        return false;
    }

    return true;
}
