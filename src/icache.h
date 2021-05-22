#ifndef RISCV_ICACHE
#define RISCV_ICACHE

#include "instr.h"

#ifdef ICACHE_CONFIG

#define CACHE_WAY 8
#define CACHE_INDEX_BIT 3
#define SET_CACHELINE_CNT 4

#include <stdbool.h>

struct ICACHE_ENTRY {
    riscv_instr instr;
    bool valid;
    uint64_t tag;

    struct ICACHE_ENTRY *next;
};
typedef struct ICACHE_ENTRY riscv_icache_entry;

typedef struct {
    riscv_icache_entry *head;
} riscv_icache_set;

typedef struct {
    riscv_icache_set set[CACHE_WAY];
} riscv_icache;

bool init_icache(riscv_icache *icache);
riscv_instr *read_icache(riscv_icache *icache, uint64_t addr);
void write_icache(riscv_icache *icache, uint64_t addr, riscv_instr instr);
void flush_icache(riscv_icache *icache);
void free_icache(riscv_icache *icache);

#endif /* ICACHE_CONFIG */
#endif /* RISCV_ICACHE */
