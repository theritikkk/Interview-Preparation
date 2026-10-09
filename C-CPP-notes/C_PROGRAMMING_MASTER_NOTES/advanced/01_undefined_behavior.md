# Undefined Behavior: The Complete Catalogue

> **Phase 11 · Undefined Behavior, Portability & Modern C — the vault's heaviest
> `[Synthesis]` load.** This file does not introduce new mechanics so much as **name and
> formally classify** something this vault has been flagging in passing since Phase 1 —
> every cross-reference below points at a file that already showed the bug in context;
> this file is where they're finally assembled into one catalogue, with the standard's own
> vocabulary attached.
> Depends on: essentially the whole vault — see the catalogue's cross-references.
> Feeds into: `02_implementation_defined_and_portability.md`, `03_aliasing_and_optimization.md`
> (this phase — the single largest UB category, strict aliasing, gets its own full file).
> Sources: **Book B's Appendix A** sprinkles the terms "implementation-defined" and
> "unspecified" throughout (confirmed by direct search: 15+ occurrences), **without ever
> formally defining them** — A.1's own introduction launches straight into lexical
> conventions with no glossary. **The four-way classification below, and the systematic
> organization of the catalogue, are `[External]`/`[Synthesis]`** — this is precisely the
> gap the task brief identified from the start (`01_BOOK_ANALYSIS.md` §6): none of the
> three books organizes this material as a standalone topic.

---

## 1. Four categories, precisely defined

`[External, ISO C's own vocabulary]` The standard doesn't describe just one kind of
"behavior it doesn't pin down" — it distinguishes four, and conflating them is itself a
common source of confusion:

| Category | Meaning | The standard's obligation |
|---|---|---|
| **Undefined behavior (UB)** | Behavior for which the standard imposes **no requirement at all** | None — anything can happen: a crash, wrong output, or (most dangerously) apparently correct output |
| **Unspecified behavior** | Behavior for which the standard provides **two or more possibilities**, and imposes no further requirement on which | The implementation picks *one* of the valid options, consistently or not, but stays within defined bounds |
| **Implementation-defined behavior** | A specific *kind* of unspecified behavior where **each implementation must document its choice** | The implementation picks one option **and tells you what it picked** (e.g., in its documentation) |
| **Locale-specific behavior** | Behavior that depends on the active locale (`libraries/04_ctype_and_locale.md` §3) | Depends on locale settings, documented per-locale |

### 🧠 The one-sentence distinction worth memorizing
`[Synthesis]` **Unspecified** behavior means "could go either way, and that's fine, just
don't assume a specific one." **Implementation-defined** behavior means "could go either
way, but your compiler is obligated to document which one it picked." **Undefined**
behavior means "the standard makes no promise whatsoever" — including the promise that your
program keeps running at all.

### Worked examples of each, already seen in this vault

- **Implementation-defined:** plain `char`'s signedness (`fundamentals/02_types.md` §2) —
  every real compiler documents its choice, and `<limits.h>`'s `CHAR_MIN`
  (`libraries/10_limits_float_stddef.md` §3) lets you query it in code.
- **Unspecified:** order of evaluation of function arguments
  (`fundamentals/03_expressions_and_operators.md` §9) — `f(a(), b())` may call `a` or `b`
  first, and no implementation is obligated to document or fix which.
- **Undefined:** signed integer overflow (§3 below) — nothing is guaranteed, not even a
  consistent wrong answer.
- **Locale-specific:** whether `isalpha` considers an accented character a letter
  (`libraries/04_ctype_and_locale.md` §3).

### 🎯 Interview point
"Is order of function-argument evaluation undefined behavior?" — **No — it's unspecified,**
a materially weaker (safer) category than undefined. This is a genuinely common conflation
worth correcting explicitly: unspecified behavior stays within defined bounds; undefined
behavior does not.

---

## 2. Why undefined behavior exists at all

`[Synthesis]` Not an oversight — a deliberate design tradeoff, for (at least) three
reasons:

1. **Performance.** If the standard mandated a defined result for, say, signed overflow
   (like Java does), every addition would need an overflow check on hardware that doesn't
   support one natively — a cost paid on *every* signed arithmetic operation, everywhere,
   to prevent a case most code never hits.
2. **Hardware diversity.** C targets everything from microcontrollers to supercomputers;
   mandating one specific behavior for, say, out-of-range shifts would be actively wrong on
   hardware where the "natural" behavior differs, forcing expensive emulation on some
   platforms just for standard conformance.
