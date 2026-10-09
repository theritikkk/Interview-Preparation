# Modern C vs. Historical C

> **Phase 11 · Undefined Behavior, Portability & Modern C — final file.** This file
> consolidates every "this is a modern addition the source books predate" aside dropped
> throughout the vault into one place, plus the handful of genuine C99→C23 changes not yet
> mentioned.
> Depends on: `01_BOOK_ANALYSIS.md` §1 (each book's exact publication era), nearly every
> earlier file (see the table's cross-references).
> Feeds into: nothing further in the language material — this is the last `advanced/` file;
> Phase 12 is projects, interviews, and reference material built on everything so far.
> Sources: **entirely `[External]`**, cppreference's own C23 changelog and GCC's
> documentation, cross-checked against current sources for the C23-specific claims below.

---

## 1. Where the three books actually sit in C's timeline

`[Cross-reference, `01_BOOK_ANALYSIS.md` §1]` **Book B** (K&R2) describes ANSI C
(C89/C90) exactly — it predates C99 by over a decade. **Book C** (Plauger) is from 1992,
squarely C89-era, with zero C99+ awareness. **Book A** (Wikibooks) was compiled in 2013 —
chronologically *after* C99 and C11 existed — but its practitioner style teaches C in a
largely version-agnostic way and doesn't explicitly call out which features are
C99/C11-specific. **Net effect: nothing in any of the three books' own text can be trusted
to reflect C99 or later without external confirmation** — which is exactly why every
"modern C" aside elsewhere in this vault was tagged `[External]`, and why this file exists
as their single consolidation point.

---

## 2. The consolidated delta table

`[Synthesis, consolidating every forward-reference made since Phase 1]`

| Feature | Introduced | Historical C (the books' world) | This vault |
|---|---|---|---|
| `<stdint.h>` exact-width types | C99 | Only `int`/`long`/etc. with platform-varying width | `fundamentals/02_types.md` §8 |
| `<stdbool.h>` / `_Bool` | C99 (keyword renamed `bool` in C23) | `0`/nonzero convention only, no boolean type | §8, `fundamentals/04_control_flow.md` §8 |
| `//` single-line comments | C99 (already a common extension earlier) | `/* */` only | New in this file |
| Declarations anywhere in a block | C99 | All declarations required at the **top** of a block, before any statement | New in this file — a real K&R-vs-modern style difference |
| `restrict` | C99 | No aliasing-promise mechanism at all | `fundamentals/02_types.md` §6, `advanced/03_aliasing_and_optimization.md` §5 |
| Variable-length arrays (VLAs) | C99 (made optional in C11) | Array sizes must be compile-time constants | `fundamentals/02_types.md` §3 (`sizeof` at runtime) |
| Designated initializers | C99 | Positional struct/array initializers only | New in this file |
| Compound literals | C99 | No anonymous, inline-constructed objects | New in this file |
| `inline` functions | C99 | `static` functions or macros as the only "avoid call overhead" options | `preprocessing/01_macros.md` §9, `preprocessing/02_includes_and_header_design.md` §2 |
| `__func__` | C99 | No standard way to get the current function's name | `preprocessing/01_macros.md` §5 |
| `snprintf` | C99 | `sprintf` only (no bound on output length) | `01_BOOK_ANALYSIS.md`'s topic matrix |
| `va_copy` | C99 | No portable way to duplicate a `va_list`'s position | `libraries/08_stdarg.md` §8 |
| `0b` binary literals | **C23** | Octal/hex/decimal only | `fundamentals/02_types.md` §7 |
| `_Generic` (type-generic expressions) | C11 | No language-level type dispatch; macros or multiple named functions only | New in this file |
| Anonymous structs/unions | C11 | Every struct/union member needs an explicit sub-name | `memory/06_struct_union_layout.md` external refs |
| `static_assert`/`_Static_assert` (keyword since C23) | C11 | Only runtime `assert` (`libraries/06_assert_and_errno.md`) | `libraries/06_assert_and_errno.md` external refs |
| `_Alignas`/`_Alignof`/`<stdalign.h>` | C11 | The union-with-a-wide-member alignment trick (`memory/05_dynamic_memory.md` §4) only | `memory/07_alignment_padding_aliasing.md` §4 |
| `<threads.h>` (optional) | C11 | No standard threading at all — POSIX threads (`pthread`) only, outside ISO C entirely | Out of this vault's core scope |
| Two's complement **mandated** | **C23** | Sign-magnitude, one's complement, or two's complement all permitted | `memory/02_pointers.md` §5, §4 below |
| Old-style (K&R) function declarations **removed** | **C23** | Legal, if discouraged (`functions/01_functions_and_stack_frames.md` §1) | §4 below |
| `nullptr`, `typeof`, `#embed` | **C23** | None of these exist | §4 below |

**C17** is deliberately absent from most rows above — it was a bug-fix-only release with no
new language features, correcting defects in C11's text.

---

## 3. The handful that actually change how you write everyday code

`[External]` Most of the table is niche; these four are worth internalizing because
they change ordinary, daily style:

### Declarations anywhere in a block (C99)

```c
/* Historical C (what Book B teaches and uses throughout) */
void f(void) {
    int i;              /* ALL declarations must come first... */
    int total = 0;
    for (i = 0; i < 10; i++)
        total += i;
}

/* Modern C (C99+) */
void f(void) {
    int total = 0;
    for (int i = 0; i < 10; i++)   /* declare i right where it's used */
        total += i;
}
```

`[Synthesis]` This is the single most visible day-to-day style difference between K&R-era
code and modern code — and *why* every `for` loop in this vault's own later-phase examples
(Phase 9 onward) declares its loop variable inline, while early-phase examples following
Book B's own style declare it beforehand.

### Designated initializers (C99)

```c
struct point p = { .x = 3, .y = 4 };     /* named — order-independent, self-documenting */
int days[12] = { [0] = 31, [1] = 28, [11] = 31 };  /* sparse array init, rest zero-filled */
```

`[Synthesis]` Directly improves on `memory/06_struct_union_layout.md` §1's positional-only
struct initialization — especially valuable for structs with many members, where positional
initialization is fragile under reordering.

### Compound literals (C99)

```c
func((struct point){ .x = 1, .y = 2 });   /* an anonymous struct, constructed inline,
                                              with no separate named variable */
```

### `_Generic` (C11) — type-dispatch without a preprocessor hack

```c
#define abs_generic(x) _Generic((x), \
    int: abs, \
    long: labs, \
    float: fabsf, \
    double: fabs)(x)
```

`[Synthesis]` A direct, language-level answer to a problem `libraries/02_stdlib.md` §2
only partially addressed (`abs`/`labs` as separate, type-specific functions) — `_Generic`
lets one macro name dispatch to the correct type-specific function automatically, based on
the argument's actual type, resolved entirely at compile time.

---

## 4. C23's two mandates, and a crucial caveat

`[External, verified]` Two genuinely structural changes:

1. **Two's complement is now the *only* permitted signed-integer representation** —
   sign-magnitude and one's complement, both legal under every prior standard, are gone.
2. **Old-style (K&R, non-prototype) function declarations and definitions are removed
   entirely** — `functions/01_functions_and_stack_frames.md` §1's "legal but discouraged"
   verdict on this style is, as of C23, simply no longer legal at all.

### ⚠️ The mandate does **not** make signed overflow defined

`[External, a precise, easily-missed distinction]` Fixing the *representation* is not the
same as fixing *overflow behavior*. `[Synthesis]` C23 guarantees `INT_MIN`'s bit pattern is
now predictable across every conforming implementation — but `INT_MAX + 1` **remains
undefined behavior**, exactly as catalogued in `advanced/01_undefined_behavior.md` §3. This
is a genuinely common point of confusion worth heading off directly: *knowing* the
representation does not *license* relying on wraparound-on-overflow — the standard
committee deliberately kept these two questions separate.

### 🎯 Interview point
"Does C23 make signed integer overflow well-defined?" — **No** — it mandates two's
complement *representation* only; overflow itself is still undefined behavior. This is a
real, specific trap: someone who knows "C23 mandated two's complement" but not this caveat
will confidently give the wrong answer.

---

## 5. What to actually write today

`[Synthesis, practical guidance closing out the whole vault's language coverage]`

- Default to `<stdint.h>` types when exact width matters; plain `int`/`long` otherwise,
  exactly as both source books teach — don't reach for `int32_t` out of habit where the
  width genuinely doesn't matter.
- Use `<stdbool.h>`'s `bool` for boolean-intent variables; it compiles to the same thing as
  `int`, but documents intent.
- Declare loop variables inline (`for (int i = ...)`) unless targeting a pre-C99
  environment specifically (some embedded toolchains still matter here).
- Use designated initializers for any struct with more than two or three members.
- Reach for `restrict` in performance-sensitive function signatures where you can
  genuinely guarantee non-aliasing — and never where you can't (`advanced/03_aliasing_and_optimization.md`
  §5).
- Treat old-style (non-prototype) function declarations as already dead, not merely
  discouraged — C23 has formally removed them.

---

## 6. ⚠️ Misconceptions to unlearn now

- **"The books' C and 'modern C' are basically the same language with minor syntax
  additions."** Some differences are stylistic (`//` comments); others are structural
  (mandatory top-of-block declarations gone, old-style function declarations now illegal)
  — worth knowing which is which before assuming old code "obviously" still compiles (§2–3).
- **"C23 mandating two's complement means overflow now wraps predictably."** It does not
  — representation and overflow behavior are separate questions, and overflow remains
  undefined behavior even in C23 (§4).
- **"`_Generic` is C's version of C++ templates/overloading."** It's a compile-time
  *dispatch* mechanism over a fixed, explicitly-listed set of types — not a way to write
  genuinely generic code for arbitrary types the way templates allow (§3).

---

## 7. 🎯 Interview questions

**Easy**
- Which C standard introduced `<stdint.h>`'s exact-width integer types? → C99
  (`fundamentals/02_types.md` §8).
- Can you declare a loop variable inline in classic ANSI C (C89)? → No — C99 introduced
  declarations anywhere in a block; C89 requires them at the top (§3).

**Medium**
- What's the practical day-to-day difference between historical and modern C regarding
  variable declarations? → Historical C (C89) requires all of a block's declarations before
  any statement; modern C (C99+) allows declaring a variable anywhere, including inline in
  a `for` loop (§3).
- Why are designated initializers a genuine improvement over positional struct
  initialization, beyond convenience? → They're order-independent and self-documenting,
  so reordering a struct's members doesn't silently misassign values the way positional
  initialization would (§3).

**Hard**
- Does C23's two's-complement mandate make `INT_MAX + 1` defined behavior? → No —
  representation and overflow behavior are distinct; the mandate fixes bit-pattern
  predictability, but signed overflow remains undefined behavior even in C23 (§4).
- Why can't code written strictly to C89 rules (all declarations at a block's top, no
  `//` comments, old-style function definitions) be assumed to compile under a strict C23
  compiler? → C23 formally removed old-style (non-prototype) function declarations/
  definitions entirely — code using that specific C89-legal style is no longer legal C23,
  even though most of the rest of C89 remains valid (§2, §4).

**Systems-level**
- A legacy embedded codebase written against C89 needs a function made type-generic (one
  macro that dispatches to `abs`/`labs`/`fabsf`/`fabs` based on argument type) without
  moving the whole codebase to C11. What are the options, and what's lost without
  `_Generic`? → Without C11's `_Generic`, the options are a preprocessor-based dispatch
  using `__builtin_types_compatible_p` (a GCC extension, not portable ISO C,
  `[External]`), or simply providing separately-named functions and requiring callers to
  pick the right one explicitly — both strictly worse (compiler-specific, or less
  ergonomic) than C11's language-level solution (§3).

---

## 8. External references

- [cppreference — C23](https://en.cppreference.com/w/c/23) `[External]` — the complete,
  current changelog this file's C23 claims were checked against, including the full
  "Removed"/"Deprecated"/"New features" lists.
- [cppreference — C99](https://en.cppreference.com/w/c/99) `[External]` and
  [C11](https://en.cppreference.com/w/c/11) `[External]` — the equivalent changelogs for
  each earlier revision in this file's table.
- [GCC documentation — C language dialect options](https://gcc.gnu.org/onlinedocs/)
  `[External]` — `-std=c89`/`-std=c99`/`-std=c11`/`-std=c17`/`-std=c23`, for compiling
  against a specific standard version deliberately.

## 9. 🛠 Practice

1. Take any code example from an early phase of this vault written in Book B's own
   top-of-block-declaration style, and rewrite it with inline C99-style declarations —
   confirm both compile with `-std=c89` and `-std=c99` respectively (and that the C89
   version, as expected, rejects inline declarations).
2. Write a struct with 6+ members, initialize it once positionally and once with
   designated initializers, then deliberately reorder the struct's member declarations —
   observe which initialization silently breaks and which doesn't.
3. Implement the `_Generic`-based `abs_generic` macro from §3, and confirm it correctly
   dispatches to `abs`, `labs`, `fabsf`, and `fabs` for `int`, `long`, `float`, and
   `double` arguments respectively.
