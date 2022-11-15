include mk/toolchain.mk

CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -O3 -g
CFLAGS += -include log.h
LDFLAGS = -lpthread -O3

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

all: $(BIN) $(GIT_HOOKS)

debug: CFLAGS += -DDEBUG
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

include mk/compliance.mk
compliance: $(BIN) $(COMPLIANCE_SRC)
	$(MAKE) -C $(COMPLIANCE_DIR) clean
	$(MAKE) -C $(COMPLIANCE_DIR)
include mk/riscv-tests.mk
RISCV_TEST_BIN = $(RISCV_TEST_DIR)/isa/rv64si-p-csr
riscv-test: $(BIN) $(RISCV_TEST_SRC)
	$(MAKE) -C $(RISCV_TEST_DIR)
	$(BIN) --binary $(RISCV_TEST_BIN) --riscv-test

clean:
	@$(RM) $(BIN) $(COBJ)
	@$(RM) *.obj *.bin *.s *.dtb
