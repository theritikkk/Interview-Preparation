# `<limits.h>`, `<float.h>` & `<stddef.h>` — The Standard's Own Vocabulary

> **Phase 7 · The Standard Library, By Design.** Depends on: `fundamentals/02_types.md`
> §1–2 (type sizes and `char` signedness — this file supplies the exact, queryable
> constants for facts that file only described qualitatively),
> `memory/07_alignment_padding_aliasing.md` §3 (`offsetof` — formalized here).
> Sources: **Book C** ch.4 (`<float.h>`), ch.5 (`<limits.h>`), ch.11 (`<stddef.h>`), all
> three backgrounds in full — including a genuinely important, standards-history concept
> ("freestanding" vs. "hosted" environments) neither other book mentions at all.

---

## 1. `<stddef.h>` and a distinction worth knowing: freestanding vs. hosted

`[Book-derived, C]` This is arguably the most conceptually important fact in this whole
file, and it belongs first. The C standard defines **two** kinds of conforming
implementation:

- A **hosted** environment provides the *entire* standard library — this is what every file
  in this vault has assumed so far, and what a normal desktop/server program targets.
- A **freestanding** environment need only support the C *language* itself, plus exactly
  **four** standard headers: `<float.h>`, `<limits.h>`, `<stdarg.h>`, and `<stddef.h>`.
  `[Synthesis]` This is precisely the environment an **operating system kernel**, a
  **bootloader**, or deeply resource-constrained **embedded firmware** typically targets —
  there's no underlying OS to provide `malloc`'s heap, `<stdio.h>`'s file streams, or most
  of what this vault's Phase 7 otherwise covers, because the code being written often *is*
  (or runs before) the OS. `<stddef.h>` exists specifically to hold whatever generically
  useful types/macros (`size_t`, `ptrdiff_t`, `NULL`, `offsetof`) a freestanding program
  might still need, since the other three "required" headers each have a narrower, specific
  focus (integer limits, floating-point limits, variadic-argument access).

### 🎯 Interview point
"What's the minimum a C compiler needs to support to be standard-conforming at all?" — Far
less than most programmers assume: the language proper, plus only four headers
(`<float.h>`/`<limits.h>`/`<stdarg.h>`/`<stddef.h>`) — everything else, including `malloc`,
`printf`, and `<string.h>`, is part of the *hosted*-environment requirement, not a baseline
every C compiler must provide.

---

## 2. `<stddef.h>`'s actual contents

`[Book-derived, C]`

| Name | What it is |
|---|---|
| `size_t` | Unsigned integer type, the result type of `sizeof` (`fundamentals/02_types.md` §3) |
| `ptrdiff_t` | Signed integer type, the result of subtracting two pointers into the same array (`memory/02_pointers.md` §4) |
| `wchar_t` | An integer type wide enough to hold any character from any locale's extended character set — largely superseded in modern practice by more precise Unicode-aware types, `[External]` |
| `NULL` | The null-pointer constant — implemented as `0`, `0L`, or `(void *)0`, whichever suits the platform |
| `offsetof(type, member)` | A member's byte offset within a struct, padding included — already covered in full in `memory/07_alignment_padding_aliasing.md` §3 |

### 💡 "A portable way to perform a nonportable operation"
`[Book-derived, C]` Plauger's own framing of `offsetof` generalizes nicely to this whole
header: none of `<stddef.h>`'s contents can be written in fully portable C by an ordinary
programmer (computing a genuinely correct `offsetof` without compiler-specific knowledge is
close to impossible, for instance) — but the *compiler vendor*, with intimate knowledge of
its own translator, can implement each one reliably. `<stddef.h>` exists to package that
vendor-specific necessity behind a **standard, portable name** you can rely on without
needing that inside knowledge yourself.

---

## 3. `<limits.h>` — a standard born before the standard existed

