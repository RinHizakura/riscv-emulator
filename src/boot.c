#include "boot.h"
#include "macros.h"
#include "memmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char dtb_filename[] = "./emu.dtb";

/*  The emulator should be compatible to QEMU RISC-V VirtIO Board:
 *  - https://github.com/qemu/qemu/blob/master/hw/riscv/virt.c
 *
 *  The codes are referenced to:
 *  - https://github.com/riscv/riscv-isa-sim/blob/master/riscv/dts.cc
 */

// TODO: don't mash all codes together for flexibility
static bool make_dtb()
{
    char dts_str[] =
        "/dts-v1/; \n"
        "\n"
        "/ {\n"
        "    #address-cells = <0x02>;\n"
        "    #size-cells = <0x02>;\n"
        "    model = \"riscv-virtio,qemu\";\n"
        "    compatible = \"riscv-virtio\";\n"
        "\n"
        "    chosen {\n"
        "        stdout-path = \"/uart@10000000\";\n"
        "    };\n"
        "\n"
        "    cpus {\n"
        "      #address-cells = <0x01>;\n"
        "      #size-cells = <0x00>;\n"
        "      timebase-frequency = <0x989680>;\n"
        "\n"
        "      CPU0: cpu@0 {\n"
        "        device_type = \"cpu\";\n"
        "        reg = <0x00>;\n"
        "        status = \"okay\";\n"
        "        compatible = \"riscv\";\n"
        "        riscv,isa = \"rv64imac\";\n"
        "        mmu-type = \"riscv,sv39\";\n"
        "        CPU0_intc: interrupt-controller {\n"
        "            #interrupt-cells = <0x01>;\n"
        "            interrupt-controller;\n"
        "            compatible = \"riscv,cpu-intc\";\n"
        "        };\n"
        "      };\n"
        "    };\n"
        "\n"
        "    memory@80000000 {\n"
        "      device_type = \"memory\";\n"
        "      reg = <0x0 0x80000000 0x0 0x8000000>;\n"
        "    };\n"
        "\n"
        "    soc {\n"
        "      #address-cells = <0x02>;\n"
        "      #size-cells = <0x02>;\n"
        "      compatible = \"simple-bus\";\n"
        "      ranges;\n"
        "\n"
        "      uart@10000000 {\n"
        "          interrupts = <0xa>;\n"
        "          interrupt-parent = <&PLIC>;\n"
        "          clock-frequency = <0x384000>;\n"
        "          reg = <0x0 0x10000000 0x0 0x100>;\n"
        "          compatible = \"ns16550a\";\n"
        "      };\n"
        "\n"
        "      virtio_mmio@10001000 {\n"
        "          interrupts = <0x01>;\n"
        "          interrupt-parent = <&PLIC>;\n"
        "          reg = <0x0 0x10001000 0x0 0x1000>;\n"
        "          compatible = \"virtio,mmio\";\n"
        "      };\n"
        "\n"
        "       PLIC: plic@c000000 {\n"
        "          compatible = \"riscv,plic0\";\n"
        "          interrupts-extended = <&CPU0_intc 0x09 &CPU0_intc 0x0b>;\n"
        "          reg = <0x00 0xc000000 0x00 0x4000000>;\n"
        "          riscv,ndev = <0x35>;\n"
        "          interrupt-controller;\n"
        "          #interrupt-cells = <0x01>;\n"
        "          #address-cells = <0x00>;\n"
        "       };\n"
        "\n"
        "       clint@2000000 {\n"
        "          compatible = \"riscv,clint0\";\n"
        "          interrupts-extended = <&CPU0_intc 0x03 &CPU0_intc 0x07>;\n"
        "          reg = <0x00 0x2000000 0x00 0x10000>;\n"
        "       };\n"
        "    };"
        "\n"
        "};\n";

    size_t dts_len = sizeof(dts_str);

    // Convert the DTS to DTB
    int dts_pipe[2];
    pid_t dts_pid;

    fflush(NULL);  // flush stdout/stderr before forking

    if (pipe(dts_pipe) != 0 || (dts_pid = fork()) < 0) {
        LOG_ERROR("Failed to fork dts child\n");
        exit(1);
    }

    // Child process to output dts
    if (dts_pid == 0) {
        close(dts_pipe[0]);
        if (write(dts_pipe[1], dts_str, dts_len) == -1) {
            LOG_ERROR("Failed to write dts\n");
            exit(1);
        }
        close(dts_pipe[1]);

        /* FIXME: Will it be too inefficient to fork a process which already
         * allocated huge amount of memory? Considering to generate dtb before
         * initialize DRAM */

        // return false to free allocated memory on child process
        return false;
    }

    pid_t dtb_pid;

    if ((dtb_pid = fork()) < 0) {
        LOG_ERROR("Failed to fork dtb child\n");
        exit(1);
    }

    // Child process to output dtb
    if (dtb_pid == 0) {
        dup2(dts_pipe[0], STDIN_FILENO);  // redirect to stdin
        close(dts_pipe[0]);
        close(dts_pipe[1]);
        execlp("dtc", "dtc", "-O", "dtb", "-o", dtb_filename, (char *) 0);
        LOG_ERROR("Failed to run dtc\n");
        exit(1);
    }

    close(dts_pipe[1]);
    close(dts_pipe[0]);

    return true;
}