3. **Optimizer freedom.** This is the one that bites the hardest in practice, covered fully
   in §5: UB gives the compiler license to assume it never happens, enabling optimizations
   that would otherwise be unsound.

`[Synthesis]` The tradeoff, stated plainly: C gives *you* the responsibility the standard
declines to enforce. This is the same "trust the programmer" philosophy visible throughout
this vault — array bounds aren't checked (`memory/02_pointers.md` §4), `malloc`'d memory
isn't zeroed (`memory/05_dynamic_memory.md` §2), and here too.

---

## 3. The catalogue

`[Synthesis, consolidating every UB instance this vault has flagged, each tagged with
where it was first shown in context]`

### Arithmetic

- **Signed integer overflow.** `INT_MAX + 1` — genuinely undefined, not a guaranteed
  wraparound (unlike unsigned overflow, which *is* defined modulo `2ⁿ`).
  First shown: `fundamentals/02_types.md` §9–10. Real UBSan capture:
  `debugging/02_sanitizers_and_valgrind.md` §4.
- **`abs(INT_MIN)` and the `printd`/`itoa` negation bug.** The same asymmetry, recurring:
  `functions/02_recursion.md` §2, `libraries/02_stdlib.md` §2.
- **Integer division by zero** (`x / 0`, `x % 0`) — undefined. **Measured in this
  vault's own build environment (GCC 13.3.0, x86-64 Linux, `-O0` for both), two nearly
  identical programs gave two completely different outcomes:**

  ```c
  volatile int num = 7, z = 0;
  return num / z;          /* ->  "Floating point exception", exit status 136 */

  volatile int z = 0;
  return 1 / z;            /* ->  no crash at all, exit status 0 */
  ```

  The first compiled to a real `idiv` instruction; the CPU raised a divide error and the
  kernel delivered `SIGFPE` (shell status `128 + 8 = 136`). The second contained **no
  `idiv` at all** — verified by disassembly: GCC special-cased division of the constant `1`
  by a variable into a compare-and-conditional-move sequence, and because `z == 0` is
  undefined, that sequence simply yields `0` and the program carries on. **Same compiler,
  same flags, same undefined behavior — one version crashes, the other silently continues.**
  Neither is "the" behavior of division by zero; the standard promises nothing, so both
  are permitted. This is also a correction worth being open about: the *expected* result
  for the second program, written down before running it, was a `SIGFPE` crash —
  precisely the "it'll just crash, probably" assumption this file's §6 warns against.
- **Floating-point division by zero** is a different category, not undefined in the same
  way on IEEE 754 platforms. `[External, measured on the same platform]`
  `1.0/0.0` printed `inf`, `-1.0/0.0` printed `-inf`, `0.0/0.0` printed `-nan` — no trap,
  no signal, just IEEE special values. `[Synthesis]` Whether a given C implementation
  follows IEEE 754 semantics here is a property of that implementation, so treat the
  `inf`/`nan` results as "documented behavior of IEEE-conforming targets," not a guarantee
  of ISO C itself.
- **Invalid shifts.** Shifting by a negative count, or by a count ≥ the type's width
  (`1 << 33` for a 32-bit `int`) — undefined. Real UBSan capture:
  `debugging/02_sanitizers_and_valgrind.md` §4.

### Pointers and memory

- **Out-of-bounds array/pointer access.** Reading or writing outside an array's bounds, or
  computing a pointer more than one-past-the-end. First shown:
  `memory/02_pointers.md` §4. Real ASan captures (heap and stack):
  `debugging/02_sanitizers_and_valgrind.md` §2.
- **Dereferencing a `NULL` pointer.** Not merely "usually crashes" — undefined, full stop;
  a real crash is simply the *typical* (not guaranteed) consequence on mainstream
  hardware. First shown: `memory/02_pointers.md` §8.
- **Dangling-pointer use (use-after-return, use-after-free).** First shown:
  `memory/02_pointers.md` §8 (use-after-return), `memory/05_dynamic_memory.md` §8
  (use-after-free). Real ASan capture: `debugging/02_sanitizers_and_valgrind.md` §2.
- **Double-free.** First shown: `memory/05_dynamic_memory.md` §8. Real ASan capture:
  `debugging/02_sanitizers_and_valgrind.md` §2.
- **Missing string terminator** (calling `strlen`/`strcpy`-family functions on a
  non-terminated `char` buffer). First shown: `memory/04_arrays_and_strings.md` §5.
- **Writing through a pointer to a string literal.** First shown:
  `memory/04_arrays_and_strings.md` §4.
