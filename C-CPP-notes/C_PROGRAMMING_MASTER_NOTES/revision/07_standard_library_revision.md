# Revision — Phase 7: The Standard Library, By Design

> Quick-reference companion to `libraries/`. **Phase 7 covers all ten files across all
> fifteen standard headers and is now complete.**

---

## `<stdio.h>` — 1-minute summary

The ISO C committee deliberately excluded raw UNIX-style `open`/`read`/`write`/`close`
from the standard — those are POSIX, not C. `FILE*` streams exist specifically to add
buffering on top of whatever the OS actually provides, because a real system call per
character would be disastrously slow. `getchar`/`putchar` are macros with a cheap-pointer-
comparison fast path that only falls through to a real function call when the buffer needs
refilling/flushing — `(getchar)()`'s parentheses specifically suppress macro re-expansion
to reach that real function. `stderr` is never fully buffered by default (so it appears
promptly); `stdin`/`stdout` are fully buffered exactly when the implementation can tell
they're *not* attached to an interactive device — which is why piped/redirected output can
seem to "hang" until an `fflush` or the buffer fills.

### 5-minute revision
- Three buffering modes: unbuffered (`_IONBF`), fully buffered (`_IOFBF`, flush on buffer-
  full), line buffered (`_IOLBF`, flush on newline).
