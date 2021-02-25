#ifndef RISCV_MEMMAP
#define RISCV_MEMMAP

/* We define the memory mapping and mapping size according this:
 * - https://github.com/qemu/qemu/blob/master/hw/riscv/virt.c*/

#define DRAM_BASE 0x80000000UL

#define CLINT_BASE 0x2000000UL
#define CLINT_END CLINT_BASE + 0x10000

/*
 *
 * Since the size of PLIC is 'VIRT_PLIC_SIZE(VIRT_CPUS_MAX * 2)', according to
 * the definition of qemu:
 *
 *  - #define VIRT_CPUS_MAX 8
 *  - #define VIRT_PLIC_CONTEXT_BASE 0x200000
 *  - #define VIRT_PLIC_CONTEXT_STRIDE 0x1000
 *  - #define VIRT_PLIC_SIZE(__num_context) \
 *      (VIRT_PLIC_CONTEXT_BASE + (__num_context) * VIRT_PLIC_CONTEXT_STRIDE)
 *
 * So the result should be 0x200000 + (8 * 2) * 0x1000 = 0x216000(although we
 * havn't implement that much CPU)
 */

#define PLIC_BASE 0xc000000UL
#define PLIC_END PLIC_BASE + 0x216000

#endif
