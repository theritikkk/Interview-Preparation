# `<math.h>` — Why Good Math Functions Are Hard

> **Phase 7 · The Standard Library, By Design.** Depends on: `libraries/06_assert_and_errno.md`
> (this phase — `errno`'s `EDOM`/`ERANGE` codes, which `<math.h>` is one of the two
> standard sources of), `fundamentals/02_types.md` §4 (floating-point basics).
> Sources: **Book C** ch.7 (primary — an unusually candid account of what makes this
> header hard to implement well), **Book A** ch.15 (practical function tour).

---

## 1. A library author's own confession — why this chapter took so long to write

`[Book-derived, C]` Plauger states directly that `<math.h>` took him roughly as long to
write and debug as the entire rest of the standard library combined — despite having
written math libraries several times before across twenty-plus years. This is worth
sitting with: **"just call `sqrt`" hides real, hard-won engineering.** His stated design
goals for this implementation are a genuinely good checklist for what a *quality* math
library should promise:

- Every function should accept every value in its true mathematical **domain**, and
  report a **domain error** for anything outside it (rather than silently returning
  nonsense).
- Every function should produce a finite result when one exists, and report a **range
  error** — not silently wrap or truncate — when a result is too large or too small to
  represent.
- Results should be accurate to within a couple of bits of the best representable
  approximation — not just "roughly right."
- No function should trigger a spurious intermediate overflow/underflow/divide-by-zero
  while computing a perfectly well-defined final result.

`[Book-derived, C]` He's equally candid about **non-goals** — distinguishing `+0` from
`-0`, full IEEE 754 NaN-signaling behavior — deliberately left unimplemented as not worth
the complexity for this library's purposes. `[Synthesis]` The lesson generalizes: a
well-engineered library documents not just what it does, but what it *deliberately* doesn't
do, and why — a genuinely useful habit to carry into your own API design, not just
something to admire in this one header.

---

## 2. Domain errors vs. range errors — precisely

`[Book-derived, C, connecting directly to `libraries/06_assert_and_errno.md`'s `EDOM`/`ERANGE`
discussion]`

| Error kind | When it happens | Example | Standard behavior |
|---|---|---|---|
| **Domain error** | The argument itself is mathematically invalid for the function | `sqrt(-1.0)`, `log(-5.0)` | `errno` set to `EDOM`; return value is implementation-defined (commonly a NaN-like value) |
| **Range error** | The argument is valid, but the *true result* can't be represented | `exp(1000.0)` (too large), `exp(-1000.0)` (too small — underflow) | `errno` set to `ERANGE`; `HUGE_VAL` (or its negation) returned for overflow, `0` for underflow |

`[Book-derived, C]` `HUGE_VAL` is a macro expanding to some large `double` value (not
necessarily representable as a `float` — a subtlety worth knowing if you're storing the
result into a `float` variable) — the standard, portable way a `<math.h>` function signals
"the true answer overflowed" without relying on any particular floating-point
representation's own notion of infinity.

### ⚠️ The `errno`-checking idiom this connects to
`[Cross-reference, applying `libraries/06_assert_and_errno.md` §2's general pattern
specifically here]` Because a math function's return value alone often can't distinguish
"a legitimately huge result" from "an overflow was reported," the standard idiom is: clear
`errno` to `0` **before** the call, then check it **after**:

```c
errno = 0;
double result = sqrt(x);
if (errno == EDOM) {
    /* x was negative — handle the domain error */
}
```

---

## 3. Why floating-point math functions are genuinely hard to get right

`[Book-derived, C]` A few concrete reasons, beyond "the math itself is complicated":

- **Finite precision** cuts both ways: it lets an implementation choose deliberately
  limited-accuracy approximations (faster to compute), but those same approximations can
  accumulate error across an *intermediate* calculation that needed more precision than the
  final answer does.
- **Finite range** similarly cuts both ways: it bounds how large an exponent needs to be
  represented, but a perfectly well-defined final answer can still be reached via an
  intermediate step that overflows or underflows *along the way* — one of Plauger's
  explicit design goals (§1) was avoiding exactly this spurious-intermediate-overflow trap.
- Real implementations often **decompose** a floating-point value into its sign, exponent,
  and fractional parts and operate on those pieces semi-numerically, rather than treating
  the value as one opaque number throughout — a level of care well beyond what "just call
  the CPU's hardware instruction" naively suggests, especially for functions the hardware
  doesn't compute directly at all (most of `<math.h>` beyond basic arithmetic).

