include mk/toolchain.mk

CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -O3 -MMD -g
CFLAGS += -include common.h
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

ifeq ("$(DEBUG)", "1")
    CFLAGS +=  -DDEBUG
endif

all: $(BIN) $(GIT_HOOKS)

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

run-xv6: $(BIN)
	$(BIN) --binary xv6/kernel.img --rfsimg xv6/fs.img

run-linux-old: $(BIN)
	$(BIN) --binary linux/kernel.img --rfsimg linux/rootfs.img

run-linux: $(BIN) $(LINUX_IMG) $(LINUX_RFS_IMG)
	$(BIN) --binary $(LINUX_IMG) --rfsimg $(LINUX_RFS_IMG)

include mk/compliance.mk
run-compliance: $(BIN)
	#riscof validateyaml --config=$(RISCV_TARGET)/config.ini
	riscof run --work-dir=$(WORK) \
		--config=$(RISCV_TARGET)/config.ini \
		--suite=$(COMPLIANCE_DIR)/riscv-test-suite \
		--env=$(COMPLIANCE_DIR)/riscv-test-suite/env

include mk/riscv-tests.mk
run-riscv-tests: $(BIN) $(RISCV_TEST_SRC)
	$(MAKE) -C $(RISCV_TEST_DIR)
	scripts/riscv-tests-run.sh
clean:
	@$(RM) $(BIN) $(COBJ) $(OUT)/*.d
	@$(RM) *.obj *.bin *.s *.dtb

-include $(OUT)/*.d
