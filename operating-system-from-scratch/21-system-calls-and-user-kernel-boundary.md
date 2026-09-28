# 21 — System Calls and the User/Kernel Boundary

## Layer map for this file

```
┌─────────────────────────────────────────────────┐
│ 1. BOOK: none — the book never leaves ring 0,   │
│    never mentions a syscall. But four separate. │
│    breadcrumbs, dropped across earlier files,   │
│    all point straight at this one               │
├─────────────────────────────────────────────────┤
│ 2. CORE CONCEPT: rings as an enforced boundary, │
│    the TSS's real job, the syscall mechanism    │
├─────────────────────────────────────────────────┤
│ 3. MODERN: strace, libc wrappers, the vDSO      │
└─────────────────────────────────────────────────┘
```

Book coverage: **None**. This file collects four things earlier files
deliberately set down and never finished, and finishes them.

## Prerequisites

Files `01`–`20`.

---

## The Breadcrumbs, Collected

- File `03`: BIOS's `mov ah,0x0e` / `int 0x10` — load registers, trigger
  a transfer of control, get a result back.
- File `08`: privilege rings named for the first time; the book uses
  ring 0 exclusively.
- File `08`: the Task State Segment, named as "needed the moment you
  build a real ring 3 → ring 0 transition."
- File `10`: System V AMD64's register-passing convention, flagged as
  "structurally identical to the Linux syscall convention."

This file is where all four connect into one real mechanism.

---

## [CORE CONCEPT] Rings Are Enforced, Not Just Labeled

File `08` introduced privilege levels as a field in a descriptor. The
enforcement is concrete: certain instructions — `in`/`out` (file `15`),
writing `CR0`/`CR3`, `lgdt`/`lidt` (file `08`/`14`), `cli`/`sti` (file
`14`) — are **privileged**, and executing them at ring 3 faults
immediately, unconditionally. This is the entire technical meaning of
"user mode can't touch hardware directly" — not a convention anyone
agreed to, a hard CPU-enforced wall.

## [CORE CONCEPT] What the TSS Is Actually For

File `08` deferred this. Here's the answer: when a ring 3 → ring 0
transition happens, the CPU cannot keep using the *user* stack — it's
attacker- or bug-controlled, and kernel code running on it would be one
stack overflow away from disaster. The **TSS**'s `ss0`/`esp0` fields
tell the CPU exactly which trusted kernel stack to switch to,
automatically, as part of the transition itself — before a single line
of kernel code runs.

## [CORE CONCEPT] The Syscall Path, Start to Finish

![Syscall path: user program in ring 3 sets rax and argument registers, executes syscall, CPU checks EFER and switches to ring 0 and the TSS's kernel stack, the kernel handler runs and validates every user pointer, then sysret returns to ring 3 and the user's own stack](images/21-syscall-path.svg)

Two real mechanisms exist, historically:

- **`int 0x80`** (the original Linux syscall path, x86-32) reuses file
  `14`'s interrupt-gate machinery — but unlike BIOS's own software
  interrupts (real mode has no rings at all to cross), this one
  *does* cross a privilege boundary, going through the full IDT lookup.
- **`syscall`/`sysret`** (modern x86-64) is a dedicated, faster
  instruction pair, bypassing the IDT entirely — the handler's address
  lives in an MSR (`LSTAR`), the same register category file `08`
  introduced for `EFER`.

Once in the kernel, the handler faces a problem file `08`'s ring-0-only
world never had: **it cannot trust anything the user program handed
it.** A user-supplied pointer might point at unmapped memory, kernel
memory it has no business reading, or nothing valid at all. Real
kernels validate every such pointer explicitly (`copy_from_user`/
`copy_to_user`) before dereferencing it — skipping this check is a
classic, real vulnerability class.

---

## [RELATED KNOWLEDGE] Why Two Mechanisms Exist At All

`int 0x80` predates `syscall` — added when x86-64 introduced `syscall`
specifically because the full interrupt-gate path (an IDT lookup, a
privilege check, more) is measurably slower than a purpose-built
instruction that skips straight to a known handler address. Both still
work on modern Linux; `syscall` is what glibc actually uses today.

---

## [MODERN SYSTEM] Watching This Boundary Directly

