#include <stdlib.h>
#include <string.h>

#include "icache.h"

bool init_icache(riscv_icache *icache)
{
    for (int way = 0; way < CACHE_WAY; way++) {
        riscv_icache_entry *prev = NULL;

        for (int i = 0; i < SET_CACHELINE_CNT; i++) {
            riscv_icache_entry *entry = malloc(sizeof(riscv_icache_entry));
            if (entry == NULL)
                return false;

            entry->valid = false;

            if (i == 0)
                icache->set[way].head = entry;
            else
                prev->next = entry;

            prev = entry;
        }
        prev->next = NULL;
    }

    return true;
}

riscv_instr *read_icache(riscv_icache *icache, uint64_t addr)
{
    // since the address is a least 2 bytes, 1 bit is for offset
    uint64_t index = (addr >> 1) & (CACHE_WAY - 1);
    uint64_t tag = (addr >> (1 + CACHE_INDEX_BIT));

    riscv_icache_entry *entry = icache->set[index].head;
    riscv_icache_entry *prev = NULL;

    for (int i = 0; i < SET_CACHELINE_CNT; i++) {
        if (entry->valid == true && entry->tag == tag) {
            // pending the recently used entry to head
            if (prev != NULL) {
                prev->next = entry->next;
                entry->next = icache->set[index].head;
                icache->set[index].head = entry;
            }
            return &entry->instr;
        }

        prev = entry;
        entry = entry->next;
    }

    return NULL;
}

void write_cache(riscv_icache *icache, uint64_t addr, riscv_instr instr)
{
    /* According to our policy, the last node in linked list should be the least
     * recently used, so it is the candidate to be replaced. The 'tail' pointer
     * can help us to access it faster */

    uint64_t index = (addr >> 1) & (CACHE_WAY - 1);
    uint64_t tag = (addr >> (1 + CACHE_INDEX_BIT));

    riscv_icache_entry *entry = icache->set[index].head;
    riscv_icache_entry *prev = NULL;

    for (int i = 0; i < SET_CACHELINE_CNT; i++) {
        if (entry->valid == true && entry->tag == tag) {
            if (prev != NULL)
                goto update_cache;
            else
                return;
        }

        // avoid update in last iteration
        if (i == SET_CACHELINE_CNT - 1) {
            prev = entry;
            entry = entry->next;
        }
    }

    // if tag doesn't match any tag in cache, the candidate wiil be the last
    // entry
    entry->valid = true;
    entry->tag = tag;
    memcpy(&entry->instr, &instr, sizeof(riscv_instr));

update_cache:
    // after replacement, pending the entry to head
    prev->next = entry->next;
    entry->next = icache->set[index].head;
    icache->set[index].head = entry;
}

void flush_cache(riscv_icache *icache)
{
    for (int way = 0; way < CACHE_WAY; way++) {
        riscv_icache_entry *entry = icache->set[way].head;

        for (int i = 0; i < SET_CACHELINE_CNT; i++) {
            entry->valid = false;
            entry = entry->next;
        }
    }
}

void free_icache(riscv_icache *icache)
{
    for (int way = 0; way < CACHE_WAY; way++) {
        riscv_icache_entry *entry = icache->set[way].head;

        for (int i = 0; i < SET_CACHELINE_CNT; i++) {
            riscv_icache_entry *next = entry->next;
            free(entry);
            entry = next;
        }
    }
}
