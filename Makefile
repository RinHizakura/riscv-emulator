CC = gcc
CFLAGS = -Wall -Wextra -O3
CFLAGS += -include src/common.h
LDFLAGS = -lpthread

OUT ?= build
BIN = $(OUT)/emu

GIT_HOOKS := .git/hooks/applied
C_FILES = $(wildcard src/*.c)
OBJ_FILES = $(C_FILES:%.c=$(OUT)/%.o)

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
all: CFLAGS += -O3
all: LDFLAGS += -O3
all: $(BIN) $(GIT_HOOKS)

debug: CFLAGS += -O3 -g -DDEBUG
debug: LDFLAGS += -O3
debug: $(BIN)

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

$(BIN): $(OBJ_FILES)
	$(CC) $(LDFLAGS) -o $@ $(OBJ_FILES) $(LDFLAGS)

$(OUT)/src/%.o: src/%.c
	mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $< -o $@ 

check: CFLAGS += -O3
check: LDFLAGS += -O3
check: $(BIN)
	 riscv64-unknown-elf-gcc -S ./test/c_test.c
	 riscv64-unknown-elf-gcc \
		 -Wl,-Ttext=0x0 \
		 -nostdlib \
		 -march=rv64g \
		 -mabi=lp64 \
		 -o c_test.obj ./c_test.s
	 riscv64-unknown-elf-objcopy -O binary c_test.obj c_test.bin
	 $(BIN) --binary ./c_test.bin

test_assembly: CFLAGS += -O3
test_assembly: LDFLAGS += -O3
test_assembly: $(BIN)
	 riscv64-unknown-elf-gcc \
		 -Wl,-Ttext=0x0 \
		 -nostdlib \
		 -march=rv64g \
		 -mabi=lp64 \
		 -o asm_test.obj ./test/asm_test.s
	 riscv64-unknown-elf-objcopy -O binary asm_test.obj asm_test.bin
	 $(BIN) --binary ./asm_test.bin

os: $(BIN)
	$(BIN) --elf xv6/kernel.img --rfsimg xv6/fs.img

# variables for compliance
COMPLIANCE_DIR ?= ./riscv-arch-test
export TARGETDIR ?= $(shell pwd)/riscv-target
export RISCV_TARGET ?= riscv-emu
export XLEN ?= 64
export RISCV_DEVICE ?= I
#export RISCV_TEST = misalign-lw-01
export RISCV_TARGET_FLAGS ?=
export RISCV_ASSERT ?= 0
export JOBS = -j1
export WORK = $(TARGETDIR)/build/compliance

$(COMPLIANCE_DIR):
	git submodule update --init
	touch $(@)

compliance: $(BIN) $(COMPLIANCE_DIR)
	$(MAKE) -C $(COMPLIANCE_DIR) clean
	$(MAKE) -C $(COMPLIANCE_DIR)

clean:
	$(RM) -rf build
	$(RM) -rf *.obj *.bin *.s
