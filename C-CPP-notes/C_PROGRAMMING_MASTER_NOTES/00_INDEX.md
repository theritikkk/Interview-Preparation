# C Programming + Systems Programming — Master Notes

A single, cross-referenced study handbook built from three source books, aimed at real
fluency: writing serious C, reading unfamiliar C, understanding memory/pointers/
compilation/linking at a deep level, and being comfortable with the UNIX system
interface, the standard library's design, and low-level debugging.

This is **not** a chapter-by-chapter summary of the source books. It's a synthesized,
dependency-ordered handbook that combines them, fills their gaps with clearly labeled
external material, and is built for both first-pass learning and later revision.

## Status: Phases 1–10 done, Phase 11 underway — 64 of 73 files generated

See `02_LEARNING_ROADMAP.md` for the full file-by-file status table across all 12 phases.

| File | What it is |
|---|---|
| `00_INDEX.md` | This file — start here. |
| `01_BOOK_ANALYSIS.md` | What each of the three source books actually is, what it's good/weak at, a full topic-coverage matrix, and the concept dependency graph. |
| `02_LEARNING_ROADMAP.md` | The 12-phase learning order, the full planned directory tree with generation status, and the recommended external references. |
| `fundamentals/` (Phase 1) | The compilation model, types & representation, expressions/operators/conversions, control flow. |
| `functions/` + `preprocessing/` (Phase 2) | Call-by-value, storage classes, the stack-frame memory model, recursion, macros and their pitfalls, headers, conditional compilation. |
| `memory/01_…` → `03_…` (Phase 3) | The brief's single most-emphasized topic: process memory regions, the full pointer/array duality, pointer arithmetic, dangling/wild pointers, parsing complicated declarations. |
| `memory/04_…`, `06_…`, `07_…` (Phase 4) | Array/string layout, `sizeof` vs. `strlen`, structs/unions/self-referential types (linked lists & trees), struct padding. |
| `systems/01_…`, `02_…` (Phase 5) | Object-file internals, static vs. shared libraries, the linker's symbol resolution/relocation, and the dynamic linker/loader's boot sequence from `./program` to `main`. |
| `memory/05_dynamic_memory.md` (Phase 6) | `malloc`/`calloc`/`realloc`/`free`'s exact contracts, a from-scratch free-list allocator, leaks, double-free, use-after-free, fragmentation, and ownership discipline. |
| `libraries/01_…` → `10_…` (Phase 7 — complete) | All fifteen standard headers by design. |
| `systems/03_file_descriptors_and_unix_io.md` (Phase 8) | The five raw POSIX system calls, two `getchar` implementations built directly on `read`, and a real `fopen`/`getc` sketch showing where `stdio` calls into the kernel. |
| `systems/04_…` → `07_…` (Phase 9 — complete) | The C-language/C-library/POSIX/Linux-syscall four-layer model, `dup`/`dup2`; `fork`'s double-return, the `exec` family, `wait`/`waitpid`, zombies vs. orphans, a minimal-shell project; pipes (and the classic unclosed-write-end hang), POSIX signals as process control, `SIGCHLD` reaping; and a bonus sockets file built on Book A's unique networking chapter. |
| `systems/06_…`, `07_…` (Phase 9) | Pipes (and the classic unclosed-write-end hang), POSIX signals as process control, `SIGCHLD` reaping; a bonus sockets file built on Book A's networking chapter. |
| `debugging/` (Phase 10) | `gdb` and core dumps; sanitizers vs. Valgrind with real captured diagnostic output for five deliberately-triggered bugs; `nm`/`objdump`/`readelf` run against a real build, making relocations literally visible; a symptom-to-tool debugging guide. |
| `advanced/01_…`, `02_…` (Phase 11, in progress) | The complete undefined-behavior catalogue under the standard's four-way classification, an optimizer example that legally deletes an overflow check, and the implementation-defined/portability facts (`char` signedness, struct padding, byte order) scattered through earlier phases, now consolidated. |
| `revision/01_…` → `11_…` | 1-minute + 5-minute revision sheets for Phases 1–10 and (partially) 11. |
| `interviews/01_…` → `11_…` | Easy → systems-level interview questions for Phases 1–10 and (partially) 11. |

**To generate more content:** say **"NEXT"** to finish Phase 11 with
`03_aliasing_and_optimization.md` and `04_modern_c_vs_historical_c.md`, or name any
specific phase/file from `02_LEARNING_ROADMAP.md` to jump there instead.

## Source books (see `01_BOOK_ANALYSIS.md` for full detail)

- **Book A** — *C Programming* (Wikibooks, 2013, CC BY-SA) — broadest practical
  coverage, weakest formal precision.
- **Book B** — *The C Programming Language*, 2nd ed. (Kernighan & Ritchie) — the
  authoritative source on language semantics and the fd/`stdio` relationship.
- **Book C** — *The Standard C Library* (P.J. Plauger, 1992) — deep design-and-
  implementation treatment of the ANSI C standard library, one chapter per header.

**Note:** the original task brief mentions four books; only these three PDFs were
actually uploaded. This is flagged in detail in `01_BOOK_ANALYSIS.md` §1. If a fourth
book exists, add it to the uploads and I'll fold it into the analysis.

The `Papyrus-C++/` folder alongside this one is a separate, pre-existing project
(unrelated notes on a C++ load-testing tool) and is untouched by this handbook.

## How to read notation used throughout this vault

- `[Book-derived]` / `[External]` / `[Synthesis]` / `[Needs verification]` — source-
  discipline tags on non-trivial claims; see `01_BOOK_ANALYSIS.md` §0.
- ⚠️ **Common misconception** · 💡 **Key insight** · 🔬 **Under the hood** ·
  🧠 **Mental model** · ⚠️ **Common bug** · 🎯 **Interview point** ·
  🛠 **Debugging technique** — recurring callout boxes inside topic files.
- Book codes **A / B / C** as above, used everywhere instead of repeating full titles.

## Planned structure (abridged — full tree with status in `02_LEARNING_ROADMAP.md`)

```
C_PROGRAMMING_MASTER_NOTES/
├── 00_INDEX.md · 01_BOOK_ANALYSIS.md · 02_LEARNING_ROADMAP.md · 03_C_MENTAL_MODEL.md
├── fundamentals/    — syntax, types, expressions, control flow
├── functions/       — functions, recursion, function pointers, variadics
├── preprocessing/   — macros, includes, conditional compilation
├── memory/          — memory model, pointers, arrays/strings, dynamic memory, structs
├── libraries/       — one file per standard-header family
├── systems/         — compilation/linking, file descriptors, syscalls, processes, IPC
├── debugging/        — gdb, sanitizers/valgrind, binary inspection, workflows
├── advanced/        — undefined behavior, portability, aliasing, modern C
├── projects/        — beginner / intermediate / systems-level lab projects
├── interviews/      — interview questions by topic
├── revision/        — 1-minute / 5-minute / deep revision sheets per topic
└── references/      — books, external sources, glossary
```

73 Markdown files total when complete (64 done, 9 pending — a direct count of the tree in
`02_LEARNING_ROADMAP.md`; earlier "~55"/"~58" figures were stale undercounts).
