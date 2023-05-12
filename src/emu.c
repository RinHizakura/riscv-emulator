#include <assert.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "emu.h"
#include "emu_private.h"

/*  The emulator should be compatible to QEMU RISC-V VirtIO Board:
 *  - https://github.com/qemu/qemu/blob/master/hw/riscv/virt.c
 *
 *  The codes are referenced to:
 *  - https://github.com/riscv/riscv-isa-sim/blob/master/riscv/dts.cc
 */

// TODO: don't mash all codes together for flexibility
static bool make_dtb(const char *dtb_filename)
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
        "        bootargs = \"root=/dev/vda rw console=ttyS0\";"
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
        "          interrupts-extended = <&CPU0_intc 0x0b &CPU0_intc 0x09>;\n"
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
        ERROR("Failed to fork dts child\n");
        exit(1);
    }

    // Child process to output dts
    if (dts_pid == 0) {
        close(dts_pipe[0]);
        if (write(dts_pipe[1], dts_str, dts_len) == -1) {
            ERROR("Failed to write dts\n");
            exit(1);
        }
        close(dts_pipe[1]);

        exit(0);
    }

    pid_t dtb_pid;

    if ((dtb_pid = fork()) < 0) {
        ERROR("Failed to fork dtb child\n");
        exit(1);
    }

    // Child process to output dtb
    if (dtb_pid == 0) {
        dup2(dts_pipe[0], STDIN_FILENO);  // redirect to stdin
        close(dts_pipe[0]);
        close(dts_pipe[1]);
        execlp("dtc", "dtc", "-O", "dtb", "-o", dtb_filename, (char *) 0);
        ERROR("Failed to run dtc\n");
        exit(1);
    }

    close(dts_pipe[1]);
    close(dts_pipe[0]);

    // Reap children
    int status;
    waitpid(dts_pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        printf("Child dts process failed\n");
        exit(1);
    }
    waitpid(dtb_pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        printf("Child dtb process failed\n");
        exit(1);
    }

    return true;
}

riscv_emu *create_emu(const char *filename, const char *rfs_name)
{
    /* Generate dtb file first before creating riscv_emu object, so
     * we can simply avoid to fork a process with a large number of
     * allocated memories (although it could be fine because of the
     * copy-on-write mechanism) */
    if (!make_dtb(DTB_FILENAME)) {
        ERROR("Fail to create dtb file!\n");
        return NULL;
    }

    riscv_emu *emu = calloc(1, sizeof(riscv_emu));
    if (!emu)
        return NULL;

    if (!init_cpu(&emu->cpu, filename, rfs_name)) {
        free_emu(emu);
        return NULL;
    }

    return emu;
}

void run_emu(riscv_emu *emu)
{
    while (step_cpu(&emu->cpu))
        ;
}

extern struct target_ops gdbstub_ops;
void run_emu_debug(riscv_emu *emu)
{
    cpu_set_debug_mode(&emu->cpu, true);

    emu->bp.is_set = false;
    emu->is_interrupted = false;
    if (!gdbstub_init(&emu->gdbstub, &gdbstub_ops,
                      (arch_info_t){
                          .reg_num = 33,
                          .reg_byte = 8,
                          .target_desc = TARGET_RV64,
                      },
                      GDBSTUB_COMM))
        return;

    gdbstub_run(&emu->gdbstub, (void *) emu);

    gdbstub_close(&emu->gdbstub);
}

int test_emu(riscv_emu *emu)
{
    while (step_cpu(&emu->cpu)) {
        /* If a riscv-tests program is done, it will write non-zero value to
         * a certain address. We can poll it in every step to terminate the
         * emulator. */
        riscv_mem *mem = &emu->cpu.bus.memory;
        uint64_t tohost_addr = mem->tohost_addr;
        assert(tohost_addr > DRAM_BASE);
        if (read_mem(mem, tohost_addr, 8, &emu->cpu.exc) != 0)
            break;
    }

    return emu->cpu.xreg[10];
}

int take_signature_emu(riscv_emu *emu, char *signature_out_file)
{
    // FIXME: Refactor to prettier code
    FILE *f = fopen(signature_out_file, "w");
    if (!f) {
        ERROR("Failed to open file %s for compilance test.\n",
              signature_out_file);
        return -1;
    }

    uint64_t begin = emu->cpu.bus.memory.sig_start;
    uint64_t end = emu->cpu.bus.memory.sig_end;

    for (uint64_t i = begin; i < end; i += 4) {
        uint32_t value =
            read_mem(&emu->cpu.bus.memory, i, 32, &emu->cpu.exc) & 0xffffffff;
        fprintf(f, "%08x\n", value);
    }
    fclose(f);
    return 0;
}

void free_emu(riscv_emu *emu)
{
    if (emu == NULL)
        return;

    free_cpu(&emu->cpu);
    free(emu);
}
