# `<assert.h>` & `<errno.h>` — Two Small Headers, Two Real Design Stories

> **Phase 7 · The Standard Library, By Design.** Depends on: `preprocessing/01_macros.md`
> (stringification, the `do`-`while(0)` idiom, macro pitfalls — this file's `assert`
> walkthrough is a direct, advanced continuation of that material),
> `fundamentals/03_expressions_and_operators.md` §7 (the conditional operator —
> `assert`'s implementation depends on it).
> Feeds into: `libraries/05_math.md` (this phase — `EDOM`/`ERANGE` in practice),
> `systems/04_system_calls_vs_libc_vs_posix.md` (Phase 8 — `errno`'s original UNIX
> system-call context).
> Sources: **Book C** ch.1 (`<assert.h>`) and ch.3 (`<errno.h>`), both in full — genuinely
> unique material among the three books, written by someone who sat in the standardization
> meetings that designed both.

---

## 1. `<assert.h>` — one macro, four hard design constraints

`[Book-derived, C]` `assert(expression)` checks a condition you believe must always be
true; if it's false, it prints a diagnostic (source file, line number, and the failed
expression's own text) to `stderr` and aborts. Its entire *purpose* is documenting and
enforcing assumptions during development, cheaply removable for production
(`#define NDEBUG` before including `<assert.h>` switches every subsequent `assert` in that
translation unit to a no-op that generates no code at all).

`[Book-derived, C]` Writing this one macro correctly turns out to require satisfying four
real constraints simultaneously:

1. **It must be a macro, not a real function.** `[Book-derived, C]` The standard says so
   explicitly, and states that accessing an actual function version instead (e.g. via
   `#undef assert` to get at a hidden real function) is undefined behavior. Why: as a
   macro, the test is inlined directly at the call site, letting an optimizing compiler
   eliminate an assertion it can prove is always true — a real function call could never
   be optimized away this cheaply, defeating the "assertions should be nearly free to
   leave in during development" goal.
2. **It must not directly call anything from another header.** `[Book-derived, C]`
   `<assert.h>` cannot call `fprintf` or reference `stderr` directly — those names are only
   properly available once `<stdio.h>` is included, and a program using `<assert.h>` has
   no obligation to also include `<stdio.h>`. The fix: the header declares its own
   **hidden library function** (conventionally with a name no ordinary program would use)
   that does the actual printing internally, keeping `<assert.h>`'s own text free of any
   dependency on `<stdio.h>`'s names. `[Synthesis]` This is a direct, concrete case of
   `preprocessing/02_includes_and_header_design.md`'s self-containment principle, applied
   in the opposite direction: a header being careful *not* to silently assume another
   header's names are already available.
3. **It must expand to a single `void` expression, not a statement.** `[Book-derived, C]`
   Because C code can legally use `assert(...)` inside a larger expression (Book C's own
   example: `(assert(0 <= x), x < y)`, using the comma operator from
   `fundamentals/03_expressions_and_operators.md` §7), the macro cannot expand to an `if`
   statement — it must be built entirely from the **conditional operator**
   (`fundamentals/03_expressions_and_operators.md` §7), the one C construct that both
   branches and yields a value in expression position:
   ```c
   #define assert(test) ((test) ? (void)0 : _Assert(__FILE__ ":" _STR(__LINE__) " " #test))
   ```
4. **It must be reasonably efficient**, or programmers will simply avoid writing
   assertions — ruling out, for instance, a naive version that always makes a multi-
   argument function call regardless of whether the test passed.

### 🔬 Under the hood — the `__LINE__` stringification trick

`[Book-derived, C]` This is a genuinely subtle, advanced continuation of
`preprocessing/01_macros.md` §3's stringification operator, worth walking through in
full. The predefined macro `__LINE__` expands to a plain **decimal integer** (not a string)
— so `#__LINE__` doesn't do what you'd hope: the `#` stringification operator only
stringifies its own macro's *literal parameter text*, and applied directly to `__LINE__`
inside a macro that doesn't take `__LINE__` as a parameter, it would stringify the literal
six characters `__LINE__`, not whatever number it expands to. The fix requires **two**
layers of indirection:

```c
#define _STR(x)  _VAL(x)     /* expands x FIRST, then hands the result to _VAL */
#define _VAL(x)  #x           /* stringifies whatever it's actually given */
```

**Why two macros, not one:** `_STR(__LINE__)` first expands `__LINE__` to its actual
decimal number (because macro arguments are expanded *before* substitution, `[Book-derived, C]`,
*except* when the parameter is used with `#` or `##` right there in the same macro) and
*then* passes that already-expanded number to `_VAL`, which stringifies it. Collapsing
this into one macro (`#define _STR(x) #x`) and calling `_STR(__LINE__)` directly would
stringify the *text* `__LINE__` rather than its numeric expansion — exactly the trap the
two-layer indirection exists to avoid. `[Synthesis]` This is one of the most genuinely
subtle, real techniques in the whole preprocessor's toolkit, and recognizing it on sight in
other code is a good marker of real macro fluency.

### ⚠️ `<assert.h>` is the one header allowed to behave differently each time you include it

`[Book-derived, C]` Every other standard header is **idempotent** — including it multiple
times has the same effect as including it once. `<assert.h>` is a deliberate,
standard-sanctioned exception: its behavior tracks `NDEBUG`'s **current** definition status
*at each point it's included*, so a source file can legitimately do:

```c
#undef NDEBUG
#include <assert.h>   /* assertions active from here... */
/* ... */
#define NDEBUG
#include <assert.h>   /* ...to here, they're switched off */
```

`[Book-derived, C]` This works safely specifically because C allows **benign
redefinition** — redefining a macro identically to its current definition (or, for
`NDEBUG`, simply defining/undefining a control macro) isn't itself an error, a standard
provision added partly to support exactly this idiom without requiring manual
`#ifndef`-guard gymnastics around each toggle.

---

## 2. `<errno.h>` — a mechanism "nobody likes," and why it endures anyway

`[Book-derived, C]` Plauger's own words are unusually blunt: he could not recall a single
defender of `errno` across two dozen-plus standards-committee meetings, and at least one
faction wanted to discard it outright — yet it survived, and the committee even *added*
machinery (`<errno.h>` itself is the committee's own invention, not an inherited UNIX
convention) rather than removing it.

### The real history, straight from a committee member

`[Book-derived, C]` Early UNIX system calls, written in assembly, signaled failure via a
CPU condition-code flag, with a small integer error code left in a register on failure.
Porting this to C is awkward: a function's return value is naturally "the answer," but
using it *also* to signal success/failure leaves no room to report *which* specific error
occurred, without sacrificing the useful return value entirely. The pragmatic (if inelegant)
compromise early UNIX C programmers settled on: reserve one "obviously invalid" return
value (`NULL`, or `-1`, chosen per function) to mean "an error occurred," and store the
*specific* error code in a single, external, shared `int` variable named `errno` — check
it, but **only** immediately after a call you already know failed.

### Why `<errno.h>` exists as its own header at all

`[Book-derived, C]` The committee's own stated reason is almost administrative: they
wanted *every* standard function/object declared in *some* header, and gave `errno`
its own specifically "to ghettoize it" — Plauger's own word — rather than let it clutter
a more central header. Beyond that housekeeping motive, the committee formalized two
portable error codes for the whole library to share: `EDOM` (a function's argument was
outside its mathematical domain) and `ERANGE` (a result overflowed or underflowed) — a
deliberate simplification, since the space of *possible* specific error conditions across
every library function is open-ended and would be unmanageable to standardize individually.

### ⚠️ The rule that makes `errno` usable at all

`[Book-derived, C, the exact standard requirement, directly underlying
`libraries/05_math.md` §2's idiom]` **`errno` is never set to zero by any library
function on success.** It starts at `0` when the program begins, and functions that fail
set it to some positive value — but a function that *succeeds* leaves whatever value was
already sitting there **completely untouched**. This is precisely why "clear `errno` before
the call, check it after" is the only reliable usage pattern (`libraries/05_math.md` §2's
worked example) — checking `errno` without having cleared it first can pick up a stale
value left over from a completely unrelated, earlier failed call.