Every line `strace` prints **is one crossing of this exact boundary** —
syscall number, decoded arguments, return value, nothing more.
Ordinary programs never call `syscall` by hand: glibc wraps each one in
an ordinary C function (`read()`, `write()`, `open()`) that hides the
register setup and the instruction itself — you've been using this
boundary in every C program you've ever written without seeing it.

## [ADVANCED / BEYOND THE BOOK] The vDSO

Some syscalls (`gettimeofday`, `clock_gettime`) are called so
frequently that even `syscall`'s overhead matters. The **vDSO**
(virtual dynamic shared object) maps a page of kernel code directly
into every process's own address space, letting these specific calls
be serviced **without crossing into ring 0 at all** — a real,
production optimization built entirely on understanding exactly where
the expensive boundary in this file actually is.

## [ADVANCED / BEYOND THE BOOK] Meltdown and Spectre

Both are, at their core, failures of exactly this boundary — CPUs
speculatively executing instructions across the ring 3/ring 0 line and
leaving observable traces of data that should never have been
readable from user mode. Mentioned here only to name the connection;
full treatment belongs to file `31`.

---

## Practical Experiment

1. `strace -c ls` — count syscalls a trivial command makes; `strace -f`
   on something more complex.
2. Write a tiny C program calling `write()` directly (not `printf`);
   `strace` it and find the exact `write(...)` line matching your call.
3. On your file `13` kernel: build a minimal TSS, load it with `ltr`,
   set `esp0` to a dedicated kernel stack, and implement one trivial
   `int 0x80`-style syscall (e.g., print a character) callable from a
   deliberately separate, lower-privileged test routine.
4. Read `/proc/<pid>/syscall` for a running process — the kernel's own
   live view of exactly where in this boundary that process currently is.

## Debugging

A process that "hangs mysteriously" during I/O is often stuck inside a
single blocking syscall — `strace -p <pid>` attached to a running
process shows you exactly which one, live, which narrows "something is
wrong" to "this specific syscall never returned" immediately.

## Source Code

Your minimal TSS setup and one working `int 0x80`-style syscall —
entirely new code, no book equivalent to check against at all.

---

## Common Misconceptions

- **"A syscall is just a fancy function call."** A function call never
  changes privilege level or stack; a syscall does both, deliberately,
  every time.
- **"The kernel can trust pointers a user program passes in."** Never
  — every user pointer must be validated before use, precisely because
  ring 3 code is untrusted by definition.
- **"`int 0x80` and `syscall` are interchangeable, no real
  difference."** Same conceptual job, genuinely different mechanisms —
  one reuses the interrupt-gate machinery, one bypasses it for speed.

## Interview Questions

**Basic:** What two things change when a syscall executes, that don't
change in an ordinary function call?
**Intermediate:** What is the TSS actually for, concretely?
**Advanced:** Why must a kernel validate user-supplied pointers before
dereferencing them?
**Systems Design:** Why does the vDSO exist — what specific cost is it
avoiding?
**Debugging:** A process appears hung during file I/O — what's the
fastest way to find out exactly where?
**Explain what happens when...:** a C program calls `read()` — trace
it from the libc wrapper through to the kernel and back?

## Active Recall

1. Name the four breadcrumbs from earlier files this one completes.
2. What does the TSS's `esp0` field actually provide?
3. Why can't the kernel trust a user-supplied pointer?
4. What does the vDSO avoid, and how?

## Answers

1. File `03`'s register-passing pattern; file `08`'s privilege rings;
   file `08`'s TSS mention; file `10`'s System V ABI note about Linux
   syscalls.
2. The address of a trusted kernel stack to switch to automatically
   during a ring 3 → ring 0 transition.
3. Because ring 3 code is untrusted by definition — a pointer could
   reference unmapped or kernel-only memory, deliberately or by bug.
4. The overhead of a full ring transition, for very frequently-called
   syscalls — by mapping a page of kernel code directly into user
   space so the call never needs to leave ring 3 at all.

---

## What You Should Now Understand

- Rings as a hard, CPU-enforced wall — not a labeling convention.
- The TSS's real, concrete job, finally answered.
- The complete syscall path, and why user-supplied pointers can never
  be trusted blindly.
- Where `strace`, libc wrappers, and the vDSO all sit relative to this
  one boundary.

---

**Next:** file `22-virtual-memory.md`.
