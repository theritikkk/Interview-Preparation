# Reference: The Three Source Books

> Full analysis, strengths, and limitations: `01_BOOK_ANALYSIS.md`. This file is the
> bibliography entry alone — what to cite, and the book codes (A/B/C) used throughout
> every other file in this vault.

---

## Book A — *C Programming*

- **Title:** *C Programming*
- **Author:** Collaborative — Wikibooks contributors (en.wikibooks.org); no single named
  author
- **Edition/date:** PDF export dated 24 November 2013
- **License:** CC BY-SA 3.0
- **Length:** 285 pages
- **Role in this vault:** broadest practical breadth of the three — compiler usage,
  file I/O, UNIX sockets (the only book of the three to touch networking), code style.
  Uneven depth across chapters due to its collaborative, wiki origin.

## Book B — *The C Programming Language*, 2nd Edition

- **Title:** *The C Programming Language*, 2nd edition
- **Authors:** Brian W. Kernighan and Dennis M. Ritchie
- **Edition/date:** describes ANSI C (C89); originally published 1988; this scan converted
  2002
- **Length:** 217 pages (reflowed; the original print edition is ~272 pages)
- **Role in this vault:** the primary source for language semantics, pointers, structures,
  and the UNIX system interface (the only one of the three books to build `stdio` on raw
  file descriptors in working code). The single most-cited book in this vault by a wide
  margin.

## Book C — *The Standard C Library*

- **Title:** *The Standard C Library*
- **Author:** P.J. Plauger — chaired the Library Subcommittee of X3J11 (the ANSI C
  standards committee) and was Secretary/Convenor of the ISO C committee
- **Publisher/year:** Prentice Hall PTR, 1992
- **Length:** 514 pages (scanned; OCR quality noticeably degraded throughout — treat any
  quoted fragment traced to this book as approximate unless independently verified)
- **Role in this vault:** the primary source for the standard library's *design
  rationale* — real, portable implementations and the history behind nearly every
  standard header, from `<assert.h>`'s four simultaneous design constraints to
  `<errno.h>`'s contested history to a full free-list `malloc`/`free` implementation.

---

## A fourth book was never provided

`01_BOOK_ANALYSIS.md` §1 flagged this at the very start of this project: the original task
brief referenced four source books, but only these three PDFs were ever uploaded. This
vault was built on three throughout, with the gap filled by `[External]`/`[Synthesis]`
material, clearly tagged at every point of use — see `02_external_sources.md` for the full
list of what filled it.

## Citation convention used throughout this vault

Every file in this vault refers to these three books by the short codes **A**, **B**, and
**C** defined above, and tags every non-trivial claim with its source:
`[Book-derived, A/B/C]`, `[External]`, `[Synthesis]`, or `[Needs verification]` — the exact
discipline `01_BOOK_ANALYSIS.md` §0 established at the project's outset and held to across
all 66 topic files.