### 🎯 Interview point
"Why might `sqrt(x*x)` not exactly equal `fabs(x)` for some `x`, even though they're
mathematically identical?" — Floating-point arithmetic is inherently an *approximation* of
real-number arithmetic; intermediate rounding in the multiplication and the square root can
each introduce a tiny error, and there's no guarantee those errors cancel out perfectly —
this is a direct, concrete instance of the finite-precision tradeoff in §3, not a bug in
any particular implementation.

---

## 4. ⚠️ Misconceptions to unlearn now

- **"An invalid math function call (like `sqrt(-1.0)`) crashes the program."** It doesn't,
  by default — it sets `errno` to `EDOM` and returns an implementation-defined value; the
  program keeps running unless *you* check for and act on the error (§2).
- **"Math functions in C are exact."** They're accurate *approximations*, engineered to a
  precision target (§1) — not exact real-number arithmetic, a distinction that matters for
  any code comparing floating-point results for exact equality.
- **"`HUGE_VAL` is the same as `float` infinity, or is somehow platform-specific
  gibberish."** It's a portable, standard-defined macro specifically designed to signal
  overflow without depending on any particular hardware representation of infinity (§2).

---

## 5. 🎯 Interview questions

**Easy**
- What's the difference between a domain error and a range error in `<math.h>`? → Domain:
  the argument itself is invalid (`sqrt(-1.0)`); range: the argument is valid but the true
  result can't be represented (`exp(1000.0)`) (§2).
- What does `HUGE_VAL` represent? → The standard's portable signal for "the true result
  overflowed," returned by math functions on a range error (§2).

**Medium**
- Why must you set `errno = 0` *before* calling a math function if you intend to check it
  afterward? → A math function's return value alone often can't distinguish a legitimately
  large valid result from an overflow signal; `errno` isn't cleared by successful calls, so
  checking it without first zeroing it can pick up a stale value from an earlier,
  unrelated call (§2, `libraries/06_assert_and_errno.md` §2).
- Why did the library's author consider *not* implementing full IEEE 754 signed-zero
  distinction a reasonable design choice? → It added meaningful implementation complexity
  for a distinction he judged wasn't worth the cost for this library's actual goals — a
  deliberate, documented non-goal rather than an oversight (§1).

**Hard**
- Explain why a mathematically well-defined final result can still trigger a spurious
  error if an implementation isn't careful about *intermediate* calculations. → Finite
  range/precision applies at every step of a computation, not just the final result; a
  naively-written function can overflow or lose precision partway through even when the
  true final answer is perfectly representable — avoiding this was one of Plauger's
  explicit, hard-won design goals (§1, §3).

**Systems-level**
- A numerical simulation accumulates small errors over millions of floating-point
  operations and eventually produces visibly wrong output, despite every individual
  operation "looking correct" in isolation. What general phenomenon does this reflect,
  and how does it connect to this file's material? → Accumulated floating-point rounding
  error from finite precision (§3) — each operation is only an approximation, and errors
  can compound across a long computation even though no single step is "buggy"; the
  general awareness this file builds (finite precision is a real, load-bearing constraint,
  not just theoretical trivia) is the practical takeaway for diagnosing this class of issue.

---

## 6. External references

- [cppreference — `<cmath>`/`<math.h>`](https://en.cppreference.com/w/c) `[External]` —
  current, complete function reference, including C99+ additions this file's source
  predates: `isnan`/`isinf`/`fpclassify`, the `INFINITY`/`NAN` macros, and
  `math_errhandling` (a more precise, standardized way to check for domain/range errors
  than the "clear `errno`, call, check `errno`" idiom in §2 alone).
- [IEEE 754 overview, via cppreference or your platform's floating-point documentation](https://en.cppreference.com/w/c)
  `[External]` — for the floating-point representation details §1 references only in
  passing.

## 7. 🛠 Practice

1. Write a small wrapper around `sqrt` that checks `errno` correctly (clearing it before
   the call) and reports a clear domain-error message for negative input, rather than
   letting an implementation-defined return value propagate silently.
2. Deliberately trigger a range error (`exp(1000.0)` or similar, depending on your
   platform's `double` range) and print both the returned value and `errno` afterward to
   see `HUGE_VAL` and `ERANGE` directly rather than just reading about them.
3. Compute `sqrt(x*x)` and `fabs(x)` for several values of `x` and compare — for most
   values they'll agree, but see if you can find (or reason about) a value where floating-
   point rounding makes them differ, to make §3's point concrete.
