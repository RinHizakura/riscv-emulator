CC = gcc
CFLAGS = -Wall -Wextra -O3
CFLAGS += -include src/common.h

OUT ?= build
BIN = $(OUT)/emu

GIT_HOOKS := .git/hooks/applied
C_FILES = $(wildcard src/*.c)
OBJ_FILES = $(C_FILES:%.c=$(OUT)/%.o)

all: CFLAGS += -O3 -g
all: $(BIN) $(GIT_HOOKS)

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

$(BIN): $(OBJ_FILES)
	$(CC) -o $@ $(OBJ_FILES)

$(OUT)/src/%.o: src/%.c
	mkdir -p $(@D)
	$(CC) -o $@ -c $(CFLAGS) $<

check: $(BIN)
	 riscv64-unknown-linux-gnu-gcc -Wl,-Ttext=0x0 -nostdlib -o test.obj ./test/test.s
	 riscv64-unknown-linux-gnu-objcopy -O binary test.obj test.bin
	 $(BIN) ./test.bin
clean:
	$(RM) -rf build
	$(RM) -rf test.obj test.bin
