#include <endian.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "exception.h"
#include "memmap.h"
#include "memory.h"

static bool parse_elf(riscv_mem *mem, uint8_t *elf_file)
{
    Elf64_Ehdr *elf_header = get_elf_header(elf_file);

    unsigned char *ident = elf_header->e_ident;
    // not a valid ELF file
    if (ident[0] != 0x7F || ident[1] != 'E' || ident[2] != 'L' ||
        ident[3] != 'F')
        return false;

    Elf64_Shdr *sectab_sec_header =
        get_section_header(elf_file, elf_header, elf_header->e_shstrndx);
    char *sectab = (char *) (elf_file + sectab_sec_header->sh_offset);

    printf("%16s %14s %14s\n", "Name", "Start", "end");

    for (int i = 0; i < elf_header->e_shnum; i++) {
        Elf64_Shdr *sec_header = get_section_header(elf_file, elf_header, i);
        char *sec_name = sectab + sec_header->sh_name;
        printf("%16s %14lx %14lx\n", sec_name, sec_header->sh_addr,
               sec_header->sh_size);

        if (sec_header->sh_type == SHT_SYMTAB ||
            sec_header->sh_type == SHT_DYNSYM) {
            Elf64_Sym *symtab_header =
                (Elf64_Sym *) (elf_file + sec_header->sh_offset);
            Elf64_Shdr *symtab_sec_header =
                get_section_header(elf_file, elf_header, sec_header->sh_link);
            char *symtab = (char *) elf_file + symtab_sec_header->sh_offset;

            int symbol_cnt = sec_header->sh_size / sizeof(Elf64_Sym);
            for (int i = 0; i < symbol_cnt; i++) {
                char *sym_name = symtab + symtab_header[i].st_name;
                if (!strcmp(sym_name, "begin_signature")) {
                    mem->elf.sig_start = symtab_header[i].st_value;
                    continue;
                }
                if (!strcmp(sym_name, "end_signature"))
                    mem->elf.sig_end = symtab_header[i].st_value;
            }
        }
    }

    printf("\nEntry point 0x%lx\n", elf_header->e_entry);
    printf("There are %d program headers, starting at offset %ld\n\n",
           elf_header->e_phnum, elf_header->e_phoff);

    for (int i = 0; i < elf_header->e_phnum; i++) {
        Elf64_Phdr *prog_header = get_program_header(elf_file, elf_header, i);

        printf("Program Headers:\n");
        printf("Offset              VirtAddr                  PhysAddr\n");
        printf("0x%-16lx  0x%-16lx        0x%-16lx\n", prog_header->p_offset,
               prog_header->p_vaddr, prog_header->p_paddr);
        printf("FileSiz             MemSiz               Flags    Align\n");
        printf("0x%-16lx  0x%-16lx   0x%x      0x%lx\n", prog_header->p_filesz,
               prog_header->p_memsz, prog_header->p_flags,
               prog_header->p_align);

        printf("\n");

        uint64_t start = prog_header->p_paddr - elf_header->e_entry;
        uint64_t size = prog_header->p_filesz;
        uint64_t offset = prog_header->p_offset;
        memcpy(mem->mem + start, elf_file + offset, size);
    }

    return true;
}

bool init_mem(riscv_mem *mem, const char *filename)
{
    // load binary file to memory
    if (!filename) {
        LOG_ERROR("Binary is required for memory!\n");
        return false;
    }

    // create memory with default size
    mem->mem = calloc(DRAM_SIZE, sizeof(uint8_t));
    if (!mem->mem) {
        LOG_ERROR("Error when allocating space through malloc for DRAM\n");
        return false;
    }

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        LOG_ERROR("Invalid binary path.\n");
        return false;
    }

    fseek(fp, 0, SEEK_END);
    size_t sz = ftell(fp) * sizeof(uint8_t);
    rewind(fp);

    uint8_t *buf = malloc(sz);
    size_t read_size = fread(buf, sizeof(uint8_t), sz, fp);
    fclose(fp);
    if (read_size != sz) {
        LOG_ERROR("Error when reading binary through fread.\n");
        free(buf);
        return false;
    }

    if (!parse_elf(mem, buf)) {
        printf("CC\n");
        memcpy(mem->mem, buf, sz);
    }

    free(buf);
    return true;
}

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define read_len(bit, ptr, value) value = (*(uint##bit##_t *) (ptr))
#else
#define read_len(bit, ptr, value) \
    value = __builtin_bswap##bit(*(uint##bit##_t *) (ptr))
#endif

uint64_t read_mem(riscv_mem *mem,
                  uint64_t addr,
                  uint64_t size,
                  riscv_exception *exc)
{
    uint64_t index = (addr - DRAM_BASE);
    uint64_t value;

    switch (size) {
    case 8:
        value = mem->mem[index];
        break;
    case 16:
        read_len(16, &mem->mem[index], value);
        break;
    case 32:
        read_len(32, &mem->mem[index], value);
        break;
    case 64:
        read_len(64, &mem->mem[index], value);
        break;
    default:
        exc->exception = LoadAccessFault;
        LOG_ERROR("Invalid memory size!\n");
        return -1;
    }
    return value;
}

/* In our emulator, the memory is little endian, so we can just casting
 * memory to target pointer type if our host architecture is also the case.
 * If our architecture is big endian, then we should revise the order first
 *
 * Note:
 * I don't sure if the index of memory would overflow. If it will, the
 * implementation now will get error.
 */

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define write_len(bit, ptr, value) \
    *(uint##bit##_t *) (ptr) = (uint##bit##_t)(value)
#else
#define write_len(bit, ptr, value) \
    *(uint##bit##_t *) (ptr) = __builtin_bswap##bit((uint##bit##_t)(value))
#endif

bool write_mem(riscv_mem *mem,
               uint64_t addr,
               uint8_t size,
               uint64_t value,
               riscv_exception *exc)
{
    uint64_t index = (addr - DRAM_BASE);

    switch (size) {
    case 8:
        mem->mem[index] = (uint8_t) value;
        break;
    case 16:
        write_len(16, &mem->mem[index], value);
        break;
    case 32:
        write_len(32, &mem->mem[index], value);
        break;
    case 64:
        write_len(64, &mem->mem[index], value);
        break;
    default:
        exc->exception = StoreAMOAccessFault;
        LOG_ERROR("Invalid memory size!\n");
        return false;
    }

    return true;
}

void free_memory(riscv_mem *mem)
{
    free(mem->mem);
}
