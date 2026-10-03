# `<stdarg.h>` — Variable Argument Lists

> **Phase 7 · The Standard Library, By Design.** Depends on: `fundamentals/03_expressions_and_operators.md`
> §3 (integer/float promotion — the exact rule this header's central restriction is built
> on), `memory/02_pointers.md` §4 (pointer arithmetic — the implementation technique in §5
> below), `libraries/07_setjmp_and_signal.md` §2 (`jmp_buf`'s array-decay trick — `va_list`
> shares the same portability wrinkle).
> Sources: **Book C** ch.10, in full (primary — a genuinely rich implementation history
> and a real, working macro-based implementation).

---

## 1. A real history: from raw pointer arithmetic to a portable macro API

`[Book-derived, C]` On the original PDP-11, walking a variable argument list needed no
special support at all — arguments sat at predictable, contiguous offsets, pointers and
`int`s were the same size, and structures couldn't even be passed as arguments, so **plain
pointer arithmetic** sufficed. As C spread to other architectures — some with argument
lists growing *downward* instead of up, some with pointers of varying sizes, some requiring
alignment padding between arguments — that simple arithmetic stopped being portable, even
though it kept *working* on any one given machine. Andy Koenig's `<varargs.h>` (a real,
influential UNIX-era header, pre-standard) was an early attempt to hide these differences
behind a small set of macros; `<stdarg.h>` is the standards committee's own, deliberately
incompatible refinement of that same idea. `[Synthesis]` This is a clean, concrete example
of a recurring theme in this vault: **portability problems get solved by adding a layer of
indirection** (here, macros hiding architecture-specific pointer arithmetic) — the same
underlying strategy behind `<stdint.h>`'s exact-width types
(`fundamentals/02_types.md` §8) and `<errno.h>`'s existence
(`libraries/06_assert_and_errno.md` §2), applied to a different problem.

---

## 2. The three macros, and the one hard rule behind their restrictions

`[Book-derived, C]` `va_list` is an opaque type for tracking "where am I in the argument
list." A variadic function must:

```c
#include <stdarg.h>

int my_func(int count, ...) {       /* at least one named (fixed) parameter is required */
    va_list ap;
    va_start(ap, count);              /* count: the LAST named parameter, by name */
    for (int i = 0; i < count; i++) {
        int arg = va_arg(ap, int);      /* fetch the next argument, given its type */
        /* ... */
    }
    va_end(ap);                        /* mandatory before returning */
    return /* ... */;
}
```

### ⚠️ The rule that trips up almost everyone: no "narrow" types in `va_arg`

`[Book-derived, C, directly grounded in `fundamentals/03_expressions_and_operators.md`
§3's promotion rules]` You must **not** write `va_arg(ap, float)`, `va_arg(ap, char)`, or
`va_arg(ap, short)`. This isn't an arbitrary restriction — it's a direct consequence of a
fact this vault already established: **when passed as a variadic argument (no prototype
governs it), `float` is promoted to `double`, and `char`/`short` are promoted to `int`,
automatically, as part of the call itself.** `va_arg` has no way to "undo" that promotion —
it can only read the argument back out **as the type it was actually widened to**. Reading
it back as the original, narrower type is undefined behavior. The fix is always to request
the *widened* type: `va_arg(ap, double)`, `va_arg(ap, int)`.

### 🎯 Interview point
"Why can't you call `va_arg(ap, char)` even if you know the caller passed a `char`?" —
Because that `char` was already promoted to `int` at the call site, before `my_func` ever
ran; there's no `char`-sized argument sitting in the list to read — asking `va_arg` for the
wrong (narrower) type doesn't recover the original value, it's simply undefined.

---

## 3. Two other real restrictions, and why each exists

`[Book-derived, C]`

- **A function must declare at least one *named* parameter.** `va_start` needs to know
  where the fixed parameters end and the variable ones begin — it locates that boundary
  relative to the *last named parameter*, so there must be at least one to anchor from.
- **`va_end` must run before the function returns**, if `va_start` ran at all. Some
  implementations genuinely need it to release or clean up internal bookkeeping — treating
  it as a harmless no-op (which it often *is*, on many implementations) is a portability
  trap Plauger calls out directly: skipping it is a "time bomb" that may not misbehave for
  years, then break unpredictably the moment code is ported to an implementation that
  actually relies on it.

---

## 4. `va_list`'s array-type portability wrinkle — the same trick as `jmp_buf`

`[Book-derived, C, echoing `libraries/07_setjmp_and_signal.md` §2 exactly]` On some
implementations, `va_list` is an array type — meaning passing `ap` to another function
*decays* it to a pointer (`memory/02_pointers.md` §3), so that function's own `va_arg`
calls **do** advance the original caller's `ap` too, since they share the same underlying
storage. On other implementations, `va_list` is an ordinary (non-array) type, genuinely
passed by value — a callee's `va_arg` calls do **not** affect the caller's own `ap`. **Both
are standard-conforming**, and code that assumes either behavior unconditionally is not
portable. The defensive fix, if you must split argument-processing across functions:
explicitly pass `va_list *` and dereference, forcing sharing regardless of which behavior
the underlying implementation happens to have — or explicitly `memcpy` the `va_list` if you
need a genuinely independent copy.

