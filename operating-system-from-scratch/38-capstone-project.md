# 38 — Capstone Project: Beyond the Book

## Layer map for this file

```
┌──────────────────────────────────────────────────┐
│ 1. BOOK: none of this project is Blundell's.     │
│    His own table of contents names several of    │
│    these stages (interrupts, keyboard, disk      │
│    driver, filesystem, processes) and then       │
│    delivers zero lines toward any of them        │
├──────────────────────────────────────────────────┤
│ 2. CORE CONCEPT: a staged, dependency-ordered    │
│    plan connecting each stage to the file that   │
│    already taught its concepts                   │
├──────────────────────────────────────────────────┤
│ 3. MODERN: every stage's modern-system context   │
│    was already covered in its home file —        │
│    this file is where you build the real thing   │
└──────────────────────────────────────────────────┘
```

This is not "write an operating system" — that's too broad to be a
real project. It's nine staged, scoped pieces of work, each with a
concrete deliverable, each connected to the specific course file that
already taught you everything you need to build it.

## Prerequisites

File `37`'s checkpoint, built and booting. Everything below assumes
that working base.

---

## The Honest Starting Point

File `37` consolidated stages 1–6 — the only part of this project
traceable to Blundell's actual draft. What follows is traceable to
*this course's own* files `14` through `27`, never to the book. Say so
in your own project's README if you share it: "stages 1–6 follow
Blundell's *Writing a Simple Operating System*; stages 7–15 are
original work built on standard OS-development references."

## The Dependency Graph

