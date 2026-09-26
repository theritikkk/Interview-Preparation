# Revision — Phase 5: Compilation & Linking Toolchain

> Quick-reference companion to `systems/01_compilation_toolchain.md` and
> `02_linking_and_loading.md`.

---

## The compilation toolchain — 1-minute summary

An object file (`.o`) contains machine code (`.text`), initialized data (`.data`), a
*size only* for zero-initialized data (`.bss` — no literal zero bytes stored), read-only
data like string literals (`.rodata`), a symbol table (what it defines, what it still
needs), and relocation entries (where to patch in addresses once known). A static library
(`.a`) is a bundle of `.o` files, pulled in selectively at build time; a shared library
(`.so`) stays external, resolved by a separate dynamic linker at load time — smaller
binaries and independently-updatable libraries, at the cost of a runtime dependency.

### 5-minute revision
- `-fPIC` generates position-independent (relative, not absolute) addressing — required
  for shared libraries, since they can load at a different address in every process.
- `.bss` doesn't bloat file size because the file only records a *size* for zero-
  initialized data, not the actual zero bytes — those get filled in at load time.
- `make` (or any build system) skips recompiling a target whose dependencies haven't
  changed since it was last built — the practical answer to "why doesn't one file change
  force a full rebuild."
- Optimization (`-O2`, etc.) is free to transform code any way that preserves
  *standard-defined* behavior — which is exactly why undefined behavior can "look
  different" at different optimization levels.

**Worth memorizing:** what `.text`/`.data`/`.bss`/`.rodata` each hold, cold.
**Worth understanding, not memorizing:** *why* static and shared libraries trade off
binary size against runtime dependency (compilation_toolchain.md §5) — you should be able
to argue the tradeoff, not just recite which is bigger.

---

## Linking & loading — 1-minute summary

The linker does exactly two things: symbol resolution (match every undefined reference to
one definition) and relocation (patch in real addresses once known). "Undefined reference"
is a link-time error meaning no definition was found anywhere on the link line — often a
missing `.o`/library, or a `static` name used from the wrong file. "Multiple definition"
usually means a full function body or non-`extern` variable definition sitting in a header
included from more than one `.c` file. Dynamic linking defers the real resolution/
relocation work to a separate program (the dynamic linker) that runs at process startup,
before your own `main`.

### 5-minute revision
- Both classic linker errors are **link-stage**, not compile-stage — every individual file
  compiled fine; check which `.o` files actually got produced to tell the two failure
  classes apart when triaging.
- The OS doesn't call `main` directly — it calls a C-runtime startup routine first, which
  does setup, calls `main`, and turns `main`'s return value into the process's real exit
  status afterward.
- `ldd program` (Linux) shows every shared library dependency and whether the dynamic
  linker actually found each one — a "not found" here is the load-time twin of a build-time
  undefined reference.
- `LD_LIBRARY_PATH` (and related search paths) determine where the dynamic linker looks for
  shared libraries at startup — a library that "exists" but isn't on that search path still
  fails to load.

**Worth memorizing:** the two linker error categories and their most common real causes.
**Worth understanding, not memorizing:** the full boot sequence from `./program` to `main`
running (linking_and_loading.md §6) — walk through it from memory rather than recalling it
as a list.

---

## Cross-topic traps worth drilling before an interview

1. "undefined reference to `foo`" despite `foo` clearly being defined somewhere in the
   project — almost always a missing file/library on the link line, or a `static`-linkage
   mismatch (linking_and_loading.md §2).
2. "multiple definition of `foo`" — a full function/variable definition living in a header,
   included from more than one `.c` file (§2).
3. A shared library that "should" be there but `ldd` reports as "not found" — a search-path
   problem (`LD_LIBRARY_PATH` etc.), not a missing-file problem, most of the time (§5).
4. Assuming `main` is the very first code that runs in a process — the C runtime's startup
   routine runs first and is what actually calls `main` (§6).
