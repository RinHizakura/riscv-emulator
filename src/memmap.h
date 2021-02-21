#ifndef RISCV_MEMMAP
#define RISCV_MEMMAP

/// The address which DRAM starts
#define DRAM_BASE 0x80000000UL

#define CLINT_BASE 0x2000000UL
#define CLINT_END CLINT_BASE + 0x10000

#endif
