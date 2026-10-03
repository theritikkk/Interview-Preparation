# `<ctype.h>` & `<locale.h>` — Character Classification, and Why It's Locale-Aware

> **Phase 7 · The Standard Library, By Design.** Depends on: `fundamentals/02_types.md`
> §2 (plain `char` signedness — this file's central warning depends on it), §3 (`getchar`
> returning `int`).
> Feeds into: `libraries/05_math.md` (this phase — the domain-error pattern reappears
> there).
> Sources: **Book C** ch.2 (`<ctype.h>`, primary) and ch.6 (`<locale.h>`, background only
> — full locale machinery is out of this vault's core scope).

---

## 1. Why these functions exist at all — a real history, not just an API

`[Book-derived, C]` Early C/UNIX programmers hand-rolled character tests constantly:

```c
if ('A' <= c && c <= 'Z' || 'a' <= c && c <= 'z')   /* "is a letter" */
```

`[Book-derived, C]` This idiom has a real, non-obvious flaw: **it only works on ASCII** —
on EBCDIC (a real, once-common character encoding on some IBM systems), the letters aren't
laid out as one contiguous range this way, so the same test silently gives wrong answers.
Beyond portability, the pattern was also simply repeated inconsistently across a growing
codebase, with genuine disagreement even about what belongs in a category like
"whitespace" (vertical tabs? form feeds?). The standard's classification functions
(`isalpha`, `isdigit`, `isspace`, …) exist to answer these questions **once**, correctly,
portably, and consistently — not as a stylistic nicety over hand-rolled range checks.

### 🔬 Under the hood — the bitmask lookup-table trick
`[Book-derived, C]` Because these functions are called extremely often (Book C's own
estimate: roughly three calls per input character in a typical text-processing program),
real implementations avoid genuine function-call overhead almost identically to
`libraries/01_stdio.md` §3's `getchar` macro trick — but with a different mechanism suited
to a *classification* question rather than a *stream position* question:

```c
#define _DIGIT_MASK 0x04
#define isdigit(c)  (_ctype_table[(c)] & _DIGIT_MASK)
```

A single **translation table**, one entry per possible character value, packs each
character's properties as bits in that entry; testing a class becomes one array index plus
one bitwise AND — no branching, no function call. This is the same "precompute, then do a
cheap lookup at run time" idea you'll see again in other performance-sensitive library
internals; worth recognizing as a general technique, not just `<ctype.h>` trivia.

---

## 2. ⚠️ The single most important warning in this file

`[Book-derived, C, the exact standard requirement]` Every function in this header requires
its `int` argument to be **either representable as `unsigned char`, or equal to `EOF`** —
*any other value is undefined behavior*, full stop, not "probably fine."

**Why this bites people, concretely:** `[Synthesis, tying directly to
`fundamentals/02_types.md` §2]` if plain `char` is signed on your platform (implementation-
defined, per that section) and you pass a `char` holding a byte value ≥ 128 directly —
`isalpha(c)` for some `char c` with its sign bit set — `c` is a **negative** `int` once
promoted, which is neither a valid `unsigned char` value nor `EOF`. The lookup-table
implementation in §1 will index the table with a negative subscript — reading memory
*before* the table's start, undefined behavior with real, observable consequences
(`memory/02_pointers.md` §4's "same array only" pointer-arithmetic rule, violated).

**The fix, stated as a fixed habit:** `isalpha((unsigned char) c)` — cast explicitly before
calling, *unless* you already know the value came from `getchar()`/`fgetc()`/similar
(which already return either a valid `unsigned char` value or `EOF`, exactly this header's
required domain, by design — `libraries/01_stdio.md`'s functions and `<ctype.h>`'s
functions were clearly designed to compose correctly together).

### 🎯 Interview point
"Why does `isalpha` take an `int`, not a `char`?" — Precisely so it can accept the full
`unsigned char` range *plus* the distinct sentinel `EOF`, mirroring `libraries/01_stdio.md`'s
`getchar`/`fgetc` design (`fundamentals/02_types.md` §3) — the same underlying reason, now
seen from the *consumer* side of that design instead of the *producer* side.

---

## 3. What "locale" means here, briefly

`[Book-derived, C]` A C program begins execution in the standard **`"C"` locale** — a
fixed, minimal, portable baseline (roughly: classic 7-bit ASCII-oriented behavior). Calling
`setlocale` can switch to a different, implementation-defined locale (commonly tied to a
user's language/region settings), which can change what these classification functions
consider, e.g., a "letter" (accented characters in various European locales, for instance)
or how `<stdio.h>`'s numeric formatting behaves (decimal-point character, digit grouping).
`[Synthesis]` This is exactly why §1's undefined-behavior warning matters even *more* in
locale-aware code: an implementation's real lookup table can legitimately be larger or
differently populated once locale-dependent behavior is in play, but the `unsigned char`-
or-`EOF` domain requirement never relaxes.

`[Synthesis]` Full locale machinery (categories like `LC_NUMERIC`/`LC_COLLATE`, the
`struct lconv` returned by `localeconv`) is genuinely niche for most application code and
is intentionally out of this vault's core scope — the takeaway worth keeping is narrower
and more load-bearing: **`<ctype.h>`'s behavior is not fixed for all time in a running
program** if `setlocale` is ever called, which matters if you're debugging classification
behavior that seems to differ between environments or across a `setlocale` call.

---

## 4. ⚠️ Misconceptions to unlearn now

- **"`'A' <= c && c <= 'Z'` is a safe, portable way to test for a letter."** It's an
  ASCII-specific idiom that silently misbehaves on non-ASCII execution character sets —
  use `isalpha` (§1).
- **"Any `char` value is safe to pass to `isalpha`/`isdigit`/etc."** Only values
  representable as `unsigned char`, or `EOF`, are — a signed `char` with its sign bit set
  is undefined behavior as an argument (§2).
- **"`<ctype.h>`'s behavior never changes during a program's run."** It can, if
  `setlocale` is called to switch locales (§3).

---

## 5. 🎯 Interview questions

**Easy**
- Why shouldn't you hand-roll `'A' <= c && c <= 'Z'` instead of calling `isalpha`? → It's
  ASCII-specific and silently wrong on other execution character sets (§1).
- What locale does a C program start in? → The `"C"` locale — a fixed, portable, minimal
  baseline (§3).

**Medium**
- What exact domain does the C standard require for `<ctype.h>` function arguments? →
  Representable as `unsigned char`, or equal to `EOF` — anything else is undefined
  behavior (§2).
- Why is `isalpha((unsigned char) c)` the correct defensive habit for a plain `char c`? →
  If plain `char` is signed and `c`'s sign bit is set, the unconverted value falls outside
  the required domain once promoted to `int`; the cast forces it into the valid
  `unsigned char` range first (§2).

**Hard**
- Explain, mechanically, why passing a negative `char` value directly to a lookup-table-
  based `isalpha` implementation is genuinely dangerous, not just "technically incorrect."
  → A negative `int` argument produces a negative table index, causing an out-of-bounds
  read before the table's start — real memory access outside the intended array, not a
  merely theoretical violation (§1–2).

**Systems-level**
- A text-processing program behaves correctly in testing but misclassifies certain bytes
  (specifically ones ≥ 128) when deployed on a different platform. What's the first thing
  worth checking? → Whether plain `char` is signed on the new platform and whether the
  code passes raw `char` values into `<ctype.h>` functions without an explicit
  `unsigned char` cast — exactly this file's central warning (§2).

---

## 6. External references

- [cppreference — `<cctype>`/`<ctype.h>`](https://en.cppreference.com/w/c) `[External]` —
  current, complete function reference, with the exact undefined-behavior domain
  requirement stated per function.
- [cppreference — `<clocale>`/`<locale.h>`](https://en.cppreference.com/w/c) `[External]`
  — for the fuller locale machinery this file only summarizes.

## 7. 🛠 Practice

1. Write a small program that reads bytes from a file via `getchar()` and classifies them
   with `isalpha`/`isdigit`/`isspace` — confirm it works correctly as written (since
   `getchar()`'s return value already satisfies §2's domain requirement).
2. Deliberately introduce §2's bug: read bytes into a plain `char` array first, then pass
   array elements directly (without a cast) to `isalpha` on a platform where plain `char`
   is signed, using a byte value ≥ 128 — observe what happens, then fix it with an explicit
   `(unsigned char)` cast.
3. Implement your own tiny `my_isdigit`/`my_isalpha` using the bitmask lookup-table
   technique from §1, sized for the `"C"` locale's ASCII range only, to internalize the
   mechanism directly rather than just reading about it.