bool init_boot(riscv_boot *boot, uint64_t entry_addr)
{
    if (!make_dtb())
        return false;

    FILE *fp = fopen(dtb_filename, "rb");
    if (!fp) {
        LOG_ERROR("Invalid boot rom path.\n");
        return false;
    }
    fseek(fp, 0, SEEK_END);
    size_t sz = ftell(fp) * sizeof(uint8_t);
    rewind(fp);

    // reset vector with size 0x20
    uint32_t reset_vec[] = {
        0x297,       // auipc t0, 0x0: write current pc to t0
        0x2028593,   // addi   a1, t0, &dtb: set dtb address(append after
                     // reset_vec) to a1
        0xf1402573,  // csrr   a0, mhartid
        0x0182b283,  // ld     t0,24(t0): load ELF entry point address
        0x28067,     // jr     t0
        0, entry_addr & 0xFFFFFFFF, (entry_addr >> 32)};

    size_t boot_mem_size = sz + sizeof(reset_vec);
    boot->boot_mem = malloc(boot_mem_size);
    if (!boot->boot_mem) {
        LOG_ERROR("Error when allocating space through malloc for BOOT_DRAM\n");
        return false;
    }
    boot->boot_mem_size = boot_mem_size;
    // copy boot rom instruction to specific address
    memcpy(boot->boot_mem, reset_vec, boot_mem_size);
    // copy dtb to specific address
    size_t read_size =
        fread(boot->boot_mem + sizeof(reset_vec), sizeof(uint8_t), sz, fp);
    fclose(fp);
    if (read_size != sz) {
        LOG_ERROR("Error when reading binary through fread.\n");
        free(boot->boot_mem);
        return false;
    }

    return true;
}

uint64_t read_boot(riscv_boot *boot,
                   uint64_t addr,
                   uint64_t size,
                   riscv_exception *exc)
{
    uint64_t index = (addr - BOOT_ROM_BASE);
    uint64_t value;

    switch (size) {
    case 8:
        value = boot->boot_mem[index];
        break;
    case 16:
        read_len(16, &boot->boot_mem[index], value);
        break;
    case 32:
        read_len(32, &boot->boot_mem[index], value);
        break;
    case 64:
        read_len(64, &boot->boot_mem[index], value);
        break;
    default:
        exc->exception = LoadAccessFault;
        LOG_ERROR("Invalid memory size!\n");
        return -1;
    }
    return value;
}

void free_boot(riscv_boot *boot)
{
    free(boot->boot_mem);
}
