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

void free_memory(riscv_mem *mem)
{
    free(mem->mem);
}
