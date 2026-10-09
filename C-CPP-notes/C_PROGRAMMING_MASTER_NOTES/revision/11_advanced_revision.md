# Revision — Phase 11: Undefined Behavior, Portability & Modern C

> Quick-reference companion to `advanced/`. **The vault's heaviest synthesis phase** —
> mostly consolidating UB/portability facts flagged throughout every earlier phase into one
> place, plus external material on aliasing and modern C. **Phase 11 is now complete.**

---

## Undefined behavior: the complete catalogue — 1-minute summary

Four categories, not one: **undefined** (no requirement at all — anything can happen,
including silently "working"), **unspecified** (one of several defined options, no
guarantee which), **implementation-defined** (unspecified, but the implementation must
document its choice), **locale-specific**. A **constraint violation** is a fifth, different
thing: a rule the standard requires a compiler to *diagnose* — UB requires no diagnostic at
all. UB exists deliberately, trading runtime safety for performance, hardware-portability,
and optimizer freedom. The single most consequential mechanism: optimizers are only
obligated to preserve *defined* behavior, so they're free to eliminate code (like an
overflow check) whose only trigger condition is itself undefined.

### 5-minute revision
- "Is X undefined or just unspecified?" is a real, frequently-tested distinction —
  argument-evaluation order is unspecified (safe bounds, no guaranteed order); signed
  overflow is undefined (no guarantee at all).
- The catalogue spans five areas: arithmetic (signed overflow, div-by-zero, bad shifts),
  pointers/memory (OOB access, NULL deref, dangling pointers, double-free, aliasing),
  uninitialized/lifetime state, expressions (unsequenced multiple modification), and
  library/I/O misuse (ctype argument domain, `va_arg` type mismatch, format-string
  mismatch, signal-handler restrictions).
- "Works at `-O0`" is not evidence of correctness — it just means fewer UB-exploiting
  optimizations ran.

**Worth memorizing:** the four-category table, cold — and specifically that unspecified
and undefined are *not* synonyms.
**Worth understanding, not memorizing:** the overflow-check-deletion example
(undefined_behavior.md §5) well enough to explain it from first principles in an interview
— this is one of the highest-value single examples in the whole vault.

---

## Implementation-defined behavior & portability — 1-minute summary

Implementation-defined behavior is *safe* to rely on for a known, fixed, documented target
— and a portability bug waiting to happen the moment that target changes. The vault's own
running examples: plain `char` signedness, exact integer widths beyond the guaranteed
minimums, struct padding, bit-field packing order, byte order. The fix is always one of:
query it (`<limits.h>`/`<float.h>`), sidestep it (`<stdint.h>` exact-width types,
`htonl`/`ntohl`), or isolate it (`#if`/`#ifdef`).

### 5-minute revision
- Dumping a struct raw to a file/socket (`fwrite(&s, sizeof s, 1, fp)`) bakes in that
  platform's padding *and* byte order — prefer field-by-field serialization for anything
  crossing a compiler/platform/network boundary.
- Two mainstream compilers agreeing is weak portability evidence — they often share
  conventions by CPU architecture, not by standard mandate.

**Worth memorizing:** `#if CHAR_MIN < 0` as the portable char-signedness test.
**Worth understanding, not memorizing:** *why* raw struct serialization breaks across
platforms (implementation_defined_and_portability.md §4) — you should be able to name both
causes (padding, endianness), not just "it's risky."

---

## Aliasing & the strict aliasing rule — 1-minute summary

An object may only be accessed through an lvalue of its own (or compatible) type, the
corresponding signed/unsigned type, an aggregate/union containing that type, or a
character type — the universal exception that's why `memcpy` always works. Violating this
(the classic `*(int*)&some_float` pattern) is undefined behavior, **verified live in this
vault**: the same source printed `result = 0` at `-O0` and `result = 1` at `-O2`, because
`-O2` assumes a `float*` and an `int*` can't alias and skips re-reading memory the `-O0`
build actually re-read. Union-based type punning, accessed through the union type, is
explicitly **permitted** in C (unlike C++) — confirmed consistent at both optimization
levels. `memcpy`-based reinterpretation is always safe and typically free (compilers fold
a fixed-size `memcpy` into a register move). `restrict` on a pointer parameter is the
programmer's promise of no aliasing, unlocking optimizations (caching, vectorization) the
aliasing rule alone can't license for same-typed pointers — breaking that promise is UB of
the same class.

**Worth memorizing:** the character-type exception (why `memcpy` is always legal) and that
union-based punning is C-legal but not C++-legal.
**Worth understanding, not memorizing:** the real `-O0`-vs-`-O2` divergence
(aliasing_and_optimization.md §2) well enough to explain *why* each answer is "correct"
from the compiler's point of view — this is the clearest single demonstration in the vault
of what "the optimizer assumes UB never happens" actually means in practice.

---

## Modern C vs. historical C — 1-minute summary

All three source books predate or ignore C99+: Book B is exactly ANSI C89; Book C is from
1992; Book A (2013) doesn't flag which features are version-specific. The day-to-day-
relevant deltas: C99 allows declarations anywhere in a block (not just at the top, as
Book B's own examples always do), adds `<stdbool.h>`, `<stdint.h>`, `restrict`, designated
initializers, compound literals, and `//` comments; C11 adds `_Generic`, anonymous structs/
unions, `_Static_assert`, `_Alignas`/`_Alignof`; C17 is a bug-fix-only release; C23 adds
`0b` literals, `nullptr`, removes old-style (K&R) function declarations entirely, and
**mandates two's complement** as the only legal signed-integer representation.

**Worth memorizing:** the two's-complement-mandate caveat — it fixes *representation*, not
*overflow behavior*; `INT_MAX + 1` is still undefined behavior even in C23. This is a
real, specific trap.
**Worth understanding, not memorizing:** *why* a C89-legal old-style function definition
no longer compiles under a strict C23 compiler (modern_c_vs_historical_c.md §4) — know
which parts of "old C" are merely discouraged versus actually removed.

---

## Cross-topic traps worth drilling before an interview

1. Calling argument-evaluation order "undefined" instead of "unspecified" — a real,
   commonly-tested conflation.
2. Assuming a clean compile and a passing test run prove the absence of UB.
3. `if (x + 1 < x)` as an overflow check — the optimizer may delete it entirely, since the
   only way it's true is via undefined behavior.
4. Serializing a struct with a raw `fwrite`/`memcpy` across a compiler or platform
   boundary.
5. Assuming plain `char`'s signedness (or any implementation-defined fact) transfers
   unchanged to a new compiler/platform without checking.
6. Reinterpreting a `float`'s bits via `*(unsigned int *)&f` — undefined, and genuinely
   gives different answers at different optimization levels (verified in this vault).
7. Assuming union-based type punning is as risky as pointer-cast punning — it isn't; it's
   explicitly permitted in C.
8. Writing a `restrict`-qualified function and then calling it with overlapping arguments
   anyway — breaks the exact promise that licensed the compiler's optimizations.
9. Assuming "C23 mandates two's complement" means signed overflow is now defined — it
   doesn't; overflow is still UB.
10. Assuming classic K&R-style (non-prototype) function definitions still compile
    everywhere — C23 removed them outright.
