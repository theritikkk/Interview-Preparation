# The Preprocessor: Macros

> **Phase 2 · Functions, Scope & the Preprocessor.** Depends on:
> `fundamentals/01_program_structure.md` §3 (the preprocessor is a distinct, syntax-blind
> text-substitution pass — this file assumes that mental model).
> Feeds into: `02_includes_and_header_design.md`, `03_conditional_compilation.md` (all
> three preprocessing files split one K&R section, §4.11, along natural topic lines).
> Sources: **Book B** §4.11.2 (primary — precise, with the exact classic pitfalls), **Book A**
> ch.18 (predefined macros, debugging patterns).

---

## 1. Object-like macros: `#define` as text substitution

`[Book-derived, B]` The simplest form:

```c
#define name replacement-text
```

Every subsequent occurrence of the token `name` is replaced by `replacement-text`, exactly
as `fundamentals/01_program_structure.md` §3 already warned: this is pure text
substitution, done **before** the compiler sees any C syntax. `name` follows identifier
naming rules; the replacement text is arbitrary and normally runs to the end of the line —
a long definition can continue across lines with a trailing `\`. The macro's scope runs
from its definition to the end of the file (or an `#undef`, §4). A definition may reference
previously-defined macros. Substitution happens only for whole tokens, and **never** inside
a quoted string — `#define YES 1` does not touch `printf("YES")` or the identifier
`YESMAN`.

```c
#define forever for (;;)    /* a new "keyword" for an infinite loop */
```

---

## 2. Function-like macros — and their real pitfalls

`[Book-derived, B]` A macro can take arguments, making its expansion vary per call site:

```c
#define max(A, B) ((A) > (B) ? (A) : (B))
```

`max(p+q, r+s)` expands, by pure textual substitution of the actual arguments for the
formal parameters `A`/`B`, to:

```c
((p+q) > (r+s) ? (p+q) : (r+s))
```

This *looks* like a function call but is inlined text — which is exactly why it works
identically for `int`, `double`, or any comparable type without needing separate `max`
functions per type (a real advantage over functions, pre-C++-templates/C11-`_Generic`).

### ⚠️ Pitfall 1 — arguments get evaluated as many times as they textually appear

```c
max(i++, j++)       /* WRONG */
```

Expands to `((i++) > (j++) ? (i++) : (j++))` — **whichever operand "wins" the comparison
gets incremented a second time**, because it's textually repeated in the expansion. A real
function would evaluate each argument exactly once, no matter how many times the parameter
name appears in the function body; a macro has no such guarantee. **Never pass an
expression with side effects (`++`, `--`, assignment, I/O) to a macro whose parameter
appears more than once in its expansion.**

### ⚠️ Pitfall 2 — missing parentheses break operator precedence

```c
#define square(x)   x * x      /* WRONG */
```

`square(z + 1)` expands to `z + 1 * z + 1`, **not** `(z+1) * (z+1)` — ordinary operator
precedence applies to the expanded text, and `*` binds tighter than the `+` the caller
wrote. The fix is to parenthesize **both** the whole replacement **and** every individual
parameter use:

```c
#define square(x)   ((x) * (x))
```

`[Synthesis]` This is exactly the `program_structure.md` §3 misconception made concrete:
because the preprocessor doesn't understand C grammar, it has no concept of "the caller's
argument is one expression that needs grouping" — the macro author has to manually build
in the parenthesization a real function gets for free from the language's own call syntax.

### 💡 Key insight — why the standard library leans on macros anyway
`[Book-derived, B]` Despite these traps, macros are genuinely valuable where function-call
overhead matters: `getchar`/`putchar` are commonly implemented as macros in `<stdio.h>`
specifically to avoid a real function call per character, and the `<ctype.h>` classification
functions (`isdigit`, `isspace`, etc.) are usually macros for the same reason. Full
implementation-level treatment of exactly *how* those macros avoid the pitfalls above is in
`libraries/01_stdio.md` and `libraries/04_ctype_and_locale.md` (Phase 7), drawing on **Book
C**'s implementation chapters.

