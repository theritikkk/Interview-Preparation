/* verify_shim.c — empirically checks the hand-written libelf.h/gelf.h
 * shim against the real libelf runtime, by opening a real binary and
 * cross-checking what we read against `readelf`'s independent parse of
 * the exact same file. If our struct layouts, enum values, or function
 * signatures were wrong, this would show up here as nonsense output or
 * a crash — not just a compile error — which a diff against readelf
 * exposes immediately.
 */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <gelf.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <elf-file>\n", argv[0]);
        return 2;
    }

    if (elf_version(EV_CURRENT) == EV_NONE) {
        fprintf(stderr, "elf_version() FAILED: %s\n", elf_errmsg(-1));
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    Elf *elf = elf_begin(fd, ELF_C_READ, NULL);
    if (!elf) {
        fprintf(stderr, "elf_begin() FAILED: %s\n", elf_errmsg(-1));
        return 1;
    }

    Elf_Kind kind = elf_kind(elf);
    printf("elf_kind() = %d (expect ELF_K_ELF = %d)\n", kind, ELF_K_ELF);
    if (kind != ELF_K_ELF) {
        fprintf(stderr, "MISMATCH\n");
        return 1;
    }

    GElf_Ehdr ehdr;
    if (!gelf_getehdr(elf, &ehdr)) {
        fprintf(stderr, "gelf_getehdr() FAILED: %s\n", elf_errmsg(-1));
        return 1;
    }
    printf("e_type=%u e_machine=%u e_shnum(hdr)=%u e_shstrndx(hdr)=%u\n",
           ehdr.e_type, ehdr.e_machine, ehdr.e_shnum, ehdr.e_shstrndx);

    size_t shstrndx;
    if (elf_getshdrstrndx(elf, &shstrndx) != 0) {
        fprintf(stderr, "elf_getshdrstrndx() FAILED: %s\n", elf_errmsg(-1));
        return 1;
    }
    printf("elf_getshdrstrndx() = %zu\n", shstrndx);

    size_t shnum;
    if (elf_getshdrnum(elf, &shnum) != 0) {
        fprintf(stderr, "elf_getshdrnum() FAILED: %s\n", elf_errmsg(-1));
        return 1;
    }
    printf("elf_getshdrnum() = %zu\n", shnum);

    printf("--- sections (name, type, size) ---\n");
    Elf_Scn *scn = NULL;
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        GElf_Shdr shdr;
        if (!gelf_getshdr(scn, &shdr)) {
            fprintf(stderr, "gelf_getshdr() FAILED: %s\n", elf_errmsg(-1));
            return 1;
        }
        const char *name = elf_strptr(elf, shstrndx, shdr.sh_name);
        printf("  [%zu] %-20s type=%u size=%lu\n",
               elf_ndxscn(scn), name ? name : "<?>", shdr.sh_type,
               (unsigned long)shdr.sh_size);
    }

    elf_end(elf);
    close(fd);
    printf("OK\n");
    return 0;
}
