# 10 — Stack Frames, the Heap, and Calling Conventions

## Layer map for this file

```
┌─────────────────────────────────────────────────┐
│ 1. BOOK: three real C→assembly disassemblies    │
│    (§5.1.2–5.1.4) — local variables, stack-     │
│    based argument passing, pointers             │
├─────────────────────────────────────────────────┤
│ 2. SUPPORTING: calling conventions formalized,  │
│    the heap (which the book never builds)       │
├─────────────────────────────────────────────────┤
│ 3. MODERN: System V AMD64's register-based      │
│    passing, frame-pointer omission, canaries    │
└─────────────────────────────────────────────────┘
```

Book coverage: **Full**, for genuinely new content — file `05` already
covered raw stack mechanics; this is what a **C compiler** does with
the stack.

## Prerequisites

Files `01`–`09`.

---

## [BOOK] A Function's Own Stack Frame (§5.1.2)

```c
int my_function() { int my_var = 0xbaba; return my_var; }
```

![Two stack frames side by side: caller's data, the argument 0xdede placed by the caller, the return address pushed by call, the saved ebp pushed by the callee's prologue, and the callee's own locals below it, with an arrow showing ebp+0x8 reaching up into the caller's frame](images/10-stack-frames.svg)

```
push ebp \ mov ebp, esp \ sub esp, 0x10
mov dword [ebp-0x4], 0xbaba
mov eax, [ebp-0x4]        ; return value → eax
leave \ ret
```

`0x10` (16) bytes for one 4-byte `int`: the book attributes this to
alignment — reserving space in fixed, maximum-width chunks rather than
the exact size needed. `[ebp-0x4]` is **effective address
computation** — resolved at *run time* from whatever `ebp` currently
holds, not folded at assembly time; the honest slower equivalent is
three instructions (`mov eax,ebp` / `sub eax,0x4` / `mov [eax],...`).
`leave` = `mov esp,ebp` + `pop ebp`, compressed.

## [BOOK] How Arguments Actually Travel (§5.1.3)

```c
void caller_function() { callee_function(0xdede); }
int callee_function(int my_arg) { return my_arg; }
```

The callee retrieves its argument via `mov eax, [ebp+0x8]` — a
**positive** offset. Look at the diagram above: positive from your own
`ebp` reaches *backward*, into whoever called you, because their frame
sits at higher addresses. The book's own definition, worth keeping
close to verbatim: **the default C calling convention pushes
arguments in reverse order, so the first argument ends up on top.**
Nothing about the CPU enforces this — pure software agreement.
Practical tip: **find the `ret` instructions** to see where one
function ends and the next begins in a disassembly.

Precision worth having: the book's own disassembly doesn't use a
literal `push` for the argument — it stores into space already
reserved by `sub esp, 0x8`. Real -O0 output often reserves
outgoing-argument space as part of the caller's own frame, rather than
pushing one argument at a time — the idealized textbook picture is a
simplification.

## [BOOK] Pointers Are Just Addresses (§5.1.4)

```c
char *video_address = 0xb8000;
*video_address = 'X';   // the direct C equivalent of file 08's raw VGA write
```

And the best moment in the book: compiling `char *s = "Hello";`
produces disassembly with lines that look like nonsense (`dec eax`,
etc.) — because **the disassembler can't tell code from data**
(file `01`'s theme, again). Everything after the function's `ret`
isn't code — it's the string's raw bytes (`48 65 6c 6c 6f`, plus C's
automatic trailing `0`), which a disassembler dutifully tries to
decode as instructions anyway. You've now seen this exact phenomenon
three times: a hand-typed boot sector (file `02`), your own hand
assembly (file `03`), and now real compiler output.

---

## [CORE CONCEPT] What a Calling Convention Actually Is

Four questions, answered differently by BIOS (file `03`), your own ad
hoc convention (file `05`), and C (this file): **how are arguments
passed? which registers may the callee clobber? where does the return
value go? who cleans up the stack?** Nothing about the CPU enforces
any answer — a mismatch produces no hardware error, just wrong values
or a crash.

## [CORE CONCEPT] The Heap: What the Book Never Builds

Every buffer in this book is a fixed global or stack-local — **the
book never implements a heap.** The **heap** is a pool for allocations
whose size or lifetime doesn't fit the stack's strict, nested pattern
— it needs an explicit allocator (`malloc`/`free`) tracking what's in
use, since nothing about the hardware manages it automatically the way
`push`/`pop` manage the stack. (Terminology note: this is unrelated to
the heap *data structure* from algorithms — same word, unconnected
meaning.)

---

## [RELATED KNOWLEDGE] Caller-Saved vs. Callee-Saved

