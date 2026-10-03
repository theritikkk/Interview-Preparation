# `<stdlib.h>` — The Hodgepodge Header

> **Phase 7 · The Standard Library, By Design.** Depends on: `memory/05_dynamic_memory.md`
> (Phase 6 — the storage-allocation group of this header is covered there in full and only
> cross-referenced here), `memory/02_pointers.md` §7 (`void *`),
> `memory/03_pointer_declarations_parsing.md` §5 (function pointers — `qsort`/`bsearch`'s
> mechanism).
> Feeds into: `libraries/03_string.md` (this phase), `advanced/01_undefined_behavior.md`
> (Phase 11 — `abs(INT_MIN)`).
> Sources: **Book C** ch.13 (primary), **Book B** Appendix B.5 (quick reference).

---

## 1. Why this header is a "hodgepodge" — straight from its designer

`[Book-derived, C]` Plauger is direct about this: `<stdlib.h>` isn't a coherent, designed-
from-scratch API — it's where the committee put every function and macro that needed *a*
standard header but didn't fit any more specific one. He organizes it into six loosely
related groups, which this vault follows: **storage allocation** (`memory/05_dynamic_memory.md`,
Phase 6 — not repeated here), **integer math**, **algorithms**, **text conversions**,
**multibyte conversions** (out of this vault's core scope — brief mention only), and
**environmental interactions**.

---

## 2. Integer math: `abs`, `div`, `labs`, `ldiv`

`[Book-derived, C]` `abs`/`labs` compute absolute value for `int`/`long`. `div`/`ldiv`
compute quotient *and* remainder in one call, returning a small struct (`div_t`/`ldiv_t`)
— `[Book-derived, C]` this exists because many CPUs have a single divide instruction that
produces both values simultaneously; calling `/` and `%` separately in your own code may
force the compiler to compute the division twice unless it's smart enough to notice the
redundancy, so `div` gives you a portable way to ask for the one-instruction version
directly.

### ⚠️ The `abs(INT_MIN)` trap
`[Book-derived, C, directly citing the exact overflow warning from `fundamentals/02_types.md`
§9]` On a two's-complement machine, `abs(INT_MIN)` **overflows** — there is no positive
`int` representation of `-INT_MIN`, the same asymmetry that broke the naive `printd`/`itoa`
negation in `functions/02_recursion.md` §2. This is a real, recurring pattern worth
internalizing as a category, not just memorizing once: **any code that negates a signed
integer without first checking it isn't the type's minimum value carries this exact bug.**

### 🎯 Interview point
"Why might a hand-written `x < 0 ? -x : x` be worse than calling `abs(x)`?" — Beyond
readability, `abs` sidesteps double-evaluating `x` if it were an expression with side
effects (echoing `preprocessing/01_macros.md` §2's macro double-evaluation trap — though
`abs` is a real function here, not a macro, the underlying hazard of "hand-rolling"
something the library already provides is the same category of risk), and — per Book C —
many compilers generate special-cased inline code for `abs` that's both smaller and faster
than the equivalent conditional expression.

---

## 3. Algorithms: `qsort` and `bsearch` — generic via function pointers

`[Book-derived, C, directly building on `memory/03_pointer_declarations_parsing.md` §5's
callback pattern preview]` Both functions are type-generic: they operate on a `void *base`
array of `size`-byte elements and take a **comparator function pointer** to decide
ordering — exactly the mechanism previewed (with this exact function) back in Phase 3.

```c
void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *));
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *));
```

**Why `void *` and an explicit `size`, instead of a typed array:** the same reasoning as
`malloc`'s `void *` return (`memory/05_dynamic_memory.md` §2) — one implementation serves
every element type, at the cost of the caller supplying the comparator that actually knows
how to interpret two `const void *` elements as their real type (typically via a cast
inside the comparator).

### 🔬 Under the hood — a genuinely clever recursion-depth bound

`[Book-derived, C]` Book C's `qsort` is Quicksort (Hoare's algorithm): pick a pivot,
partition the array around it, then recursively sort each side. The choice of pivot is
explicitly called out as debatable — picking the first or last element degrades badly on
already-sorted or reverse-sorted input; Plauger picks the last element as a reasonable
default. The genuinely clever part: **the implementation recurses on the *smaller*
partition and loops (iterates) on the *larger* one**, rather than recursing on both.
`[Synthesis, connecting directly to `functions/02_recursion.md` §6's stack-overflow
discussion]` Since the recursive call always handles a partition **at most half the size**
of the one just processed, sorting `N` elements this way needs recursion no deeper than
`log₂(N)` calls — sorting a million elements needs at most about 20 nested calls,
regardless of how poorly the pivot choice happens to perform on a given input. This is a
direct, practical answer to "how do you keep a recursive algorithm's stack usage bounded
even in a bad case" — a real technique worth recognizing, not just this one function's
trivia.

