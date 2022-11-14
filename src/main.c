#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include "emu.h"

#define MAX_FILE_LEN 256

static char input_file[MAX_FILE_LEN];
static char rfsimg_file[MAX_FILE_LEN];
static char signature_out_file[MAX_FILE_LEN];

static char opt_input = false;
static char opt_rfsimg = false;
static bool opt_compliance = false;

int main(int argc, char *argv[])
{
    int option_index = 0;
    struct option opts[] = {
        {"binary", 1, NULL, 'B'},
        {"rfsimg", 1, NULL, 'R'},
        {"compliance", 1, NULL, 'C'},
    };

    log_begin();

    int c;
    while ((c = getopt_long(argc, argv, "B:R:C:", opts, &option_index)) != -1) {
        switch (c) {
        case 'B':
            opt_input = true;
            strncpy(input_file, optarg, MAX_FILE_LEN - 1);
            input_file[MAX_FILE_LEN - 1] = '\0';
            break;
        case 'R':
            opt_rfsimg = true;
            strncpy(rfsimg_file, optarg, MAX_FILE_LEN - 1);
            rfsimg_file[MAX_FILE_LEN - 1] = '\0';
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

    if (!opt_input) {
        LOG_ERROR("An input image is needed!\n");
        return -1;
    }

    // if no specific root file system, then passing the null string
    if (!opt_rfsimg)
        rfsimg_file[0] = '\0';

    int ret = 0;
    riscv_emu *emu = calloc(1, sizeof(riscv_emu));
    if (!emu) {
        ret = -1;
        goto clean_up;
    }

    if (!init_emu(emu, input_file, rfsimg_file)) {
        ret = -1;
        goto clean_up;
    }

    run_emu(emu);

    // FIXME: Refactor to prettier code
    if (opt_compliance) {
        FILE *f = fopen(signature_out_file, "w");
        if (!f) {
            LOG_ERROR("Failed to open file %s for compilance test.\n",
                      signature_out_file);
            return -1;
        }

        uint64_t begin = emu->cpu.bus.memory.elf.sig_start;
        uint64_t end = emu->cpu.bus.memory.elf.sig_end;

        for (uint64_t i = begin; i < end; i += 4) {
            uint32_t value =
                read_mem(&emu->cpu.bus.memory, i, 32, &emu->cpu.exc) &
                0xffffffff;
            fprintf(f, "%08x\n", value);
        }
        fclose(f);
    }

    log_end();

clean_up:
    free_emu(emu);
    free(emu);
    return ret;
}
