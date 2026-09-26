# Linking & Loading

> **Phase 5 · Compilation & Linking Toolchain.** Depends on: `01_compilation_toolchain.md`
> §3 (object file contents — symbol tables and relocation entries), `memory/01_memory_model.md`
> §2 (the process memory-layout picture this file explains the *construction* of).
> Feeds into: `systems/03_file_descriptors_and_unix_io.md` (Phase 8), `debugging/03_static_and_binary_inspection.md`
> (Phase 10 — the tools that inspect exactly what's described here).
> Sources: **Book A** §9.4 (the "missing/duplicate functions" linker-error framing —
> brief but accurate). **Everything else in this file — relocation mechanics, the dynamic
> linker, the loader's exact job — is `[External]`/`[Synthesis]`**; none of the three books
> goes below "the linker combines object code into a program."

---

## 1. The linker's job, precisely

`[Synthesis, formalizing `01_compilation_toolchain.md` §3]` Given a set of object files
(and libraries), the linker does exactly two things:

1. **Symbol resolution** — match every *undefined* symbol reference in every object file
   against exactly one *definition* of that symbol, somewhere among all the inputs.
2. **Relocation** — now that every symbol has a known final address, patch every place in
   the code/data that referenced it (recorded as a relocation entry, per
   `01_compilation_toolchain.md` §3) with that real address.

Both steps are mechanical and exhaustive: if step 1 fails for *any* referenced symbol, or
step 2 finds a symbol resolved to *more than one* definition, the link fails outright — a
program either links completely or not at all; there's no partial linking of "the parts
that worked."

---

## 2. Symbol resolution failures, decoded

`[Book-derived, A, the two error categories; the exact message text and worked diagnosis
below is `[Synthesis]`]` Book A names exactly the two failure modes; here's what each
actually looks like and means in practice.

### "undefined reference to `foo`"

```
main.o: In function `main':
main.c:5: undefined reference to `compute_result'
```

**What happened:** some object file references `compute_result`, but **no** object file or
library on the link line provides a definition for it. Common real causes: forgetting to
list a `.c`/`.o` file (or a library, `-lfoo`) on the link command; a typo in the function
name that the *compiler* couldn't catch (a call to an undeclared-but-later-declared
function compiles fine — the compiler only checks it exists eventually — but the linker
finds nothing matching); or the name existing but with `static` linkage
(`functions/01_functions_and_stack_frames.md` §5) in a *different* file than the one trying
to call it, making it invisible outside its own translation unit by design.

### "multiple definition of `foo`"

```
/usr/bin/ld: b.o: in function `helper':
b.c:3: multiple definition of `helper'; a.o:a.c:3: first defined here
```

**What happened:** two different object files both provide a full (non-`static`)
definition of the same name — violating the "exactly one definition" rule from
`functions/01_functions_and_stack_frames.md` §3 and
`preprocessing/02_includes_and_header_design.md` §2. The overwhelmingly common real cause:
a full function body (or a non-`extern` variable definition) placed directly in a
**header**, then that header included from more than one `.c` file — each including file's
object file ends up with its own definition, and the linker refuses to arbitrarily pick one.

### 🛠 Debugging technique
Both error categories are link-stage errors, not compile-stage ones — each individual
`.c` file compiled *successfully* (it was internally self-consistent); the failure only
becomes visible once the linker tries to assemble the whole picture. This is worth
remembering when triaging a build failure: "which `.o` files were actually produced before
the failure" tells you whether you're looking at a compile problem (some `.o` never got
made) or a genuine link-stage symbol problem (every `.o` exists, but they don't fit
together).

---

## 3. What a relocation entry actually patches

`[External]` A relocation entry says, in effect, "at byte offset X within this section,
there's a reference to symbol Y that needs to be patched in once Y's real address is
known, using patching method Z." Two broad relocation kinds matter conceptually:

- **Absolute relocation** — write the symbol's literal final address directly into the
  instruction/data. Simple, but means the code is only valid if actually loaded at the
  address the linker assumed — fine for a statically-linked, fixed-load-address executable,
  but incompatible with position-independent shared libraries (§5 below).
- **Relative (PC-relative) relocation** — write an *offset* from the referencing
  instruction's own address to the symbol's address, rather than an absolute address. This
  is what `-fPIC` code (`01_compilation_toolchain.md` §5) relies on: the offset between two
  pieces of the *same* library stays constant no matter where the whole library ends up
  loaded, so relative relocations remain valid regardless of load address.

