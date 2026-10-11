TOOLCHAIN_DIR = ../../../04-core-libbpf/toolchain
LIBBPF_DIR    = $(TOOLCHAIN_DIR)/libbpf/src
LIBELF_DIR    = $(TOOLCHAIN_DIR)/libelf-shim
MULTIARCH_DIR = /usr/include/x86_64-linux-gnu

CLANG = clang
CC    = gcc
BPF_CFLAGS = -target bpf -D__TARGET_ARCH_x86 -I$(MULTIARCH_DIR) -g -O2 -Wall
CFLAGS  = -g -O2 -Wall -I$(LIBBPF_DIR) -I$(LIBELF_DIR)/include
LDFLAGS = -L$(LIBELF_DIR)/lib -Wl,-rpath,$(abspath $(LIBELF_DIR)/lib)
LIBS    = $(LIBBPF_DIR)/libbpf.a -lelf -lz -lpthread

all: tc_probe

tc_probe.bpf.o: tc_probe.bpf.c bpf_helpers.h bpf_helper_defs.h
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@

tc_probe: tc_probe.c tc_probe.bpf.o $(LIBBPF_DIR)/libbpf.a
	$(CC) $(CFLAGS) tc_probe.c -o $@ $(LIBS) $(LDFLAGS)

clean:
	rm -f tc_probe.bpf.o tc_probe

.PHONY: all clean
