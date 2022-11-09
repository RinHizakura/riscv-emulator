TOOLCHAIN_LIST := riscv64-unknown-elf- \
		  riscv64-unknown-linux-gnu-

RISCV_GCC := $(word 1, $(foreach exec,$(TOOLCHAIN_LIST), \
             $(if $(shell which $(exec)gcc), $(exec)gcc ,)))
ifeq ($(RISCV_GCC),)
$(warning You may need GNU Toolchain for RISC-V to run compliance test or build image)
endif
