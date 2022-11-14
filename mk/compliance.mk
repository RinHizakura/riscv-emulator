# variables for compliance
COMPLIANCE_SRC ?= ./riscv-arch-test/Makefile
COMPLIANCE_DIR ?= ./riscv-arch-test
export TARGETDIR ?= $(shell pwd)/riscv-target
export RISCV_TARGET ?= riscv-emu
export XLEN ?= 64
export RISCV_DEVICE ?= I
export RISCV_TEST = lbu-align-01
export RISCV_TARGET_FLAGS ?=
export RISCV_ASSERT ?= 0
export JOBS = -j1
export WORK = $(TARGETDIR)/build/compliance

$(COMPLIANCE_SRC):
	git submodule update --init
	touch $(@)
