CC = gcc
CFLAGS = -Wall -Wextra -O3
CFLAGS += -include src/common.h

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
	$(CC) $(LDFLAGS) -o $@ $(OBJ_FILES)

$(OUT)/src/%.o: src/%.c
	mkdir -p $(@D)
	$(CC) -o $@ -c $(CFLAGS) $<

check: CFLAGS += -O3
check: LDFLAGS += -O3
check: $(BIN)
	 riscv64-unknown-linux-gnu-gcc -S ./test/c_test.c
	 riscv64-unknown-linux-gnu-gcc -Wl,-Ttext=0x0 -nostdlib -o c_test.obj ./c_test.s
	 riscv64-unknown-linux-gnu-objcopy -O binary c_test.obj c_test.bin
	 $(BIN) ./c_test.bin

test_assembly: CFLAGS += -O3
test_assembly: LDFLAGS += -O3
test_assembly: $(BIN)
	 riscv64-unknown-linux-gnu-gcc -Wl,-Ttext=0x0 -nostdlib -o asm_test.obj ./test/asm_test.s
	 riscv64-unknown-linux-gnu-objcopy -O binary asm_test.obj asm_test.bin
	 $(BIN) ./asm_test.bin

clean:
	$(RM) -rf build
	$(RM) -rf *.obj *.bin *.s
