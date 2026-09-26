# The Preprocessor: Conditional Compilation

> **Phase 2 · Functions, Scope & the Preprocessor.** Depends on: `01_macros.md`,
> `02_includes_and_header_design.md` §3 (include guards are this file's mechanism, applied).
> Feeds into: `advanced/04_modern_c_vs_historical_c.md` (Phase 11 — conditional compilation
> is how real code supports multiple C standard versions in one source tree).
> Sources: **Book B** §4.11.3 (primary), **Book A** ch.18.1.8–18.1.9 (the `#if 1`/`#if 0`
> toggle idiom).

---

## 1. `#if` / `#elif` / `#else` / `#endif`

`[Book-derived, B]` `#if` evaluates a **constant integer expression** *at preprocessing
time*; if nonzero, the lines up to the matching `#elif`/`#else`/`#endif` are kept (fed to
the compiler); otherwise they're discarded before the compiler ever sees them. `#elif` is
`#if`'s else-if counterpart; a trailing `#else` (optional) catches everything else, exactly
mirroring the language-level `if`/`else if`/`else` chain from `fundamentals/04_control_flow.md`
§3 — but operating one level earlier, on the *text*, before any C syntax is parsed.

### ⚠️ A real constraint, not a style choice
`[Book-derived, B]` The controlling expression **cannot** use `sizeof`, casts, or `enum`
constants — the preprocessor runs before the compiler has any notion of types, so it has no
way to evaluate anything that depends on type information. `[Book-derived, A]` More
precisely (Book A's phrasing): the expression may use any C operator *except* the
assignment operators, increment/decrement, address-of, and `sizeof`. This is a direct,
concrete consequence of the "preprocessor is syntax-blind" mental model established in
`fundamentals/01_program_structure.md` §3 — it's worth having this specific list in mind
the first time a `#if sizeof(int) == 4` seems like a reasonable thing to write (it isn't;
`<stdint.h>`'s `[External]` exact-width types exist partly to sidestep needing this).

### The `defined` operator

`[Book-derived, B]` `defined(name)` (parentheses optional) evaluates to `1` if `name` has
been `#define`d, `0` otherwise — the only operator that exists solely in preprocessor
expressions, nowhere else in C.

### Worked example — platform-conditional header selection

`[Book-derived, B]`

```c
#if SYSTEM == SYSV
#define HDR "sysv.h"
#elif SYSTEM == BSD
#define HDR "bsd.h"
#elif SYSTEM == MSDOS
#define HDR "msdos.h"
#else
#define HDR "default.h"
#endif
#include HDR
```

`[Synthesis]` Notice `#include HDR` — because macro expansion happens before `#include`'s
own filename is resolved, `HDR` (itself a macro, defined conditionally above) expands
first, so *which literal header gets included* is decided per-platform at preprocess time.
This exact pattern — parameterizing an `#include` target through a macro — is how
significant real-world portability layers are built.

### `#ifdef NAME` / `#ifndef NAME`

`[Book-derived, A + B]` Shorthand for the extremely common `defined`-only case:

```c
#ifdef NAME      /* equivalent to: #if defined(NAME) */
#ifndef NAME     /* equivalent to: #if !defined(NAME) */
```

---

## 2. Include guards — the canonical use case

`[Book-derived, B]` This is the pattern introduced in `02_includes_and_header_design.md`
§3, now fully explained: guarding an entire header's content with an `#ifndef`/`#define`
pair keyed to a macro unique to that header prevents the same content being pasted in twice
by nested/repeated `#include`s within one translation unit.

```c
#ifndef HDR
#define HDR
/* contents of hdr.h go here */
#endif
```

Walking through it: the **first** time this header is included in a translation unit,
`HDR` isn't yet defined, so `#ifndef HDR` succeeds — the content is included, and (crucially)
`#define HDR` runs as part of that included content, marking the header as "already seen."
**Every subsequent** inclusion (in the same translation unit, however indirectly it's
reached) finds `HDR` already defined, so `#ifndef HDR` fails and everything down to
`#endif` is skipped — pasting nothing the second time. `[Book-derived, B]` If used
consistently, this lets each header safely `#include` every other header it actually
depends on (the self-containment principle from `02_includes_and_header_design.md` §4)
without the *user* of a header having to manually resolve the resulting web of repeated,
indirect inclusions themselves.

---

## 3. Practical uses beyond include guards

`[Synthesis, organizing standard real-world patterns built from §1's primitives]`

- **Debug-only code:**
  ```c
  #ifdef DEBUG
      fprintf(stderr, "x = %d\n", x);
  #endif
  ```
  compiled in only when `DEBUG` is defined (typically via a compiler flag, `-DDEBUG`,
  `[External]`), with zero runtime cost when it isn't — unlike an `if (debug_flag)` check,
  which still costs a branch and still requires the debug code to compile at all.

- **Portability shims:** conditionally supplying a platform-specific implementation of
  something, selected by a predefined compiler/platform macro (e.g. `_WIN32`, `__linux__`
  — `[External]`, compiler-specific, not part of ISO C itself).

- **Feature/version gating:** `#if __STDC_VERSION__ >= 201112L` (`[External]`) to compile
  different code depending on which C standard the current compiler targets — the concrete
  mechanism behind `advanced/04_modern_c_vs_historical_c.md`'s (Phase 11) "how do I write
  code that works on both old and new compilers" question.

### 🛠 Debugging technique — the `#if 0` toggle

`[Book-derived, A]` A fast, reliable way to comment out a block of code that may itself
contain `/* ... */` comments (which don't nest in C, so wrapping such a block in one more
`/* */` pair breaks at the first inner `*/`):

```c
#if 0
/* this entire block, comments included, is excluded from compilation */
#endif
```

Because `#if 0`/`#endif` don't care what's inside them syntactically (the preprocessor
just skips to the matching `#endif`), this sidesteps the comment-nesting problem entirely
— a small trick that shows up constantly in real debugging sessions.

---

## 4. ⚠️ Misconceptions to unlearn now

- **"`#if` can test anything a runtime `if` can."** No — only constant integer expressions,
  and specifically *not* `sizeof`, casts, or `enum` constants, because the preprocessor has
  no type information at all (§1).
- **"Include guards and `#pragma once` are two names for the same standard feature."**
  Related in effect, not in standing: `#ifndef`/`#define`/`#endif` guards are portable ISO
  C; `#pragma once` (`02_includes_and_header_design.md` §3) is a near-universal but
  **non-standard** compiler extension.
- **"Code inside a false `#if` branch still needs to be syntactically valid C."** It does
  not — the preprocessor operates on tokens, not on parsed C, so a `#if 0` block can
  contain genuinely broken or incomplete C (this is exactly what makes the `#if 0` comment-
  out trick in §3 work even across code containing stray `/* */`).

---

## 5. 🎯 Interview questions

**Easy**
- What's the difference between `#ifdef NAME` and `#if NAME`? → `#ifdef` tests whether
  `NAME` is defined at all (regardless of its value); `#if NAME` requires `NAME` to expand
  to a valid constant integer expression, and treats an *undefined* macro as `0` (§1).
- What's the purpose of an include guard? → Preventing a header's content from being pasted
  in more than once per translation unit (§2).

**Medium**
- Why can't a `#if` expression use `sizeof`? → The preprocessor runs before the compiler
  has any type information, and `sizeof` requires knowing a type's size (§1).
- Walk through exactly what happens the *second* time a guarded header is included in the
  same file. → The guard macro is already defined from the first inclusion, so the
  `#ifndef` test fails and every line down to `#endif` — the entire header body — is
  skipped (§2).

**Hard**
- A codebase uses `#ifdef DEBUG`-gated logging throughout. What's the tradeoff versus a
  runtime `if (debug_enabled)` check, in both directions? → The preprocessor version has
  zero cost (and doesn't even need to compile) when `DEBUG` is undefined, but requires a
  recompile to toggle; the runtime version can be toggled without recompiling but always
  pays a branch (and requires the debug code to always compile cleanly) even when disabled
  (§3).

**Systems-level**
- Explain how `#include HDR` (where `HDR` is itself a macro, conditionally defined per
  platform) lets one source file target multiple platforms without `#ifdef`-riddled code
  inside the file itself. → Macro expansion resolves `HDR` to a specific header name before
  `#include` processes it, so the platform-specific branching lives in one small block at
  the top rather than scattered through the file; the rest of the code just includes
  "the platform header" generically (§1).

---

## 6. External references

- [cppreference — Conditional inclusion](https://en.cppreference.com/w/c) `[External]` —
  the full, current grammar for `#if`/`#ifdef`/`#elif`/`#else`/`#endif`.
- [GCC documentation — predefined macros](https://gcc.gnu.org/onlinedocs/) `[External]` —
  compiler- and platform-identifying macros (like `__linux__`, `_WIN32`) referenced in §3,
  beyond the ISO-standard ones already covered in `01_macros.md` §5.

## 7. 🛠 Practice

1. `[Book-derived, B]` Write your own small "platform header selector" using `#if`/`#elif`
   keyed to a macro you define yourself (rather than a real platform macro), and confirm
   with `gcc -E` (`fundamentals/01_program_structure.md` §2) that only the intended branch
   survives preprocessing.
2. Write a header, deliberately omit its include guard, force double-inclusion, and read
   the resulting error; then add a guard and confirm it's fixed — this is the include-guard
   exercise from `02_includes_and_header_design.md` §7, but now you should be able to
   explain *exactly* which preprocessor line does what as it runs.
3. Add `#ifdef DEBUG`-gated diagnostic output to a small program, and build it twice — once
   plain, once with `-DDEBUG` — confirming the compiled binaries actually differ (e.g. by
   checking whether the debug strings appear in the binary via `strings`,
   `debugging/03_static_and_binary_inspection.md`, Phase 10).
