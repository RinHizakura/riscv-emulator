#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include "elf.h"
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

    // Extra elf parsing for future use
    if (opt_compliance)
        elf_parser(input_file);

    riscv_emu *emu = malloc(sizeof(riscv_emu));

    if (!init_emu(emu, input_file)) {
        /* FIXME: should properly cleanup first */
        exit(1);
    }

    start_emu(emu);
    close_emu(emu);

    if (opt_compliance) {
        FILE *f = fopen(signature_out_file, "w");
        if (!f) {
            LOG_ERROR("Failed to open file %s for compilance test.\n",
                      signature_out_file);
            return -1;
        }
        fprintf(f, "%s\n", signature_out_file);
        fclose(f);
    }
    return 0;
}
