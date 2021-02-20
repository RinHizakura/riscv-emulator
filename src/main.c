#include <stdlib.h>

#include "emu.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        LOG_ERROR("Binary is required for memory!\n");
        return -1;
    }

    riscv_emu *emu = malloc(sizeof(riscv_emu));

    if (!init_emu(emu, argv[1])) {
        /* FIXME: should properly cleanup first */
        exit(1);
    }

    start_emu(emu);

    close_emu(emu);
    return 0;
}
