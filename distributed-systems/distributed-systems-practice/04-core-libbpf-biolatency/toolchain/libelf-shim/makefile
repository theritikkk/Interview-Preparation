# libelf-shim verification tools — verify_shim.c and verify_phdr.c each
# parse a real ELF file through this shim's libelf.h/gelf.h declarations
# AND independently via readelf, then diff the two. See ../../biolatency
# ANALYSIS.md for why this shim exists (libelf-dev isn't installable here)
# and how it was verified function-by-function against the real
# libelf.so.1's exported symbol table before being trusted.

CC = gcc
CFLAGS = -g -O2 -Wall -Iinclude
LDFLAGS = -Llib -Wl,-rpath,$(abspath lib)
LIBS = -lelf

all: verify_shim verify_phdr

verify_shim: verify_shim.c
	$(CC) $(CFLAGS) $< -o $@ $(LIBS) $(LDFLAGS)

verify_phdr: verify_phdr.c
	$(CC) $(CFLAGS) $< -o $@ $(LIBS) $(LDFLAGS)

clean:
	rm -f verify_shim verify_phdr

.PHONY: all clean
