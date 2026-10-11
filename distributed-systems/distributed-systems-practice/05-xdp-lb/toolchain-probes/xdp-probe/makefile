# Three small, real diagnostic tools, each answering one empirical
# question that shaped xdp-lb's design — see ../README.md for which
# question each one answers, and ANALYSIS.md (in 05-xdp-lb/) for what
# the answers were and why they mattered.

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

all: xdp_probe xdp_probe_port xdp_framing_probe

%.bpf.o: %.bpf.c bpf_helpers.h bpf_helper_defs.h bpf_endian.h
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@

xdp_probe: xdp_probe.c xdp_probe.bpf.o $(LIBBPF_DIR)/libbpf.a
	$(CC) $(CFLAGS) xdp_probe.c -o $@ $(LIBS) $(LDFLAGS)

xdp_probe_port: xdp_probe_port.c xdp_probe_port.bpf.o $(LIBBPF_DIR)/libbpf.a
	$(CC) $(CFLAGS) xdp_probe_port.c -o $@ $(LIBS) $(LDFLAGS)

xdp_framing_probe: xdp_framing_probe.c xdp_framing_probe.bpf.o $(LIBBPF_DIR)/libbpf.a
	$(CC) $(CFLAGS) xdp_framing_probe.c -o $@ $(LIBS) $(LDFLAGS)

clean:
	rm -f *.bpf.o xdp_probe xdp_probe_port xdp_framing_probe

.PHONY: all clean
