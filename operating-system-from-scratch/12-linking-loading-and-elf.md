# 12 — Linking, Loading, and ELF

## Layer map for this file

```
┌──────────────────────────────────────────────┐
│ 1. BOOK: the exact bug that shows why entry-  │
│    point position can't be assumed, and the   │
│    kernel_entry.asm fix (§5.2.1–5.2.3)        │
├──────────────────────────────────────────────┤
│ 2. SUPPORTING: what the linker actually does  │
│    to symbols; ELF sections vs. segments       │
├──────────────────────────────────────────────┤
│ 3. MODERN: every hosted C program has its own  │
│    "kernel_entry.asm" — it's called _start,    │
│    and it runs before main(), even today       │
└──────────────────────────────────────────────┘
```

Book coverage: **Full** (§5.2.1–5.2.3). This file does not repeat
file `11`'s pipeline mechanics — it picks up exactly where linking
meets a real, working (if fragile) kernel.

## Prerequisites

Files `01`–`11`.

---

## [BOOK] The Bug That Motivates This Whole File

The book's first kernel is one function: `main()` writes `'X'` to
`0xb8000` directly (§5.2.1) — literally the C version of file `08`'s
raw-assembly VGA write. Compiled with `-Ttext 0x1000` and loaded to
that exact address, the boot sector can just `call KERNEL_OFFSET`
(`0x1000`) and land right on `main`'s first instruction. It works —
by luck, not by design.

Add one more function, placed *before* `main` in the source:

```c
void some_function() { }
void main() {
    char *video_memory = 0xb8000;
    *video_memory = 'X';
    some_function();
}
```