### 🎯 Interview point
"Why can't you just check `if (errno) { ... }` right after any library call to see if it
failed?" — Because `errno` is never *cleared* on success, a nonzero value might be a stale
leftover from an earlier, unrelated failure rather than evidence this specific call failed
— you must zero it yourself immediately before any call whose failure you intend to detect
this way, and even then, only for functions the standard actually documents as setting
`errno` (§2's `EDOM`/`ERANGE` cases, plus a short additional list from other headers).

---

## 3. ⚠️ Misconceptions to unlearn now

- **"`assert` is basically an `if` statement in disguise."** It has to be built from the
  conditional operator specifically so it can appear inside expression contexts an `if`
  statement never could (§1).
- **"`#__LINE__` correctly stringifies the current line number."** It stringifies the
  literal text `__LINE__` unless you go through the two-layer `_STR`/`_VAL` indirection
  first (§1).
- **"Checking `errno` right after any function call reliably tells you if that call
  failed."** Only if you cleared `errno` to `0` immediately beforehand, and only for
  functions actually documented to set it — `errno` is never zeroed on success by the
  library itself (§2).

---

## 4. 🎯 Interview questions

**Easy**
- What does defining `NDEBUG` before `#include <assert.h>` do? → Switches every
  subsequent `assert` in that translation unit to a no-op that generates no code (§1).
- What are `EDOM` and `ERANGE` for? → Domain errors (invalid argument) and range errors
  (overflow/underflow), the two portable error codes the library shares across functions (§2).

**Medium**
- Why must `assert` be implemented as a macro rather than a real function? → So an
  optimizing compiler can eliminate the check entirely when it can prove the condition
  always holds — impossible to do as cheaply with a genuine function call (§1).
- Why can't `<assert.h>`'s implementation call `fprintf` directly? → `fprintf`/`stderr`
  are only properly available once `<stdio.h>` is included, which a program using
  `<assert.h>` isn't obligated to do — the header uses its own hidden internal function
  instead (§1).

**Hard**
- Walk through why `#define _STR(x) #x` alone, called as `_STR(__LINE__)`, doesn't give
  you the line number as a string — and why the two-macro `_STR`/`_VAL` version does. →
  `#` stringifies its own macro's literal parameter text without first expanding it (except
  via the two-layer trick); one macro alone stringifies the text `__LINE__` itself, while
  `_STR(x) = _VAL(x)` first lets `__LINE__` expand to its actual number as an ordinary
  argument, *then* passes that expanded number into `_VAL` to be stringified (§1).
