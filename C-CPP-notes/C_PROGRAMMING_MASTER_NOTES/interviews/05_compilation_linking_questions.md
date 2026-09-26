# Interview Questions — Compilation & Linking Toolchain

> Consolidated from `systems/01_compilation_toolchain.md` and `02_linking_and_loading.md`.

---

## Easy

1. **What does `.bss` store, and why doesn't it bloat the binary's file size?** Reserved
   space for zero-initialized static/global data; only a size is recorded, not literal zero
   bytes. (`01_compilation_toolchain.md` §3)
2. **What two things does the linker do?** Symbol resolution and relocation.
   (`02_linking_and_loading.md` §1)
3. **Is "undefined reference to `foo`" a compile-time or link-time error?** Link-time —
   each file compiled fine individually; no definition was found across the whole link
   line. (§2)

## Medium

1. **What's the practical difference between a static and a shared library, from a
   deployed program's point of view?** Static is fully baked into the executable at build
   time (self-contained, larger); shared stays external, resolved by the dynamic linker at
   load time (smaller, but a runtime dependency). (`01_compilation_toolchain.md` §4–5)
2. **What does `-fPIC` actually change about generated code?** It generates address-
   relative rather than absolute addressing, since a shared library may load at a different
   address in every process. (§5)
3. **What's the most common real-world cause of a "multiple definition" linker error?** A
   full function body or non-`extern` variable definition sitting in a header included
   from more than one `.c` file. (`02_linking_and_loading.md` §2)
4. **Why does an object file need both a symbol table and relocation entries?** Compilation
   is per translation unit, so an object file can't know final addresses for names defined
   elsewhere; the symbol table records what it needs/provides, relocations mark exactly
   where addresses must be patched in later. (`01_compilation_toolchain.md` §3)

## Hard

1. **Explain precisely why optimization level can change whether a program "appears
   correct."** The optimizer only has to preserve standard-defined behavior; code relying
   on undefined behavior has no guaranteed behavior to preserve, so different optimization
   levels can expose (or hide) its consequences differently. (`01_compilation_toolchain.md` §2)
2. **Walk through everything that happens between running a dynamically-linked executable
   and its `main` actually executing.** OS maps the executable's own segments in; the
   dynamic linker locates and maps every shared library dependency and finishes their
   symbol resolution/relocation; the stack is prepared with `argc`/`argv`/environment;
   control transfers to a C-runtime startup routine, which does final setup and then calls
   `main`. (`02_linking_and_loading.md` §5–6)
3. **Why does `.bss` need to be zero-filled by the loader at process-start time rather than
   the linker writing zeros into the file at build time?** The entire point of `.bss` is
   that the file doesn't store the zero bytes at all — writing them at build time would
   defeat that space saving; zeroing is deferred to the OS mapping the region in fresh at
   load time. (§6)

## Systems-level

1. **A colleague replaces a static library dependency with the equivalent shared library in
   production. What operational changes does this introduce, beyond binary size?** A new
   runtime dependency on locating the correct `.so` at startup (missing/incompatible
   becomes a runtime failure instead of a build-time one); independent updatability without
   rebuilding the program; potential memory savings from multiple processes sharing one
   physical copy of the library's code pages. (`01_compilation_toolchain.md` §5)
2. **`ldd` reports "not found" for a shared library dependency on one machine, though the
   same executable runs fine elsewhere. What's the likely explanation?** A search-path
   problem specific to that machine (`LD_LIBRARY_PATH`, `-rpath`, or default library
   directories) — the load-time analog of a build-time undefined-reference error, not
   necessarily a missing file in any absolute sense. (`02_linking_and_loading.md` §5)
3. **What's the most likely explanation for "undefined reference to `foo`" when `foo`
   clearly exists in the project's source?** A missing `.o`/library on the link line, or
   `foo` having `static` (internal) linkage in a file other than the one trying to call it.
   (§2)