---

## 3. Stringification (`#`) and token pasting (`##`)

`[Book-derived, B]` Two special operators, usable only inside a macro's replacement text:

- **`#parameter`** turns the actual argument into a quoted string literal (with any
  internal `"` or `\` automatically escaped so the result is a legal string constant):

```c
#define dprint(expr)   printf(#expr " = %g\n", expr)

dprint(x/y)
/* expands to: printf("x/y" " = %g\n", x/y);            */
/* adjacent string literals concatenate (see fundamentals/02_types.md §7), giving: */
/* printf("x/y = %g\n", x/y);                                                       */
```

  This is a genuinely clever, self-documenting debug-print idiom: it prints both the
  *source text* of an expression and its runtime value from one macro invocation.

- **`##`** concatenates two tokens during expansion — if a parameter in the replacement
  text is adjacent to `##`, the parameter is replaced by its actual argument, the `##` and
  any surrounding whitespace vanish, and the result is re-scanned as a single new token:

```c
#define paste(front, back)   front ## back
paste(name, 1)      /* produces the single token: name1 */
```

`[Book-derived, B]` The rules for *nested* `##` uses get genuinely obscure; treat this
handbook's coverage as the practical 90% case and consult the standard/cppreference
directly if you hit an edge case.

---

## 4. `#undef`

`[Book-derived, B]` Removes a macro definition, most often used defensively to guarantee a
name is a real function rather than a macro at a particular point:

```c
#undef getchar
int getchar(void) { /* ... */ }
```

---

## 5. Predefined macros

`[Book-derived, A]` The standard (and common compilers) predefine several always-available
macros, mainly useful for diagnostics and debugging:

| Macro | Expands to |
|---|---|
| `__FILE__` | current source file name, as a string literal |
| `__LINE__` | current line number, as an integer literal |
| `__DATE__` / `__TIME__` | compilation date/time, as strings |
| `__func__` | the enclosing function's name, as a string (C99) |
| `__STDC_VERSION__` | which C standard the compiler is targeting `[External, not in Book A's list but standard]` |

### 🛠 Debugging technique
A classic use is a self-locating assertion/logging macro:

```c
#define TRACE(msg) fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, msg)
```

— every call site reports *where* it was called from, for free, because `__FILE__`/`__LINE__`
are re-substituted at each expansion site, not evaluated once globally.

---

## 6. 🧠 A macro-safety idiom worth knowing: wrapping multi-statement macros

`[External]` (Foreshadowed in `functions/01_functions_and_stack_frames.md`'s cross-reference
— not covered by either source book directly, but directly relevant here.) A macro that
expands to *multiple* statements is dangerous if used as the body of an `if` without braces:

```c
#define LOG_AND_RETURN(x)  log(x); return;

if (error)
    LOG_AND_RETURN("oops")     /* expands to TWO statements — only the first is
                                   controlled by the if! The return always runs. */
```

The standard idiom for safely bundling multiple statements into something that behaves like
*one* statement (requiring, and syntactically demanding, a trailing semicolon) is:

```c
#define LOG_AND_RETURN(x)  do { log(x); return; } while (0)
```

`do { ... } while (0)` runs its body exactly once and, as a single `do`-`while`
*statement*, correctly absorbs the trailing `;` at the call site without leaving a stray
empty statement or breaking an enclosing `if`/`else` — see
`fundamentals/04_control_flow.md` §9 for this same idiom flagged from the control-flow side.

---

## 7. 🔬 Under the hood — X-Macros (bonus pattern)

`[Book-derived, A]` A less common but genuinely useful technique: put a list of similar
macro invocations in their own header (conventionally `.def`), then `#include` that file
repeatedly, each time with a different definition of the component macro, `#undef`-ing and
redefining between inclusions:

```c
/* colors.def: */
#define COLOR(name) name

/* usage: */
#define COLOR(name) name,
enum Color { COLOR(RED) COLOR(GREEN) COLOR(BLUE) };
#undef COLOR

#define COLOR(name) #name,
const char *color_names[] = { COLOR(RED) COLOR(GREEN) COLOR(BLUE) };
#undef COLOR
```

The payoff: a single source-of-truth list (the `.def` file, or a simple macro-call list)
drives multiple generated artifacts (an enum, a name table, …) without manually keeping
them in sync — a real, if niche, technique you'll recognize in some codebases even if you
rarely reach for it yourself.

---

## 8. ⚠️ Misconceptions to unlearn now

- **"A macro argument behaves like a function argument — evaluated once, then used."**
  False — it's substituted as raw text everywhere its parameter name appears in the
  replacement (§2, Pitfall 1). Side-effecting arguments are a real hazard.
- **"Parenthesizing the macro's overall replacement text is enough."** Not sufficient —
  every individual parameter use inside the replacement needs its own parentheses too, or
  precedence from the call site can still leak in (§2, Pitfall 2).
- **"`#define X Y` is basically a `const` variable."** No — `X` has no type, no address,
  and no scope in the C sense; it's gone entirely by the time the compiler runs (reinforcing
  `program_structure.md` §3).