File `05`'s `pusha`/`popa` saved everything, bluntly. Real conventions
are precise: **caller-saved** registers the callee may freely clobber;
**callee-saved** ones it must itself preserve — real efficiency
`pusha`/`popa` can't offer.

## [RELATED KNOWLEDGE] Alignment Can Be a Hard Requirement

The book frames its 16-byte allocation as pure efficiency. On System V
AMD64, 16-byte stack alignment at `call` boundaries is sometimes a
**correctness** requirement — SSE instructions fault on misalignment.

---

## [MODERN SYSTEM] x86-64 Mostly Skips the Stack for Arguments

**System V AMD64** passes the first six integer/pointer arguments in
`rdi, rsi, rdx, rcx, r8, r9` — no stack involved at all. Only a 7th+
argument spills. The same register-passing shape as BIOS (file `03`)
and Linux syscalls (file `21`) — recurring at every layer of this
course. A **red zone** (128 bytes below `rsp`) lets leaf functions use
scratch space without adjusting the stack pointer — no book-era
32-bit equivalent.

---

## [ADVANCED / BEYOND THE BOOK] Frame-Pointer Omission

`-O2` often stops maintaining `ebp` as a dedicated frame pointer,
using it as an ordinary register and tracking everything from `esp`
instead — faster, but harder to unwind without extra metadata (DWARF
CFI). Directly affects files `27` and `32`.

## [ADVANCED / BEYOND THE BOOK] Stack Canaries

`-fstack-protector` plants a known value between locals and the saved
return address, checked right before `ret`. A mismatch aborts with
"stack smashing detected" instead of blindly using a corrupted return
address — direct defense against file `05`'s ROP mention.

---

## Practical Experiment

1. Reproduce all three book examples matched to the book's own 32-bit
   output: `gcc -m32 -O0 -c *.c && objdump -d *.o`.
2. Compile the same files **without** `-m32` (native x86-64) — find the
   argument in a register (`edi`), not on the stack.
3. Compare `-O0` vs. `-O2` on the local-variable example — is `ebp`
   still a frame pointer?
4. Prove the heap is a different region:
   ```c
   int stack_var = 1;
   int *heap_var = malloc(sizeof(int));
   printf("stack: %p\nheap:  %p\n", (void*)&stack_var, (void*)heap_var);
   ```
   Compare the two addresses.

## Debugging

"Stack smashing detected" is now fully explainable: a canary, using
this exact frame layout, catching corruption before a bad return
address is used. A `gdb` backtrace (`bt`) works the same way — walking
the chain of saved `ebp` values back through every caller, one
`[ebp]` at a time. Frame-pointer omission is exactly why a backtrace
can come out wrong on optimized code.

## Source Code

`local_var.c`, `caller_callee.c`, `my_string.c`, the
stack-vs-heap-address program — both `-m32` and native disassembly.

---

## Common Misconceptions

- **"A calling convention is CPU-enforced."** Pure software agreement
  — no hardware catches a mismatch.
- **"`sub esp, 0x10` for one `int` is a bug."** Deliberate,
  alignment-related -O0 behavior.
- **"`ebp+0x8` makes no sense given the stack grows down."** It makes
  sense *because* of it — positive offsets from your own `ebp` reach
  backward into the higher-addressed caller.
- **"Heap and stack are basically the same thing."** Managed
  completely differently, and live in separate regions — proven above.

## Interview Questions

**Basic:** What do `push ebp` / `mov ebp,esp` accomplish?
**Intermediate:** How does the callee retrieve a stack-passed argument?
**Advanced:** Why does `[ebp+0x8]` reach into the *caller's* frame?
**Systems Design:** With far more registers than original x86, would
you still pass arguments on the stack? What does System V AMD64 suggest?
**Debugging:** "Stack smashing detected" — what mechanism, relying on
what layout fact?
**Explain what happens when...:** a function with two parameters and
one local is called, from just before `call` to just after `leave`?

## Active Recall

1. Why 16 bytes reserved for one 4-byte `int`?
2. What is effective address computation?
3. The book's own definition of C's calling convention.
4. Biggest practical difference vs. System V AMD64?

## Answers

1. Alignment — fixed, maximum-width chunks rather than exact size.
2. A real addressing mode computed at run time from a register's
   current value — not folded at assembly time.
3. Arguments pushed in reverse order, first argument on top.
4. System V AMD64 passes the first six arguments in registers, not on
   the stack at all.

---

## What You Should Now Understand

- The book's three C-to-assembly disassemblies, and its best
  recurrence of "code and data are just bytes."
- A calling convention as a precise four-part contract, seen three
  different ways across this course.
- The heap — absent from the book, now understood and proven distinct
  from the stack on your own machine.

---

**Next:** file `11-c-compilation-and-machine-code.md`.
