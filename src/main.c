#include <assert.h>
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "dtb.h"
#include "emu.h"

#define MAX_FILE_LEN 256

static char input_file[MAX_FILE_LEN];
static char rfsimg_file[MAX_FILE_LEN];
static char signature_out_file[MAX_FILE_LEN];

static char opt_input = false;
static char opt_rfsimg = false;
static bool opt_compliance = false;
static bool opt_riscv_test = false;

int main(int argc, char *argv[])
{
    if (!make_dtb(DTB_FILENAME)) {
        ERROR("Fail to create dtb file!\n");
        return -1;
    }

    if (!log_begin()) {
        ERROR("Fail to initialize the debug logger\n");
        return -1;
    }

    int option_index = 0;
    struct option opts[] = {
        {"binary", 1, NULL, 'B'},
        {"rfsimg", 1, NULL, 'R'},
        {"compliance", 1, NULL, 'C'},
        {"riscv-test", 0, NULL, 'T'},
    };

    int c;
    while ((c = getopt_long(argc, argv, "B:R:C:T", opts, &option_index)) !=
           -1) {
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
        case 'T':
            opt_riscv_test = true;
            break;
        default:
            ERROR("Unknown option\n");
        }
    }

    if (!opt_input) {
        ERROR("An input image is needed!\n");
        return -1;
    }

    // if no specific root file system, then passing the null string
    if (!opt_rfsimg)
        rfsimg_file[0] = '\0';

    int ret = 0;
    riscv_emu *emu = create_emu(input_file, rfsimg_file);
    if (!emu) {
        ERROR("Fail to create the emulator\n");
        ret = -1;
        goto clean_up;
    }

    if (opt_riscv_test || opt_compliance) {
        ret = test_emu(emu);
    } else {
        run_emu(emu);
    }

    if (opt_compliance) {
        ret = take_signature_emu(emu, signature_out_file);
    }

clean_up:
    log_end();
    free_emu(emu);
    return ret;
}
