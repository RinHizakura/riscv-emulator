include mk/toolchain.mk

CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -O3 -g
CFLAGS += -include common.h
LDFLAGS = -lpthread

OUT ?= build
BIN = $(OUT)/emu
SHELL_HACK := $(shell mkdir -p $(OUT))

GIT_HOOKS := .git/hooks/applied
C_FILES = $(wildcard src/*.c)

CSRCS = $(shell find ./src -name '*.c')
_COBJ =  $(notdir $(CSRCS))
COBJ = $(_COBJ:%.c=$(OUT)/%.o)

ifeq ("$(UBSAN)","1")
    CFLAGS +=  -fsanitize=undefined -fno-sanitize-recover
    LDFLAGS += -fsanitize=undefined
endif

ifeq ("$(ASAN)","1")
    CFLAGS +=  -fsanitize=address -fno-omit-frame-pointer -fno-common
    LDFLAGS += -fsanitize=address
endif

ifeq ("$(TSAN)", "1")
    CFLAGS +=  -fsanitize=thread
    LDFLAGS += -fsanitize=thread
endif

ifeq ("$(ICACHE)", "1")
    CFLAGS +=  -DICACHE_CONFIG
endif

all: CFLAGS += -O3 -g
all: LDFLAGS += -O3
all: $(BIN) $(GIT_HOOKS)

debug: CFLAGS += -O3 -g -DDEBUG
debug: LDFLAGS += -O3
debug: $(BIN)

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

include mk/external.mk

$(BIN): $(COBJ)
	@echo "  LD\t$@"
	@$(CC) $(LDFLAGS) -o $@ $(COBJ) $(LDFLAGS)

$(OUT)/%.o: src/%.c
	@echo "  CC\t$@"
	@$(CC) -c $(CFLAGS) $< -o $@

check: CFLAGS += -O3
check: LDFLAGS += -O3
check: $(BIN)
	 $(RISCV_GCC) -S -nostdlib -mcmodel=medany ./test/c_test.c
	 $(RISCV_GCC) \
		 -T ./test/linker.ld \
		 -nostdlib \
		 -march=rv64g \
		 -mabi=lp64 \
		 -mcmodel=medany \
		 -o c_test.obj ./c_test.s
	 $(BIN) --binary ./c_test.obj

xv6: $(BIN)
	$(BIN) --binary xv6/kernel.img --rfsimg xv6/fs.img

linux: $(BIN) $(LINUX_IMG) $(LINUX_RFS_IMG)
	$(BIN) --binary $(LINUX_IMG) --rfsimg $(LINUX_RFS_IMG)

# variables for compliance
COMPLIANCE_SRC ?= ./riscv-arch-test/Makefile
COMPLIANCE_DIR ?= ./riscv-arch-test/
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

$(COMPLIANCE_DIR): $(COMPLIANCE_SRC)

compliance: $(BIN) $(COMPLIANCE_DIR)
	$(MAKE) -C $(COMPLIANCE_DIR) clean
	$(MAKE) -C $(COMPLIANCE_DIR)

clean:
	@$(RM) $(BIN) $(COBJ)
	@$(RM) *.obj *.bin *.s *.dtb
