#ifdef ICACHE_CONFIG

#include <stdlib.h>
#include <string.h>

#include "icache.h"

bool init_icache(riscv_icache *icache)
{
    for (int set = 0; set < CACHE_SET_CNT; set++) {
        riscv_icache_entry *prev = NULL;

        for (int i = 0; i < CACHE_WAY_CNT; i++) {
            riscv_icache_entry *entry = malloc(sizeof(riscv_icache_entry));
            if (entry == NULL)
                return false;

            entry->valid = false;

            if (i == 0)
                icache->set[set].head = entry;
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
    uint64_t index = (addr >> 1) & (CACHE_SET_CNT - 1);
    uint64_t tag = (addr >> (1 + CACHE_INDEX_BIT));

    riscv_icache_entry *entry = icache->set[index].head;
    riscv_icache_entry *prev = NULL;

    for (int i = 0; i < CACHE_WAY_CNT; i++) {
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

void write_icache(riscv_icache *icache, uint64_t addr, riscv_instr instr)
{
    /* According to our policy, the last node in linked list should be the least
     * recently used, so it is the candidate to be replaced. The 'tail' pointer
     * can help us to access it faster */

    uint64_t index = (addr >> 1) & (CACHE_SET_CNT - 1);
    uint64_t tag = (addr >> (1 + CACHE_INDEX_BIT));

    riscv_icache_entry *entry = icache->set[index].head;
    riscv_icache_entry *prev = NULL;

    for (int i = 0; i < CACHE_WAY_CNT; i++) {
        if (entry->valid == true && entry->tag == tag) {
            if (prev != NULL)
                goto update_cache;
            else
                return;
        }

        // avoid update in last iteration
        if (i == CACHE_WAY_CNT - 1) {
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

void invalid_icache(riscv_icache *icache)
{
    for (int set = 0; set < CACHE_SET_CNT; set++) {
        riscv_icache_entry *entry = icache->set[set].head;

        for (int i = 0; i < CACHE_WAY_CNT; i++) {
            entry->valid = false;
            entry = entry->next;
        }
    }
}

void invalid_icache_by_vaddr(riscv_icache *icache, uint64_t vaddr)
{
    uint64_t vpn = vaddr >> 12;

    for (int set = 0; set < CACHE_SET_CNT; set++) {
        riscv_icache_entry *entry = icache->set[set].head;

        for (int i = 0; i < CACHE_WAY_CNT; i++) {
            // reconstruct the vaadr by index and tag
            uint64_t cur_vaddr =
                (entry->tag << (1 + CACHE_INDEX_BIT)) | (set << 1);
            if (cur_vaddr >> 12 == vpn)
                entry->valid = false;
            entry = entry->next;
        }
    }
}

void free_icache(riscv_icache *icache)
{
    for (int set = 0; set < CACHE_SET_CNT; set++) {
        riscv_icache_entry *entry = icache->set[set].head;

        for (int i = 0; i < CACHE_WAY_CNT; i++) {
            riscv_icache_entry *next = entry->next;
            free(entry);
            entry = next;
        }
    }
}

#endif
