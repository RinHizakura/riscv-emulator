#ifndef RISCV_PTE
#define RISCV_PTE

#define PAGE_SHIFT 12

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
} pte_t;

typedef struct {
    int levels;
    int ptesize;

    uint64_t *(*create_vpn)(uint64_t addr);
    uint64_t *(*create_ppn)(uint64_t pte_ppn);
} sv_t;

static inline pte_t pte_new(uint64_t input)
{
    return (pte_t){
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

static inline uint64_t *sv39_create_vpn(uint64_t addr)
{
    static uint64_t vpn[3];
    vpn[0] = (addr >> 12) & 0x1ff;
    vpn[1] = (addr >> 21) & 0x1ff;
    vpn[2] = (addr >> 30) & 0x1ff;
    return vpn;
}

static inline uint64_t *sv39_create_ppn(uint64_t pte_ppn)
{
    static uint64_t ppn[3];
    ppn[0] = pte_ppn & 0x1ff;
    ppn[1] = (pte_ppn >> 9) & 0x1ff;
    ppn[2] = (pte_ppn >> 18) & 0x3ffffff;
    return ppn;
}

static inline uint64_t *sv48_create_vpn(uint64_t addr)
{
    static uint64_t vpn[4];
    vpn[0] = (addr >> 12) & 0x1ff;
    vpn[1] = (addr >> 21) & 0x1ff;
    vpn[2] = (addr >> 30) & 0x1ff;
    vpn[3] = (addr >> 39) & 0x1ff;
    return vpn;
}

static inline uint64_t *sv48_create_ppn(uint64_t pte_ppn)
{
    static uint64_t ppn[4];
    ppn[0] = pte_ppn & 0x1ff;
    ppn[1] = (pte_ppn >> 9) & 0x1ff;
    ppn[2] = (pte_ppn >> 18) & 0x1ff;
    ppn[3] = (pte_ppn >> 27) & 0x1ffff;
    return ppn;
}

static inline uint64_t *sv57_create_vpn(uint64_t addr)
{
    static uint64_t vpn[5];
    vpn[0] = (addr >> 12) & 0x1ff;
    vpn[1] = (addr >> 21) & 0x1ff;
    vpn[2] = (addr >> 30) & 0x1ff;
    vpn[3] = (addr >> 39) & 0x1ff;
    vpn[4] = (addr >> 48) & 0x1ff;
    return vpn;
}

static inline uint64_t *sv57_create_ppn(uint64_t pte_ppn)
{
    static uint64_t ppn[5];
    ppn[0] = pte_ppn & 0x1ff;
    ppn[1] = (pte_ppn >> 9) & 0x1ff;
    ppn[2] = (pte_ppn >> 18) & 0x1ff;
    ppn[3] = (pte_ppn >> 27) & 0x1ff;
    ppn[4] = (pte_ppn >> 36) & 0xff;
    return ppn;
}

#endif
