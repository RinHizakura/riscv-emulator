#include <stdlib.h>
#include <string.h>

#include "boot.h"
#include "macros.h"
#include "memmap.h"

bool init_boot(riscv_boot *boot, uint64_t entry_addr)
{
    uint32_t reset_vec[] = {
        0x297,       // auipc t0, 0x0: write current pc to t0
        0x2028593,   // placeholder[addi   a1, t0, &dtb]: set dtb address to a1
        0xf1402573,  // csrr   a0, mhartid
        0x0182b283,  // ld     t0,24(t0): load ELF entry point address
        0x28067,     // jr     t0
        0,          entry_addr & 0xFFFFFFFF, (entry_addr >> 32)};

    size_t boot_mem_size = sizeof(reset_vec);
    boot->boot_mem = malloc(boot_mem_size);
    if (!boot->boot_mem) {
        LOG_ERROR("Error when allocating space through malloc for BOOT_DRAM\n");
        return false;
    }

    boot->boot_mem_size = boot_mem_size;
    // copy these boot rom instruction to specific address
    memcpy(boot->boot_mem, reset_vec, boot_mem_size);

    // FIXME: learning about device tree blob(dtb), I may have to generate it
    // for emulator.
    return true;
}

uint64_t read_boot(riscv_boot *boot,
                   uint64_t addr,
                   uint64_t size,
                   riscv_exception *exc)
{
    uint64_t index = (addr - BOOT_ROM_BASE);
    uint64_t value;

    switch (size) {
    case 8:
        value = boot->boot_mem[index];
        break;
    case 16:
        read_len(16, &boot->boot_mem[index], value);
        break;
    case 32:
        read_len(32, &boot->boot_mem[index], value);
        break;
    case 64:
        read_len(64, &boot->boot_mem[index], value);
        break;
    default:
        exc->exception = LoadAccessFault;
        LOG_ERROR("Invalid memory size!\n");
        return -1;
    }
    return value;
}

void free_boot(riscv_boot *boot)
{
    free(boot->boot_mem);
}