---

## 4. Static linking's timeline vs. dynamic linking's timeline

`[Synthesis, consolidating `01_compilation_toolchain.md` §4–5 into an explicit timeline]`

```
STATIC:  compile → assemble → LINK (symbol resolution + relocation, fully, here) → executable → load → run
                                        ↑ all the work happens once, at build time

SHARED:  compile → assemble → LINK (only records "depends on libfoo.so", plus a table
          of stub references) → executable → LOAD (the dynamic linker locates libfoo.so,
          maps it in, and finishes resolving + relocating the stub references) → run
                                        ↑ the real symbol resolution is deferred to load time
```

This is the precise mechanical difference behind the table in
`01_compilation_toolchain.md` §5: a shared library's linking work is genuinely incomplete
at build time, finished by a **separate program** — the dynamic linker — every time the
executable actually starts running.

---

## 5. The dynamic linker (loader): resolving shared libraries at startup

`[External]` On Linux, this program is conventionally `ld.so`/`ld-linux.so` (often invoked
automatically, listed inside the executable itself as its "interpreter"). When a
dynamically-linked executable starts:

1. The OS loads the executable's own code/data into a fresh process's virtual address
   space (§6 below), then hands control to the dynamic linker instead of the program's own
   `main` first.
2. The dynamic linker reads the executable's list of needed shared libraries, and searches
   a defined set of locations for each (a compiled-in default path, directories named via
   `-rpath` at link time, and the `LD_LIBRARY_PATH` environment variable, checked in a
   specific, implementation-defined order — `[External]`, exact precedence is
   platform/configuration-dependent).
3. Each found library is mapped into the process's address space, and the *remaining*
   symbol resolution + relocation work (§4) is completed against the now-known real
   addresses.
4. Only once every dependency is resolved does control finally transfer to the program's
   own entry point.

### 🛠 Debugging technique
`ldd program` (`[External]`, Linux) lists every shared library a dynamically-linked
executable depends on, and where (if anywhere) the dynamic linker found each one at that
moment — "not found" entries here are exactly the runtime version of the build-time
"undefined reference" error from §2, just discovered at process-startup time instead of
link time. Full tool coverage is `debugging/03_static_and_binary_inspection.md`, Phase 10.

---

## 6. From executable to running process — the loader's other job

`[External]/[Synthesis, connecting directly back to `memory/01_memory_model.md` §2's
process-layout diagram]` Loading isn't only about shared libraries — it's also what
actually *builds* the memory layout that file described conceptually:

1. The OS reads the executable's own section headers (`.text`, `.data`, `.bss`, `.rodata`
   — `01_compilation_toolchain.md` §3) and maps each into the new process's virtual address
   space, with appropriate permissions (`.text` typically read+execute but not write;
   `.data`/`.bss` read+write but not execute — a real, enforced security boundary on modern
   systems, `[External]`).
2. `.bss`'s reserved region is zero-filled at this point (`01_compilation_toolchain.md`
   §3's "BSS trick," now happening for real) — this is where those zero-initialized
   `static`/global variables actually get their zero bytes, lazily, as pages are touched.
3. A stack region is set up, and `argc`/`argv`/the environment (`envp` —
   `[External]`, a third, conventional parameter to `main` on many platforms, not part of
   ISO C itself) are placed on it for the program to find.
4. The heap starts out essentially empty — nothing is reserved for it until the program
   actually calls `malloc` (Phase 6) for the first time.
5. Control transfers to the program's real entry point — conventionally a small runtime
   startup routine (often named `_start`, `[External]`, supplied by the C runtime/libc, not
   by your own code) that does final setup and **then** calls your `main`, and, when `main`
   returns, calls `exit` with `main`'s return value — which is *why* `return 0;` in `main`
   (`fundamentals/01_program_structure.md` §5) actually becomes the process's observable
   exit status: `main` returning is not the process ending, it's a return into this
   startup routine, which then explicitly ends the process.

### 🎯 Interview point
"What happens between typing `./program` and your `main` running?" — This entire section,
condensed: the OS creates a process and maps the executable's segments in; if dynamically
linked, the dynamic linker resolves and maps in every shared library dependency first;
`.bss` gets zeroed; the stack is prepared with `argc`/`argv`/environment; and a small
runtime startup routine — not the OS directly — is what actually calls `main`, and handles
what happens after it returns.

---

## 7. ⚠️ Misconceptions to unlearn now

