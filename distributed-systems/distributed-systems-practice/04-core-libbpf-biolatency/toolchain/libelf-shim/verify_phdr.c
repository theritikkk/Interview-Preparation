// Extends the shim-verification methodology to the newly-added
// phdr/note APIs: cross-checks elf_getphdrnum/gelf_getphdr against
// `readelf -l`, and gelf_getnote against `readelf -n`, on the same
// real binary.
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <gelf.h>

int main(int argc, char **argv) {
    if (argc != 2) { fprintf(stderr, "usage: %s <elf-file>\n", argv[0]); return 2; }
    elf_version(EV_CURRENT);
    int fd = open(argv[1], O_RDONLY);
    Elf *elf = elf_begin(fd, ELF_C_READ, NULL);
    if (!elf) { fprintf(stderr, "elf_begin failed\n"); return 1; }

    size_t phnum;
    if (elf_getphdrnum(elf, &phnum)) { fprintf(stderr, "elf_getphdrnum failed\n"); return 1; }
    printf("phnum=%zu\n", phnum);
    for (size_t i = 0; i < phnum; i++) {
        GElf_Phdr phdr;
        if (!gelf_getphdr(elf, i, &phdr)) { fprintf(stderr, "gelf_getphdr failed\n"); return 1; }
        printf("  [%zu] type=0x%x offset=0x%lx vaddr=0x%lx filesz=0x%lx memsz=0x%lx flags=0x%x\n",
               i, phdr.p_type, (unsigned long)phdr.p_offset, (unsigned long)phdr.p_vaddr,
               (unsigned long)phdr.p_filesz, (unsigned long)phdr.p_memsz, phdr.p_flags);
    }

    // Walk notes in whichever section is of type SHT_NOTE (7).
    Elf_Scn *scn = NULL;
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        GElf_Shdr shdr;
        gelf_getshdr(scn, &shdr);
        if (shdr.sh_type != 7 /* SHT_NOTE */) continue;
        Elf_Data *data = elf_getdata(scn, NULL);
        size_t off = 0, name_off, desc_off;
        GElf_Nhdr nhdr;
        printf("--- notes in section ---\n");
        while ((off = gelf_getnote(data, off, &nhdr, &name_off, &desc_off)) > 0) {
            const char *name = (const char *)data->d_buf + name_off;
            printf("  note: namesz=%u descsz=%u type=%u name=%s\n",
                   nhdr.n_namesz, nhdr.n_descsz, nhdr.n_type, name);
        }
    }
    printf("OK\n");
    return 0;
}
