# Reference: External Sources

> Every external reference cited across this vault's 66 topic files, consolidated and
> organized by kind. The three source books have their own entry: `01_books.md`. First
> compiled in `02_LEARNING_ROADMAP.md`'s planning pass; this is the complete, final list
> as actually used throughout the finished vault.

---

## Language and library reference

- **[cppreference.com — C](https://en.cppreference.com/w/c)** — the single
  most-cited external source in this vault. Version-annotated (C89/C99/C11/C17/C23)
  coverage of the language and every standard header; used wherever a source book is
  silent or predates a feature.
- **[The Open Group Base Specifications, Issue 8 (POSIX.1-2024)](https://pubs.opengroup.org/onlinepubs/9799919799/)**
  — the formal POSIX standard text, used throughout `systems/04_…` → `07_…` (Phase 9) for
  `fork`/`exec`/`wait`/`pipe`/`dup2`/`dirent` and everything else none of the three books
  cover.

## Platform and tooling documentation

- **[man7.org — Linux man-pages](https://man7.org/linux/man-pages/)** — the second
  most-cited source, for every POSIX/Linux system call and library function used from
  Phase 8 onward: `read(2)`, `write(2)`, `open(2)`, `fork(2)`, `execve(2)`, `wait(2)`,
  `pipe(7)`, `sigaction(2)`, `signal-safety(7)`, `environ(7)`, `dup(2)`,
  `gethostbyname(3)`, `getaddrinfo(3)`, `strtok(3)`, `core(5)`, `proc(5)`, `ldd(1)`,
  `nm(1)`, `objdump(1)`, `readelf(1)`, `size(1)`, `strings(1)`, `file(1)`,
  `addr2line(1)`, `strace(1)`.
- **[GCC documentation](https://gcc.gnu.org/onlinedocs/)** — compiler flags
  (`-E`/`-S`/`-c`, `-fPIC`, `-shared`, `-fsanitize=`), GCC-specific predefined macros, and
  `-fstrict-aliasing`'s documented behavior.
- **[Clang/LLVM documentation](https://clang.llvm.org/docs/)** — primary source for
  AddressSanitizer and UndefinedBehaviorSanitizer's documented report formats
  (`debugging/02_sanitizers_and_valgrind.md`).
- **[Valgrind/Memcheck manual](https://valgrind.org/docs/manual/mc-manual.html)** —
  Memcheck's report format and leak-category definitions (described, not captured live,
  since Valgrind wasn't installed in this vault's build environment).
- **[GNU C Library (glibc) manual](https://www.gnu.org/software/libc/manual/)** — glibc's
  concrete choices where the standard leaves a behavior open, and `make`/`Makefile`
  syntax reference.
- **[GDB User Manual](https://www.gnu.org/software/gdb/documentation/)** — the
  command reference behind `debugging/01_toolchain_gdb.md` (documented usage; gdb itself
  wasn't installed in this vault's build environment, so no live transcript exists there).

## Security and coding-standard references

- **[CERT C Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c)** — systematic,
  rule-numbered coverage cited for string-handling pitfalls, memory-management rules,
  the `system()` command-injection risk, and — most centrally —
  **EXP39-C**, the exact source for `advanced/03_aliasing_and_optimization.md`'s
  strict-aliasing effective-type list and its union-type-punning exception.

## Companion learning resources

- **Beej's Guide to Network Programming** (`beej.us/guide/bgnet/`) — recommended
  alongside `systems/07_sockets_and_networking.md` as the natural next step for
  socket programming beyond this vault's bonus-chapter treatment.
- **cdecl.org** — a declaration ⇄ English translator tool, recommended in
  `memory/03_pointer_declarations_parsing.md` for checking manual declaration-parsing
  practice against.

## Specific facts verified live against an external source while writing this vault

`[Synthesis]` A short, honest list of claims that weren't simply recalled, but actively
checked during this vault's construction, because they mattered enough to get exactly
right:

- `gcc -shared -fPIC`/`ar rcs` exact command syntax (`systems/01_compilation_toolchain.md`)
- `fork`/`execve`/`waitpid` signatures and semantics (`systems/05_processes_fork_exec_wait.md`)
- `pipe(7)`'s exact `PIPE_BUF`/EOF/`SIGPIPE` semantics (`systems/06_pipes_and_signals.md`)
- `gethostbyname`'s obsolescence and `getaddrinfo`'s role as its replacement
  (`systems/07_sockets_and_networking.md`)
- `opendir`/`readdir`'s `d_name` sizing warning, quoted directly from POSIX's own text
  (`projects/03_systems_projects.md`)
- The strict-aliasing effective-type list and the union-punning exception, against CERT
  C's EXP39-C (`advanced/03_aliasing_and_optimization.md`)
- C23's two's-complement mandate and its explicit non-effect on signed-overflow UB
  (`advanced/04_modern_c_vs_historical_c.md`)
- The C99+ rule that reaching the `}` of `main` returns 0, against the standard's own
  wording (§5.1.2.2.3) (`fundamentals/01_program_structure.md`)

## Facts *measured* directly in this vault's build environment

`[Synthesis]` Where a claim could be settled by compiling and running a few lines rather
than citing anyone, that was done (GCC 13.3.0, x86-64 Linux, glibc). Results are specific to
that environment and are labeled as such wherever they appear:

- AddressSanitizer / LeakSanitizer / UndefinedBehaviorSanitizer transcripts for five bug
  programs (`debugging/02_sanitizers_and_valgrind.md`)
- `nm`, `size`, `readelf`, `objdump -d -r`, `strings`, `file`, `ldd`, and both classic
  linker errors against a real two-file build (`debugging/03_static_and_binary_inspection.md`)
- A strict-aliasing violation printing `0` at `-O0` and `1` at `-O2`, plus consistent union
  and `memcpy` punning (`advanced/03_aliasing_and_optimization.md`)
- Integer division by zero: a real `idiv` + `SIGFPE` (exit 136) with a non-constant
  numerator, versus **no `idiv` and a silent exit 0** with the constant numerator `1` —
  same compiler, same flags; plus IEEE `inf`/`-inf`/`-nan` for floating point
  (`advanced/01_undefined_behavior.md`)
- Tail-call optimization: a 100-million-deep recursion segfaulting at `-O0`/`-O1` and
  succeeding at `-O2`/`-O3` (`functions/02_recursion.md`)
- `sizeof(struct sockaddr)` = `sizeof(struct sockaddr_in)` = 16, but
  `sizeof(struct sockaddr_in6)` = 28 (`systems/07_sockets_and_networking.md`)

## What this vault deliberately did *not* verify live

`[Synthesis, for honesty]` Well-established, decades-stable technical facts (ELF section
names, `nm` symbol-type letters, the shell's `128 + signal` exit-status convention, and
similar) were drawn from training knowledge rather than re-verified against a live source
for each one — these are the kind of facts stable enough that re-verification would have
added citation overhead without meaningfully changing accuracy. Where this vault's own
`debugging/` phase could instead *generate real, live output* (compiling and running actual
programs in its own build environment), it did exactly that in preference to either
approach — see `debugging/02_…` and `03_…` for the results.
