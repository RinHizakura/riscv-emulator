#ifndef _ELF_H
#define _ELF_H

#include <stdbool.h>
#include <stdint.h>

#define EI_NIDENT 16
#define Elf64_Half uint16_t
#define Elf64_Word uint32_t
#define Elf64_Addr uint64_t
#define Elf64_Off uint64_t
#define Elf64_Xword uint64_t

// elf header
typedef struct {
    unsigned char e_ident[EI_NIDENT];
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry;
    Elf64_Off e_phoff;
    Elf64_Off e_shoff;
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize;
    Elf64_Half e_phnum;
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
} Elf64_Ehdr;

// program header
typedef struct {
    Elf64_Word p_type;
    Elf64_Word p_flags;
    Elf64_Off p_offset;
    Elf64_Addr p_vaddr;
    Elf64_Addr p_paddr;
    Elf64_Xword p_filesz;
    Elf64_Xword p_memsz;
    Elf64_Xword p_align;
} Elf64_Phdr;

#define SHT_SYMTAB 2
#define SHT_DYNSYM 11

// section header
typedef struct {
    Elf64_Word sh_name;
    Elf64_Word sh_type;
    Elf64_Xword sh_flags;
    Elf64_Addr sh_addr;
    Elf64_Off sh_offset;
    Elf64_Xword sh_size;
    Elf64_Word sh_link;
    Elf64_Word sh_info;
    Elf64_Xword sh_addralign;
    Elf64_Xword sh_entsize;
} Elf64_Shdr;

// symbol table
typedef struct {
    Elf64_Word st_name;
    unsigned char st_info;
    unsigned char st_other;
    Elf64_Half st_shndx;
    Elf64_Addr st_value;
    Elf64_Xword st_size;
} Elf64_Sym;

// FIXME: need more program header
#define MAX_PROGRAM_HEADER 2

typedef struct {
    uint64_t sig_start;
    uint64_t sig_end;
} riscv_elf;

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
#endif