![Three-panel comparison: single-function kernel works by luck, adding some_function() before main breaks it because KERNEL_OFFSET now lands on some_function's immediate ret, and the fix is linking kernel_entry.o first so its call-main stub always sits at KERNEL_OFFSET regardless of source order](images/12-kernel-entry-fix.svg)

The compiler is free to place `some_function` first in the output.
The boot sector, jumping blindly to `KERNEL_OFFSET`, now hits
`some_function`'s immediate `ret` and returns to the boot sector —
`main` never runs, no `'X'`. **Entering the kernel correctly depended
on function ordering the source code never actually guaranteed.**

## [BOOK] The Fix: a Tiny, Deterministic Entry Stub

```asm
[bits 32]
[extern main]     ; the linker must find this symbol elsewhere
call main
jmp $
```

Assembly is used specifically because its translation to machine code
is fully predictable — you *know* the first instruction here becomes
a call to `main`, wherever `main` ends up. The book states the
linker's job precisely: given object files with an unresolved
reference (`call main`) and, elsewhere, main's real definition, the
linker joins everything and resolves that reference to main's actual
final address — failing loudly if `main` is never found anywhere.

Two commands, and one detail that matters as much as the code itself:

```
nasm kernel_entry.asm -f elf -o kernel_entry.o
ld -o kernel.bin -Ttext 0x1000 kernel_entry.o kernel.o --oformat binary
```

`-f elf` — an object file, not raw binary, since this code can't stand
alone until `main` is resolved (echoing file `11`'s object-vs-binary
distinction). **File order on the `ld` command line places
`kernel_entry.o`'s code first** — exactly at `KERNEL_OFFSET` — so
`call main` is always the first thing that runs, regardless of how
`kernel.c`'s own functions happen to be ordered or compiled.

---

## [CORE CONCEPT] Why This Problem Exists At All

Worth naming directly, since the book doesn't connect these two dots
itself: **this entire problem is a direct consequence of file `11`'s
choice to strip ELF down to `--oformat binary`.** A real ELF
executable's header carries an explicit `e_entry` field — the exact
address to jump to — which a real loader reads and uses regardless of
where any function's bytes physically sit in the file. Raw binary
extraction throws that field away along with everything else, which
is exactly why the book has to reconstruct "always jump to the right
place" by brute force: put a deterministic stub first, by file order,
because nothing in the format itself can say "start here" anymore.

## [CORE CONCEPT] Sections vs. Segments — Two Different Views of One File

ELF describes itself two ways, for two different audiences:

| | Sections | Segments |
|---|---|---|
| Used by | linkers, debuggers | the OS loader, at runtime |
| Granularity | fine (`.text`, `.data`, `.bss`, `.symtab`, `.debug_info`) | coarse (a handful of loadable regions) |
| Tool | `readelf -S` | `readelf -l` |

A section is a link-time/debug-time concept; a segment is "here's a
chunk to map into memory, with these permissions" — a running process
never sees section boundaries at all, only segments.

---

## [MODERN SYSTEM] Every C Program Has Its Own kernel_entry.asm

This is the payoff fact for this whole file: **`main` is not actually
the first thing that runs in an ordinary, hosted C program either.**
A small startup routine — historically `crt0`, today usually reached
through glibc's `_start` symbol — runs first, does runtime setup
(argc/argv, environment, static initializers), and *then* calls
`main` by name, exactly like `kernel_entry.asm` does. Check it
yourself: `readelf -h` on any Linux binary's `e_entry` almost never
points at `main` directly — it points at `_start`. The book's trick
isn't a toy workaround; it's a miniature of what every C program on
your machine does right now.

## [MODERN SYSTEM] Static vs. Dynamic Linking

The book only ever does **static** linking — everything resolved once,
at build time, into one file. Most hosted Linux programs link
**dynamically** instead: many symbols stay unresolved until the
program actually starts, when the OS invokes an ELF interpreter (the
dynamic linker, `ld.so`, named in a `PT_INTERP` segment) to resolve
them against shared libraries — a whole extra stage with no equivalent
anywhere in this book's bare-metal world.

---

## [ADVANCED / BEYOND THE BOOK] Position-Independent Executables

The book's kernel is linked to run at exactly one fixed address
(`0x1000`) — if loaded anywhere else, every baked-in absolute address
would be wrong. Modern executables are commonly compiled
**position-independent (PIE)**, correct at any load address, which is
what makes **ASLR** (randomizing that address per-run, file `31`) possible
at all.

---

## Practical Experiment

1. **Reproduce the bug.** Build the single-function kernel, boot it —
   `'X'` appears. Add `some_function()` before `main`, rebuild the
   *naive* way (`ld ... kernel.o --oformat binary`, no entry stub),
   reboot — `'X'` is gone.
2. **Apply the fix.** Build `kernel_entry.asm` as shown, relink with it
   listed first — `'X'` reappears, regardless of function order in
   `kernel.c`. Try reordering the C functions again to confirm it no
   longer matters.
3. `readelf -h kernel_entry.o` and `readelf -S kernel_entry.o` —
   inspect a real ELF object directly; find the `elf32-i386` format
   name from file `11` in context.
4. On any ordinary Linux machine: compile `int main(){return 42;}`,
   then `readelf -h a.out | grep Entry`. Confirm the entry address is
   **not** `main`'s address — it's `_start`'s.

## Debugging

A crash *before your own code's first line seems to run* is this
file's signature bug class — exactly what the book's broken example
produces, and exactly what a corrupted or misconfigured entry point
produces on a real system. The fix is always the same first move:
check the entry point directly (`readelf -h`, or the book's own
"which function's code is actually first") rather than debugging
source logic that never got a chance to execute.

## Source Code

The single-function kernel, the broken two-function version, and the
fixed version with `kernel_entry.asm` — a genuine before/broken/fixed
set worth keeping together.

---

## Common Misconceptions

- **"`main()` is the first thing that runs."** Even in ordinary hosted
  C, a startup stub (`_start`) runs first — confirmed via `readelf -h`
  above.
- **"Functions execute in source-file order."** No — compiler and
  linker are free to reorder; the book's own bug demonstrates this
  directly.
- **"Sections and segments are the same thing."** Sections serve
  linkers/debuggers; segments serve the runtime loader — genuinely
  different views.
- **"Object file order on a linker command line never matters."** For
  raw-binary output with no entry-point metadata, it can determine
  correctness entirely, as this file shows.

## Interview Questions

**Basic:** What does `[extern main]` tell the assembler and,
ultimately, the linker?
**Intermediate:** Why did adding one function before `main` break the
book's kernel?
**Advanced:** Why does a raw-binary kernel need a hand-placed entry
stub when a normal ELF executable doesn't?
**Systems Design:** Sections vs. segments — why does ELF need both
views instead of just one?
**Debugging:** A program crashes before any of your own code seems to
run — what's the first thing to check?
**Explain what happens when...:** you run any ordinary compiled C
program, from `_start` to `main` to `exit`?

## Active Recall

1. What broke when `some_function()` was added before `main`?
2. What does `[extern main]` do, and why is assembly used for this
   stub instead of C?
3. Why does raw-binary output need this stub at all, when normal ELF
   executables don't?
4. What does a real Linux binary's `e_entry` usually point to?

## Answers

1. The compiler placed `some_function`'s code first; the boot sector's
   blind jump to `KERNEL_OFFSET` hit its immediate `ret` and returned
   without ever reaching `main`.
2. It declares a promise that `main` is defined elsewhere, to be
   resolved by the linker; assembly is used because its translation
   to machine code is fully predictable, guaranteeing the first
   instruction becomes exactly `call main`.
3. Because `--oformat binary` strips out ELF's `e_entry` field, which
   would otherwise tell a loader exactly where to start regardless of
   file order.
4. `_start`, not `main` — a startup stub runs first even in ordinary
   hosted programs.

---

## What You Should Now Understand

- The book's own entry-point bug, and why the fix has to be assembly,
  placed first by link order.
- That this problem is a direct, avoidable-only-by-choice consequence
  of stripping ELF metadata in file `11`.
- Sections vs. segments as genuinely different views of one file.
- That `kernel_entry.asm` isn't a toy workaround — it's what every C
  program's real `_start` does, today, on your own machine.

---

**Next:** file `13-kernel-entry-and-kernel-architecture.md`.
