#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include "emu.h"

#define MAX_FILE_LEN 256

static char input_file[MAX_FILE_LEN];
static bool opt_binary = false;
static bool opt_elf = false;
static char signature_out_file[MAX_FILE_LEN];
static bool opt_compliance = false;

int main(int argc, char *argv[])
{
    int option_index = 0;
    struct option opts[] = {
        {"binary", 1, NULL, 'B'},
        {"elf", 1, NULL, 'E'},
        {"compliance", 1, NULL, 'C'},
    };

    int c;
    while ((c = getopt_long(argc, argv, "B:E:C:", opts, &option_index)) != -1) {
        switch (c) {
        case 'B':
            if (opt_elf == true) {
                printf("Can only choose one in --binary and --elf !\n");
                return -1;
            }
            opt_binary = true;
            strncpy(input_file, optarg, MAX_FILE_LEN - 1);
            input_file[MAX_FILE_LEN - 1] = '\0';
            break;
        case 'E':
            if (opt_binary == true) {
                printf("Can only choose one in --binary and --elf !\n");
                return -1;
            }
            opt_elf = true;
            strncpy(input_file, optarg, MAX_FILE_LEN - 1);
            input_file[MAX_FILE_LEN - 1] = '\0';
            break;
        case 'C':
            opt_compliance = true;
            strncpy(signature_out_file, optarg, MAX_FILE_LEN - 1);
            signature_out_file[MAX_FILE_LEN - 1] = '\0';
            break;
        default:
            LOG_ERROR("Unknown option\n");
        }
    }

    if ((!opt_binary && !opt_elf) || (opt_binary && opt_elf)) {
        LOG_ERROR("Need one and only input file for memory!\n");
        return -1;
    }

    if (opt_compliance && !opt_elf) {
        LOG_ERROR("Compliance test should use elf format as input!\n");
        return -1;
    }

    riscv_emu *emu = malloc(sizeof(riscv_emu));

    if (!init_emu(emu, input_file, opt_elf)) {
        /* FIXME: should properly cleanup first */
        exit(1);
    }

    start_emu(emu);

    // FIXME: Refactor to prettier code
    if (opt_compliance) {
        FILE *f = fopen(signature_out_file, "w");
        if (!f) {
            LOG_ERROR("Failed to open file %s for compilance test.\n",
                      signature_out_file);
            return -1;
        }

        uint64_t begin = emu->cpu.bus.memory.elf.sig_start + DRAM_BASE;
        uint64_t end = emu->cpu.bus.memory.elf.sig_end + DRAM_BASE;

        for (uint64_t i = begin; i < end; i += 4) {
            uint32_t value =
                read_mem(&emu->cpu.bus.memory, i, 32, &emu->cpu.exc) &
                0xffffffff;
            fprintf(f, "%08x\n", value);
        }
        fclose(f);
    }

    close_emu(emu);
    return 0;
}
