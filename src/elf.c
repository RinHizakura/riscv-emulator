#include "elf.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static Elf64_Ehdr read_elf_header(uint8_t *elf_file)
{
    Elf64_Ehdr elf_header;
    memcpy(&elf_header, elf_file, sizeof(Elf64_Ehdr));
    return elf_header;
}

static Elf64_Shdr read_section_header(uint8_t *elf_file,
                                      Elf64_Ehdr elf_header,
                                      int offset)
{
    Elf64_Shdr sec_header;
    uint8_t *sec =
        elf_file + elf_header.e_shoff + elf_header.e_shentsize * offset;
    memcpy(&sec_header, sec, sizeof(Elf64_Shdr));
    return sec_header;
}

static Elf64_Phdr read_program_header(uint8_t *elf_file,
                                      Elf64_Ehdr elf_header,
                                      int offset)
{
    Elf64_Phdr prog_header;
    uint8_t *prog =
        elf_file + elf_header.e_phoff + elf_header.e_phentsize * offset;

    memcpy(&prog_header, prog, sizeof(Elf64_Phdr));
    return prog_header;
}

bool elf_parser(riscv_elf *elf, const char *filename)
{
    // read file into memory
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        LOG_ERROR("Invalid binary path.\n");
        return false;
    }

    fseek(fp, 0, SEEK_END);
    size_t sz = ftell(fp) * sizeof(uint8_t);
    rewind(fp);

    elf->elf_file = malloc(sz);
    size_t read_size = fread(elf->elf_file, sizeof(uint8_t), sz, fp);

    if (read_size != sz) {
        LOG_ERROR("Error when reading binary through fread.\n");
        fclose(fp);
        return false;
    }
    fclose(fp);

    Elf64_Ehdr elf_header = read_elf_header(elf->elf_file);

    Elf64_Shdr sectab_sec_header =
        read_section_header(elf->elf_file, elf_header, elf_header.e_shstrndx);
    char *sectab = (char *) elf->elf_file + sectab_sec_header.sh_offset;

    printf("%16s %14s %14s\n", "Name", "Start", "end");

    bool first_text = true;
    bool first_data = true;
    for (int i = 0; i < elf_header.e_shnum; i++) {
        Elf64_Shdr sec_header =
            read_section_header(elf->elf_file, elf_header, i);
        char *sec_name = sectab + sec_header.sh_name;
        printf("%16s %14lx %14lx\n", sec_name, sec_header.sh_addr,
               sec_header.sh_size);

        // here we assume the text section will follow the order
        if (!strncmp(sec_name, ".text", 5)) {
            if (first_text == true) {
                elf->code_start = sec_header.sh_addr;
                elf->code_end = sec_header.sh_addr + sec_header.sh_size;
                first_text = false;
            } else
                elf->code_end = sec_header.sh_addr + sec_header.sh_size;
        }

        if (!strncmp(sec_name, ".data", 5)) {
            if (first_data == true) {
                elf->data_start = sec_header.sh_addr;
                elf->data_end = sec_header.sh_addr + sec_header.sh_size;
                first_data = false;
            } else
                elf->data_end = sec_header.sh_addr + sec_header.sh_size;
        }
        if (sec_header.sh_type == SHT_SYMTAB ||
            sec_header.sh_type == SHT_DYNSYM) {
            Elf64_Sym *symtab_header =
                (Elf64_Sym *) (elf->elf_file + sec_header.sh_offset);
            Elf64_Shdr symtab_sec_header = read_section_header(
                elf->elf_file, elf_header, sec_header.sh_link);
            char *symtab = (char *) elf->elf_file + symtab_sec_header.sh_offset;

            int symbol_cnt = sec_header.sh_size / sizeof(Elf64_Sym);
            for (int i = 0; i < symbol_cnt; i++) {
                char *sym_name = symtab + symtab_header[i].st_name;
                if (!strcmp(sym_name, "begin_signature")) {
                    elf->sig_start = symtab_header[i].st_value;
                    continue;
                }
                if (!strcmp(sym_name, "end_signature"))
                    elf->sig_end = symtab_header[i].st_value;
            }
        }
    }

    printf("\nEntry point 0x%lx\n", elf_header.e_entry);
    printf("There are %d program headers, starting at offset %ld\n\n",
           elf_header.e_phnum, elf_header.e_phoff);

    for (int i = 0; i < elf_header.e_phnum; i++) {
        Elf64_Phdr prog_header =
            read_program_header(elf->elf_file, elf_header, i);

        printf("Program Headers:\n");
        printf("Offset              VirtAddr                  PhysAddr\n");
        printf("0x%-16lx  0x%-16lx        0x%-16lx\n", prog_header.p_offset,
               prog_header.p_vaddr, prog_header.p_paddr);
        printf("FileSiz             MemSiz               Flags    Align\n");
        printf("0x%-16lx  0x%-16lx   0x%x      0x%lx\n", prog_header.p_filesz,
               prog_header.p_memsz, prog_header.p_flags, prog_header.p_align);

        printf("\n");

        if (prog_header.p_vaddr == elf->code_start) {
            assert(prog_header.p_filesz == (elf->code_end - elf->code_start));
            elf->code_offset = prog_header.p_offset;
            continue;
        }
        if (prog_header.p_vaddr == elf->data_start) {
            assert(prog_header.p_filesz == (elf->data_end - elf->data_start));
            elf->data_offset = prog_header.p_offset;
        }
    }

    printf(".text start from %lx to %lx\n", elf->code_start, elf->code_end);
    printf(".data start from %lx to %lx\n", elf->data_start, elf->data_end);
    return true;
}
