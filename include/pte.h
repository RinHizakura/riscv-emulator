#ifndef RISCV_PTE
#define RISCV_PTE

#define PAGE_SHIFT 12
#define LEVELS 3
// Sv39 page tables contain 2^9 page table entries (PTEs), eight bytes each.
#define PTESIZE 8

typedef struct {
    uint8_t v : 1;
    uint8_t r : 1;
    uint8_t w : 1;
    uint8_t x : 1;
    uint8_t u : 1;
    uint8_t g : 1;
    uint8_t a : 1;
    uint8_t d : 1;
    uint64_t ppn;
} sv39_pte_t;

static inline sv39_pte_t sv39_pte_new(uint64_t input)
{
    return (sv39_pte_t){
        .v = input & 1,
        .r = (input >> 1) & 1,
        .w = (input >> 2) & 1,
        .x = (input >> 3) & 1,
        .u = (input >> 4) & 1,
        .g = (input >> 5) & 1,
        .a = (input >> 6) & 1,
        .d = (input >> 7) & 1,
        .ppn = (input >> 10) & 0xfffffffffffUL,
    };
}

#endif
