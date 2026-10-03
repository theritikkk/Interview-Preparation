# Interview Questions — Undefined Behavior, Portability & Modern C

> Consolidated from `advanced/`. **Generated across multiple parts.** Currently covers:
> `01_undefined_behavior.md`, `02_implementation_defined_and_portability.md`.

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
