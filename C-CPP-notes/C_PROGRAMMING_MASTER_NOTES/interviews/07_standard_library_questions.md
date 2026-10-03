# Interview Questions — The Standard Library, By Design

> Consolidated from `libraries/`. **Phase 7 covers all ten files across all fifteen
> standard headers and is now complete.**

---

## Easy

1. **Are `open`/`read`/`write`/`close` part of the C standard?** No — POSIX, not ISO C.
   (`01_stdio.md` §1)
2. **Which standard stream is never fully buffered by default?** `stderr` — so diagnostic
   output appears promptly even right before a crash. (§4)
3. **What does `div()` return?** A struct holding both quotient and remainder from one
   division. (`02_stdlib.md` §2)
4. **Does `strncpy` always null-terminate its destination?** No — only if the source is
   shorter than `n`. (`03_string.md` §4)

## Medium

1. **Why does `getchar()`'s macro form check a pointer comparison before doing anything
   else?** It's testing whether a character is already buffered — the cheap common case —
   before falling back to the real, comparatively expensive function call. (`01_stdio.md` §3)
2. **Why can piped output appear to "hang" with no visible output?** Output to a
   non-terminal destination is typically fully buffered by default; text sits in the
   buffer until it fills or is explicitly flushed. (§4)
3. **Why do `qsort`/`bsearch` take a function-pointer comparator?** Type-generic design —
   one implementation works for any element type, as long as the caller supplies the
   comparison logic. (`02_stdlib.md` §3)
4. **Why is `strtol` generally preferable to `atoi` for untrusted input?** `strtol` reports
   how much input was actually consumed, letting you detect invalid input; `atoi` can't
   distinguish "invalid" from "legitimately zero." (§5)
5. **Why is `memcpy` undefined behavior on overlapping buffers while `memmove` is safe?**
   `memmove`'s contract is defined as if copying through a temporary buffer, guaranteeing
   correctness regardless of overlap; `memcpy` makes no such promise, which is what lets an
   implementation optimize it more aggressively. (`03_string.md` §3)

## Hard

1. **Explain exactly why `(getchar)()`, with parentheses, is necessary inside the
   `getchar` macro's own fallback path.** `getchar` is itself a macro; a bare `getchar()`
   there would risk re-triggering macro expansion, while `(getchar)` doesn't match the
   "identifier immediately followed by `(`" pattern the preprocessor looks for, so it
   correctly reaches the real, compiled function. (`01_stdio.md` §3)
2. **Why does Book C's `qsort` recurse on the smaller partition and loop on the larger
   one?** Since the recursive call always handles at most half the previous partition's
   size, recursion depth is bounded to `log₂(N)` regardless of pivot-choice quality —
   bounding stack usage even in bad cases. (`02_stdlib.md` §3)
3. **Why does the standard require implementations to behave "as if no library function
   calls `strtok`"?** It's an acknowledgment that `strtok` keeps its tokenization state
   internally (typically shared/static), making it unsafe for more than one tokenization
   sequence to be in flight at once. (`03_string.md` §5)

## Easy (continued)

5. **What locale does a C program start in?** The `"C"` locale — a fixed, portable,
   minimal baseline. (`04_ctype_and_locale.md` §3)
6. **What does defining `NDEBUG` before `#include <assert.h>` do?** Switches every
   subsequent `assert` in that translation unit to a no-op generating no code.
   (`06_assert_and_errno.md` §1)
7. **What are `EDOM` and `ERANGE` for?** Domain errors (invalid argument) and range
   errors (overflow/underflow) — the two portable error codes the library shares across
   functions. (§2, `05_math.md` §2)

## Medium (continued)

6. **What exact domain does the C standard require for `<ctype.h>` function arguments?**
   Representable as `unsigned char`, or equal to `EOF` — anything else is undefined
   behavior. (`04_ctype_and_locale.md` §2)
7. **Why must you set `errno = 0` before calling a math function if you intend to check it
   afterward?** A math function's return value alone often can't distinguish a
   legitimately large result from an overflow; `errno` isn't cleared on success, so an
   unzeroed check can pick up a stale value from an earlier call. (`05_math.md` §2)
8. **Why must `assert` be implemented as a macro rather than a real function?** So an
   optimizing compiler can eliminate the check entirely when it can prove the condition
   always holds. (`06_assert_and_errno.md` §1)

## Hard (continued)

4. **Explain mechanically why passing a negative `char` value to a lookup-table-based
   `isalpha` is genuinely dangerous, not just technically wrong.** A negative `int`
   argument produces a negative table index — an actual out-of-bounds memory read, not a
   merely theoretical violation. (`04_ctype_and_locale.md` §1–2)
5. **Why doesn't `#define _STR(x) #x` alone give you `__LINE__`'s value as a string?** `#`
   stringifies its own macro's literal parameter text without expanding it first; a
   two-layer `_STR`/`_VAL` pair lets `__LINE__` expand to its number as an ordinary
   argument before that number is stringified. (`06_assert_and_errno.md` §1)

## Systems-level

