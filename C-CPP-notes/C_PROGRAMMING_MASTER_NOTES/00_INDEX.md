# C Programming + Systems Programming — Master Notes

A single, cross-referenced study handbook built from three source books, aimed at real
fluency: writing serious C, reading unfamiliar C, understanding memory/pointers/
compilation/linking at a deep level, and being comfortable with the UNIX system
interface, the standard library's design, and low-level debugging.

This is **not** a chapter-by-chapter summary of the source books. It's a synthesized,
dependency-ordered handbook that combines them, fills their gaps with clearly labeled
external material, and is built for both first-pass learning and later revision.

## Status: Phases 1–5 done — 30 of ~55 files generated

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
| `revision/01_…` → `05_…` | 1-minute + 5-minute revision sheets for Phases 1–5. |
| `interviews/01_…` → `05_…` | Easy → systems-level interview questions for Phases 1–5. |

**To generate more content:** say **"NEXT"** to begin Phase 6 (Dynamic Memory), or name
any specific phase/file from `02_LEARNING_ROADMAP.md` to jump there instead.

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

~55 Markdown files total when complete (30 done, ~25 pending).
