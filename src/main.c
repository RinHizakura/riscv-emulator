#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include "emu.h"

#define MAX_FILE_LEN 256

static char binary_file[MAX_FILE_LEN];
static bool opt_file = false;
static char signature_out_file[MAX_FILE_LEN];
static bool opt_compliance = false;

int main(int argc, char *argv[])
{
    int option_index = 0;
    struct option opts[] = {
        {"file", 1, NULL, 'F'},
        {"compliance", 1, NULL, 'C'},
    };

    int c;
    while ((c = getopt_long(argc, argv, "C:", opts, &option_index)) != -1) {
        switch (c) {
        case 'F':
            opt_file = true;
            strncpy(binary_file, optarg, MAX_FILE_LEN);
            binary_file[MAX_FILE_LEN - 1] = '\0';
            break;
        case 'C':
            opt_compliance = true;
            strncpy(signature_out_file, optarg, MAX_FILE_LEN);
            signature_out_file[MAX_FILE_LEN - 1] = '\0';
            break;
        default:
            LOG_ERROR("Unknown option\n");
        }
    }

    if (!opt_file) {
        LOG_ERROR("Binary is required for memory(--file <binary>) !\n");
        return -1;
    }

    riscv_emu *emu = malloc(sizeof(riscv_emu));

    if (!init_emu(emu, binary_file)) {
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