- **"The OS calls `main` directly."** It calls a small C-runtime startup routine first,
  which does its own setup and *then* calls `main` — and also handles turning `main`'s
  return value into the process's actual exit status afterward (§6).
- **"A link error means my code has a bug in it."** Not necessarily *your* code's logic —
  it means the *set of inputs given to the linker* doesn't fully resolve; a perfectly
  correct function can trigger "undefined reference" simply because its `.o` file wasn't
  listed on the link command (§2).
- **"Shared libraries are resolved once, when they're installed."** They're resolved fresh
  by the dynamic linker **every time** a dependent program starts (§5) — nothing is
  "pre-baked" the way static linking's work is.

---

## 8. 🎯 Interview questions

**Easy**
- What two things does the linker do? → Symbol resolution and relocation (§1).
- Is "undefined reference to `foo`" a compile-time or link-time error? → Link-time — each
  individual file compiled fine; the linker just couldn't find a definition anywhere on
  the link line (§2).

**Medium**
- What's the most common real-world cause of a "multiple definition" linker error? → A
  full function body (or non-`extern` variable definition) placed in a header that's
  included from more than one `.c` file (§2).
- What's the practical difference between absolute and PC-relative relocations? → Absolute
  relocations bake in a literal final address, valid only if the code loads exactly where
  assumed; PC-relative relocations use an offset that stays valid regardless of where the
  whole unit is actually loaded — the basis for position-independent shared libraries (§3).

**Hard**
- Walk through, step by step, everything that happens between running a dynamically-linked
  executable and its `main` actually executing. → OS maps the executable's own segments in;
  control passes to the dynamic linker, which locates and maps every needed shared library
  and finishes their symbol resolution/relocation; the stack is prepared with `argc`/
  `argv`/environment; control transfers to a C-runtime startup routine, which performs
  final setup and *then* calls `main` (§5–6).
- Why does `.bss` need to be zero-filled by the *loader* at process-start time, rather than
  the linker simply writing zero bytes into the executable file at build time? → The whole
  point of `.bss` is that the file doesn't need to store the zero bytes at all — only a
  size; writing them at build time would defeat that space-saving entirely, so the actual
  zeroing is deferred to the OS mapping the region in fresh (often via cheap, lazily-
  faulted-in zero pages) at load time (§6, `01_compilation_toolchain.md` §3).

**Systems-level**
- `ldd` reports "not found" for one of a program's shared library dependencies on a
  particular machine, even though the same executable runs fine elsewhere. What's the most
  likely explanation, and how does it map onto this file's model? → The dynamic linker's
  search — compiled-in paths, `-rpath`, `LD_LIBRARY_PATH` — isn't finding that library's
  file *on this machine specifically*; this is the runtime, load-time analog of a
  build-time "undefined reference" error (§2, §5) — the library genuinely exists somewhere
  in the world (the program links successfully elsewhere), just not anywhere this
  particular machine's dynamic linker is configured to search.

---

## 9. External references

- [man7.org — `ld.so(8)`](https://man7.org/linux/man-pages/) `[External]` — the Linux
  dynamic linker/loader's own manual page: search order, `LD_LIBRARY_PATH`, and related
  environment variables in full, current detail.
- [man7.org — `ldd(1)`](https://man7.org/linux/man-pages/) `[External]` — the tool
  previewed in §5's debugging box.
- [Oracle — Linker and Libraries Guide](https://gcc.gnu.org/onlinedocs/) `[External, a
  well-known cross-platform-relevant reference despite the vendor name — verify the
  current live URL before relying on it]` — a thorough, freely available treatment of ELF
  linking concepts beyond this file's scope.

## 10. 🛠 Practice

1. Deliberately create both linker error categories from §2 (omit an object file from a
   link line; define the same non-`static` function in two `.c` files both included via a
   header) and read the exact error messages your toolchain produces, matching each phrase
   back to this file's explanation.
2. Build the 3-file program from `01_compilation_toolchain.md`'s practice exercise as a
   dynamically-linked executable against your own shared library, then run `ldd` on it and
   identify every dependency it lists, including ones you didn't explicitly link against
   (the C standard library itself will appear here).
3. On Linux, set `LD_LIBRARY_PATH` to a directory *not* containing your shared library,
   attempt to run the executable, and read the resulting runtime error — then fix it by
   pointing `LD_LIBRARY_PATH` at the right directory, to see the exact difference between a
   build-time and a load-time "can't find this" failure.
