# C Programming + Systems Programming — Master Notes

A single, cross-referenced study handbook built from three source books, aimed at real
fluency: writing serious C, reading unfamiliar C, understanding memory/pointers/
compilation/linking at a deep level, and being comfortable with the UNIX system
interface, the standard library's design, and low-level debugging.

This is **not** a chapter-by-chapter summary of the source books. It's a synthesized,
dependency-ordered handbook that combines them, fills their gaps with clearly labeled
external material, and is built for both first-pass learning and later revision.

## Status: Phases 1–2 done — 16 of ~56 files generated

See `02_LEARNING_ROADMAP.md` for the full file-by-file status table across all 12 phases.

| File | What it is |
|---|---|
| `00_INDEX.md` | This file — start here. |
| `01_BOOK_ANALYSIS.md` | What each of the three source books actually is, what it's good/weak at, a full topic-coverage matrix, and the concept dependency graph. |
| `02_LEARNING_ROADMAP.md` | The 12-phase learning order, the full planned directory tree with generation status, and the recommended external references. |
| `fundamentals/01_program_structure.md` → `04_control_flow.md` | Phase 1: the compilation model, types & representation, expressions/operators/conversions, control flow. |
| `functions/01_functions_and_stack_frames.md`, `02_recursion.md` | Phase 2: call-by-value, storage classes, scope, the stack-frame memory model, recursion. |
| `preprocessing/01_macros.md` → `03_conditional_compilation.md` | Phase 2: macros and their pitfalls, `#include`/header design, conditional compilation. |
| `revision/01_…` & `02_…` | 1-minute + 5-minute revision sheets for Phases 1–2. |
| `interviews/01_…` & `02_…` | Easy → systems-level interview questions for Phases 1–2. |

**To generate more content:** say **"NEXT"** to begin Phase 3 (Pointers & Memory — the
brief's single most-emphasized topic), or name any specific phase/file from
`02_LEARNING_ROADMAP.md` to jump there instead.

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

~56 Markdown files total when complete (16 done, ~40 pending).