---

## 9. 🎯 Interview questions

**Easy**
- What does `#define PI 3.14159` actually do to the source text? → Textually replaces every
  subsequent token `PI` with `3.14159`, before compilation (§1).
- What's the difference between `#` and `##` in a macro? → `#` stringifies a parameter;
  `##` concatenates two tokens into one (§3).

**Medium**
- Why does `#define square(x) x*x` break on `square(z+1)`? → No parentheses around `x` in
  the replacement — the caller's `+` and the macro's `*` collide via ordinary precedence on
  the expanded text (§2).
- Why is `max(i++, j++)` dangerous? → `i++`/`j++` are textually repeated in the expansion,
  so one of them gets incremented twice (§2).

**Hard**
- Why is `do { ... } while (0)` the standard way to wrap a multi-statement macro, instead
  of plain braces `{ ... }`? → Braces alone leave a stray trailing semicolon issue and can
  desync an `if`/`else` pairing at the call site; `do`-`while(0)` is a single genuine
  statement that correctly demands and absorbs a trailing `;` (§6).

**Systems-level**
- A teammate proposes replacing a small, hot, frequently-called function with a
  function-like macro for performance. What do you actually gain, and what new bug classes
  does the change introduce? → Gain: avoided call overhead (real for tiny, extremely hot
  functions, though a modern compiler may already inline the function). New risk: every
  pitfall in this file — double-evaluation of side-effecting arguments, precedence leaks
  without careful parenthesization, no type checking at all (a macro accepts any argument
  that parses, silently) — versus a real function's argument-evaluated-once and
  type-checked guarantees. In most modern code, `static inline` functions (`[External]`,
  C99) get the same performance benefit with none of these hazards.

---

## 10. External references

- [cppreference — Preprocessor](https://en.cppreference.com/w/c) `[External]` — the current,
  precise rules for macro expansion, including edge cases this file's practical treatment
  simplifies.
- [GCC documentation — the C preprocessor (cpp)](https://gcc.gnu.org/onlinedocs/) `[External]`
  — GCC-specific preprocessor extensions and diagnostics.

## 11. 🛠 Practice

1. `[Book-derived, B, Exercise 4-14]` Define a macro `swap(t, x, y)` that exchanges two
   values of type `t`, using a block to declare a temporary without leaking its name into
   the caller's scope.
2. Deliberately write both pitfall macros from §2 (`square(x) x*x` and a double-evaluating
   `max`), trigger each bug with a concrete call site, and then fix both.
3. Write a `TRACE`-style debug macro using `__FILE__`/`__LINE__`/`__func__`, and confirm
   each call site reports its own correct location rather than one fixed location.