![Capstone dependency graph: from the checkpoint, stages 7, 10, and 12 can begin independently; stage 7 leads to stages 8 and 9; stages 9 and 10 feed stage 11; stage 10 also feeds stage 13; stages 12 and 13 feed stage 14; stages 8, 11, and 14 all feed into stage 15's continuous debugging pass](images/38-capstone-dependencies.svg)

Three stages can start immediately and independently once file `37`'s
checkpoint boots: interrupts, paging, and a filesystem reader share no
real dependency on each other. Pick whichever interests you most first.

---

## Stage 7 — IDT, PIC, and a Real ISR

**Built on:** file `14`. **Goal:** a working interrupt handler for at
least one source. **Deliverable:** build a minimal IDT, remap the
8259 PIC off its colliding default vectors, write one ISR stub (save
registers, call a C handler, send EOI, `iret`), and wire it to IRQ0 so
it increments a counter you can print. **Acceptance test:** the
counter visibly increases over time with no crash, and removing the
PIC remap step (temporarily) demonstrably breaks it — proof you
understand *why* the remap matters, not just that you copied the
sequence.

## Stage 8 — Keyboard Driver

**Built on:** file `16`. **Deliverable:** an ISR on IRQ1 reading port
`0x60`, a scancode-to-ASCII table for at least the alphanumeric keys,
and visible characters printed as you type. **Acceptance test:** typing
on the host keyboard (through QEMU/Bochs) produces correct characters
on your kernel's own screen driver output.

## Stage 9 — PIT Timer

**Built on:** file `20`. **Deliverable:** program the PIT (port I/O,
file `15`) to fire IRQ0 at a chosen, visible frequency; print a tick
counter. **Acceptance test:** the rate is controllable (changing the
PIT's configured divisor visibly changes how fast the counter climbs).

## Stage 10 — Paging and a Minimal Allocator

**Built on:** file `22`. **Deliverable:** a page directory and at
least one page table identity-mapping the kernel's own memory, loaded
via `CR3`, with paging enabled (`CR0`); then a trivial `kmalloc`
(even a simple bump allocator is a legitimate first version) layered
on top. **Acceptance test:** the kernel keeps running immediately
after paging is enabled (proof the kernel's own code is correctly
mapped), and two separate `kmalloc` calls return distinct, usable
addresses.

## Stage 11 — Scheduler and Context Switch

**Built on:** files `19`, `20`. **Requires:** stage 9 (for preemption);
benefits from, but doesn't strictly require, stage 10. **Deliverable:**
a PCB structure (minimally, just a saved stack pointer, per file `20`'s
stack-swap design), two or more tasks, and a round-robin switch
triggered from the stage 9 timer ISR. **Acceptance test:** multiple
tasks visibly interleave their output with no manual `yield()` call
anywhere — the timer alone drives the switching.

## Stage 12 — FAT12 Reader

**Built on:** file `18`. **Deliverable:** parse a FAT12 BPB, read the
root directory, find a named file, follow its cluster chain, and print
its contents. **Acceptance test:** create a FAT12 floppy image on your
host machine with a known text file on it, and have your kernel print
that file's exact contents, read entirely through your own code.

## Stage 13 — Ring-3 Syscall Gate

**Built on:** file `21`. **Requires:** stage 10 (paging) for the
separation to mean anything real. **Deliverable:** a minimal TSS with
`esp0` set to a dedicated kernel stack, a GDT entry for a ring-3 code
segment, a way to actually drop to ring 3, and one working `int
0x80`-style syscall callable from there. **Acceptance test:** code
running at ring 3 successfully invokes your syscall and gets a result
back; a ring-3 attempt at a privileged instruction (`cli`, say) visibly
faults instead of executing.

## Stage 14 — User-Mode Loader and a Toy Shell

**Built on:** stages 12 and 13 together. **Deliverable:** load a
separately-compiled, simple program's bytes from your FAT12 filesystem
into memory and jump into it at ring 3, using your stage 13 syscall for
its only I/O; then a minimal shell loop reading a command name and
loading/running the matching program. **Acceptance test:** at least two
different user programs can be run by name, each using your syscall
gate for output, neither able to execute a privileged instruction
directly.

## Stage 15 — Debugging and Observability Pass

**Built on:** file `27`, applied throughout, not saved entirely for
last. **Deliverable:** at least one deliberately-reproduced and fixed
bug per earlier stage, documented (what broke, how you found it, which
tool from file `27` found it), plus a final pass using GDB over `qemu
-s -S` to step through the full boot-to-shell sequence in one session.

---

## What's Deliberately Out of Scope

Consistent with the roadmap's own audit: **SMP (file `24`) stays out**
— the capstone is single-core throughout, by design, not as an
oversight. A production-grade filesystem, real networking, and a real
dynamic linker are all out of scope for the same reason the audit gave
for not covering them in the course proper — they're real engineering
projects in their own right, not capstone-scale additions.

---

## Practical Experiment

The nine stages above **are** this file's practical experiment — there
is no separate exercise layered on top. Work through them in an order
your dependency graph permits, keep each stage's working code before
starting the next, and keep the acceptance test for each stage as your
definition of "done" rather than "it compiled."

## Debugging

Treat every stage's first attempt as expected to fail at least once —
that's what stage 15 formalizes, not a sign anything is wrong with your
approach. The systematic checklists built across files `06`, `08`,
`14`, `20`, and `27` are the actual debugging content for this entire
project; this file adds nothing new to them.

## Source Code

Your complete, staged capstone project — the terminal deliverable of
this entire course.

---

## Common Misconceptions

- **"The capstone is 'finish the book.'"** The book has no more to
  finish — stages 7 onward are entirely original construction, clearly
  distinguished from Blundell's own six delivered chapters.
- **"Every stage must be completed in the listed order."** Only the
  dependency graph's actual edges are required; stages 7, 10, and 12
  are genuinely independent starting points.
- **"SMP being out of scope is a limitation of this course."** It's a
  deliberate scope boundary, stated from the very first audit, not an
  oversight.

## Interview Questions

**Basic:** Which stages of this capstone trace back to Blundell's book,
and which don't?
**Intermediate:** Why can stages 7, 10, and 12 all start immediately
after the checkpoint, independent of each other?
**Advanced:** Why does stage 13 list stage 10 (paging) as a real
requirement rather than a nice-to-have?
**Systems Design:** Why is a round-robin scheduler (stage 11) a
reasonable first scheduler to build, given file `20`'s own comparison
to Linux's real scheduler?
**Debugging:** You've just enabled paging (stage 10) and the kernel
immediately crashes — what's the most likely cause, and which earlier
file's experiment warned you about exactly this?
**Explain what happens when...:** a user program, loaded in stage 14,
calls your stage 13 syscall to print a string — trace it through every
stage this capstone built?

## Active Recall

1. Which three stages have no dependency on each other and can be
   started in any order?
2. What makes stage 13 meaningfully different without stage 10 completed?
3. What is explicitly out of scope for this capstone, and why?

## Answers

1. Stage 7 (IDT/PIC/ISR), stage 10 (paging/kmalloc), and stage 12
   (FAT12 reader).
2. Without paging, a "ring 3" process shares the same flat address
   space as the kernel — the privilege separation exists, but the
   memory isolation that makes it meaningful doesn't.
3. SMP (file `24`) — a deliberate scope boundary stated from the first
   audit, not an oversight; multi-core work is real engineering scope
   beyond a capstone.

---

## What You Should Now Understand

- That this capstone is genuinely new construction, not "finishing" a
  book that has nothing left to finish.
- The real dependency structure between stages — not a strict line, a
  graph with real parallel starting points.
- Exactly what each stage requires, delivers, and how to know it's
  actually done, not just compiled.

---

**This is the end of the course.** Forty files, twelve-plus diagrams,
one book audited down to its last empty section heading, and one
complete map from a 2010 teaching draft to the machine you're reading
this on. `final-knowledge-map.md`, `glossary.md`, `source-code-map.md`,
`interview-questions.md`, `debugging-playbook.md`, and
`book-to-course-map.md` remain as living reference documents — say
**NEXT** for any of them, by name, whenever you want one.