- **Pointer comparisons/subtraction across unrelated objects.** First shown:
  `memory/02_pointers.md` §4.
- **`memcpy` on overlapping regions.** First shown: `libraries/03_string.md` §3.
- **Strict aliasing violations** (accessing an object through an incompatible pointer
  type). Previewed: `memory/07_alignment_padding_aliasing.md` §5. Full treatment:
  `03_aliasing_and_optimization.md`, this phase.

### Uninitialized and lifetime

- **Reading an uninitialized automatic variable.** First shown:
  `functions/01_functions_and_stack_frames.md` §8.
- **Modifying a non-`volatile` local across `setjmp`/`longjmp`,** then relying on its
  value — indeterminate, a close relative of uninitialized-read UB. First shown:
  `libraries/07_setjmp_and_signal.md` §2.

### Expressions

- **Multiple unsequenced modifications of the same object** (`a[i] = i++;`, `i = i++ + 1;`)
  — the standard requires a *sequence point* between separate modifications; violating it
  is undefined, not merely "compiler-dependent in a harmless way." First shown:
  `fundamentals/03_expressions_and_operators.md` §9.
- **`*ip++` vs. `(*ip)++` confusion** is a precedence trap, not UB itself — but chaining it
  into a second modification of the same object in the same expression can cross into this
  same unsequenced-modification territory.

### Library and I/O

- **Passing a `char` with its sign bit set to a `<ctype.h>` function, uncast.** First
  shown: `libraries/04_ctype_and_locale.md` §2.
- **`va_arg` requesting the wrong (un-promoted) type.** First shown:
  `libraries/08_stdarg.md` §2.
- **A `printf`/`scanf` format specifier mismatched against its argument's actual type.**
  First shown: `libraries/01_stdio.md` §5.
- **Calling a signal handler that does more than the standard's short safe-function list
  permits.** First shown: `libraries/07_setjmp_and_signal.md` §3,
  `systems/06_pipes_and_signals.md` §4.
- **`recv`/similar network functions: using a negative-on-error return value as a buffer
  index without checking first.** First shown: `systems/07_sockets_and_networking.md` §5.

### 🎯 Interview point
"Name five undefined behavior traps in C, from memory." — Notice this catalogue is
organized so you can answer from *categories* (arithmetic, pointers, uninitialized state,
expressions, library misuse) rather than a flat list — reciting one solid example from each
category demonstrates structural understanding, not just memorization of a list.

---

## 4. Constraint violations — a fifth, different category

`[External]` Distinct from all four categories above: a **constraint violation** is a
*syntax or semantic rule* the standard requires a conforming compiler to **diagnose** (at
minimum, a warning) — e.g., calling a function with the wrong number of arguments when a
prototype is in scope. `[Synthesis]` The crucial practical difference: **a constraint
violation must produce a diagnostic; undefined behavior need not produce any diagnostic at
all.** A program can have a constraint violation and still legally compile and run (with
merely a required warning); a program with undefined behavior may compile completely
silently and misbehave only in production, on a specific input, on a specific platform.

---

## 5. How optimizers actually exploit UB

`[Cross-reference, consolidating `systems/01_compilation_toolchain.md` §2 and
`debugging/02_sanitizers_and_valgrind.md` §4]` The mechanism, stated precisely: **the
optimizer is only required to preserve the behavior the standard actually defines.** If a
transformation is only "wrong" in a case that's undefined behavior, the transformation is
legal — because there's no defined behavior for it to violate.

### Worked example — a "safety check" silently deleted

```c
int compute(int x) {
    int y = x + 1;
    if (y < x)          /* "check for overflow" */
        handle_error();
    return y;
}
```

**Why an optimizing compiler may delete the entire `if`:** `y < x` after `y = x + 1` can
only be true if `x + 1` **overflowed** — and signed overflow is undefined behavior (§3). The
compiler is entitled to assume UB never happens in a correct program; under that
assumption, `y < x` is *never* true, so the whole branch is dead code, eligible for
removal. `[Synthesis]` This is not a hypothetical — exactly this class of "optimized-away
overflow check" has caused real, documented security vulnerabilities in shipped software,
precisely because the check looked correct to a human reader and compiled silently.

### The direct link to `debugging/02_sanitizers_and_valgrind.md`'s UBSan transcript
`[Cross-reference]` That file's real, captured UBSan output showed a signed-overflow-then-
continue-anyway pattern producing a corrupted printed value — this section explains the
*other* half of the danger: at a higher optimization level, the compiler might not just
produce a wrong value, it might eliminate reachable code entirely, based on the same UB.

