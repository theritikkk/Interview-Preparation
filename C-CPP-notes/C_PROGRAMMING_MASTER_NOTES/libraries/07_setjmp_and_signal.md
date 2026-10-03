# `<setjmp.h>` & `<signal.h>` — Escaping Normal Control Flow

> **Phase 7 · The Standard Library, By Design.** Depends on:
> `fundamentals/04_control_flow.md` §7 (`goto` — this file's mechanisms are, in effect, a
> `goto` that can cross function boundaries), `fundamentals/02_types.md` §6 (`volatile`),
> `memory/02_pointers.md` §3 (array decay — `jmp_buf`'s pass-by-reference behavior depends
> on it), `functions/01_functions_and_stack_frames.md` §6 (the stack-frame model these
> mechanisms bypass).
> Sources: **Book C** ch.8 (`<setjmp.h>`) and ch.9 (`<signal.h>`), both in full — genuinely
> rich comparative-language-design and OS-history material unique among the three books.

---

## 1. `<setjmp.h>` — why C needs this at all

`[Book-derived, C]` C deliberately disallows nested function definitions — every function
in a translation unit is visible at the same level, with no hierarchy. One real consequence:
C has no built-in way to transfer control **out of** a function except by an ordinary
`return`. Most of the time that's exactly the discipline you want
(`functions/01_functions_and_stack_frames.md`'s whole stack-frame model depends on calls
and returns nesting cleanly) — but occasionally, especially for serious-error handling deep
inside nested calls, you want to jump straight back to an *earlier* function invocation in
one stroke, skipping every intermediate return.

### 🧠 A genuine cross-language design comparison

`[Book-derived, C]` Other languages solved this differently, and comparing them sharpens
what C's choice actually costs:

- **Pascal** allows a *nested* function to `goto` a label in any *enclosing* function —
  safe, because Pascal's nested-function structure means the compiler can always tell
  which enclosing invocations are still active; you cannot construct a jump to a function
  that isn't currently on the call stack at all.
- **PL/I** introduced *label variables*: you can store "a place to jump to" in a variable
  and jump to it later from anywhere — more flexible, but genuinely dangerous: nothing
  stops you jumping to a label variable that was never initialized, or whose target
  function has already returned.
- **C** takes essentially the PL/I approach, but *without even PL/I's structure*: `setjmp`
  records "a place to jump back to" in an opaque `jmp_buf` object; `longjmp` jumps there.
  `[Book-derived, C]` If that `jmp_buf` is uninitialized, or refers to a function
  invocation that has already returned, **the behavior is undefined** — C's mechanism is,
  in Plauger's own words, "even more primitive than the unstructured `goto` of PL/I."

---

## 2. The exact mechanism, and its two real dangers

`[Book-derived, C]`

```c
#include <setjmp.h>
jmp_buf env;

int main(void) {
    if (setjmp(env) == 0) {
        /* first time through: setjmp itself just returned 0 */
        do_work();
    } else {
        /* we got here via a longjmp(env, someval) from deep inside do_work */
        handle_error();
    }
}
```