- `<assert.h>` is explicitly allowed to behave differently each time it's included in the
  same file — every other standard header must be idempotent. Why is this exception safe?
  → It's tied to C's "benign redefinition" allowance for macros, and the header's own
  design deliberately keys its expansion to `NDEBUG`'s *current* status at each inclusion
  point, a documented, intentional exception rather than an oversight (§1).

**Systems-level**
- A function you're calling is documented to set `errno` on failure. You check
  `if (errno != 0)` immediately after calling it, without having cleared `errno`
  beforehand, and get a false positive. What happened? → `errno` was already nonzero from
  an earlier, unrelated failed call — since it's never cleared on success, that stale value
  persisted until your check, which had no way to distinguish it from a fresh failure from
  the call you actually cared about (§2).

---

## 5. External references

- [cppreference — `<cassert>`/`<assert.h>`](https://en.cppreference.com/w/c) `[External]`
  — current reference, including `static_assert` (C11), a compile-time sibling this book
  predates.
- [cppreference — `<cerrno>`/`<errno.h>`](https://en.cppreference.com/w/c) `[External]` —
  the current, complete list of standard `E`-prefixed error codes beyond `EDOM`/`ERANGE`.
- [man7.org — `errno(3)`](https://man7.org/linux/man-pages/) `[External]` — the much
  larger, POSIX/Linux-specific set of `errno` values system calls actually use in
  practice, previewed here ahead of `systems/`, Phase 8.

## 6. 🛠 Practice

1. `[Book-derived, C]` Write your own minimal `assert.h` from scratch, including the
   `_STR`/`_VAL` stringification trick, and confirm `__FILE__`/`__LINE__`/the failed
   expression's text all appear correctly in your failure message.
2. Toggle `NDEBUG` on and off around different assertions in the same file (per §1's
   example), and use `gcc -E` (`fundamentals/01_program_structure.md` §2) to confirm
   exactly which assertions expand to real checks and which vanish entirely.
3. Write a function that calls a `<math.h>` function likely to fail for some inputs,
   correctly clearing and checking `errno` around it (per §2 and `libraries/05_math.md`
   §2's idiom) — then write a *broken* version that checks `errno` without clearing it
   first, and construct an input sequence that makes the broken version report a false
   error.
