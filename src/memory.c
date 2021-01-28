#include <endian.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "memory.h"

bool init_mem(riscv_mem *mem, const char *filename)
{
    if (!filename) {
        LOG_ERROR("Binary is required for memory!\n");
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

    mem->mem = malloc(sz);
    if (!mem->mem) {
        LOG_ERROR("Error when allocating space through malloc.\n");
        fclose(fp);
        return false;
    }
    size_t read_size = fread(mem->mem, sizeof(uint8_t), sz, fp);

    if (read_size != sz) {
        LOG_ERROR("Error when reading binary through fread.\n");
        fclose(fp);
        return false;
    }
    fclose(fp);
    mem->code_size = read_size;
    return true;
}

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define read_len(bit, ptr, value) value = *(uint##bit##_t *) (ptr)
#else
#define read_len(bit, ptr, value) \
    value = __builtin_bswap##bit(*(uint##bit##_t *) (ptr))
#endif

uint64_t read_mem(riscv_mem *mem, uint64_t addr, uint64_t size)
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
    *(uint##bit##_t *) (ptr) = (uint##bit##_t) __builtin_bswap##bit((value))
#endif

void write_mem(riscv_mem *mem, uint64_t addr, uint64_t value, uint8_t size)
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
        LOG_ERROR("Invalid memory size!\n");
    }
}

void free_memory(riscv_mem *mem)
{
    free(mem->mem);
}
