# Revision — Phase 1: C Fundamentals

> Quick-reference companion to `fundamentals/01_program_structure.md` through
> `04_control_flow.md`. Read the topic files for the full "why"; use this file for
> pre-interview or pre-project refreshers.

---

## Program structure & compilation — 1-minute summary

A `.c` file passes through **preprocess → compile → assemble → link → load → execute**.
Preprocessing is pure text substitution (`#include`, macros) and happens before the
compiler sees any C syntax. Each source file compiles independently into an object file
(`.o`) containing machine code plus unresolved symbol references; the **linker** matches
those references against definitions across all object files/libraries and produces one
executable. This split is what makes separate compilation — recompiling only what changed
— possible at all.

### 5-minute revision
- `gcc -E/-S/-c` stop the pipeline after preprocessing/compiling/assembling respectively —
  use them to inspect intermediate artifacts when debugging build issues.
- Declaration ≠ definition: a declaration promises a name/type exists; a definition
  provides the actual body/storage. Separate compilation only works because a translation
  unit needs just declarations for anything defined elsewhere.
- `main(void)`, not `main()` — bare `()` disables argument-list checking.
- Falling off the end of `main` without `return` is unspecified under classic C89, defined
  to return `0` from C99 onward — write `return 0;` explicitly regardless.

**Worth memorizing:** the `gcc -E/-S/-c/`(link) flag meanings; `main(void)` vs `main()`.
**Worth understanding, not memorizing:** *why* separate compilation needs a linker at all
(program_structure.md §2) — this explains half of all "undefined reference" errors you'll
ever debug.

---

## Types & representation — 1-minute summary

C fixes only *minimum* type widths (`char` ≥ 8 bits, `short`/`int` ≥ 16, `long` ≥ 32,
`long long` ≥ 64), not exact sizes — always use `sizeof`, never a hardcoded number. Plain
`char` signedness is implementation-defined; use `signed char`/`unsigned char` when it
matters. `sizeof` is a compile-time operator, not a function, yielding a `size_t`.

### 5-minute revision
- Ordering guarantee: `sizeof(short) ≤ sizeof(int) ≤ sizeof(long) ≤ sizeof(long long)` —
  not a guarantee about which are *actually* different sizes.
- `getchar()`/`fgetc()` return `int`, not `char`, specifically so `EOF` is distinguishable
  from every real byte value — storing into a `char` first can break the comparison.
- Unsigned overflow is well-defined (wraps mod `2ⁿ`); **signed overflow is undefined
  behavior**, not "wraps like unsigned does."
- `const` is a compile-time contract the compiler must diagnose violations of — not a
  hardware write-protection guarantee. `volatile` tells the compiler not to optimize away
  "redundant" reads/writes (hardware registers, signal handlers).
- Modern C (`[External]`, not in the source books): use `<stdint.h>` (`int32_t`, etc.) when
  exact width matters; `<stdbool.h>`'s `bool` for boolean intent.

**Worth memorizing:** the minimum-width table; `sizeof(char) == 1` always, by definition.
**Worth understanding, not memorizing:** why plain-`char` signedness is left open (types.md
§2) — it explains a whole class of "works on my machine" bugs.

---

## Expressions & operators — 1-minute summary

`&&`/`||`/`?:`/`,` are the *only* operators with guaranteed left-to-right, short-circuited
evaluation — everywhere else (including function-argument lists), evaluation order is
unspecified, so never write an expression whose correctness depends on it. The "usual
arithmetic conversions" promote `char`/`short` to `int` first, then widen toward
`long`/`float`/`double`/`long double` as needed; mixing signed and unsigned types silently
converts the signed value to unsigned, which is a live bug source, not a theoretical one.

### 5-minute revision
- Precedence trap: `&` `^` `|` bind **looser** than `==`/`!=` — `if (x & MASK == 0)` is a
  bug; always parenthesize bit-tests: `if ((x & MASK) == 0)`.
- `x *= y + 1` means `x = x * (y + 1)` — compound assignment implicitly parenthesizes the
  right-hand side.
- Postfix `s[j++] = s[i]` is the "use-then-advance" idiom — equivalent to
  `s[j] = s[i]; j++;`, and it's everywhere in real C string/copy code.
- `while ((c = getchar()) != EOF)` needs those parens — `!=` binds tighter than `=`.
- `if (c = 20)` is *legal* C (assigns, evaluates true) — not a compiler-caught typo.

**Worth memorizing:** the precedence table's key surprises (bitwise below equality;
assignment/ternary near the bottom, right-associative).
**Worth understanding, not memorizing:** the full conversion ladder (expressions.md §3) —
you should be able to *derive* what type an expression like `char + unsigned long` produces
rather than recalling it from a table.

---

## Control flow — 1-minute summary

`if`/`else if`/`else`, `switch` (falls through unless `break`ed), `while`/`for`
(interchangeable except for `continue`'s interaction with the increment step),
`do`-`while` (tests after, so the body always runs once), `break`/`continue` (innermost
loop/switch only), and `goto` (formally never necessary; legitimate mainly for multi-level
loop exit).

### 5-minute revision
- Dangling `else` binds to the nearest `else`-less `if` — brace nested `if`s to avoid
  surprises.
- `switch` fallthrough is specified behavior, not a bug in the compiler — the bug is always
  a missing `break` you meant to write. Put a `break` after the last case too, defensively.
- `for (expr1; expr2; expr3)` differs from its `while` expansion in exactly one way:
  `continue` still runs `expr3` before re-testing.
- `do`-`while` is the right choice specifically when the body must run at least once
  (classic case: digit-generation loops like `itoa`, which must emit at least one digit
  for input `0`).
- `break` only exits the *innermost* loop/switch; there's no multi-level `break` in C —
  that's the one job `goto` still legitimately does.

**Worth memorizing:** the dangling-else resolution rule; that `continue` in `for` still
runs the increment.
**Worth understanding, not memorizing:** when `do`-`while` is *necessary* rather than just
stylistically different from `while` (control_flow.md §5) — being able to justify the
choice matters more than recalling the syntax.

---

## Cross-topic traps worth drilling before an interview

1. `for (size_t i = n - 1; i >= 0; i--)` — infinite loop when `n == 0`, because unsigned
   wraparound plus an always-true `i >= 0` test (types.md + expressions.md).
2. `if (x & MASK == 0)` — precedence bug, not a logic bug (expressions.md §8).
3. Forgetting `break` in `switch` — specified fallthrough, not UB (control_flow.md §3).
4. `printf("%d %d\n", ++n, power(2, n));` — unspecified argument evaluation order
   (expressions.md §9).
5. `main()` vs `main(void)` — the former disables argument checking (program_structure.md §5).
