# Interview Questions — Undefined Behavior, Portability & Modern C

> Consolidated from `advanced/`. **Phase 11 is now complete**: UB catalogue,
> implementation-defined behavior/portability, aliasing/optimization, modern C.

---

## Easy

1. **What's the difference between unspecified and undefined behavior?** Unspecified
   guarantees one of several defined outcomes; undefined guarantees nothing at all.
   (`01_undefined_behavior.md` §1)
2. **Name one arithmetic operation that's undefined behavior in C.** Signed integer
   overflow, or division by zero. (§3)
3. **Is plain `char`'s signedness implementation-defined or undefined?**
   Implementation-defined — every conforming compiler must pick one and document it.
   (`02_implementation_defined_and_portability.md` §1–2)
4. **What's the portable way to test plain `char`'s signedness in code?**
   `#if CHAR_MIN < 0`. (§2)

## Medium

1. **Why does the C standard leave so much behavior undefined instead of pinning it
   down?** Performance (avoiding mandatory runtime checks), hardware diversity (avoiding
   forced emulation of one behavior everywhere), and optimizer freedom.
   (`01_undefined_behavior.md` §2)
2. **What's a constraint violation, and how does it differ from undefined behavior?** A
   rule the standard requires a compiler to diagnose (at least a warning); UB requires no
   diagnostic and may compile and run silently. (§4)
3. **Why is dumping a struct directly to a file with `fwrite(&s, sizeof s, 1, fp)` a
   portability risk?** The struct's padding/layout is implementation-defined; a different
   compiler or platform reading it back may interpret the bytes differently.
   (`02_implementation_defined_and_portability.md` §4)
4. **Why doesn't two mainstream compilers agreeing prove code is portable?** Many
   implementation-defined choices are shared by convention across compilers targeting the
   same CPU architecture, masking bugs that only surface on a genuinely different target. (§5)

## Hard

1. **Walk through why an optimizing compiler may legally delete an overflow check written
   as `if (x + 1 < x)`.** The check can only be true if `x + 1` overflowed, which is
   undefined behavior; the compiler may assume UB never happens, so the branch is
   unreachable under that assumption and eligible for removal.
   (`01_undefined_behavior.md` §5)
2. **Why is "works at `-O0`" not evidence that code is free of undefined behavior?** Lower
   optimization levels perform fewer of the transformations that exploit UB's "assume it
   never happens" license — the bug is latent, not absent. (§5)
3. **Design a strategy for sending a C struct's data over a network connection
   portably.** Serialize field by field, converting each multi-byte numeric field to
   network byte order explicitly, rather than transmitting the struct's raw in-memory
   bytes — sidesteps both padding and endianness differences at once.
   (`02_implementation_defined_and_portability.md` §3–4)

## Systems-level

1. **A security audit finds a bounds/overflow check in shipped, optimized code never
   actually executes, despite being reachable in source. Most likely root cause, and how
   would you confirm it?** The check's condition can only be satisfied via undefined
   behavior (commonly signed overflow), which the optimizer has assumed away; confirm by
   rebuilding at `-O0` and with `-fsanitize=undefined`. (`01_undefined_behavior.md` §5)
2. **A file format written and read by the same program works for years, then silently
   corrupts data after a routine compiler upgrade. Most likely explanation?** The upgrade
   changed a previously-unrelied-upon implementation-defined choice — most commonly struct
   padding/alignment defaults — exposed because the program used raw struct serialization
   rather than an explicit, portable format. (`02_implementation_defined_and_portability.md` §4)

## Easy (aliasing, modern C)

1. **Is type punning through a union legal in C?** Yes, when accessed through the union
   type — a genuine, standard-permitted pattern. (`03_aliasing_and_optimization.md` §3)
2. **What's the fully portable way to reinterpret a `float`'s bits as an `unsigned
   int`?** `memcpy` between the two. (§4)
3. **Which C standard introduced `<stdint.h>`'s exact-width integer types?** C99.
   (`04_modern_c_vs_historical_c.md` §2)
4. **Can you declare a loop variable inline (`for (int i = ...)`) in classic ANSI C
   (C89)?** No — C99 introduced declarations anywhere in a block. (§3)

## Medium (aliasing, modern C)

1. **Why does writing a `float` and reading it back through an `int *` produce different
   results at `-O0` versus `-O2`?** At `-O0` the compiler re-reads memory and sees the
   aliased write; at `-O2`, strict-aliasing optimization assumes the two pointer types
   can't alias, so it returns a stale cached value instead.
   (`03_aliasing_and_optimization.md` §2)
2. **What does `restrict` let the compiler do that the strict aliasing rule alone
   doesn't, for two same-typed pointer parameters?** Assume no overlap at all, even though
   same-typed pointers are always permitted to alias under the aliasing rule itself —
   unlocking optimizations like safely caching a value across loop iterations. (§5)
3. **What's the practical day-to-day difference between historical and modern C regarding
   variable declarations?** C89 requires all of a block's declarations before any
   statement; C99+ allows declaring a variable anywhere, including inline in a `for` loop.
   (`04_modern_c_vs_historical_c.md` §3)
4. **Why are designated initializers a genuine improvement over positional struct
   initialization, beyond convenience?** They're order-independent and self-documenting,
   so reordering a struct's members doesn't silently misassign values. (§3)

## Hard (aliasing, modern C)

1. **Explain precisely why union-member-reading is exempt from the strict aliasing rule
   while an equivalent pointer-cast reinterpretation is not.** The aliasing rule's
   effective-type list explicitly includes "an aggregate or union type that includes" the
   accessed type among its members; a pointer cast has no such standing, since the
   pointed-to object's effective type never changed. (`03_aliasing_and_optimization.md` §1, §3)
2. **Why is a `restrict` violation the same class of danger as a strict-aliasing
   violation, even though they're nominally different rules?** Both are promises the
   programmer makes that the compiler then optimizes on faith in; breaking either means the
   generated code's assumptions no longer match reality. (§5)
3. **Does C23's two's-complement mandate make `INT_MAX + 1` defined behavior?** No —
   representation and overflow behavior are distinct; the mandate fixes bit-pattern
   predictability, but signed overflow remains undefined behavior even in C23.
   (`04_modern_c_vs_historical_c.md` §4)
4. **Why can't code written strictly to C89 rules necessarily compile under a strict C23
   compiler?** C23 formally removed old-style (non-prototype) function declarations/
   definitions entirely, even though most of the rest of C89 remains valid. (§2, §4)

## Systems-level (aliasing, modern C)

1. **A numeric library's inner loop isn't auto-vectorizing despite identical arithmetic to
   a hand-vectorized version. First thing to check?** Whether the loop's pointer
   parameters lack `restrict` — without it, the compiler must conservatively assume
   potential aliasing. (`03_aliasing_and_optimization.md` §5)
2. **A codebase relying on `*(int *)&some_float` for years suddenly produces silent wrong
   output after a compiler upgrade. Cause and fix?** A latent strict-aliasing violation
   that an earlier compiler/optimization level happened not to exploit; fix by switching to
   `memcpy`-based reinterpretation, which is immune to this class of regression. (§4)
3. **A legacy embedded codebase needs one macro dispatching to `abs`/`labs`/`fabsf`/`fabs`
   by argument type, without moving to C11. What are the options, and what's lost without
   `_Generic`?** A GCC-specific `__builtin_types_compatible_p`-based dispatch (not portable
   ISO C), or separately-named functions requiring the caller to pick — both strictly worse
   than C11's language-level solution. (`04_modern_c_vs_historical_c.md` §7)
