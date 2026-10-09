/* libelf.h — minimal libelf public API declarations for this sandbox.
 *
 * WHY THIS FILE EXISTS: this sandbox has the real libelf runtime library
 * installed (libelf1t64, providing libelf.so.1 — confirmed via
 * `dpkg -l` and `nm -D`) but NOT the `libelf-dev` package that normally
 * ships libelf.h/gelf.h. apt is network-blocked here (every Ubuntu
 * mirror 403s — same constraint as Projects 2 and 3), and no GitHub
 * mirror of elfutils was reachable through this sandbox's proxy
 * allowlist either. Rather than fall back to a hand-rolled, non-libelf
 * ELF reader (which would mean NOT exercising libbpf's real code at
 * all), this header declares exactly the subset of libelf's genuine,
 * long-stable public C API that libbpf's build actually calls — cross-
 * checked function-by-function against the real exported symbol table
 * (`nm -D libelf-0.190.so`) so every declaration here corresponds to a
 * real symbol in the linked library, not a made-up one. See
 * ANALYSIS.md for the full verification story, including why the two
 * enum constants (ELF_T_BYTE, ELF_T_SYM) that libbpf sets but never
 * reads back can't actually cause a wrong result even if memory of their
 * ordinal is imperfect.
 *
 * Opaque types (Elf, Elf_Scn): libelf has NEVER exposed their internal
 * layout — callers only ever hold pointers and pass them back to libelf
 * functions — so there is zero struct-layout ABI risk for these two,
 * unlike the nostd::shared_ptr ABI issue hit in Project 3.
 *
 * Elf_Data: its handful of public fields ARE part of the stable ABI
 * (documented in `man elf_getdata`) and are reproduced exactly below.
 *
 * GElf_* types: elfutils implements "generic ELF" by aliasing every
 * GElf_* type directly to the real Elf64_* type on a 64-bit build —
 * see gelf.h in this same directory, which pulls those straight from
 * the system's own <elf.h> (glibc's, unmodified, already installed
 * here) rather than redefining anything.
 */
#ifndef LIBELF_SHIM_LIBELF_H
#define LIBELF_SHIM_LIBELF_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <elf.h>   /* glibc's own — already installed, provides Elf64_* */

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque — never dereferenced by callers, only passed by pointer. */
typedef struct Elf Elf;
typedef struct Elf_Scn Elf_Scn;

typedef enum {
	ELF_C_NULL = 0,
	ELF_C_READ,
	ELF_C_RDWR,
	ELF_C_WRITE,
	ELF_C_CLR,
	ELF_C_SET,
	ELF_C_FDDONE,
	ELF_C_FDREAD,
	ELF_C_READ_MMAP,
	ELF_C_RDWR_MMAP,
	ELF_C_WRITE_MMAP,
	ELF_C_READ_MMAP_PRIVATE,
	ELF_C_EMPTY,
	ELF_C_NUM
} Elf_Cmd;

/* Only BYTE/SYM are ever assigned by libbpf, and only on the
 * static-linking (bpf_linker__*) write path this project doesn't
 * exercise — see the comment above. Exact ordinals for the rest are
 * irrelevant here; kept in canonical elfutils order for completeness.
 */
typedef enum {
	ELF_T_BYTE = 0,
	ELF_T_ADDR,
	ELF_T_DYN,
	ELF_T_EHDR,
	ELF_T_HALF,
	ELF_T_OFF,
	ELF_T_PHDR,
	ELF_T_RELA,
	ELF_T_REL,
	ELF_T_SHDR,
	ELF_T_SWORD,
	ELF_T_SYM,
	ELF_T_WORD,
	ELF_T_XWORD,
	ELF_T_SXWORD,
	ELF_T_VDEF,
	ELF_T_VDAUX,
	ELF_T_VNEED,
	ELF_T_VNAUX,
	ELF_T_NHDR,
	ELF_T_SYMINFO,
	ELF_T_MOVE,
	ELF_T_LIB,
	ELF_T_GNUHASH,
	ELF_T_AUXV,
	ELF_T_CHDR,
	ELF_T_NHDR8,
	ELF_T_NUM
} Elf_Type;

typedef enum {
	ELF_K_NONE = 0,
	ELF_K_AR,
	ELF_K_COFF,
	ELF_K_ELF,
	ELF_K_NUM
} Elf_Kind;

/* The stable, documented subset of Elf_Data's fields (man elf_getdata). */
typedef struct {
	void *d_buf;
	Elf_Type d_type;
	unsigned int d_version;
	size_t d_size;
	int64_t d_off;
	size_t d_align;
} Elf_Data;

unsigned int elf_version(unsigned int __version);

Elf *elf_begin(int __fildes, Elf_Cmd __cmd, Elf *__ref);
Elf *elf_memory(char *__image, size_t __size);
int elf_end(Elf *__elf);
const char *elf_errmsg(int __error);

Elf_Kind elf_kind(Elf *__elf);

Elf_Scn *elf_getscn(Elf *__elf, size_t __index);
Elf_Scn *elf_nextscn(Elf *__elf, Elf_Scn *__scn);
Elf_Scn *elf_newscn(Elf *__elf);
size_t elf_ndxscn(Elf_Scn *__scn);

Elf_Data *elf_getdata(Elf_Scn *__scn, Elf_Data *__data);
Elf_Data *elf_rawdata(Elf_Scn *__scn, Elf_Data *__data);
Elf_Data *elf_newdata(Elf_Scn *__scn);

int elf_getshdrnum(Elf *__elf, size_t *__dst);
int elf_getshdrstrndx(Elf *__elf, size_t *__dst);
int elf_getphdrnum(Elf *__elf, size_t *__dst);

char *elf_strptr(Elf *__elf, size_t __index, size_t __offset);

off_t elf_update(Elf *__elf, Elf_Cmd __cmd);

/* Class-specific (not "generic GElf") accessors — real elfutils
 * declares these in libelf.h, not gelf.h. This sandbox is x86_64-only,
 * so only the 64-bit forms are declared (the 32-bit elf32_* forms
 * exist in the real library too but nothing here calls them).
 */
Elf64_Ehdr *elf64_getehdr(Elf *__elf);
Elf64_Shdr *elf64_getshdr(Elf_Scn *__scn);
Elf64_Ehdr *elf64_newehdr(Elf *__elf);

#ifdef __cplusplus
}
#endif

#endif /* LIBELF_SHIM_LIBELF_H */
