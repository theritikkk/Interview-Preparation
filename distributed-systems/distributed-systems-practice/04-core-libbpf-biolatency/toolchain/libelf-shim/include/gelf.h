/* gelf.h — minimal "generic ELF" declarations, matching real elfutils'
 * own approach: on a 64-bit build, every GElf_* type is just the real
 * Elf64_* type (already defined in the system's own <elf.h>, included
 * via libelf.h) — elfutils' gelf_get*() functions upconvert from
 * 32-bit on the fly internally so callers only ever see the 64-bit
 * shape. This sandbox only ever opens x86_64 BPF object files, so that
 * holds throughout. See libelf.h for why this shim exists at all and
 * how it was verified against the real installed library.
 */
#ifndef LIBELF_SHIM_GELF_H
#define LIBELF_SHIM_GELF_H

#include "libelf.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef Elf64_Ehdr    GElf_Ehdr;
typedef Elf64_Shdr    GElf_Shdr;
typedef Elf64_Sym     GElf_Sym;
typedef Elf64_Dyn     GElf_Dyn;
typedef Elf64_Rel     GElf_Rel;
typedef Elf64_Rela    GElf_Rela;
typedef Elf64_Phdr    GElf_Phdr;
typedef Elf64_Verdef  GElf_Verdef;
typedef Elf64_Verdaux GElf_Verdaux;
typedef Elf64_Verneed GElf_Verneed;
typedef Elf64_Vernaux GElf_Vernaux;
typedef Elf64_Versym  GElf_Versym;
typedef Elf64_Half    GElf_Half;
typedef Elf64_Word    GElf_Word;
typedef Elf64_Xword   GElf_Xword;
typedef Elf64_Addr    GElf_Addr;
typedef Elf64_Off     GElf_Off;
typedef Elf64_Section GElf_Section;
/* Note headers (ELF notes, e.g. USDT probe descriptors) are the same
 * layout in 32- and 64-bit ELF — real elfutils aliases GElf_Nhdr
 * straight to Elf64_Nhdr too, which glibc's <elf.h> already defines. */
typedef Elf64_Nhdr GElf_Nhdr;

GElf_Ehdr *gelf_getehdr(Elf *__elf, GElf_Ehdr *__dst);
GElf_Shdr *gelf_getshdr(Elf_Scn *__scn, GElf_Shdr *__dst);
GElf_Phdr *gelf_getphdr(Elf *__elf, int __ndx, GElf_Phdr *__dst);
GElf_Sym *gelf_getsym(Elf_Data *__data, int __ndx, GElf_Sym *__dst);
int gelf_getclass(Elf *__elf);
GElf_Verdaux *gelf_getverdaux(Elf_Data *__data, int __offset, GElf_Verdaux *__dst);
GElf_Verdef *gelf_getverdef(Elf_Data *__data, int __offset, GElf_Verdef *__dst);
GElf_Versym *gelf_getversym(Elf_Data *__data, int __ndx, GElf_Versym *__dst);
/* Walks ELF notes one at a time: pass offset=0 first, then feed back
 * the returned offset until it returns 0 (end) or sets errno on error. */
size_t gelf_getnote(Elf_Data *__data, size_t __offset, GElf_Nhdr *__result,
		     size_t *__name_offset, size_t *__desc_offset);

/* st_info bit-twiddling macros — real elfutils just forwards these to
 * the ELF64_ST_* macros already in glibc's <elf.h> (identical to the
 * ELF32 ones; the st_info byte layout doesn't change between classes). */
#define GELF_ST_BIND(val)       ELF64_ST_BIND(val)
#define GELF_ST_TYPE(val)       ELF64_ST_TYPE(val)
#define GELF_ST_INFO(bind, type) ELF64_ST_INFO((bind), (type))

#ifdef __cplusplus
}
#endif

#endif /* LIBELF_SHIM_GELF_H */