### `bsearch`, and the echo of Phase 4's own binary search

`[Book-derived, C]` `bsearch` performs the same halving-search logic as the `binsearch`
worked example already covered in `fundamentals/04_control_flow.md` §2 and reused in
`memory/06_struct_union_layout.md` §4 — Plauger's own assessment: "the logic is simple but
easy to get wrong," echoing exactly the off-by-one and pointer-midpoint care those earlier
files already flagged. `bsearch` **requires** its input already sorted by the same
comparator being passed in — passing an unsorted array produces unspecified, not merely
"slow," results.

---

## 4. `rand`/`srand`, briefly

`[Book-derived, C]` `rand()` generates a pseudo-random sequence using an algorithm the
standard itself suggests; `srand(seed)` re-seeds it. Book C flags a real, easy-to-miss
virtue: **reproducibility is often as valuable as randomness** — seeding with a known,
fixed value during testing lets you reproduce a specific "random" sequence on demand, which
genuine unpredictability (e.g. hardware entropy sources, `[External]`, out of scope here)
would prevent. The idiomatic scaling pattern to a `[0.0, 1.0)` float range:
`(float) rand() / RAND_MAX` (`RAND_MAX` is guaranteed at least `32767`, per the standard —
`[Book-derived, C]`).

### ⚠️ Common bug — modulo bias
`[External]` A very common (if slightly beyond this vault's book-derived core) pattern,
`rand() % n` to get a value in `[0, n)`, is subtly biased toward smaller values whenever
`n` doesn't evenly divide `RAND_MAX + 1` — worth knowing exists, even though the fix (a
rejection-sampling loop, or a better generator) is outside this file's scope.

---

## 5. Text conversions: `atoi`/`atof`/`atol` vs. `strtol`/`strtod`

`[Book-derived, C]` `atoi(s)` is explicitly documented as equivalent to
`(int) strtol(s, NULL, 10)`; `atof`/`atol` are analogous convenience wrappers. **The
catch, stated directly by Plauger:** `atoi`/`atof`/`atol` give you **no way to detect a
conversion error** — an unparseable string and a string that legitimately means `0` are
indistinguishable through their return value alone, and the standard doesn't obligate them
to report anything through `errno` either. `strtol`/`strtod` (via their `endptr` output
parameter) tell you exactly how much of the string was actually consumed, which is the only
reliable way to detect "this wasn't a valid number at all" versus "this was a valid number
followed by other text."

### 🎯 Interview point
"When would you use `atoi` over `strtol`?" — Essentially only when you already trust the
input completely (e.g., a hardcoded string, or input already validated elsewhere) — for
anything derived from user input, file content, or network data, `strtol`'s error-detection
capability makes it the correct default, not `atoi`.

---

## 6. Environmental interactions: `exit`, `atexit`, `abort`, `getenv`, `system`