---

## 5. Under the hood — a real, working implementation

`[Book-derived, C, paraphrased — the header shown below preserves the technique, not the
book's exact internal names]` One real, working approach: represent `va_list` as a plain
`char *` pointing into the argument list's storage, and use pointer arithmetic (with
alignment rounding) to advance it:

```c
typedef char *va_list;

#define va_start(ap, last)  ((ap) = (char *)&(last) + ROUND_UP(sizeof(last)))
#define va_arg(ap, T)       (*(T *)(((ap) += ROUND_UP(sizeof(T))) - ROUND_UP(sizeof(T))))
#define va_end(ap)          ((void)0)
```

**Reading `va_arg` closely:** it first **advances** `ap` past the argument being fetched
(`(ap) += ROUND_UP(sizeof(T))`), then **backs up** by that same amount to point at the
*start* of that argument, casts the resulting address to `T *`, and dereferences it — a
compact way to "consume and return" in one expression. `[Book-derived, C]` Real
implementations must additionally account for whether each architecture's argument-padding
"hole" (for alignment) falls before or after each value — a detail `<stdarg.h>`'s macros
must get right per-platform, which is exactly why this header, alone among the ones in this
vault, is essentially impossible to implement in fully portable C — it inherently depends
on compiler/platform-specific knowledge of how arguments are actually laid out in memory.

---

## 6. ⚠️ Misconceptions to unlearn now

- **"You can request any argument's exact original type from `va_arg`."** Only the type it
  was actually promoted to at the call site — `float`→`double`, `char`/`short`→`int` (§2).
- **"Skipping `va_end` is harmless if it happens to compile and run fine."** It's
  undefined behavior per the standard, and "happens to work" on one implementation says
  nothing about others (§3).
- **"A `va_list` argument always behaves like an ordinary by-value parameter."** It may
  or may not, depending on whether the implementation defines `va_list` as an array type —
  genuinely implementation-defined behavior, not something to assume either way (§4).

---

## 7. 🎯 Interview questions

**Easy**
- What must every variadic function declare at least one of? → A named (fixed) parameter,
  which `va_start` uses to locate the start of the variable arguments (§3).
- What must run before a variadic function returns, if `va_start` ran? → `va_end` (§3).

**Medium**
- Why can't you call `va_arg(ap, char)` to fetch an argument the caller passed as a
  `char`? → The `char` was already promoted to `int` at the call site (no prototype governs
  a variadic argument); `va_arg` can only correctly read back the promoted type (§2).
- Why is skipping `va_end` a real, if often invisible, portability bug? → Many
  implementations treat it as a no-op, but some genuinely rely on it for cleanup — code
  that skips it can work for years and then fail only once ported to a stricter
  implementation (§3).

**Hard**
- Explain why passing a `va_list` to a helper function can behave differently across
  conforming implementations. → `va_list` may or may not be an array type; if it is, it
  decays to a pointer and the helper's `va_arg` calls affect the caller's own position; if
  not, it's passed by value and they don't — both are standard-conforming, so code relying
  on either behavior without explicit `va_list *`/`memcpy` handling isn't portable (§4).
- Walk through why `va_arg`'s macro implementation both advances and then "backs up" a
  pointer in the same expression. → It needs to move `ap` past the current argument for the
  *next* call to work correctly, while still yielding the *current* argument's address to
  dereference — advancing first and then subtracting the same offset achieves both in one
  self-contained expression (§5).

**Systems-level**
- Why is `<stdarg.h>`, uniquely among the headers in this vault, essentially impossible to
  implement in fully portable, standard C alone? → It depends on knowing, per-platform,
  exactly how arguments are physically laid out in memory (order, alignment padding
  location, growth direction) — information that isn't expressible in portable C and must
  come from compiler/architecture-specific knowledge (§5).

---

## 8. External references

- [cppreference — `<cstdarg>`/`<stdarg.h>`](https://en.cppreference.com/w/c) `[External]`
  — current, complete reference, including C99's `va_copy` (a standardized answer to §4's
  "explicit copy" workaround, added after this book's era).

## 9. 🛠 Practice — completing the phase project

1. `[Book-derived, C]` Write `va_fputs(FILE *str, ...)`, taking a `NULL`-terminated list of
   `char *` arguments and writing each to `str` in turn (Book C's own worked example) —
   this, plus your `<string.h>` slice from `libraries/03_string.md`, completes this
   phase's stated project of a small `printf`-lite formatter's building blocks.
2. Deliberately call `va_arg(ap, float)` inside a function that receives a `float`
   argument, and (if your compiler permits it at all) observe the mismatch — then fix it to
   `va_arg(ap, double)` and confirm correct behavior, to make §2's promotion rule concrete
   rather than abstract.
3. `[Book-derived, C]` Write `tryit`-style test code (per the source chapter's own test
   program) that exercises several different argument types via a format-string-driven
   variadic function, deliberately including a small struct argument, to probe your
   platform's actual `va_list`/alignment behavior directly.
