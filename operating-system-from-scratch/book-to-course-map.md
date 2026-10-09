# Book → Course Map

Every section of Blundell's *Writing a Simple Operating System — from
Scratch* (draft, Dec 2, 2010), cross-referenced to this course. Keep
the PDF open alongside this file — the whole point is being able to
flip to the exact paragraph.

**Status legend:** 🟢 Full (the draft has real content) · 🟡 Seed (a
sentence or two, not developed) · 🔴 Stub (a section heading with zero
body text) · ⚫ Not in book at all

---

## Chapter 1 — Introduction

| Book section | Status | Course file(s) | What's there | Modern equivalent |
|---|---|---|---|---|
| Ch.1 (whole) | 🟡 | `01` | Five stated goals; motivation; explicit scope limit ("not a complete OS, a stepping stone toward Minix") | — |

**Related topics:** file `36` returns to the Minix line directly — the
book's own stated aspiration is a microkernel, while everything it
builds is the opposite.

---

## Chapter 2 — Computer Architecture and the Boot Process

| Book section | Status | Course file(s) | What's there | Modern equivalent |
|---|---|---|---|---|
| §2.1 Boot Process | 🟢 | `06` | Why BIOS can't use a filesystem; boot sector convention | UEFI's filesystem-aware ESP (`07`) |
| §2.2 BIOS, Boot Blocks, Magic Number | 🟢 | `02`, `06` | The exact `e9 fd ff...55 aa` hex dump; little-endian proof; infinite-loop warning | `--oformat binary` still used by real bootloaders (`11`) |
| §2.3 CPU Emulation (Bochs, QEMU) | 🟢 | `06` | Emulation vs. virtualization distinction; `.bochsrc` config | QEMU+KVM blurs the line (`06`, `28`) |
| §2.4 Hexadecimal Notation | 🟢 | `02` | Why hex converts cleanly; byte/short/int/long/word terminology | — |

**Practical experiment:** file `06`'s full boot-and-break sequence
(corrupt the magic number, remove the infinite loop).

---

## Chapter 3 — Boot Sector Programming in 16-bit Real Mode

