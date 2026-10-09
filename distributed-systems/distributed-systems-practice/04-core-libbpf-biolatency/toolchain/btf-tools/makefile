# btf-tools — small diagnostic programs that query the live kernel's BTF
# directly via libbpf's own API, rather than guessing struct names for
# CO-RE programs and finding out only when the verifier rejects them.
# See ANALYSIS.md (in 04-core-libbpf/) for the real bug these tools found.

LIBBPF_DIR = ../libbpf/src
LIBELF_DIR = ../libelf-shim

CC = gcc
CFLAGS = -g -O2 -Wall -I$(LIBBPF_DIR) -I$(LIBELF_DIR)/include
LDFLAGS = -L$(LIBELF_DIR)/lib -Wl,-rpath,$(abspath $(LIBELF_DIR)/lib)
LIBS = $(LIBBPF_DIR)/libbpf.a -lelf -lz -lpthread

all: btf_probe btf_search

btf_probe: btf_probe.c $(LIBBPF_DIR)/libbpf.a
	$(CC) $(CFLAGS) $< -o $@ $(LIBS) $(LDFLAGS)

btf_search: btf_search.c $(LIBBPF_DIR)/libbpf.a
	$(CC) $(CFLAGS) $< -o $@ $(LIBS) $(LDFLAGS)

clean:
	rm -f btf_probe btf_search

.PHONY: all clean
