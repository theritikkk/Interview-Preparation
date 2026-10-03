# Revision — Phase 11: Undefined Behavior, Portability & Modern C

> Quick-reference companion to `advanced/`. **The vault's heaviest synthesis phase** —
> mostly consolidating UB/portability facts flagged throughout every earlier phase into one
> place, plus new external material on aliasing and modern C. Generated across multiple
> parts. Currently covers: `01_undefined_behavior.md`, `02_implementation_defined_and_portability.md`.

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