### 🎯 Interview point
"Why is 'my code works fine at `-O0`' not evidence of correctness?" — Because `-O0`
performs few of the transformations that exploit UB's "assume it never happens" license;
the bug is **latent**, not absent, and a compiler upgrade, a different optimization flag,
or even an unrelated code change nearby can be enough to expose it (`systems/01_compilation_toolchain.md`
§2, `debugging/04_debugging_workflows_labs.md` §7).

---

## 6. ⚠️ Misconceptions to unlearn now

- **"Undefined behavior means the program crashes."** A crash is the *best-case*, most
  visible outcome — silently wrong output, or "appears to work perfectly," are both
  equally valid (and far more dangerous) consequences the standard permits (§1).
- **"Unspecified and undefined are basically the same thing, just different words."** They
  are not — unspecified behavior guarantees the result stays within a defined set of
  possibilities; undefined behavior guarantees nothing at all (§1).
- **"If it compiles without warnings and runs correctly in testing, it doesn't have
  UB."** Neither compilation nor a clean test run are reliable evidence — UB's defining
  property is that its consequences are not required to be visible, consistent, or
  triggered by every input/platform/optimization level (§5).

---

## 7. 🎯 Interview questions

**Easy**
- What's the difference between unspecified and undefined behavior? → Unspecified
  guarantees one of several defined outcomes; undefined guarantees nothing at all (§1).
- Name one arithmetic operation that's undefined behavior in C. → Signed integer overflow,
  or division by zero (§3).

**Medium**
- Why does the C standard leave so much behavior undefined instead of pinning it down?
  → Performance (avoiding mandatory runtime checks), hardware diversity (avoiding forced
  emulation of one specific behavior everywhere), and optimizer freedom (§2).
- What's a constraint violation, and how does it differ from undefined behavior? → A rule
  the standard requires a compiler to diagnose (at least a warning); UB requires no
  diagnostic at all and may compile and run silently (§4).

**Hard**
- Walk through why an optimizing compiler is legally permitted to delete an overflow check
  written as `if (x + 1 < x)`. → The check can only be true if `x + 1` overflowed, which is
  undefined behavior; the compiler may assume UB never happens in a correct program, so
  under that assumption the branch is unreachable and eligible for removal (§5).
- Why is "works at `-O0`" not evidence that code is free of undefined behavior? → Lower
  optimization levels perform fewer of the transformations that actually exploit UB's
  "assume it never happens" license — the bug is latent, and higher optimization,
  different compilers, or even unrelated nearby code changes can expose it (§5).

**Systems-level**
- A security audit finds that a bounds/overflow check in shipped, optimized code never
  actually executes, despite being reachable in the source. What's the most likely root
  cause, and how would you confirm it? → The check's condition can only be satisfied via
  undefined behavior (commonly signed overflow), which the optimizer has assumed away and
  eliminated; confirm by rebuilding at `-O0` (restoring the check) and with
  `-fsanitize=undefined` (`debugging/02_sanitizers_and_valgrind.md` §4), which will report
  the underlying UB directly rather than requiring you to infer it from disassembly.

---

## 8. External references

- [cppreference — Undefined behavior](https://en.cppreference.com/w/c) `[External]` — the
  current, comprehensive, community-maintained list of every UB category the standard
  defines, organized by clause.
- [John Regehr — "A Guide to Undefined Behavior in C and C++" (series)](https://gcc.gnu.org/onlinedocs/)
  `[External, verify the current live URL — this is a well-known independent blog series,
  not a GCC document; search for it directly if the link doesn't resolve]` — a widely-cited,
  accessible treatment of exactly how optimizers exploit UB, expanding on §5.
- [CERT C Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c) `[External]` —
  systematic, rule-numbered coverage of nearly every catalogue entry in §3.

## 9. 🛠 Practice

1. Compile the `compute()` overflow-check example from §5 at `-O0` and `-O2`, inspect the
   generated assembly (`fundamentals/01_program_structure.md` §2's `gcc -S` technique) at
   each level, and confirm the check survives at `-O0` but is eliminated at `-O2`.
2. Pick five entries from §3's catalogue you haven't personally triggered yet, and
   reproduce each one under AddressSanitizer or UndefinedBehaviorSanitizer
   (`debugging/02_sanitizers_and_valgrind.md`), reading the real diagnostic each produces.
3. Write a short list, from memory, of which of this file's catalogue entries are
   "undefined" versus "merely unspecified" versus "implementation-defined" — then check
   yourself against §1 and §3. This distinction is worth being able to make instantly,
   not just recognize when explained.