`[Book-derived, C]` A genuinely interesting bit of real history: `<limits.h>` predates
ANSI C entirely — it originated with **`/usr/group`** (later UniForum), an industry
association that began standardizing UNIX-compatible C environments back in 1980, well
before X3J11 (the ANSI C committee) existed. That earlier work saved the ANSI committee
substantial effort, and — in a nice bit of standards-history trivia — the same `/usr/group`
effort also fed directly into **POSIX** (IEEE 1003.1), meaning `<limits.h>` and POSIX share
a common ancestor.

### The constants, and the `char` connection back to Phase 1

`[Book-derived, C]` `<limits.h>` gives you the exact, queryable minimums
`fundamentals/02_types.md` §1 already described qualitatively: `CHAR_BIT`, `SCHAR_MIN`/
`MAX`, `UCHAR_MAX`, `SHRT_MIN`/`MAX`, `INT_MIN`/`MAX`, `LONG_MIN`/`MAX`, and their unsigned
counterparts. **`CHAR_MIN`/`CHAR_MAX` are defined in terms of whichever signedness plain
`char` actually has on a given platform** — if plain `char` behaves as signed, `CHAR_MIN`
equals `SCHAR_MIN`; otherwise `CHAR_MIN` is `0` and `CHAR_MAX` equals `UCHAR_MAX` — a
direct, queryable answer to the implementation-defined signedness question
`fundamentals/02_types.md` §2 raised without a way to check it in code. You can write
`#if CHAR_MIN < 0` to test plain `char`'s signedness portably, right in your own source.

### The "adapting types" pattern

`[Book-derived, C]`

```c
#include <limits.h>

#if VAL_MIN < INT_MIN || INT_MAX < VAL_MAX
typedef long Val_t;    /* int isn't wide enough on this platform — use long instead */
#else
typedef int Val_t;      /* int suffices here — use it, for efficiency */
#endif
```

`[Synthesis]` This is a genuinely useful, real technique: rather than hardcoding a "safe"
wide type everywhere (wasting space/performance on platforms where a narrower type would
do) or hardcoding a narrow type that silently breaks on a platform where it's too small,
`<limits.h>` lets you choose the *narrowest sufficient* type **per platform, automatically**,
resolved entirely at compile time.

### ⚠️ A dead technique `<limits.h>` was specifically created to kill

`[Book-derived, C]` Before `<limits.h>` existed, some programs tried to probe integer
widths via preprocessor arithmetic tricks like `#if (-1 + 0x0) >> 1 > 0x7fff`, hoping the
preprocessor's own arithmetic would reveal the *execution* environment's integer sizes.
This is fragile: nothing guarantees the preprocessor computes `#if` expressions using the
same widths the compiled program will actually use. `<limits.h>`'s named constants exist
specifically so no one needs this fragile trick again.

---

## 4. `<float.h>` — the floating-point analog

`[Book-derived, C]` Parallel to `<limits.h>`, but for `float`/`double`/`long double`:
`FLT_MIN`/`MAX`/`DBL_MIN`/`MAX`/etc. for range, `FLT_DIG`/`DBL_DIG` for decimal digits of
precision, and — worth a dedicated, precise explanation — the `*_EPSILON` constants.

### `FLT_EPSILON`/`DBL_EPSILON`, defined precisely

`[Book-derived, C, the exact standard definition]` `FLT_EPSILON` is **the difference
between 1 and the smallest representable value greater than 1**, for that floating-point
type. This is *the* concrete, standard-given number behind the extremely common piece of
advice "never compare floating-point values with `==`" — `[Synthesis]` since rounding error
from any nontrivial floating-point computation is routinely larger than the *gaps between
adjacent representable values* near the numbers you're working with, an exact equality
test is testing something far stricter than "are these two computed values close enough to
be considered the same result." The standard idiom this constant enables:

```c
if (fabs(a - b) < DBL_EPSILON * some_reasonable_scale_factor)
    /* "close enough" */
```

`[External]` Choosing the right scale factor (since `DBL_EPSILON` alone is only meaningful
relative to a comparison near `1.0`, not near very large or very small numbers) is itself a
genuinely nuanced numerical-methods topic, outside this vault's core scope — the concrete
takeaway worth keeping is narrower: `*_EPSILON` gives you the *precise, standard-defined
number* behind "floating-point precision is finite," rather than a vague gesture at the
idea.

