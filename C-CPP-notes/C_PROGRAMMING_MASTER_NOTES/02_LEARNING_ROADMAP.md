# 02 — Learning Roadmap

> Depends on `01_BOOK_ANALYSIS.md`. Book codes **A** = Wikibooks *C Programming*,
> **B** = K&R *The C Programming Language*, **C** = Plauger *The Standard C Library*.

This is the dependency-aware, phase-by-phase plan the rest of the vault is written to.
Each phase lists prerequisites, the topics it covers, which book(s) drive it, the planned
file(s) it will produce, a project, and a rough difficulty rating.

---

## Phase 1 — C Fundamentals
**Prerequisites:** none (some general programming exposure helps).
**Primary sources:** B ch.1–3 (primary), A ch.7–16 (breadth, compiler usage).
**Topics:** program structure, compilation model overview, types & sizes, literals,
expressions, operators, conversions/promotions, control flow, `sizeof`, basic I/O with
`printf`/`scanf`.
**Files:** `fundamentals/01_program_structure.md`, `02_types.md`,
`03_expressions_and_operators.md`, `04_control_flow.md`.
**Project:** calculator, temperature converter, simple I/O tools (beginner tier).
**Difficulty:** ★☆☆☆☆

## Phase 2 — Functions, Scope & the Preprocessor
**Prerequisites:** Phase 1.
**Primary sources:** B ch.4 (primary), A ch.17–19 (macro debugging patterns).
**Topics:** function declarations/definitions, call-by-value, storage classes (`static`,
`extern`, `register`), scope & linkage, block structure, recursion, the preprocessor
(`#define`, `#include`, conditional compilation, include guards, macro pitfalls).
**Files:** `functions/01_functions_and_stack_frames.md`, `02_recursion.md`,
`preprocessing/01_macros.md`, `02_includes_and_header_design.md`,
`03_conditional_compilation.md`.
**Project:** a small multi-file program with a proper header, to make "why do we need
headers/linkage" concrete.
**Difficulty:** ★★☆☆☆

## Phase 3 — Pointers & Memory
**Prerequisites:** Phases 1–2.
**Primary sources:** B ch.5 (primary, including §5.12 complicated declarations), A ch.25
(extra worked examples).
**Topics:** addresses, pointer variables, dereferencing, pointer arithmetic, pointer/array
duality and decay, pointers to pointers, `const`-qualified pointers, function pointers,
`NULL`, dangling/wild pointers, parsing complicated declarations.
**Files:** `memory/01_memory_model.md`, `02_pointers.md`,
`03_pointer_declarations_parsing.md`.
**Project:** implement `strlen`/`strcpy`/`strcmp` from scratch using only pointer
arithmetic, to internalize decay and arithmetic.
**Difficulty:** ★★★★☆ — the single most important phase per the task brief.

## Phase 4 — Arrays, Strings, Structs & Unions
**Prerequisites:** Phase 3.
**Primary sources:** B ch.1.6/5.3–5.9/ch.6 (primary), C ch.14 `<string.h>` (design +
implementation angle), A ch.24/27/28 (breadth).
**Topics:** array layout, multidimensional arrays, string literals vs. char arrays,
`sizeof` vs. `strlen`, struct layout, padding & alignment, nested/self-referential structs,
unions, bit-fields.
**Files:** `memory/04_arrays_and_strings.md`, `06_struct_union_layout.md`,
`07_alignment_padding_aliasing.md`.
**Project:** singly linked list, then a binary search tree, using self-referential structs.
**Difficulty:** ★★★☆☆

