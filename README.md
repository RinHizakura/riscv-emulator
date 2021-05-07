# riscv-emulator

**(Note: This project is still on the process for completion!)**

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

Binary file in raw or ELF format are both supported. The emulator will check which format is 
used itself. You should notice that the work of ELF parsing now could be non-general and 
ugly, so you may fail to load your ELF binary by the bad implementation.
```
$ ./build/emu --binary <binary name>
```

The emulator is also validated to run [xv6-riscv](https://github.com/mit-pdos/xv6-riscv),
which is a simple UNIX operating system. You are welcome to play with the provided binary with
the following command. Note that I compile it with `-march=rv64g` flag since we don't support 
fully RV64C now.

You'll find that it takes times from booting to execute a shell, and that's because no 
optimization is applied now and may be added in the future.
```
$ ./build/emu --binary xv6/kernel.img --rfsimg xv6/fs.img
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
export RISCV_DEVICE ?= C // running RV64C test
```
