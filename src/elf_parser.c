#include "elf_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

static inline Elf64_Ehdr *get_elf_header(uint8_t *elf_file)
{
    return (Elf64_Ehdr *) elf_file;
}

static inline Elf64_Shdr *get_section_header(uint8_t *elf_file,
                                             Elf64_Ehdr *elf_header,
                                             int offset)
{
    return (Elf64_Shdr *) (elf_file + elf_header->e_shoff +
                           elf_header->e_shentsize * offset);
}

static inline Elf64_Phdr *get_program_header(uint8_t *elf_file,
                                             Elf64_Ehdr *elf_header,
                                             int offset)
{
    return (Elf64_Phdr *) (elf_file + elf_header->e_phoff +
                           elf_header->e_phentsize * offset);
}

static int elf_check_valid(elf_t *elf)
{
    Elf64_Ehdr *elf_header = elf->inner->header;
    unsigned char *ident = elf_header->e_ident;

    if (ident[0] != 0x7F || ident[1] != 'E' || ident[2] != 'L' ||
        ident[3] != 'F')
        return -1;

    return 0;
}

int elf_init(elf_t *elf, uint8_t *data, size_t sz)
{
    if (data == NULL)
        return -1;

    elf->inner = malloc(sizeof(elf_inner_t));
    if (elf == NULL)
        return -1;

    elf->sz = sz;
    elf->inner->data = data;

    return elf_check_valid(elf);
}

int elf_lookup_shdr(elf_t *elf, char *name, Elf64_Shdr **output_sec_header)
{
    uint8_t *elf_data = elf->inner->data;
    Elf64_Ehdr *elf_header = elf->inner->header;
    Elf64_Shdr *shstrtab_sec_header =
        get_section_header(elf_data, elf_header, elf_header->e_shstrndx);
    char *shstrtab = (char *) (elf_data + shstrtab_sec_header->sh_offset);

    for (Elf64_Half i = 0; i < elf_header->e_shnum; i++) {
        Elf64_Shdr *sec_header = get_section_header(elf_data, elf_header, i);
        char *sec_name = shstrtab + sec_header->sh_name;

        if (!strcmp(name, sec_name)) {
            *output_sec_header = get_section_header(elf_data, elf_header, i);
            return 0;
        }
    }

    return -1;
}

int elf_lookup_symbol(elf_t *elf, char *symbol, Elf64_Sym **sym)
{
    Elf64_Shdr *symtab_sec_header;
    uint8_t *elf_data = elf->inner->data;
    Elf64_Ehdr *elf_header = elf->inner->header;
    int ret = elf_lookup_shdr(elf, ".symtab", &symtab_sec_header);
    if (ret)
        return ret;

    Elf64_Sym *symtab = (Elf64_Sym *) (elf_data + symtab_sec_header->sh_offset);

    Elf64_Shdr *strtab_sec_header =
        get_section_header(elf_data, elf_header, symtab_sec_header->sh_link);
    char *strtab = (char *) elf_data + strtab_sec_header->sh_offset;

    unsigned int symbol_cnt = symtab_sec_header->sh_size / sizeof(Elf64_Sym);
    for (unsigned int i = 0; i < symbol_cnt; i++) {
        const char *f_symbol = strtab + symtab[i].st_name;

        if (!strcmp(symbol, f_symbol)) {
            *sym = &symtab[i];
            return 0;
        }
    }
    return -1;
}

uint64_t elf_e_entry(elf_t *elf)
{
    Elf64_Ehdr *elf_header = elf->inner->header;
    return elf_header->e_entry;
}

void elf_phdr_iter_start(phdr_iter_t *it, uint32_t p_type)
{
    it->next = 0;
    it->p_type = p_type;
}

int elf_phdr_iter_next(elf_t *elf,
                       phdr_iter_t *it,
                       Elf64_Phdr **output_prog_header)
{
    uint8_t *elf_data = elf->inner->data;
    Elf64_Ehdr *elf_header = elf->inner->header;

    for (; it->next < elf_header->e_phnum; it->next++) {
        Elf64_Phdr *prog_header =
            get_program_header(elf_data, elf_header, it->next);
        if (prog_header->p_type == it->p_type) {
            *output_prog_header = prog_header;
            it->next++;
            return 0;
        }
    }

    return -1;
}

void elf_close(elf_t *elf)
{
    free(elf->inner);
}
