# 37 — Building a Simple OS: the Checkpoint

## Layer map for this file

```
┌──────────────────────────────────────────────────┐
│ 1. BOOK: this entire file — a consolidated,      │
│    working build of exactly what Blundell's      │
│    draft delivers, Ch.1 through §6.2, and        │
│    not one line further                          │
├──────────────────────────────────────────────────┤
│ 2. SUPPORTING: none new — this is assembly,      │
│    not new concepts                              │
├──────────────────────────────────────────────────┤
│ 3. MODERN: none — file 38 is where modern/       │
│    beyond-the-book construction resumes          │
└──────────────────────────────────────────────────┘
```

Book coverage: **Full.** This file is different from every other one in
this course: it introduces nothing new. It's the checkpoint — proof, in
one working build, of everything files `01`–`17` actually taught, and
the explicit line where the book's own draft runs out.

## Prerequisites

Files `01`–`36`. This is best attempted only once everything through
file `17` has actually been built and booted individually.

---

## Where the Book's Draft Actually Stops

![The book's table of contents, revisited: Chapters 1 through 5 and sections 6.1-6.2 shown solid green as fully delivered and built; sections 6.3 through Chapter 8 shown hatched as stubs — headings with no body text in the draft, left for file 38 to build as clearly-marked beyond-the-book material](images/37-book-completion-map.svg)

This is the audit's opening finding, paid off: you have now built,
faithfully, everything in the green boxes. Nothing in the hatched boxes
exists in the draft at all — file `38` builds all of them, and will
never pretend otherwise.

---

## The Complete Project

```
systems-from-scratch-os/
├── boot/
│   ├── boot_sect.asm      ← entry point; includes everything below
│   ├── gdt.asm            ← file 08
│   ├── print_string.asm   ← file 05
│   ├── print_string_pm.asm← file 08
│   ├── disk_load.asm      ← file 17
│   └── switch_to_pm.asm   ← file 08
├── kernel/
│   ├── kernel.c           ← file 13 (§5.2.1)
│   ├── kernel_entry.asm   ← file 12 (§5.2.3)
│   ├── low_level.c        ← file 15 (port I/O wrappers)
│   └── low_level.h
├── drivers/
│   ├── screen.c           ← file 16 (§6.2.2–6.2.3)
│   └── screen.h
└── Makefile               ← file 13 (§5.3)
```

Every path above traces to a specific file in this course, and every
one of those files traces to a specific, real section of Blundell's
draft — `book-to-course-map.md` gives the exhaustive version.

## Assembling It

`kernel.c` now calls the real driver instead of writing to `0xb8000`
directly:

```c
#include "../drivers/screen.h"

void main() {
    clear_screen();
    print("Hello from the kernel, with a real screen driver!\n");
    for (int i = 0; i < 30; i++) {
        print("Line to force scrolling...\n");
    }
}
```

The boot sector (file `06`'s magic number, file `08`'s mode switch,
file `17`'s disk load) stays exactly as built across those files —
nothing about it changes just because the kernel got more capable.

```
make            # assembles boot_sect.bin and kernel.bin, links kernel_entry.o + object files, cats them into os-image
make run        # qemu-system-i386 -fda os-image, or your Makefile's equivalent target
```

**What you should see:** the real-mode message first (BIOS teletype),
then the protected-mode landing message, then your kernel's own output
via the real screen driver — scrolling correctly once it fills the
screen, exactly as file `16` built it.

## The Full Audit Trail

| Piece | Built in | Book section |
|---|---|---|
| Magic number, infinite loop | `06` | §2.2 |
| Real-mode registers, interrupts | `03`, `05` | §3.3 |
| Labels, strings, stack, functions | `05` | §3.4 |
| CHS, BIOS disk read | `17` | §3.6 |
| GDT, protected-mode switch | `08` | §4.1–4.5 |
| C compilation pipeline | `11` | §5.1.1 |
| Stack frames, calling convention | `10` | §5.1.2–5.1.4 |
| Kernel bootstrap, disk-loaded kernel | `13`, `07` | §5.2.1–5.2.2 |
| `kernel_entry.asm` fix | `12` | §5.2.3 |
| Make, code organization | `13` | §5.3 |
| Port I/O, `low_level.c` | `15` | §6.1 |
| Screen driver, scrolling | `16` | §6.2 |

Everything in this table is real, checkable against the actual PDF.
Nothing past it is.

---

## Practical Experiment

1. Build the complete tree above from scratch, file by file, rather
   than copying anything forward — the point is confirming you can
   reconstruct the whole thing unaided.
2. Boot it, and deliberately trigger each of this course's "break it on
   purpose" experiments one more time, now against the *complete*
   system rather than an isolated example: corrupt the magic number
   (file `06`), skip the far jump (file `08`), feed it a bad disk read
   (file `17`).
3. Add one small, genuinely new feature using only book-grounded
   techniques — a second message, a different screen color (the
   attribute byte from file `08`/`16`) — as proof you understand the
   pieces well enough to extend them, not just assemble them.
4. Write, in your own words, a one-paragraph summary of what happens
   from power-on to your kernel's first printed character. If you can't
   write it without checking notes, that's a signal of exactly where to
   review before file `38`.

## Debugging

Nothing new here either — this is the moment to confirm the debugging
skills from file `27` work end to end on the *whole* system, not just
isolated pieces: attach GDB via `qemu -s -S`, set a breakpoint at
`kernel_entry`, step into `main()`, and watch the real screen driver's
`print_char`/`handle_scrolling` execute, live, exactly as file `16`
described it.

## Source Code

The complete project tree above — this is the single artifact worth
keeping intact and well-commented going forward, since file `38`'s
capstone builds directly on top of it.

---

## Common Misconceptions

- **"This checkpoint adds something new to the book's content."** It
  doesn't — it's pure assembly of files `01`–`17`'s already-built,
  book-grounded pieces into one coherent, running whole.
- **"The hatched sections in the diagram above are just harder
  versions of what's already built."** They're not graded difficulty —
  they're sections with *zero* body text in the draft, requiring
  entirely constructed material, which is exactly what file `38` does,
  clearly labeled throughout.

## Interview Questions

**Basic:** What does this file actually add that wasn't in files
`01`–`17`?
**Intermediate:** Trace the complete boot sequence of the assembled
system, naming which course file covers each stage.
**Advanced:** If you removed the screen driver and reverted to file
`08`'s raw `0xb8000` writes, what in the Makefile and project structure
would need to change?
**Systems Design:** Why does organizing the project into `boot/`,
`kernel/`, `drivers/` (file `13`) matter more once you reach this
checkpoint than it did for any single earlier file in isolation?
**Debugging:** Your assembled `os-image` boots but shows nothing — what
three things (spanning multiple earlier files) would you check first?
**Explain what happens when...:** `make run` executes on this complete
project — trace every stage from the Makefile's first command to your
kernel's first printed character?

## Active Recall

1. What is this file's actual content, in one sentence?
2. Name three sections of the book this checkpoint faithfully
   reconstructs, with their section numbers.
3. What's the one honest thing this file (and the whole course) refuses
   to do regarding §6.3–8?

## Answers

1. A complete, working, book-faithful assembly of everything files
   `01`–`17` built individually — no new concepts.
2. Any three of: §2.2 (magic number), §4.1–4.5 (GDT/protected mode),
   §5.2.1–5.2.3 (kernel bootstrap and entry), §6.2 (screen driver).
3. Present invented content as if it were Blundell's — §6.3 onward is
   always explicitly labeled constructed, never implied to be in the
   draft.

---

## What You Should Now Understand

- That you can build, unaided, everything Blundell's draft actually
  contains — a real, working, if simple, operating system.
- Exactly where that draft stops, shown visually against its own table
  of contents.
- The complete audit trail connecting every file of this course back to
  a specific, checkable section of the source PDF.

---

**Next:** file `38-capstone-project.md` — everything from here is staged,
clearly-labeled construction beyond what Blundell's draft ever wrote.