| Book section | Status | Course file(s) | What's there | Modern equivalent |
|---|---|---|---|---|
| §3.1 Boot Sector Revisited | 🟡 | `06` | Brief recap | — |
| §3.2 16-bit Real Mode | 🟡 | `04`, `08` | Backward-compatibility argument for real-mode boot, stated directly | Every x86-64 CPU still boots this way (`04`) |
| §3.3.1 Interrupts | 🟢 | `03` | Software interrupts, `int 0x10` | Linux syscalls (`21`); full IDT (`14`) |
| §3.3.2 CPU Registers | 🟢 | `03` | `ax`/`bx`/`cx`/`dx`, `ah`/`al` splitting | `rax`/`eax`/`ax`/`al` nesting (`03`, `04`) |
| §3.3.3 Putting it Together | 🟢 | `03` | Full "Hello" boot sector decoded byte-by-byte (Figures 3.2–3.3) | — |
| §3.4.1 Memory, Addresses, Labels | 🟢 | `05` | "Find the byte" — 4 attempts, `[org 0x7c00]` | — |
| §3.4.2 'X' Marks the Spot | 🟢 | `05` | Figure 3.5's exact hex dump, offset `0x1e` | — |
| §3.4.3 Defining Strings | 🟢 | `05` | `db`, null-termination | — |
| §3.4.4 Using the Stack | 🟢 | `05` | `push`/`pop`, downward growth, Figure 3.6 (**corrected** in this course — the book's third read proves `'A'`, not `'C'`) | — |
| §3.4.5 Control Structures | 🟢 | `05` | `cmp` + conditional jumps | Compiler-generated `cmp`/`je` (`05`, `10`) |
| §3.4.6 Calling Functions | 🟢 | `05` | Naive jump → `call`/`ret`; `pusha`/`popa` | System V ABI (`10`); removed in x86-64 (`05`) |
| §3.4.7 Include Files | 🟢 | `05` | `%include` | C's `#include` (`11`) |
| §3.4.8–3.4.9 Putting it Together | 🟢 | `05` | Question 4: `print_string.asm` | — |
| §3.5 Nurse, Fetch me my Stethoscope | 🟡 | `27` | `print_hex` designed; **Question 5 (Advanced) left unsolved in the draft** | GDB, `printk`/`dmesg` (`27`) |
| §3.6.1 Extended Memory Access | 🟢 | `09` | Segment:offset formula, overlap, segment-register restriction | Paging collapses this (`09`, `22`) |
| §3.6.2 How Disk Drives Work | 🟢 | `17` | CHS geometry | LBA (`17`) |
| §3.6.3 Using BIOS to Read the Disk | 🟢 | `17` | `int 0x13` register setup, carry-flag errors | ATA/AHCI/NVMe (`17`) |
| §3.6.4 Putting it Together | 🟢 | `17` | Full disk-read boot sector | — |

---

## Chapter 4 — Entering 32-bit Protected Mode

| Book section | Status | Course file(s) | What's there | Modern equivalent |
|---|---|---|---|---|
| §4.1 Life Without BIOS | 🟢 | `08` | Why BIOS breaks after the switch; memory-mapped VGA at `0xb8000` | Every driver you'll ever write (`15`, `16`) |
| §4.2 Understanding the GDT | 🟢 | `08` | Full descriptor byte layout, flat model, null descriptor | Mostly vestigial on 64-bit Linux (`08`) |
| §4.3 Defining the GDT in Assembly | 🟢 | `08` | Exact byte values, label-arithmetic selectors | — |
| §4.4 Making the Switch | 🟢 | `08` | `cli`/`lgdt`/`CR0`/far jump, step by step | Long mode's own walk-up (`08`) |
| §4.5 Putting it Together | 🟢 | `08` | Full real→protected boot sector | — |

---

## Chapter 5 — Writing, Building, and Loading Your Kernel

| Book section | Status | Course file(s) | What's there | Modern equivalent |
|---|---|---|---|---|
| §5.1.1 Generating Raw Machine Code | 🟢 | `11` | `gcc -ffreestanding`, object files, `ld -Ttext`, `ndisasm` | Compiler Explorer; cross-compilers (`11`) |
| §5.1.2 Local Variables | 🟢 | `10` | Stack frame disassembly, effective address computation | `-O2` frame-pointer omission (`10`) |
| §5.1.3 Calling Functions | 🟢 | `10` | Stack-based argument passing, the book's own calling-convention definition | System V AMD64 registers (`10`) |
| §5.1.4 Pointers, Addresses, Data | 🟢 | `10` | VGA write in C; disassembler-confused-by-a-string example | — |
| §5.2.1 Writing our Kernel | 🟢 | `13` | `main()` writing `'X'` directly | — |
| §5.2.2 Bootstrapping the Kernel | 🟢 | `13` | Disk-loaded kernel, 15-sector over-read | GRUB (`07`) |
| §5.2.3 Finding Our Way into the Kernel | 🟢 | `12` | The exact entry-point bug (function order breaks it); `kernel_entry.asm` fix | Every program's real `_start` (`12`) |
| §5.3 Automating Builds with Make | 🟢 | `13` | Chained rules, `$</$@/$^`, wildcards, pattern rules | Kbuild (`25`) |
| §5.3.1 Organising the Code Base | 🟢 | `13` | `boot/`, `kernel/`, `drivers/` | Linux's own tree shape (`25`) |
| §5.4.1 Preprocessor and Directives | 🟡 | `11`, `13` | Brief | — |
| §5.4.2 Function Declarations, Headers | 🟡 | `16` | `.h`/`.c` split enabling swappable implementations | Linux driver model (`16`) |

---

## Chapter 6 — Developing Essential Device Drivers and a Filesystem

| Book section | Status | Course file(s) | What's there | Modern equivalent |
|---|---|---|---|---|
| §6.1.1–6.1.2 Hardware I/O, Buses | 🟢 | `15` | I/O bus hierarchy, static vs. dynamic (ISA/PCI) assignment | — |
| §6.1.3 I/O Programming | 🟢 | `15` | Real floppy DOR register example, `port_byte_in/out` GAS inline asm | — |
| §6.1.4 DMA | 🟡 | `15` | **One paragraph, an analogy, zero code** | Real DMA setup, scatter-gather, IOMMU (`15`) |
| §6.2.1 Understanding the Display Device | 🟢 | `16` | Cursor as hardware, not memory | — |
| §6.2.2 Basic Screen Driver | 🟢 | `16` | Full `print`/`print_at`/`print_char`, index/data cursor ports | Linux driver model (`16`, `25`) |
| §6.2.3 Scrolling the Screen | 🟢 | `16` | **The last finished code in the draft** | — |
| §6.3 Handling Interrupts | 🔴 | `14` | **Heading only. No body text at all.** | — |
| §6.4 Keyboard Driver | 🔴 | `16` | **Heading only.** | — |
| §6.5 Hard-disk Driver | 🔴 | `17` | **Heading only.** | — |
| §6.6 File System | 🔴 | `18` | **Heading only.** | — |

---

## Chapter 7 — Implementing Processes

| Book section | Status | Course file(s) | What's there |
|---|---|---|---|
| §7.1 Single Processing | 🔴 | `19` | **Heading only.** |
| §7.2 Multi-processing | 🔴 | `20` | **Heading only.** (Almost certainly means multitasking, not multi-core — file `24` covers the latter, entirely separately, also absent from the book.) |

## Chapter 8 — Summary

| Book section | Status | Course file(s) |
|---|---|---|
| Ch.8 (whole) | 🔴 | — (no course file; nothing to summarize from) |

## Bibliography

🔴 **Completely empty.** Every in-text citation in the draft is an
unresolved `[?]` — the reference list was never filled in.

---

## Files With No Book Section At All

Everything from `21` onward that doesn't appear above has **no**
corresponding book section — files `21`–`24`, `26`, `28`–`36`, `38`
are entirely this course's own construction, built on general systems
knowledge and real-world references rather than filling a named gap in
the draft. File `25` is the bridge connecting everything above to real
Linux source.

## Summary Count

- 🟢 Full: 38 sections
- 🟡 Seed: 7 sections
- 🔴 Stub: 7 sections (§6.3–6.6, §7.1–7.2, Ch.8) — plus an empty
  bibliography, listed separately above the tables
- (Counts verified by tallying the status column of the tables above,
  52 rows total.)
- The finished draft is genuinely complete and well-built through
  **§6.2.3** — roughly the first 70 of its 77 pages — and promises,
  then doesn't deliver, the remaining 6.3–8.