- Text vs. binary `fopen` modes exist because not every platform treats raw bytes and text
  lines identically (MS-DOS's `\r\n`, for instance) — UNIX just happens to make the
  distinction a no-op.
- `printf`/`scanf` have no compiler-enforced type safety between the format string and the
  variadic arguments by the C standard itself — a mismatch is a real, classic bug class.

**Worth memorizing:** which of the three standard streams is never fully-buffered by
default, and why.
**Worth understanding, not memorizing:** the `getchar` macro's exact fast-path/fallback
structure (stdio.md §3) — you should be able to explain *why* `(getchar)()` needs those
parentheses, not just that it does.

---

## `<stdlib.h>` (non-allocation groups) — 1-minute summary

`<stdlib.h>` is an admitted "hodgepodge" — six loosely related groups sharing one header
because nothing else fit. `abs`/`div`/`labs`/`ldiv` are integer math (`div` exists because
many CPUs compute quotient+remainder together in one instruction). `qsort`/`bsearch` are
type-generic via a comparator function pointer, exactly the callback pattern from Phase 3;
`qsort`'s recursion is deliberately bounded to `log₂(N)` depth by always recursing on the
*smaller* partition and looping on the larger. `atoi`/`atof`/`atol` give no way to detect a
parse error — `strtol`/`strtod` do, via their `endptr` output. `atexit`-registered
functions run in reverse registration order; `abort()` skips orderly cleanup that
`exit()` performs.

### 5-minute revision
- `abs(INT_MIN)` overflows on a two's-complement machine — the same negation asymmetry as
  the `printd`/`itoa` bug from Phase 2.
- `system()` on untrusted input is a command-injection risk — treat it as dangerous unless
  its argument is a fixed, hardcoded string.
- Storage allocation (`malloc`/`calloc`/`realloc`/`free`) is also technically part of
  `<stdlib.h>`, but is covered in full back in `memory/05_dynamic_memory.md`, Phase 6.

**Worth memorizing:** `atoi`/`atof`/`atol`'s complete inability to signal a parse error —
this is asked constantly.
**Worth understanding, not memorizing:** why bounding recursion to the smaller partition
(stdlib.md §3) actually guarantees `log₂(N)` depth — walk through the argument, don't just
recall the conclusion.

---

## `<string.h>` — 1-minute summary

Every function falls into one of three naming families: `mem*` (explicit count, any
bytes), `strn*` (bounded by count *or* `'\0'`, whichever comes first), `str*` (relies on
`'\0'` alone). The header is an acknowledged historical accretion, not a clean design —
inconsistent return values and surprising functions (`strncat`) are real, admitted warts,
not things you're missing hidden wisdom about. `memcpy` is undefined behavior on
overlapping buffers; `memmove` is always safe (defined as if copying through a temporary
buffer) — the difference is exactly what licenses `memcpy`'s `restrict`-qualified,
potentially-faster implementation. `strncpy` does **not** guarantee null-termination if the
source is `>= n` characters; `strncat` **always** appends a terminator. `strtok` mutates its
input and keeps state internally — not safe for nested or concurrent tokenization; use
`strtok_r` (POSIX) instead when that matters.

### 5-minute revision
- `strncpy`'s padding-to-`n` behavior exists because it was originally designed for
  fixed-width filename fields, not as a "safe `strcpy`" — using it as one requires manually
  force-terminating afterward.
- `memcpy`'s real prototype qualifies both pointers `restrict` — the caller's promise of
  non-overlap is what licenses the implementation's freedom.
- Nesting two `strtok`-based tokenization loops corrupts both — the "current position"
  state is shared, not per-string.

**Worth memorizing:** the `strncpy`-doesn't-always-terminate vs. `strncat`-always-
terminates asymmetry, cold.
**Worth understanding, not memorizing:** *why* `memcpy` is allowed to be undefined on
overlap while `memmove` isn't (string.md §3) — this is the clearest, most concrete
introduction to what `restrict` actually buys an implementation.

---

## `<ctype.h>` & `<locale.h>` — 1-minute summary

Hand-rolled character tests like `'A' <= c && c <= 'Z'` are ASCII-specific and silently
wrong on other execution character sets — `isalpha`/`isdigit`/etc. exist to answer these
questions portably. Real implementations use a bitmask lookup table for speed (the same
"precompute, cheap lookup" idea as `getchar`'s macro trick, applied differently). Every
function in this header requires its argument to be representable as `unsigned char` or
equal to `EOF` — anything else, including a plain signed `char` with its sign bit set, is
undefined behavior, concretely because it produces a negative, out-of-bounds table index.
A program starts in the `"C"` locale; `setlocale` can change classification behavior later.

**Worth memorizing:** `isalpha((unsigned char) c)` as a fixed defensive habit whenever `c`
isn't already known to come from `getchar`/`fgetc`.

---

## `<math.h>` — 1-minute summary

Good math functions are genuinely hard: real implementations chase domain-error detection
(`EDOM`, invalid argument), range-error detection (`ERANGE` + `HUGE_VAL`, result too
large/small), avoiding spurious *intermediate* overflow even when the final answer is
fine, and tight accuracy targets — all while working within finite precision and finite
range. `errno` isn't cleared on success, so the standard idiom is: clear it to `0` before
the call, check it after.

**Worth memorizing:** the clear-before/check-after `errno` idiom, cold — it's the direct,
practical payoff of `<errno.h>`'s "never cleared on success" rule.

---

## `<assert.h>` & `<errno.h>` — 1-minute summary

`assert` must be a macro (so an optimizer can eliminate a provably-true check entirely),
must not call `<stdio.h>` functions directly (a hidden internal function does the real
printing, keeping the header self-contained), and must expand to a single `void`
conditional-operator expression (so it's legal even inside a larger expression) — three
real, simultaneous design constraints, not arbitrary rules. Getting `__LINE__` into a
string requires two layers of macro indirection (`_STR`/`_VAL`), because `#` stringifies
literal parameter text, not an already-expanded value, unless the expansion happens first
in a separate macro. `<assert.h>` is the one standard header allowed to behave differently
each time it's included, tracking `NDEBUG`'s current status. `errno` itself is a UNIX-
inherited mechanism nobody on the standards committee much liked but that endures anyway;
it is never cleared to zero by a successful library call.

**Worth memorizing:** the `_STR(x) → _VAL(x) → #x` two-layer stringification pattern —
recognizing it elsewhere is a real fluency marker.
**Worth understanding, not memorizing:** *why* `assert` can't just call `fprintf` directly
(assert_and_errno.md §1) — this is header self-containment discipline from Phase 2, seen
from a new angle.

---

## `<setjmp.h>` & `<signal.h>` — 1-minute summary

`setjmp`/`longjmp` implement a `goto` that can cross function boundaries — C has no nested
functions, so this is the only built-in way to unwind several call levels at once.
`jmp_buf` is an array type, so it's secretly passed by reference even though the call
syntax looks like pass-by-value — the same trick `va_list` (below) sometimes uses.
`setjmp` may only appear in a short list of permitted expression forms (never
`n = setjmp(buf)`), and a local variable modified between `setjmp` and a later `longjmp`
back into it has an indeterminate value unless declared `volatile`. Signals inherit UNIX's
genuinely weak historical semantics (signals can be lost; a handler must re-arm itself,
racily); an async-signal handler may only call `signal` and assign to a
`volatile sig_atomic_t` — nothing else is safe.

**Worth memorizing:** the `volatile sig_atomic_t` flag pattern for async signal handlers,
cold.
**Worth understanding, not memorizing:** *why* `setjmp`'s permitted-expression restriction
exists (setjmp_and_signal.md §2) — it's about what the code generator can safely preserve,
not an arbitrary rule.

---

## `<stdarg.h>` — 1-minute summary

`va_arg` must always request the *promoted* type of a variadic argument (`double`, not
`float`; `int`, not `char`/`short`) — a direct consequence of Phase 1's default-argument-
promotion rules, since a variadic call has no prototype to prevent them. Every variadic
function needs at least one named parameter (to anchor `va_start`) and must call `va_end`
before returning, even though that macro is a no-op on many implementations — skipping it
is a portability time bomb, not a harmless shortcut. `va_list` shares `jmp_buf`'s
array-type portability wrinkle: passing it to a helper function may or may not let that
helper's `va_arg` calls affect the caller's position, depending on the implementation.

**Worth memorizing:** which types must never be requested directly from `va_arg`
(`float`, `char`, `short`, and their `unsigned`/`signed` narrow cousins).

---

## `<time.h>` — 1-minute summary

The standard only requires a "best approximation" of the current date/time — never assume
accuracy without checking your specific platform. `tm_year` is years *since 1900*;
`tm_mon` is 0-indexed while `tm_mday` is 1-indexed — a classic pair to transpose.
`clock()` measures CPU time consumed, not wall-clock elapsed time — use `time()` for the
latter.

**Worth memorizing:** `tm_year + 1900` for the real year, and `clock()` vs. `time()`'s
genuinely different meanings.

---

## `<limits.h>`, `<float.h>` & `<stddef.h>` — 1-minute summary

A conforming C implementation can be *hosted* (the full standard library) or *freestanding*
(the language plus only `<float.h>`/`<limits.h>`/`<stdarg.h>`/`<stddef.h>` — the world of
OS kernels, bootloaders, and embedded firmware). `<limits.h>` predates ANSI C itself,
originating with `/usr/group`'s pre-standard UNIX-portability effort (a common ancestor
with POSIX). `CHAR_MIN < 0` is the portable way to test plain `char`'s signedness in code.
`*_EPSILON` (the gap between 1 and the next representable float above it) is the precise,
standard-given number behind "never compare floats with `==`."

**Worth memorizing:** the four freestanding-required headers — a genuinely useful fact for
any embedded/systems-adjacent interview.
**Worth understanding, not memorizing:** why a fixed epsilon isn't sufficient for comparing
floats of very different magnitudes (limits_float_stddef.md §4) — the reasoning matters
more than the one-liner.

---

## Cross-topic traps worth drilling before an interview

1. `p = realloc(p, n);` without a temporary — covered in Phase 6, still worth re-drilling
   alongside `<stdlib.h>`'s other functions.
2. `atoi("garbage")` returning `0` — indistinguishable from a legitimately-parsed zero.
3. `strncpy(dst, src, n)` leaving `dst` unterminated when `src` is `>= n` characters long.
4. `memcpy` used on overlapping buffers "because it happened to work in testing."
6. `isalpha(c)` for a plain (possibly signed) `char c` holding a byte ≥ 128 — undefined
   behavior; cast to `unsigned char` first.
7. Checking `errno` after a math (or other) library call without clearing it to `0`
   beforehand — a stale value from an earlier, unrelated failure looks identical to a real
   new failure.
8. `#define _STR(x) #x` used directly on `__LINE__` — stringifies the literal text
   `__LINE__`, not the line number; needs the two-layer `_STR`/`_VAL` indirection.
9. `n = setjmp(buf);` — outside the small set of permitted expression forms; undefined
   behavior despite looking harmless.
10. A signal handler calling `printf` or touching a non-atomic global — only `signal`
    calls and `volatile sig_atomic_t` assignments are safe inside an async handler.
11. `va_arg(ap, float)` or `va_arg(ap, char)` — must request the promoted type
    (`double`/`int`) instead.
12. `tm_year` used directly as the calendar year, without `+ 1900`.
13. Comparing two `double`s with `==` after any nontrivial computation — use an
    epsilon-based comparison instead.
