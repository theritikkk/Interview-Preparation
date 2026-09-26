# Revision — Phase 2: Functions, Scope & the Preprocessor

> Quick-reference companion to `functions/01_functions_and_stack_frames.md`,
> `02_recursion.md`, and `preprocessing/01_macros.md` → `03_conditional_compilation.md`.

---

## Functions & storage classes — 1-minute summary

C is **always** call-by-value — a function never receives the caller's actual variable,
only a copy; the "pass a pointer to let a function modify caller state" idiom works
*within* call-by-value, not around it, because the pointer's copy still refers to shared
storage. `static` means two different things depending on context: file-scope linkage
restriction on externals/functions, or persistent storage on a function-local variable.
`register` is an ignorable hint, mostly a historical artifact against modern optimizers.

### 5-minute revision
- Declaration vs. definition (recap): `extern int sp;` declares; `int sp;` at file scope
  defines and allocates storage. Exactly one definition per external name, program-wide.
- Scope: automatic variables/parameters are local to their function (or block); external
  names are visible from their declaration point to end-of-file (or program-wide via
  `extern` in other files).
- Default initial values: **zero** for external/`static`; **garbage** for automatic/`register`.
- A stack frame holds: return address, saved registers, parameters (call-by-value copies),
  locals. Unbounded recursion exhausts the stack → stack overflow, a distinct failure mode
  from heap exhaustion.

**Worth memorizing:** "C is always call-by-value" as a fixed fact, not a rule of thumb.
**Worth understanding, not memorizing:** why the pointer-passing idiom doesn't contradict
that fact (functions.md §2) — you should be able to explain it from the stack-frame model,
not just recite it.

---

## Recursion — 1-minute summary

Every recursive function needs a base case and guaranteed progress toward it. Each call
gets an independent stack frame, which is *why* recursion is safe (not magic) and *why*
missing a base case crashes via stack overflow rather than corrupting other calls' data.
Recursion buys clarity for recursively-structured problems, not speed or memory savings —
per-call overhead makes it generally slower than an equivalent loop.

### 5-minute revision
- `printd`'s "recurse on the rest, then act" ordering is the general pattern for turning a
  low-order-first data stream into high-order-first output — contrast with `itoa`'s
  iterative reverse-afterward approach.
- Both `itoa` and `printd` share an edge-case bug: negating `INT_MIN` overflows (signed
  overflow UB, `fundamentals/02_types.md` §9) — a good reminder that UB traps hide in
  ordinary-looking utility code, not just contrived examples.
- Tail-call optimization is real but **not guaranteed** by C compilers — don't rely on it
  to save a deeply-recursive function from stack overflow.

**Worth memorizing:** the two-part checklist (base case + progress).
**Worth understanding, not memorizing:** *why* recursion isn't inherently faster or more
memory-efficient than iteration (recursion.md §4) — a common, testable misconception.

---

## The preprocessor — 1-minute summary

The preprocessor is a distinct, syntax-blind text-substitution pass that runs before the
compiler sees any C. `#define` macros are substituted as raw text — arguments can be
evaluated multiple times (dangerous with side effects) and need full parenthesization to
survive operator-precedence collisions with the call site. `#include` pastes a file's
contents in verbatim; headers should hold declarations, not definitions, to avoid
multiple-definition link errors. `#if`/`#ifdef`/`#ifndef` gate code at preprocess time,
using only constant expressions with no type information available (`sizeof`, casts, and
`enum` constants are all off-limits inside `#if`).

### 5-minute revision
- `max(i++, j++)` double-increments the "winner" — macro arguments aren't evaluated once
  like function arguments are.
- `#define square(x) x*x` breaks on `square(z+1)` — always parenthesize both the whole
  replacement and every individual parameter use: `((x) * (x))`.
- `do { ... } while (0)` is the standard way to make a multi-statement macro behave like one
  statement, correctly absorbing a trailing semicolon.
- Include guards (`#ifndef`/`#define`/`#endif`) prevent a header's content from being
  pasted in twice in one translation unit; `#pragma once` is a widely-supported but
  non-standard shortcut.
- `#if` cannot use `sizeof`, casts, or `enum` constants — the preprocessor has no type
  information at all.

**Worth memorizing:** the double-parenthesization rule for function-like macros; the
`#ifndef`/`#define`/`#endif` include-guard skeleton, cold.
**Worth understanding, not memorizing:** *why* headers should hold declarations rather than
definitions (includes.md §2) — this explains a large fraction of real "multiple definition"
linker errors you'll hit in practice.

---

## Cross-topic traps worth drilling before an interview

1. `max(i++, j++)` — double side-effect evaluation from macro argument repetition
   (macros.md §2).
2. `#define square(x) x*x` invoked as `square(z+1)` — precedence leak without full
   parenthesization (macros.md §2).
3. Two `.c` files both `#include` a header containing a full function body — multiple-
   definition link error (includes.md §2).
4. `#if sizeof(int) == 4` — illegal; the preprocessor has no type information
   (conditional_compilation.md §1).
5. A recursive function with a subtly-wrong base case (e.g. `n <= 1` written as `n == 1`,
   missing `n == 0` or negative inputs) — silent infinite recursion for certain inputs,
   not caught at compile time (recursion.md §2).
