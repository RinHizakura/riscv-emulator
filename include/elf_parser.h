#ifndef ELF_PARSER_H
#define ELF_PARSER_H

#include <elf.h>
#include <stddef.h>

typedef struct elf elf_t;
typedef union elf_inner elf_inner_t;

typedef struct {
    uint16_t next;
    uint32_t p_type;
} phdr_iter_t;

union elf_inner {
    Elf64_Ehdr *header;
    uint8_t *data;
};

struct elf {
    elf_inner_t *inner;
    size_t sz;
};

int elf_init(elf_t *elf, uint8_t *data, size_t sz);
int elf_lookup_shdr(elf_t *elf, char *name, Elf64_Shdr **output_sec_header);
int elf_lookup_symbol(elf_t *elf, char *symbol, Elf64_Sym **sym);
uint64_t elf_e_entry(elf_t *elf);
void elf_phdr_iter_start(phdr_iter_t *it, uint32_t p_type);
int elf_phdr_iter_next(elf_t *elf,
                       phdr_iter_t *it,
                       Elf64_Phdr **output_prog_header);
void elf_close(elf_t *elf);

#endif
