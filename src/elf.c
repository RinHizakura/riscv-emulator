#include <stdlib.h>
#include <string.h>

#include "elf.h"

void elf_parser(char *filename)
{
    Elf64_Ehdr elf_header;
    Elf64_Phdr prog_header;

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        LOG_ERROR("Invalid binary path.\n");
        return;
    }

    fseek(fp, 0, SEEK_END);
    size_t sz = ftell(fp) * sizeof(uint8_t);
    rewind(fp);

    char *elf = malloc(sz);
    size_t read_size = fread(elf, sizeof(uint8_t), sz, fp);

    if (read_size != sz) {
        LOG_ERROR("Error when reading binary through fread.\n");
        fclose(fp);
        return;
    }
    fclose(fp);

    memcpy(&elf_header, elf, sizeof(Elf64_Ehdr));

    printf("Entry point 0x%lx\n", elf_header.e_entry);
    printf("There are %d program headers, starting at offset %ld\n\n",
           elf_header.e_phnum, elf_header.e_phoff);

    for (int i = 0; i < elf_header.e_phnum; i++) {
        char *prog = elf + elf_header.e_phoff + elf_header.e_phentsize * i;

        memcpy(&prog_header, prog, sizeof(Elf64_Phdr));

        printf("Program Headers:\n");
        printf("Offset              VirtAddr                  PhysAddr\n");
        printf("0x%-16lx  0x%-16lx        0x%-16lx\n", prog_header.p_offset,
               prog_header.p_vaddr, prog_header.p_paddr);
        printf("FileSiz             MemSiz               Flags    Align\n");
        printf("0x%-16lx  0x%-16lx   0x%x      0x%lx\n", prog_header.p_filesz,
               prog_header.p_memsz, prog_header.p_flags, prog_header.p_align);
        printf("\n");
    }
    free(elf);
}