## Phase 5 — Compilation & Linking Toolchain
**Prerequisites:** Phase 2 (headers), helps to have Phase 3 done too.
**Primary sources:** A ch.9 (primary — this is A's strongest chapter), `[External]` for
object-file/symbol/relocation depth.
**Topics:** preprocess → compile → assemble → object file → link → executable;
`gcc -E/-S/-c`; static vs. shared libraries; symbols and relocation; the linker's job; why
headers exist; why C can't compile "the whole program at once."
**Files:** `systems/01_compilation_toolchain.md`, `02_linking_and_loading.md`.
**Project:** manually drive each toolchain stage on a 3-file program and inspect the
intermediate artifacts (`.i`, `.s`, `.o`) by hand.
**Difficulty:** ★★★☆☆

## Phase 6 — Dynamic Memory
**Prerequisites:** Phases 3–4.
**Primary sources:** C ch.13 `<stdlib.h>` (primary, real allocator implementation), B
§8.7 (a from-scratch allocator, different angle), A ch.26 (practical usage).
**Topics:** stack vs. heap vs. static/global storage, `malloc`/`calloc`/`realloc`/`free`,
allocator bookkeeping strategies, fragmentation, leaks, double-free, use-after-free,
ownership.
**Files:** `memory/05_dynamic_memory.md`.
**Project:** a fixed-block custom allocator (`myalloc`/`myfree`) with a free-list, then run
it under a sanitizer (bridges into Phase 9).
**Difficulty:** ★★★★☆

## Phase 7 — The Standard Library, By Design
**Prerequisites:** Phases 1–6 conceptually complete; can start once Phase 4 is done.
**Primary sources:** C, all 15 chapters (primary — this is C's entire reason for being), B
Appendix B (quick-reference companion).
**Topics:** one file per header family — `<stdio.h>` (incl. buffering & the `getchar`
macro story), `<stdlib.h>`, `<string.h>`, `<ctype.h>`, `<math.h>`, `<assert.h>`/
`<errno.h>`, `<setjmp.h>`/`<signal.h>`, `<stdarg.h>`, `<time.h>`,
`<limits.h>`/`<float.h>`/`<stddef.h>`/`<locale.h>`.
**Files:** `libraries/01_stdio.md` … `10_limits_float_stddef_locale.md` (10 files, grouped
by natural cohesion rather than 1:1 with all 15 headers — see file plan below).
**Project:** re-implement a slice of `<string.h>` and a `printf`-lite formatter.
**Difficulty:** ★★★☆☆ (per header) / ★★★★☆ (`stdio.h` internals)

## Phase 8 — File I/O, Both Layers
**Prerequisites:** Phases 6–7.
**Primary sources:** all three — A (`stdio` breadth), C (`stdio` internals & standard
contract), B ch.8.1–8.5 (the fd layer *and* `stdio`-on-fd bridge implementation).
**Topics:** streams vs. file descriptors, buffering modes, the `Application → stdio →
fd → kernel → filesystem` chain, `open`/`read`/`write`/`close`/`lseek`, implementing a
`FILE*`-like layer over raw fds.
**Files:** `systems/03_file_descriptors_and_unix_io.md`.
**Project:** a `cat`-like tool written twice — once with `stdio`, once with raw fds — and
compare.
**Difficulty:** ★★★☆☆

## Phase 9 — UNIX / POSIX System Interface
**Prerequisites:** Phase 8.
**Primary sources:** B ch.8 (only in-book source, file-I/O slice only), `[External]` for
everything process-related.
**Topics:** C language vs. C standard library vs. POSIX vs. Linux syscalls (kept
explicitly distinct, per the brief); `dup`/`dup2`; processes; `fork`/`exec`/`wait`/`waitpid`;
pipes; signals as process control (distinct from `<signal.h>` as a language feature);
environment and `argv`.
**Files:** `systems/04_system_calls_vs_libc_vs_posix.md`, `05_processes_fork_exec_wait.md`,
`06_pipes_and_signals.md`. *(Bonus, from Book A's unique content:)*
`07_sockets_and_networking.md`.
**Project:** a minimal shell (parse → `fork` → `exec` → `wait`); a two-process pipeline
using `pipe()` + `dup2()`.
**Difficulty:** ★★★★★ — heaviest external-synthesis load in the whole vault.

## Phase 10 — Debugging & Tooling
**Prerequisites:** useful any time after Phase 3; essential by Phase 6.
**Primary sources:** entirely `[External]`.
**Topics:** `gcc`/`clang` diagnostics, `gdb`, `nm`/`objdump`/`readelf`/`size`/`strings`/`file`,
`strace`, Valgrind, AddressSanitizer/UndefinedBehaviorSanitizer, `make`.
**Files:** `debugging/01_toolchain_gdb.md`, `02_sanitizers_and_valgrind.md`,
`03_static_and_binary_inspection.md`, `04_debugging_workflows_labs.md`.
**Project:** deliberately inject a segfault, a leak, a double-free, and a buffer overflow
into working code, then find each with the appropriate tool.
**Difficulty:** ★★★☆☆

## Phase 11 — Undefined Behavior, Portability & Modern C
**Prerequisites:** everything above, conceptually.
**Primary sources:** `[Synthesis]` of B's Appendix A and C's "What the Standard Says"
sections (both implicitly define the UB/IDB boundary without naming it as a topic),
`[External]` for the systematic catalogue and for C99–C23.
**Topics:** undefined vs. unspecified vs. implementation-defined behavior; a worked
catalogue of classic UB traps (signed overflow, OOB access, strict aliasing, use-after-
free, invalid shifts, format-string mismatches); how optimizers exploit UB; historical
C89 → C99 → C11 → C17 → C23 deltas relevant to what B and C describe as current.
**Files:** `advanced/01_undefined_behavior.md`, `02_implementation_defined_and_portability.md`,
`03_aliasing_and_optimization.md`, `04_modern_c_vs_historical_c.md`.
**Difficulty:** ★★★★★

## Phase 12 — Projects & Interview Preparation
**Prerequisites:** rolling — each project draws on whichever phases it needs.
**Files:** `projects/01_beginner_projects.md`, `02_intermediate_projects.md`,
`03_systems_projects.md` (mini shell, allocator, hexdump, grep-like tool, directory
lister, Unix pipeline); `interviews/01_c_fundamentals_questions.md` …
`04_debugging_questions.md`; `revision/` — 1-minute/5-minute/deep summaries per topic,
built incrementally alongside every phase above rather than as an afterthought.

---

## Proposed directory structure & file plan

This expands on the tree implied above into the full proposed layout, with a generation
status column. Nothing in this column is generated yet except the three files marked
**Done** — that's intentional: per the task brief, this response is the analysis-and-planning
pass only.

```
C-CPP-notes/
├── C_Programming.pdf          (existing — Book A)
├── c.pdf                      (existing — Book B)
├── clib.pdf                   (existing — Book C)
├── Papyrus-C++/               (existing, unrelated project — left untouched)
└── C_PROGRAMMING_MASTER_NOTES/
    ├── 00_INDEX.md                                    · Done
    ├── 01_BOOK_ANALYSIS.md                             · Done
    ├── 02_LEARNING_ROADMAP.md                          · Done
    ├── 03_C_MENTAL_MODEL.md                            · Pending
    │
    ├── fundamentals/
    │   ├── 01_program_structure.md                     · Done
    │   ├── 02_types.md                                 · Done
    │   ├── 03_expressions_and_operators.md             · Done
    │   └── 04_control_flow.md                          · Done
    │
    ├── functions/
    │   ├── 01_functions_and_stack_frames.md            · Done
    │   └── 02_recursion.md                             · Done
    │       (function-pointer syntax folded into memory/03 below; deeper callback/
    │        dispatch-table patterns will land wherever they're next needed, and
    │        variadic functions moved to libraries/08_stdarg.md, Phase 7, matching
    │        Book C's own chapter — see the note after this tree)
    │
    ├── preprocessing/
    │   ├── 01_macros.md                                · Done
    │   ├── 02_includes_and_header_design.md            · Done
    │   └── 03_conditional_compilation.md               · Done
    │
    ├── memory/
    │   ├── 01_memory_model.md                          · Done
    │   ├── 02_pointers.md                              · Done
    │   ├── 03_pointer_declarations_parsing.md          · Done
    │   ├── 04_arrays_and_strings.md                    · Done
    │   ├── 05_dynamic_memory.md                        · Pending (Phase 6)
    │   ├── 06_struct_union_layout.md                   · Done
    │   └── 07_alignment_padding_aliasing.md            · Done
    │
    ├── libraries/
    │   ├── 01_stdio.md                                 · Pending
    │   ├── 02_stdlib.md                                · Pending
    │   ├── 03_string.md                                · Pending
    │   ├── 04_ctype_and_locale.md                      · Pending
    │   ├── 05_math.md                                  · Pending
    │   ├── 06_assert_and_errno.md                      · Pending
    │   ├── 07_setjmp_and_signal.md                     · Pending
    │   ├── 08_stdarg.md                                · Pending
    │   ├── 09_time.md                                  · Pending
    │   └── 10_limits_float_stddef.md                   · Pending
    │
    ├── systems/
    │   ├── 01_compilation_toolchain.md                 · Done
    │   ├── 02_linking_and_loading.md                   · Done
    │   ├── 03_file_descriptors_and_unix_io.md          · Pending
    │   ├── 04_system_calls_vs_libc_vs_posix.md         · Pending
    │   ├── 05_processes_fork_exec_wait.md              · Pending
    │   ├── 06_pipes_and_signals.md                     · Pending
    │   └── 07_sockets_and_networking.md                · Pending (bonus, Book A)
    │
    ├── debugging/
    │   ├── 01_toolchain_gdb.md                         · Pending
    │   ├── 02_sanitizers_and_valgrind.md               · Pending
    │   ├── 03_static_and_binary_inspection.md          · Pending
    │   └── 04_debugging_workflows_labs.md              · Pending
    │
    ├── advanced/
    │   ├── 01_undefined_behavior.md                    · Pending
    │   ├── 02_implementation_defined_and_portability.md· Pending
    │   ├── 03_aliasing_and_optimization.md             · Pending
    │   └── 04_modern_c_vs_historical_c.md              · Pending
    │
    ├── projects/
    │   ├── 01_beginner_projects.md                     · Pending
    │   ├── 02_intermediate_projects.md                 · Pending
    │   └── 03_systems_projects.md                      · Pending
    │
    ├── interviews/
    │   ├── 01_c_fundamentals_questions.md              · Done
    │   ├── 02_functions_and_preprocessor_questions.md  · Done
    │   ├── 03_pointers_and_memory_questions.md         · Done
    │   ├── 04_arrays_structs_questions.md              · Done
    │   ├── 05_compilation_linking_questions.md         · Done
    │   ├── 06_systems_and_unix_questions.md            · Pending
    │   └── 07_debugging_questions.md                   · Pending
    │
    ├── revision/
    │   ├── 01_fundamentals_revision.md                 · Done
    │   ├── 02_functions_and_preprocessor_revision.md   · Done
    │   ├── 03_pointers_and_memory_revision.md          · Done
    │   ├── 04_arrays_structs_revision.md               · Done
    │   ├── 05_compilation_linking_revision.md          · Done
    │   ├── 06_stdlib_revision.md                       · Pending
    │   └── 07_systems_and_advanced_revision.md         · Pending
    │
    └── references/
        ├── 01_books.md                                 · Pending
        ├── 02_external_sources.md                      · Pending
        └── 03_glossary.md                               · Pending
```

**Estimated total file count: ~55 Markdown files** (30 done — the 3 planning files, all of
Phases 1–5 — ~25 pending). One planned file,
`functions/03_function_pointers_and_variadics.md`, was dropped from the plan: function-
pointer syntax turned out to belong with `memory/03_pointer_declarations_parsing.md` (both
K&R and the declaration-parsing skill treat it as a pointer topic), and variadic functions
were moved to `libraries/08_stdarg.md` (Phase 7), matching Book C's own chapter structure
rather than the original guess. The `revision/`/`interviews/` folders were expanded from
the original 4-file sketch to roughly one entry per phase, since that scales better as
content is actually generated. This tree is deliberately more granular than the brief's
example in a few other places too (e.g. splitting `libraries/` per header-family rather
than lumping) because Book C's own structure — one deep chapter per header — maps naturally
onto one focused note file per header family, which keeps individual files a manageable
revision unit instead of one giant library mega-file.

---

## Recommended external references

Used only where the three books have a gap (see `01_BOOK_ANALYSIS.md` §6), and
always labeled `[External]` at the point of use. No URL below is invented — each is a
real, stable, well-known reference; a couple were spot-checked live while preparing this
roadmap.

| Source | Why it's useful |
|---|---|
| [cppreference — C language](https://en.cppreference.com/w/c) | The best free, example-rich, version-annotated (C89/C99/C11/C17/C23) reference for exact language and library semantics — used wherever a book is silent or pre-C99. |
| [Linux man-pages — man7.org](https://man7.org/linux/man-pages/) | The canonical reference for every syscall (`man2`) and libc function (`man3`) on Linux — used throughout Phases 8–9 for `open`, `read`, `fork`, `pipe`, `dup2`, etc. |
| [POSIX.1-2024 / Open Group Base Specs, Issue 8](https://pubs.opengroup.org/onlinepubs/9799919799/) | The formal POSIX standard text — used to keep "this is POSIX" claims precise and distinct from "this is Linux-specific" or "this is ISO C." |
| [GNU C Library (glibc) manual](https://www.gnu.org/software/libc/manual/) | glibc-specific behavior and extensions beyond the standard — useful when a book's "implementation-defined" note needs a concrete real-world answer. |
| [GCC documentation](https://gcc.gnu.org/onlinedocs/) | Compiler flags, diagnostics, and GCC-specific extensions/attributes referenced in the compilation and debugging phases. |
| [Clang / LLVM documentation](https://clang.llvm.org/docs/) | Same role as GCC docs, plus the primary source for sanitizer usage (ASan/UBSan) in Phase 10. |
| [CERT C Secure Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c) | Systematic catalogue of C pitfalls with rule IDs — the backbone for turning the books' scattered "be careful here" notes into the dedicated UB/portability chapters in Phase 11. |
| Beej's Guides (Network Programming, and the C guide) | Widely used, precise, beginner-friendly companion for exactly the socket-programming material Book A introduces but doesn't fully explain — will be cited by name and general site (`beej.us`) with the specific page verified at the point of use rather than guessed here. |

Anything from the ISO/IEC C standard itself will be described rather than quoted at
length (it isn't freely republishable); where a precise clause matters, the reference will
point to the public *drafts* the ISO committee makes available (e.g. the N-numbered
working drafts) rather than the paywalled final standard, verified at the point of use.

---

## How the PART system works from here

**Phases 1–5 are now done.** Phases 1–4 covered C fundamentals, functions/scope/the
preprocessor, pointers & memory (the brief's single most-emphasized phase), and arrays/
strings/structs/unions — see each phase's revision file for a condensed recap. Phase 5
(Compilation & Linking Toolchain) added `systems/01_compilation_toolchain.md` and
`02_linking_and_loading.md`, going well past Book A's brief chapter 9 into object-file
internals, static/shared libraries, and the dynamic linker/loader — necessarily mostly
`[External]`/`[Synthesis]`, since none of the three books goes this deep. Every phase
re-opened the actual relevant book pages for its specific topics, with book-derived claims
tagged — including catching one real transcription error in the source `binsearch` listing
back in Phase 1.

- Say **"NEXT"** to generate Phase 6 (Dynamic Memory), or name a different phase/file from
  the plan above to jump there instead.
- Each subsequent part follows the same process: re-open the relevant book pages for that
  specific topic, write the topic file(s), then that phase's revision and interview entries.
