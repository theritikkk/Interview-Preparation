# Writing a Simple Operating System — From Scratch, and Everything Around It

A self-study systems course built on top of Nick Blundell's 2010 draft
*Writing a Simple Operating System — from Scratch* (University of
Birmingham). The book is the **spine**. This repository is the
skeleton, muscle, and nervous system built around that spine.

Goal: go from "I know how to use Linux" to "I understand what Linux
is standing on," and eventually "I can reason about a computer from
the CPU and memory upward through boot, kernel, drivers, filesystems,
processes, networking, and applications."

---

## What this is

A sequence of focused Markdown files, each teaching one concept in
enough depth to actually use it — grounded first in Blundell's real,
working, hand-verified example code, then widened into the computer
science you need to understand *why* that code works, then extended
into how the same idea appears in a modern Linux/x86-64/UEFI machine.

## What this is *not*

Not "finish reading a 2010 tutorial." The book is a genuine draft —
parts of it were never written (see below). This course does not
pretend otherwise. It also does not silently swap in modern material
and call it Blundell's. Every file is honest about which parts came
from where.

---

## The rule every file in this course follows

This is the single most important structural rule in the whole
repository. It is not optional and it is not a style suggestion —
every content file (01 through 38, and the relevant appendices) is
built around it:

```
┌──────────────────────────────────────────────┐
│ 1. WHAT THE BOOK ACTUALLY TEACHES             │
│    Nick Blundell's original material          │
├──────────────────────────────────────────────┤
│ 2. WHAT YOU NEED TO KNOW TO UNDERSTAND IT     │
│    Supporting computer-systems knowledge       │
├──────────────────────────────────────────────┤
│ 3. WHERE THIS LEADS IN MODERN SYSTEMS         │
│    Linux / x86-64 / UEFI / VMs / containers    │
│    eBPF / networking / performance / etc.      │
└──────────────────────────────────────────────┘
```

**Layer 1 is a factual claim, not a teaching convenience.** If
something is in Layer 1, it means Blundell's draft actually contains
it — you could open the PDF and find the paragraph. Nothing invented,
paraphrased-to-sound-original, or "in the spirit of the book" is
allowed to sit in Layer 1.

**When Layer 1 would otherwise be empty, the file says so out loud.**
Blundell's table of contents promises interrupt handling, a keyboard
driver, a hard-disk driver, a filesystem, and single/multi-processing
— and then delivers a bare section heading and nothing else for every
one of those. Where that happens, the relevant file states plainly:
*"The book's table of contents promises this; the draft never wrote
it. Everything below is constructed, not transcribed."* Layer 1
becomes a short paragraph explaining the gap, and the real teaching
moves entirely into Layers 2 and 3, clearly marked.

**Layer 2 and Layer 3 are still tiered internally.** Within "what you
need to know" and "where this leads," content is further marked
*Essential*, *Important Adjacent Knowledge*, or *Advanced / Optional*,
so the course doesn't turn into an encyclopedia — see
`00-course-roadmap.md` for how those tiers are used per file.

### Tag legend

Inside a file, you'll see these inline tags — they're the fine-grained
version of the three layers above:

| Tag | Layer | Meaning |
|---|---|---|
| `[BOOK]` | 1 | Directly from Blundell's text — traceable to a specific page/section |
| `[BOOK — STUB]` | 1 | The book's TOC names this topic; the draft has no body text for it |
| `[CORE CONCEPT]` | 2 | Foundational knowledge required to understand the book material |
| `[RELATED KNOWLEDGE]` | 2 | Adjacent knowledge that deepens understanding but isn't strictly required |
| `[MODERN SYSTEM]` | 3 | How the idea works on a real, current Linux/x86-64/UEFI machine |
| `[ADVANCED / BEYOND THE BOOK]` | 3 | Research-, performance-, or kernel-dev-level material; optional |

---

## The Source → Bytes → CPU thread

