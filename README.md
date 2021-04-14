# riscv-emulator

**This project is still on the process for completion!!**

The project is built to learn RISC-V architecture as the purpose for me. It mainly comes 
from [rvemu](https://github.com/d0iasm/rvemu), which is a RISC-V emulator with 
Rust implementation. Also, some of the idea of CPU implementation is borrowed from 
[riscv_em](https://github.com/franzflasch/riscv_em).

The emulator now supports fully RV64I, M, Zicsr, and Zifencei instructions. Some RV64A 
instructions are also supported.

## Build and Run

To build the emulator.
```
$ make
```

Then you can load and run a RISC-V bare-metal raw binary file.
```
$ ./build/emu --binary <binary name>
```

This emulator is also validated to run [xv6-riscv](https://github.com/mit-pdos/xv6-riscv), 
which is a simple UNIX operating system(note that I compile it with `-march=rv64g` flag 
since we don't support RV64C now). You'll find that it takes times from booting to execute 
a shell, since no optimization is applied now and may be added in the future.
```
$ make os
```

In fact, we do have the feature to load the ELF format binary. But since I don't fully 
understand the ELF format. So the implementation now is non-general and ugly. It is 
tested on compliance test and xv6 binary only, so you may fail to load the other ELFs.
```
$ ./build/emu --elf <ELF name>
```

## Compliance Test

The [riscv-arch-test](https://github.com/riscv/riscv-arch-test) is applied to check if 
implementing the specifications correctly. We can pass RV64I and RV64M now. You can run the 
compliance test by the following command.
```
$ git submodule update --init
$ make compliance
```

Note that you should change `Makefile` to choose from RV64I or RV64M test.
```
export RISCV_DEVICE ?= I // running RV64I test
export RISCV_DEVICE ?= M // running RV64M test
```