`[Book-derived, C]`
- **`exit(status)`** terminates the program, first flushing and closing all open streams
  (`libraries/01_stdio.md` §4's buffering discussion is exactly what "flushing" means here)
  — use `EXIT_SUCCESS`/`EXIT_FAILURE` (or `0`, historically also success) rather than an
  arbitrary integer, since other values' meanings vary by platform.
- **`atexit(func)`** registers a function to run automatically when the program terminates
  normally (via `return` from `main`, or an explicit `exit` call) — **registered functions
  run in reverse order of registration** (last registered, first run), and only *after*
  every registered function has run does the library flush streams and remove temporary
  files. A real, practical use Book C gives directly: register a cleanup function that
  deletes temporary files the program created, so it runs automatically no matter which
  code path actually calls `exit`.
- **`abort()`** is explicitly for "things have gone terribly wrong" — it does *not*
  guarantee streams are flushed or files closed properly; prefer `exit(EXIT_FAILURE)`
  whenever an orderly shutdown is possible at all.
- **`getenv(name)`** reads an environment variable's value (or `NULL` if unset) — the
  program's read-only view into the shell/parent-process environment it was launched from.
- **`system(command)`** hands a string to the host environment's command processor
  (typically a shell) to execute.

### ⚠️ `system()` and untrusted input
`[External]` Passing any string influenced by untrusted input (user input, network data,
file content from an untrusted source) to `system()` is a classic, serious command-
injection vulnerability — the string is interpreted by a real shell, with all of a shell's
usual metacharacter/expansion behavior. Neither source book frames this as a security
issue explicitly, but it's important enough to flag plainly: **treat `system()` as
dangerous by default whenever its argument isn't a fixed, hardcoded string.**

---

## 7. ⚠️ Misconceptions to unlearn now

- **"`atoi` reports errors somehow."** It doesn't — an invalid string and a string meaning
  `0` are indistinguishable through its return value; use `strtol` when error detection
  matters (§5).
- **"`abs(x)` always works correctly for any `int x`."** It overflows for `INT_MIN` on a
  two's-complement machine, exactly like the negation bug in `printd`/`itoa` (§2).
- **"`atexit`-registered functions run in the order they were registered."** They run in
  **reverse** order — last registered, first run (§6).

---

## 8. 🎯 Interview questions

**Easy**
- What does `div()` return, and why does it exist instead of just using `/` and `%`
  separately? → A struct with both quotient and remainder; many CPUs compute both in one
  instruction, so `div` can be a genuinely cheaper single operation (§2).
- In what order do `atexit`-registered functions run? → Reverse registration order (§6).

**Medium**
- Why do `qsort` and `bsearch` take a function-pointer comparator instead of assuming a
  fixed ordering? → Type-generic design — the same implementation works for any element
  type, as long as the caller supplies the comparison logic for that type (§3).
- Why is `strtol` generally preferable to `atoi` for parsing untrusted input? → `strtol`
  reports exactly how much input was consumed (via `endptr`), letting you detect invalid
  input; `atoi` gives no way to distinguish "invalid" from "legitimately zero" (§5).

**Hard**
- Explain why Book C's `qsort` recurses on the smaller partition and loops on the larger
  one, and what guarantee this provides. → Since the recursive call always handles at most
  half the previous partition's size, recursion depth is bounded to `log₂(N)` regardless of
  pivot-choice quality on a given input — bounding stack usage even in otherwise-bad cases
  (§3).
- Why is `abs(INT_MIN)` undefined behavior on a typical two's-complement machine? → There's
  no positive `int` representation of `INT_MIN`'s magnitude — negating it overflows, the
  same asymmetry underlying the `printd`/`itoa` bug from `functions/02_recursion.md` §2 (§2).

**Systems-level**
- A function reads a filename from `getenv("CONFIG_PATH")` and passes it directly to
  `system("cat " )` concatenated with that filename to display it. What's wrong? → This is
  a command-injection vulnerability — an environment variable (which, depending on how the
  process was launched, may not be as trustworthy as assumed) is passed unsanitized into a
  real shell invocation; a value like `; rm -rf ~` would be interpreted by the shell, not
  treated as a literal filename (§6).

---

## 9. External references

- [cppreference — `<cstdlib>`/`<stdlib.h>`](https://en.cppreference.com/w/c) `[External]` —
  current, complete reference for every function in this file.
- [CERT C — `ENV` (environment) and `MSC` (miscellaneous) rules](https://wiki.sei.cmu.edu/confluence/display/c) `[External]`
  — including the `system()` injection risk flagged in §6, treated systematically.

## 10. 🛠 Practice

1. Implement your own `qsort`-alike (`my_qsort`) using the smaller-recurse/larger-loop
   technique from §3, and confirm its recursion depth stays bounded even on adversarial
   (already-sorted, reverse-sorted) input by counting recursive calls.
2. Write a small config-file reader using `getenv` to allow environment-variable overrides
   of hardcoded defaults — a common, legitimate real-world pattern.
3. Compare `atoi("42abc")`, `atoi("abc")`, and `strtol("42abc", &endptr, 10)` /
   `strtol("abc", &endptr, 10)` side by side, printing `endptr`'s position each time, to
   make §5's error-detection gap concrete rather than theoretical.
