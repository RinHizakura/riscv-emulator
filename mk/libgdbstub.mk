GDBSTUB_SRC = mini-gdbstub/Makefile
GDBSTUB_DIR = mini-gdbstub/
GDBSTUB_OUT = $(abspath $(OUT)/mini-gdbstub)
GDBSTUB_LIB = $(GDBSTUB_OUT)/libgdbstub.a
GDBSTUB_COMM = 127.0.0.1:1234

CFLAGS += -D'GDBSTUB_COMM="$(GDBSTUB_COMM)"'
LDFLAGS += $(GDBSTUB_LIB)

$(GDBSTUB_LIB): $(GDBSTUB_SRC)
	@$(MAKE) -C $(GDBSTUB_DIR) O=$(dir $@)

$(GDBSTUB_SRC):
	git submodule update --init
	touch $(@)
