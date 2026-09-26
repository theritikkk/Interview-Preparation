# Interview Questions — C Fundamentals

> Consolidated from `fundamentals/01_program_structure.md` through `04_control_flow.md`.
> Full reasoning and follow-ups live in each topic file's own "🎯 Interview questions"
> section (cross-referenced below) — this file is for rapid-fire self-testing.

---

## Easy

1. **What stages does a `.c` file pass through before it runs?** Preprocess → compile →
   assemble → link → load → execute. (`01_program_structure.md` §1–2)
2. **What does `#include <stdio.h>` actually do?** Textually pastes the header's contents
   in at that point, before compilation proper begins — it's a preprocessor action, not a
   language feature. (§3)
3. **Is `sizeof(int)` guaranteed to be 4?** No — only ≥16 bits and
   `sizeof(short) ≤ sizeof(int) ≤ sizeof(long)` are guaranteed. (`02_types.md` §1)
4. **What's the difference between `'x'` and `"x"`?** `'x'` is an `int` (a character
   constant); `"x"` is a 2-element `char` array (`'x'` plus `'\0'`). (§7)
5. **Difference between `x++` and `++x` as expressions?** Postfix yields the old value then
   increments; prefix increments then yields the new value. (`03_expressions_and_operators.md` §4)
6. **What does `x % y` do for negative `x`?** Implementation-defined under classic C;
   truncation toward zero mandated from C99 onward. (§1)
7. **Does a `switch` case stop automatically after its statements?** No — execution falls
   through to the next case unless a `break` (or other jump) stops it.
   (`04_control_flow.md` §3)
8. **When is `do`-`while`'s body guaranteed to run, versus `while`'s?** `do`-`while` always
   runs the body at least once (tests after); `while` may run it zero times (tests before). (§5)

## Medium

1. **Why doesn't changing one `.c` file in a 50-file project require recompiling all 50?**
   Compilation is per translation unit; only the changed file (plus anything whose exported
   declarations it affects) needs recompiling — only the link step needs re-running for the
   rest. (`01_program_structure.md` §6)
2. **Declaration vs. definition — what's the actual distinction, and why does it matter?**
   A declaration promises a name/type exists; a definition provides the body/storage.
   Separate compilation depends on this split. (§4)
3. **Why do `getchar()`/`fgetc()` return `int`, not `char`?** So `EOF` can be represented
   outside the range of every valid byte value; storing into a `char` first can break the
   comparison depending on `char`'s signedness. (`02_types.md` §3)
4. **What does `const` actually guarantee?** The compiler must diagnose direct reassignment
   through that name — it does not guarantee hardware write-protection, and modification
   through a non-`const` alias to an originally-`const` object is undefined behavior. (§6)
5. **Why does `if (x & MASK == 0)` not test what it looks like it tests?** `==` binds
   tighter than `&`; it parses as `x & (MASK == 0)`. Always write `(x & MASK) == 0`.
   (`03_expressions_and_operators.md` §8)
6. **Why is `-1 < 1u` false in C?** The usual arithmetic conversions convert `-1` to
   `unsigned int` before comparing, turning it into `UINT_MAX`. (§3)
7. **Explain the dangling-`else` ambiguity and how C resolves it.** `else` binds to the
   nearest preceding `else`-less `if`; braces are the only way to force a different
   association. (`04_control_flow.md` §2)
8. **Where does "`for` is just sugar over `while`" break down?** `continue`'s effect —
   `for`'s increment step still runs before re-testing on a `continue`; a naive `while`
   hand-expansion would skip it. (§4)

## Hard

1. **"The preprocessor is just part of the compiler" — what's wrong with that, and what
   bug pattern does it tend to cause?** The preprocessor is a distinct, syntax-blind
   text-substitution pass that runs first; the misunderstanding causes macro-argument bugs
   (e.g. `#define SQUARE(x) x*x` breaking on `SQUARE(a+b)`).
   (`01_program_structure.md` §3, full treatment in `preprocessing/01_macros.md`, Phase 2)
2. **Why is unsigned overflow well-defined but signed overflow undefined behavior — and
   what does that let a compiler do?** Unsigned arithmetic is defined modulo `2ⁿ`; signed
   overflow has no defined result, so the compiler may assume it never happens and optimize
   on that assumption — including eliminating overflow checks that could only trigger via
   UB. (`02_types.md` §10, full catalogue in `advanced/01_undefined_behavior.md`, Phase 11)
3. **Is `printf("%d %d\n", ++n, power(2, n));` well-defined?** No — function-argument
   evaluation order is unspecified, so whether `power` observes the incremented or
   un-incremented `n` is compiler-dependent. (`03_expressions_and_operators.md` §9)
4. **What's actually wrong with `a[i] = i++;`, precisely?** Whether the array index uses
   the pre- or post-increment value of `i` is unspecified by the standard — this isn't
   "usually fine," it's already broken, just not always visibly so. (§9)
5. **When, if ever, is `goto` the right tool in C — and why does that case resist a clean
   `break`/`continue` rewrite?** Breaking out of multiple nested loops at once; `break`
   only exits the innermost loop, and flag-variable rewrites trade a clear jump for extra
   state checked on every iteration. (`04_control_flow.md` §7)

## Systems-level

1. **Trace what a build system must re-run after a single-file change, and why re-linking
   alone can fail even when every file compiled cleanly.** Recompile the changed file only;
   re-link against all object files and any newly-required library — if that library isn't
   on the link line, `ld` fails with an undefined-reference error even though compilation
   succeeded everywhere, because compiling only checks per-translation-unit consistency
   while linking verifies every used symbol has exactly one definition somewhere.
   (`01_program_structure.md` §6)
2. **A 4-byte value arrives over a network socket and must become an `int` on both a
   little-endian x86 host and a big-endian embedded target. What breaks with a naive
   `memcpy`, and what's the fix?** Byte order differs by platform, so raw bytes decode to
   different numeric values on each machine; use defined-order conversion functions
   (`ntohl`/`htonl`) instead of relying on native byte order.
   (`02_types.md` §5, `[External]`, full context in `systems/07_sockets_and_networking.md`)
3. **A PR replaces a nested-loop `goto error;` cleanup with a boolean flag and compound
   loop conditions. What's actually gained and lost?** Gained: no `goto`. Lost: the flag
   and compound conditions run on every iteration of every nesting level (a small but real
   cost) and can make "why are we still looping" harder to read than a single jump,
   especially as nesting grows — a genuine style tradeoff, not a strict improvement either
   way. (`04_control_flow.md` §10)
4. **Why does `for (size_t i = n - 1; i >= 0; i--)` infinite-loop when `n == 0`?**
   `size_t` is unsigned; `n - 1` wraps to `SIZE_MAX` instead of going negative
   (well-defined unsigned wraparound), and `i >= 0` is then always true for an unsigned `i`.
   (`02_types.md` + `03_expressions_and_operators.md`, cross-topic trap)