`setjmp` saves the current calling environment into `env` and returns `0` on this direct
call. `longjmp(env, val)` restores that environment — control resumes as if `setjmp` had
just returned again, this time yielding `val` (or `1`, if `val` was `0` — `longjmp` can
never make `setjmp` appear to return `0`, since `0` is reserved to mean "this is the direct
call, not a jump back").

### ⚠️ Danger 1 — where `setjmp` is legally allowed to appear

`[Book-derived, C, the exact standard restriction]` Because `setjmp` must capture enough
of the machine's state (registers holding intermediate expression results, in particular)
to correctly resume execution later, the standard **severely restricts** the expressions
`setjmp` may appear in — specifically: the entire controlling expression of an `if`/
`switch`/loop; one side of a `<`/`<=`/`==`/etc. comparison against a constant, as the
entire controlling expression; negated with `!`, as the entire controlling expression; or
alone as a full expression statement. **You may not write `n = setjmp(env);`** — assigning
its result to a variable is not one of the permitted forms, even though it might appear to
work on some implementations. `[Synthesis]` This is a direct, if unusually strict,
consequence of the same "the standard restricts what it can't reliably reason about"
principle that shows up throughout this vault's undefined-behavior discussions
(`fundamentals/02_types.md` §9, foreshadowing `advanced/01_undefined_behavior.md`, Phase
11) — here applied to protect the code generator's own register-allocation freedom.

### ⚠️ Danger 2 — local variables can revert after a `longjmp`

`[Book-derived, C, the exact standard rule]` Automatic (non-`static`, non-`volatile`)
variables local to the function that called `setjmp`, if modified **between** the `setjmp`
call and a later `longjmp` back into it, have **indeterminate** values once control
resumes — because the code generator may have kept such variables in registers that
`setjmp`/`longjmp` save and restore as part of the calling environment, silently reverting
any changes made in between. **The fix, per the standard:** declare any such variable
`volatile` — this tells the code generator not to keep it in a register across the
`setjmp`, avoiding the reversion.

### 🎯 Interview point
"Why does declaring a variable `volatile` fix the `setjmp`/`longjmp` reversion problem?"
— `volatile` forces the compiler to keep the variable in real memory (not a register) and
re-read/re-write it around any point it might be observed — including across
`setjmp`/`longjmp`'s save/restore of register state — so the variable's true, current value
survives the jump instead of reverting to whatever was captured at the `setjmp` call.

### 💡 `jmp_buf` is secretly passed by reference — a direct callback to Phase 3
`[Book-derived, C]` `jmp_buf` is defined as an **array type**. Per
`memory/02_pointers.md` §3, an array-typed function argument decays to a pointer — so
even though `setjmp(env)`/`longjmp(env, val)` *look* like ordinary pass-by-value calls,
`env` is actually passed by reference, which is exactly what lets `setjmp` write the
calling environment into storage the *caller* (not just the callee) can see. This is a
genuinely elegant, deliberate exploitation of array decay — not an accident.

### The recommended usage pattern

`[Book-derived, C]`

```c
static jmp_buf jmpbuf;

void top_level(void) {
    for (;;)
        switch (setjmp(jmpbuf)) {   /* setjmp as the ENTIRE controlling expression — legal */
        case 0:  process(); return;           /* first time through */
        case 1:  /* report error */ break;     /* restart, via longjmp(jmpbuf, 1) deep inside */
        case 2:  /* report error */ return;     /* terminate, via longjmp(jmpbuf, 2) */
        default: /* report error */ return;
        }
}
```

`[Book-derived, C]` Isolating the `setjmp` call in its own small function (here,
`top_level`, with the real work done in `process`) minimizes exactly which local variables
are at risk under Danger 2, since `top_level` itself has almost none.

---

## 3. `<signal.h>` — a second, messier way to escape normal control flow

`[Book-derived, C]` A **signal** is an extraordinary event — **synchronous** (caused by
the program's own action, like division by zero) or **asynchronous** (caused by something
outside the program, like an interrupt keystroke or another process). Three ways to handle
a given signal: **default** (typically, terminate), **ignore** it, or install a **handler**
function that runs when the signal occurs.

### ⚠️ Why signal handling is inherently fragile — real, historical reasons

`[Book-derived, C]` The semantics `<signal.h>` inherited are, by Plauger's own account,
based on early UNIX's genuinely weak signal model:
- **Signals could be lost** — UNIX didn't queue them; if a second signal arrived before a
  handler finished processing the first, it could simply vanish.
- **A handler must re-arm itself, and there's a race in doing so** — when a signal fires,
  handling for that signal reverts to default *before* your handler runs; if you want your
  handler to keep handling future occurrences of the same signal, it must call `signal`
  again to reinstall itself — and if the same signal fires again *between* your handler
  starting and that reinstall call, the default handler (often, terminate) fires instead.
- **No clean "done handling" mechanism** exists at the language level.

`[Book-derived, C]` The C standard's own signal semantics are, in Plauger's words, even
weaker than UNIX's original: a given signal may never occur unless your own code calls
`raise`, and it may be freely ignored — meaning **no portable use of `<signal.h>` can be
guaranteed to actually happen** on an arbitrary conforming implementation. Any real
signal-handling code is inherently platform-specific in practice, whatever the standard
formally allows.

### ⚠️ What a signal handler may safely do — a genuinely short list

`[Book-derived, C, the exact standard restriction]` If a signal handler is invoked for
anything other than a call to `abort`/`raise` by your own code, the handler **must not**
call any standard library function other than `signal` itself, and must not access any
`static`-duration object **except** by assigning to one of type `volatile sig_atomic_t`.
`sig_atomic_t` (defined in `<signal.h>`) is an integer type guaranteed to be accessed
**atomically** — a signal can never catch such an access half-done. The idiomatic,
standard-safe async handler:

```c
static volatile sig_atomic_t intflag = 0;

static void field_int(int sig) {
    signal(SIGINT, &field_int);   /* re-arm first, before anything else (Danger above) */
    intflag = 1;                    /* the ONLY other safe action: set an atomic flag */
}
```

with the main program periodically polling `intflag` and acting on it **outside** the
handler, where ordinary code (I/O, non-atomic variable access) is safe again.

### The standard's own signal list, and how to think about each

`[Book-derived, C]`

| Signal | Meaning | Handler guidance |
|---|---|---|
| `SIGABRT` | Program called `abort` | Do minimal work; end with `return` or `exit`; **don't** call `abort` again from here |
| `SIGFPE` | Arithmetic exception (overflow, zero divide, …) | A handler generally shouldn't return — the computation that trapped can't safely continue |
| `SIGILL` | Illegal instruction | Rarely handled meaningfully by application code |
| `SIGINT` | Interactive attention (e.g. Ctrl-C) | Safe to ignore or handle; a well-behaved async handler as shown above |
| `SIGSEGV` | Invalid storage access | Must not return; the program's memory state can no longer be trusted |
| `SIGTERM` | A polite external request to terminate | Safe to ignore, though impolite to do so |

### 💡 Two categories, one design decision
`[Book-derived, C]` Every signal handler falls into one of two categories: handlers that
**must not return** (they end in `abort`, `exit`, or `longjmp` — `SIGFPE`, `SIGSEGV`,
typically `SIGABRT`) and handlers that **must return** (ending in a plain `return` — the
asynchronous, "just note it happened and keep going" signals like `SIGINT`/`SIGTERM`).
Deciding which category a given signal belongs to, before writing its handler, is the
single most important design decision in any signal-handling code.

---

## 4. ⚠️ Misconceptions to unlearn now

- **"`n = setjmp(buf);` is a convenient, if slightly unusual, way to capture the result."**
  It's explicitly outside the small set of contexts the standard permits `setjmp` to appear
  in (§2) — not merely unconventional, but undefined behavior.
- **"A signal handler can safely call `printf` to log what happened."** Almost never — a
  handler invoked asynchronously must not call standard library functions beyond `signal`
  itself; `printf` mid-handler can corrupt an in-progress, interrupted stdio operation
  (§3, `libraries/01_stdio.md`'s buffering internals — exactly the kind of shared, non-
  reentrant state a signal can catch "mid-update").
- **"`jmp_buf` is passed by value, like it looks."** It's an array type, so it decays to a
  pointer — genuinely passed by reference (§2).

---

## 5. 🎯 Interview questions

**Easy**
- What does `setjmp` return on its direct call, versus after a `longjmp`? → `0` directly;
  the value passed to `longjmp` (or `1`, if that value was `0`) after a jump back (§2).
- Name the three ways a signal can be handled. → Default, ignore, or a custom handler (§3).

**Medium**
- Why does the standard restrict `setjmp` to appearing only in a handful of specific
  expression forms? → It needs to capture enough machine state (including registers
  holding intermediate expression results) to resume correctly later; unrestricted use
  could hide state changes the mechanism can't reliably preserve (§2).
- Why must an async-signal handler only touch `volatile sig_atomic_t` variables? →
  `sig_atomic_t` guarantees atomic access, so a signal can never interrupt such an access
  partway through, unlike an ordinary variable, which could be caught mid-update (§3).

**Hard**
- Explain why a local variable in the function that called `setjmp` can revert to an
  earlier value after a `longjmp`, and how `volatile` fixes it. → The code generator may
  keep such a variable in a register that `setjmp`/`longjmp` save and restore as part of
  the calling environment; changes made between the calls can be silently lost when that
  register is restored. `volatile` forces the compiler to keep the variable in real memory
  and re-read/write it consistently, so its true value survives the jump (§2).
- Why is `jmp_buf`'s array-type definition load-bearing, not incidental? → Array decay
  (`memory/02_pointers.md` §3) makes `env` in `setjmp(env)`/`longjmp(env, val)` a pointer
  in practice, even though the call syntax looks like ordinary pass-by-value — this is
  precisely what lets `setjmp` write shared state the *caller* can later see via `longjmp`
  (§2).

**Systems-level**
- Why can UNIX-derived signal semantics lose a second occurrence of the same signal while
  a handler for the first is still running? → Historically, UNIX didn't queue signals — it
  remembered only the most recent occurrence, so a second signal arriving before the first
  is fully processed could simply overwrite or be discarded rather than queued for later
  delivery (§3).
- A signal handler for `SIGINT` calls `printf` to report the interrupt, and the program
  occasionally corrupts its own output afterward. Why? → The signal may have interrupted
  the library partway through an unrelated `stdio` operation; calling `printf` from within
  the handler risks re-entering non-reentrant buffering state
  (`libraries/01_stdio.md` §2's `FILE` internals), corrupting it (§3).

---

## 6. External references

- [cppreference — `<csetjmp>`/`<setjmp.h>`](https://en.cppreference.com/w/c) `[External]`
  — current, complete reference including the exact undefined-behavior conditions.
- [cppreference — `<csignal>`/`<signal.h>`](https://en.cppreference.com/w/c) `[External]`
  — current signal list and the async-signal-safety rules in full.
- [man7.org — `signal-safety(7)`](https://man7.org/linux/man-pages/) `[External]` — the
  much longer, POSIX/Linux-specific list of functions considered safe to call from a
  signal handler in practice, beyond the ISO-C-only list in §3.

## 7. 🛠 Practice

1. `[Book-derived, C]` Implement the `top_level`/`process` `setjmp`/`switch` pattern from
   §2 end to end, with `process` calling `longjmp(jmpbuf, 1)` from several levels of nested
   function calls deep, to feel the "skip every intermediate return" effect directly.
2. Reproduce the local-variable-reversion bug from Danger 2: modify a non-`volatile` local
   between a `setjmp` and a later `longjmp` back into the same function, observe the
   reverted value, then fix it by declaring the variable `volatile`.
3. `[Book-derived, C]` Write the `SIGINT` handler pattern from §3 (a `volatile sig_atomic_t`
   flag, re-armed on entry), and a main loop that polls the flag and performs real cleanup
   work (safely, outside the handler) once it's set.