The single strongest thing about Blundell's draft is that it never
stays abstract: it shows you the actual hex bytes of a boot sector,
the actual bit-packed layout of a GDT descriptor, the actual
disassembly of a C function next to the C that produced it. That
thread — **source code → compiler/assembler → machine bytes → CPU
execution → observable effect** — is preserved and made explicit
throughout this course, not just in the chapters that come straight
from the book. Files `02`, `05`, `06`, `09`, `11`, `12`, `13`, and
`37` carry this thread most directly; later files (`14`–`38`) keep
returning to it ("here's the C, here's the disassembly, here's the
register state, here's what changed in memory").

---

## Directory structure

```
systems-from-scratch/
├── README.md                          ← you are here
├── 00-course-roadmap.md               ← learning order, dependency graph, difficulty
├── book-to-course-map.md              ← book section ↔ course file, side-by-side with the PDF
│
├── 01-computer-systems-foundation.md
├── 02-binary-hex-memory-representation.md
├── 03-cpu-and-instruction-set.md
├── 04-x86-architecture.md
├── 05-assembly-language.md
├── 06-boot-process.md
├── 07-bios-uefi-bootloaders.md
├── 08-real-mode-protected-mode-long-mode.md
├── 09-memory-and-addressing.md
├── 10-stack-heap-and-calling-conventions.md
├── 11-c-compilation-and-machine-code.md
├── 12-linking-loading-and-elf.md
├── 13-kernel-entry-and-kernel-architecture.md
├── 14-interrupts-exceptions-and-traps.md
├── 15-device-io-and-dma.md
├── 16-device-drivers.md
├── 17-storage-and-block-io.md
├── 18-filesystems.md
├── 19-processes-and-threads.md
├── 20-scheduling-and-context-switching.md
├── 21-system-calls-and-user-kernel-boundary.md
├── 22-virtual-memory.md
├── 23-concurrency-and-synchronization.md
├── 24-multiprocessor-and-smp-systems.md
├── 25-linux-kernel-architecture.md
├── 26-linux-source-code-navigation.md
├── 27-kernel-debugging.md
├── 28-virtualization.md
├── 29-containers-and-namespaces.md
├── 30-networking-from-hardware-to-socket.md
├── 31-security-and-protection.md
├── 32-performance-and-observability.md
├── 33-modern-hardware.md
├── 34-modern-boot-and-efi.md
├── 35-ebpf-and-kernel-observability.md
├── 36-operating-system-design.md
├── 37-building-a-simple-os.md         ← checkpoint: exactly what the book leaves you with
├── 38-capstone-project.md             ← staged build of everything the book didn't finish
│
├── glossary.md
├── source-code-map.md
├── debugging-playbook.md
├── experiments.md
├── interview-questions.md
└── final-knowledge-map.md
```

Files are added one at a time, in this order, as the course
progresses — this tree shows the target shape, not everything that
exists yet (see **Status** below).

## How to study this

1. Read `00-course-roadmap.md` once, fully, before file `01`.
2. Read files in numeric order. Each assumes the ones before it.
3. Keep Blundell's PDF open next to files that are tagged `[BOOK]` —
   `book-to-course-map.md` gives you the exact section to flip to.
4. Do the practical experiment in each file before moving on. This
   material does not stick from reading alone.
5. Treat `glossary.md`, `source-code-map.md`, `interview-questions.md`,
   and `debugging-playbook.md` as living references — dip into them
   throughout, not sequentially.

## Where the book actually stops

Blundell's draft is fully written and correct through **§6.2.3
(Scrolling the Screen)** — a working boot sector → GDT → protected
mode → linked C kernel → scrolling VGA text driver. From **§6.3
onward** (interrupt handling, keyboard driver, hard-disk driver,
filesystem) and all of **Chapter 7** (single/multi-processing), the
table of contents lists the section but the draft body is empty. The
bibliography is also empty — every in-text citation is an unresolved
`[?]`. File `37-building-a-simple-os.md` marks this checkpoint
explicitly; file `38-capstone-project.md` picks up from there using
constructed, clearly-labeled material.

## Status

- [x] `README.md`
- [x] `00-course-roadmap.md`
- [x] `01`–`11` — rewritten with embedded diagrams (`images/`) and a
      corrected stack-growth example in file `05`
- [ ] everything else — generated one file at a time on request

## Diagrams

Files `01`–`11` embed original SVG figures under `images/` — byte-field
layouts, memory/stack diagrams, flowcharts — generated for this course,
not pulled from the web. Keep the `images/` folder alongside the
`.md` files for them to render.