---

## 5. ⚠️ Misconceptions to unlearn now

- **"Every C compiler must support `malloc`, `printf`, and the full standard library."**
  Only *hosted* implementations must; a conforming *freestanding* implementation (kernels,
  bootloaders, embedded firmware) needs only the language plus four headers (§1).
- **"You can't portably tell whether plain `char` is signed on a given platform."** You
  can, precisely: `#if CHAR_MIN < 0` (§3).
- **"Comparing floats with `==` is just a style nitpick."** It's testing exact bit-pattern
  equality, far stricter than "close enough after rounding error" — `*_EPSILON` (§4) is the
  standard's own precise measure of exactly how fine that gap can be.

---

## 6. 🎯 Interview questions

**Easy**
- What four headers must even a minimal, freestanding C implementation support? →
  `<float.h>`, `<limits.h>`, `<stdarg.h>`, `<stddef.h>` (§1).
- What does `FLT_EPSILON` represent, precisely? → The difference between 1 and the
  smallest representable value greater than 1, for `float` (§4).

**Medium**
- How can you portably test whether plain `char` is signed on a given platform, without
  writing platform-specific code? → `#if CHAR_MIN < 0` (§3).
- Why does `<limits.h>`'s "adapting types" pattern (choosing `int` vs. `long` via `#if`)
  matter in practice? → It picks the narrowest sufficient type per platform automatically,
  avoiding both wasted space/performance (always using a wide type) and silent overflow
  (always using a narrow type that might not fit on some platform) (§3).

**Hard**
- Why does a "freestanding" C environment only require four specific headers, and what do
  they have in common? → They're the headers usable without assuming an underlying
  operating system — pure language-level facts (integer/float limits, variadic-argument
  access, and `<stddef.h>`'s catch-all definitions) rather than OS-mediated services like
  file I/O or dynamic memory (§1).
- Why is `fabs(a - b) < DBL_EPSILON` alone often *not* sufficient for a "close enough"
  floating-point comparison? → `DBL_EPSILON` measures the representable gap near `1.0`
  specifically; the actual gap between representable values scales with a number's
  magnitude, so a fixed epsilon is too strict for very large numbers and too loose for very
  small ones without an appropriate scale factor (§4).

**Systems-level**
- You're writing code intended to compile under a freestanding embedded C environment.
  Which of this vault's Phase 7 material can you actually rely on being present? → Only
  what's covered by `<float.h>`, `<limits.h>`, `<stdarg.h>`, and `<stddef.h>` — `<stdio.h>`,
  `<stdlib.h>`'s allocation functions, `<string.h>`, and the rest of this phase's headers
  are hosted-environment-only and may simply not exist (§1).

---

## 7. External references

- [cppreference — `<climits>`/`<limits.h>`](https://en.cppreference.com/w/c) `[External]`
  — current, complete constant reference.
- [cppreference — `<cfloat>`/`<float.h>`](https://en.cppreference.com/w/c) `[External]` —
  including C99+ additions this book's era predates.
- [cppreference — `<cstddef>`/`<stddef.h>`](https://en.cppreference.com/w/c) `[External]`
  — including `max_align_t` (C11), added after this book's era.
- [cppreference — Freestanding and hosted implementations](https://en.cppreference.com/w/c)
  `[External]` — the current, precise definition of this file's §1 distinction.

## 8. 🛠 Practice

1. Write `#if CHAR_MIN < 0` / `#else` / `#endif` and confirm, via a compiled message or
   `#error`, whether plain `char` is signed on your own platform — then compare against
   what you already know from `fundamentals/02_types.md` §2's discussion.
2. Implement the "adapting types" pattern from §3 for a value range you choose, and confirm
   (via `sizeof`) that it actually selects `int` or `long` correctly on your platform.
3. Write a small floating-point accumulation loop (repeatedly adding `0.1`, say) and
   compare the result against the "expected" value with both `==` and an
   epsilon-based comparison — observe the `==` comparison fail despite the values being
   "obviously" equal mathematically, making §4's warning concrete.
