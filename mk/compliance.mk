# variables for compliance
COMPLIANCE_SRC ?= ./riscv-arch-test/riscv-test-suite
COMPLIANCE_DIR ?= ./riscv-arch-test
export RISCV_TARGET ?= riscof-test
export RISCV_TEST = lbu-align-01
export WORK = $(RISCV_TARGET)/build/compliance
export PATH := riscof-test:$(PATH)
$(COMPLIANCE_SRC):
	git submodule update --init
	touch $(@)
