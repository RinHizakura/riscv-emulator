# variables for compliance
COMPLIANCE_SRC ?= ./riscv-arch-test/riscv-test-suite
COMPLIANCE_DIR ?= ./riscv-arch-test
export RISCV_TARGET ?= riscof-test
export XLEN ?= 64
export RISCV_DEVICE ?= I
export RISCV_TEST = lbu-align-01
export RISCV_TARGET_FLAGS ?=
export RISCV_ASSERT ?= 0
export JOBS = -j1
export WORK = $(RISCV_TARGET)/build/compliance

$(COMPLIANCE_SRC):
	git submodule update --init
	touch $(@)
