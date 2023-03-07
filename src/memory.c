#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "exception.h"
#include "macros.h"
#include "memmap.h"
#include "memory.h"

static uint64_t entry_addr = DRAM_BASE;

static void load_elf(riscv_mem *mem, uint8_t *elf_file)
{
    Elf64_Shdr *tohost_shdr;
    if (elf_lookup_shdr(&mem->elf, ".tohost", &tohost_shdr) == 0) {
        mem->tohost_addr = tohost_shdr->sh_addr;
    }

    Elf64_Sym *sym;
    if (elf_lookup_symbol(&mem->elf, "begin_signature", &sym) == 0) {
        mem->sig_start = sym->st_value;
    }
    if (elf_lookup_symbol(&mem->elf, "end_signature", &sym) == 0) {
        mem->sig_end = sym->st_value;
    }

    entry_addr = elf_e_entry(&mem->elf);

    Elf64_Phdr *phdr;
    phdr_iter_t it;
    elf_phdr_iter_start(&it, PT_LOAD);
    while (elf_phdr_iter_next(&mem->elf, &it, &phdr) == 0) {
        uint64_t start = phdr->p_paddr - entry_addr;
        uint64_t size = phdr->p_filesz;
        uint64_t offset = phdr->p_offset;
        memcpy(mem->mem + start, elf_file + offset, size);
    }
}

uint64_t get_entry_addr()
{
    return entry_addr;
}

bool init_mem(riscv_mem *mem, const char *filename)
{
    // load binary file to memory
    if (!filename) {
        ERROR("Binary is required for memory!\n");
        return false;
    }

    // create memory with default size
    mem->mem = calloc(DRAM_SIZE, sizeof(uint8_t));
    if (!mem->mem) {
        ERROR("Error when allocating space through malloc for DRAM\n");
        return false;
    }

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        ERROR("Invalid binary path.\n");
        return false;
    }

    fseek(fp, 0, SEEK_END);
    size_t sz = ftell(fp) * sizeof(uint8_t);
    rewind(fp);

    uint8_t *buf = malloc(sz);
    if (buf == NULL) {
        ERROR(
            "Error when allocating space through malloc for ELF file buffer\n");
        fclose(fp);
        free(mem->mem);
        return false;
    }

    size_t read_size = fread(buf, sizeof(uint8_t), sz, fp);
    fclose(fp);
    if (read_size != sz) {
        ERROR("Error when reading binary through fread.\n");
        free(buf);
        free(mem->mem);
        return false;
    }

    if (elf_init(&mem->elf, buf, sz) == -1) {
        memcpy(mem->mem, buf, sz);
    } else {
        load_elf(mem, buf);
        elf_close(&mem->elf);
    }

    free(buf);
    return true;
}

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
        ERROR("Invalid memory size!\n");
        return -1;
    }
    return value;
}

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
        ERROR("Invalid memory size!\n");
        return false;
    }

    return true;
}

void free_memory(riscv_mem *mem)
{
    free(mem->mem);
}
