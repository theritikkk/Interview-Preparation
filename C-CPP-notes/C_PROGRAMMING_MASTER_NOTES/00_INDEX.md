# C Programming + Systems Programming — Master Notes

A single, cross-referenced study handbook built from three source books, aimed at real
fluency: writing serious C, reading unfamiliar C, understanding memory/pointers/
compilation/linking at a deep level, and being comfortable with the UNIX system
interface, the standard library's design, and low-level debugging.

This is **not** a chapter-by-chapter summary of the source books. It's a synthesized,
dependency-ordered handbook that combines them, fills their gaps with clearly labeled
external material — in several places verified live against real, executed code rather
than taken on faith — and is built for both first-pass learning and later revision.

## Status: complete — all 73 files generated across 12 phases

See `02_LEARNING_ROADMAP.md` for the full file-by-file map and generation history, or
`03_C_MENTAL_MODEL.md` for the one-file synthesis of the entire vault.

| File / folder | What it covers |
|---|---|
| `00_INDEX.md` | This file — start here. |
| `01_BOOK_ANALYSIS.md` | What each of the three source books actually is, strengths/weaknesses, the topic-coverage matrix, and the concept dependency graph. |
| `02_LEARNING_ROADMAP.md` | The full 12-phase learning order and directory plan, plus the complete, real generation history. |
| `03_C_MENTAL_MODEL.md` | The capstone: every "chain" diagram from every phase assembled into one picture, and the five recurring patterns that explain most of what's confusing about C. |
| `fundamentals/` | The compilation model, types & representation, expressions/operators/conversions, control flow. |
| `functions/` + `preprocessing/` | Call-by-value, storage classes, the stack-frame memory model, recursion, macros and their pitfalls, headers, conditional compilation. |
| `memory/` | The brief's single most-emphasized topic, in full: process memory regions, pointer/array duality, pointer arithmetic, parsing complicated declarations, arrays & strings, dynamic memory (a from-scratch allocator), structs/unions, padding & alignment. |
| `systems/` | The compilation/linking toolchain, file descriptors beneath `stdio`, the C/libc/POSIX/kernel-syscall distinction, `fork`/`exec`/`wait`, pipes & signals, and a bonus sockets chapter. |
| `libraries/` | All fifteen ISO C standard headers, by design — not just by API. |
| `debugging/` | `gdb` and core dumps; sanitizers vs. Valgrind with real captured diagnostic output; `nm`/`objdump`/`readelf` against a real build; a symptom-to-tool debugging guide. |
| `advanced/` | The complete undefined-behavior catalogue; implementation-defined behavior & portability; the strict aliasing rule (verified with a real optimization-level divergence); modern C vs. historical C. |
| `projects/` | Beginner, intermediate, and systems-level labs — each with requirements, architecture, pitfalls, testing, and extensions. |
| `revision/` | 1-minute + 5-minute revision sheets, one per phase. |
| `interviews/` | Easy → systems-level interview questions, one set per phase. |
| `references/` | The book bibliography, every external source this vault cited, and a glossary. |

## Source books (see `01_BOOK_ANALYSIS.md` for full detail)

- **Book A** — *C Programming* (Wikibooks, 2013, CC BY-SA) — broadest practical
  coverage, weakest formal precision.
- **Book B** — *The C Programming Language*, 2nd ed. (Kernighan & Ritchie) — the
  authoritative source on language semantics and the fd/`stdio` relationship.
- **Book C** — *The Standard C Library* (P.J. Plauger, 1992) — deep design-and-
  implementation treatment of the ANSI C standard library, one chapter per header.

**Note:** the original task brief mentions four books; only these three PDFs were
actually uploaded. This is flagged in detail in `01_BOOK_ANALYSIS.md` §1.

The `Papyrus-C++/` folder alongside this one is a separate, pre-existing project
(unrelated notes on a C++ load-testing tool) and is untouched by this handbook.

## How to read the notation used throughout this vault

- `[Book-derived]` / `[External]` / `[Synthesis]` / `[Needs verification]` — source-
  discipline tags on non-trivial claims; see `01_BOOK_ANALYSIS.md` §0.
- ⚠️ **Common misconception** · 💡 **Key insight** · 🔬 **Under the hood** ·
  🧠 **Mental model** · ⚠️ **Common bug** · 🎯 **Interview point** ·
  🛠 **Debugging technique** — recurring callout boxes inside topic files.
- Book codes **A / B / C** as above, used everywhere instead of repeating full titles.

## Full structure

```
C_PROGRAMMING_MASTER_NOTES/
├── 00_INDEX.md · 01_BOOK_ANALYSIS.md · 02_LEARNING_ROADMAP.md · 03_C_MENTAL_MODEL.md
├── fundamentals/    — syntax, types, expressions, control flow                  (4 files)
├── functions/       — functions & stack frames, recursion                      (2 files)
├── preprocessing/   — macros, includes, conditional compilation                (3 files)
├── memory/          — memory model, pointers, arrays/strings, dynamic memory,
│                      structs/unions, alignment & padding                      (7 files)
├── systems/         — compilation/linking, file descriptors, syscalls,
│                      processes, pipes & signals, sockets                      (7 files)
├── libraries/       — one file per standard-header family, all 15 headers     (10 files)
├── debugging/       — gdb, sanitizers/Valgrind, binary inspection, workflows   (4 files)
├── advanced/        — undefined behavior, portability, aliasing, modern C      (4 files)
├── projects/        — beginner / intermediate / systems-level lab projects     (3 files)
├── interviews/      — interview questions by topic, one set per phase        (11 files)
├── revision/        — 1-minute / 5-minute revision sheets, one per phase      (11 files)
└── references/      — books, external sources, glossary                       (3 files)
```

**73 Markdown files, all generated.** Where to go from here is up to you — jump to
whatever phase you need, or start at `03_C_MENTAL_MODEL.md` for the big picture before
diving into any one topic in depth.
