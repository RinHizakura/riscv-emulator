# riscv-emulator

**This project is still on the process for completion!!**

The project is built to learn RISC-V architecture as the purpose for me. It mainly comes 
from [rvemu](https://github.com/d0iasm/rvemu), which is a RISC-V emulator with 
Rust implementation. And borrow the idea of CPU implementation from [riscv_em](https://github.com/franzflasch/riscv_em).

The emulator now supports fully RV64I, M, Zicsr, and Zifencei instructions. Some RV64A 
instructions are also supported.

## Build and Run

To build the emulator.
```
$ make
```

Then you can load and run the raw RISC-V binary file.
```
$ ./build/emu --binary <binary name>
```

You can also complie [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) with `-march=rv64g` 
flag(since we don't support RV64C now), taking the raw kernel image(by tool `objcopy`) and 
file system image, then executing the following command to run a simple operating system.
```
$ ./build/emu --binary <xv6 kernel image> --rfsimg <xv6 root file system image>
```

In fact, we do have a ELF parser. But it is originally specific for compliance test with 
non-general and ugly implementation. So you may fail to load the ELF but can stiil take 
a try with the command.
```
$ ./build/emu --elf <ELF name>
```

## Compliance Test

The [riscv-arch-test](https://github.com/riscv/riscv-arch-test) is applied to check if 
implementing the specifications correctly. We can pass RV64I and RV64M now. You can run the 
compliance test by the following command.
```
$ make compliance
```

Note that you should change `Makefile` to choose from RV64I or RV64M test.
```
export RISCV_DEVICE ?= I // running RV64I test
export RISCV_DEVICE ?= M // running RV64M test
```
