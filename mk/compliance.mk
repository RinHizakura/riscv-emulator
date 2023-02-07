# variables for compliance
export PATH := riscof-test:$(PATH)
COMPLIANCE_SRC ?= ./riscv-arch-test/riscv-test-suite
COMPLIANCE_DIR ?= ./riscv-arch-test
WORK_DIR ?= $(RISCV_TARGET)/build
RISCV_TARGET ?= riscof-test

WORK_DIR_SHELL_HACK := $(shell mkdir -p $(WORK_DIR))

$(COMPLIANCE_SRC):
	git submodule update --init
	touch $(@)