1. **A program processes a large file character-by-character using `getchar()` in a tight
   loop. Why isn't this the performance disaster naive reasoning might suggest?** The
   macro's buffered fast path means most calls never touch the OS at all — the real cost
   (a `read` system call) is paid once per buffer-full, amortized across many characters.
   (`01_stdio.md` §9)
2. **A function reads a filename from `getenv` and passes it unsanitized into `system()`.
   What's wrong?** Command-injection risk — a shell interprets the string, including any
   metacharacters, rather than treating it as a literal filename. (`02_stdlib.md` §6)
3. **Two nested loops each call plain `strtok` on different strings. What breaks, and
   what's the fix?** Both share the same internal "current position" state, so the nested
   call corrupts the outer call's in-progress tokenization; use `strtok_r` (POSIX) or
   equivalent explicit-state alternative instead. (`03_string.md` §5)

## Systems-level (continued)

4. **A text-processing program misclassifies bytes ≥ 128 only on a different platform.
   First thing to check?** Whether plain `char` is signed there and whether raw `char`
   values are passed into `<ctype.h>` functions without an `unsigned char` cast.
   (`04_ctype_and_locale.md` §2)
5. **You check `if (errno != 0)` after a documented call without clearing `errno` first,
   and get a false positive. What happened?** `errno` was already nonzero from an earlier,
   unrelated failure — never cleared on success, so it persisted until your check.
   (`06_assert_and_errno.md` §2)

## Easy (setjmp/signal, stdarg, time, limits/float/stddef)

1. **What does `setjmp` return on its direct call, versus after a `longjmp`?** `0`
   directly; the value passed to `longjmp` (or `1`, if that value was `0`) after a jump
   back. (`07_setjmp_and_signal.md` §2)
2. **What must every variadic function declare at least one of?** A named (fixed)
   parameter, which `va_start` uses to locate the variable arguments. (`08_stdarg.md` §3)
3. **What does `tm_year` actually store?** Years since 1900, not the literal year.
   (`09_time.md` §3)
4. **What four headers must even a minimal, freestanding C implementation support?**
   `<float.h>`, `<limits.h>`, `<stdarg.h>`, `<stddef.h>`. (`10_limits_float_stddef.md` §1)

## Medium (continued 2)

1. **Why does the standard restrict `setjmp` to appearing only in a handful of specific
   expression forms?** It needs to capture enough machine state (including registers
   holding intermediate results) to resume correctly later. (`07_setjmp_and_signal.md` §2)
2. **Why must async-signal handlers only touch `volatile sig_atomic_t` variables?**
   That type guarantees atomic access, so a signal can never interrupt it partway through.
   (§3)
3. **Why can't you call `va_arg(ap, char)` to fetch an argument passed as a `char`?** It
   was already promoted to `int` at the call site, with no prototype to prevent it.
   (`08_stdarg.md` §2)
4. **How would you measure wall-clock elapsed time versus CPU time consumed?** `time()`
   for wall-clock; `clock()` (divided by `CLOCKS_PER_SEC`) for CPU time — genuinely
   different questions. (`09_time.md` §3)
5. **How can you portably test whether plain `char` is signed?** `#if CHAR_MIN < 0`.
   (`10_limits_float_stddef.md` §3)

## Hard (continued 2)

1. **Explain why a local variable can revert to an earlier value after a `longjmp`, and
   how `volatile` fixes it.** The code generator may keep it in a register that
   `setjmp`/`longjmp` save and restore; `volatile` forces real-memory access so its true
   value survives. (`07_setjmp_and_signal.md` §2)
2. **Why is `jmp_buf`'s array-type definition load-bearing, not incidental?** Array decay
   makes it a pointer in practice, letting `setjmp` write shared state the caller can see
   via `longjmp` despite call syntax that looks like pass-by-value. (§2)
3. **Explain why passing a `va_list` to a helper function can behave differently across
   conforming implementations.** `va_list` may or may not be an array type; if so, it
   decays to a pointer and the helper's `va_arg` calls affect the caller's position — both
   behaviors are standard-conforming. (`08_stdarg.md` §4)
4. **Why isn't `fabs(a - b) < DBL_EPSILON` alone sufficient for comparing floats?**
   `DBL_EPSILON` measures the gap near `1.0` specifically; the true gap scales with
   magnitude, so a fixed epsilon is wrong for very large or very small numbers without a
   scale factor. (`10_limits_float_stddef.md` §4)

## Systems-level (continued 2)

1. **Why can UNIX-derived signal semantics lose a second occurrence of the same signal
   while a handler for the first is still running?** UNIX historically didn't queue
   signals — it remembered only the most recent occurrence. (`07_setjmp_and_signal.md` §3)
2. **Why is `<stdarg.h>` essentially impossible to implement in fully portable standard C
   alone?** It depends on per-platform knowledge of argument layout (order, alignment
   padding, growth direction) that portable C can't express. (`08_stdarg.md` §5)
3. **You're targeting a freestanding embedded C environment. Which Phase 7 material can
   you actually rely on?** Only what `<float.h>`/`<limits.h>`/`<stdarg.h>`/`<stddef.h>`
   cover — `<stdio.h>`, dynamic allocation, and `<string.h>` are hosted-only and may not
   exist. (`10_limits_float_stddef.md` §1)
