# Control Flow

> **Phase 1 · Fundamentals.** Depends on: `03_expressions_and_operators.md` (every
> condition below is just an expression tested for zero/nonzero).
> Feeds into: `functions/01_functions_and_stack_frames.md` (Phase 2 — control flow inside
> a function is what a stack frame's instruction pointer is actually walking through).
> Sources: **Book B** ch.3 (primary — precise and complete), **Book A** ch.16.1 (the
> "logic is arithmetic" framing).

---

## 1. Statements and blocks — the recap

`[Book-derived, B]` An expression becomes a **statement** when followed by `;` — in C the
semicolon *terminates* a statement, it doesn't *separate* statements the way it does in
some other languages. `{ }` groups declarations and statements into a **block** (a
compound statement), which is syntactically equivalent to a single statement — this is
why every construct below can take either one bare statement or a brace-delimited block
after it. There's no semicolon after a block's closing `}`.

---

## 2. `if`-`else`

```c
if (expression)
    statement1
else
    statement2                 /* the else part is optional */
```

`[Book-derived, B]` `expression` is evaluated; nonzero → `statement1` runs; zero (and an
`else` exists) → `statement2` runs. Because `if` tests a numeric value, `if (expression)`
and `if (expression != 0)` are equivalent — which one reads better is a judgment call.

### ⚠️ Common bug — the dangling-else ambiguity

`[Book-derived, B]` Because `else` is optional, omitting it from a *nested* `if` creates a
genuine grammar ambiguity, resolved by a fixed rule: **`else` always binds to the nearest
preceding `else`-less `if`.**

```c
if (n > 0)
    if (a > b)
        z = a;
    else
        z = b;          /* indentation suggests this pairs with (n > 0) ... */
                         /* ... but the language pairs it with (a > b) instead! */
```

The indentation here is a lie the compiler doesn't see. If you actually want the `else` to
pair with the *outer* `if`, you must force it with braces:

```c
if (n > 0) {
    if (a > b)
        z = a;
}
else
    z = b;
```

This exact ambiguity is why deeply nested, brace-free `if` chains are a real, not merely
stylistic, hazard — Book B's own advice is blunt: *use braces when there are nested ifs.*

### `else if` chains

`[Book-derived, B]` A chain of `else if`s is the general way to write a multi-way decision;
conditions are tested in order, and the first true one wins — the rest are never evaluated
(this is the `if`-level analog of `&&`/`||` short-circuiting). A trailing bare `else`
(no condition) handles the "none of the above" case, and can be omitted or used for
defensive error-checking. Example — a corrected binary search (`[Book-derived, B, with a
transcription bug in the source PDF's `high = mid + 1` line fixed below — it should be
`high = mid - 1`, confirmed against the algorithm's own logic]`):

```c
/* binsearch: find x in v[0] <= v[1] <= ... <= v[n-1] */
int binsearch(int x, int v[], int n)
{
    int low, high, mid;
    low = 0;
    high = n - 1;
    while (low <= high) {
        mid = (low + high) / 2;
        if (x < v[mid])
            high = mid - 1;      /* search the lower half */
        else if (x > v[mid])
            low = mid + 1;       /* search the upper half */
        else
            return mid;          /* found it */
    }
    return -1;                   /* not found */
}
```

**Execution flow:** each iteration halves the search range by comparing `x` against the
midpoint; the loop invariant is "if `x` is in `v` at all, it's in `v[low..high]`" — when
`low > high`, that range is empty, so the search is done and `x` isn't present.

### 🛠 Debugging technique
The dangling-else trap and similar structural bugs are exactly why `-Wall` (or better,
`-Wall -Wextra`) matters: GCC/Clang will warn about a misleadingly-indented `if` under
`-Wmisleading-indentation`. `[External]`

---

## 3. `switch`

```c
switch (expression) {
case const-expr: statements
case const-expr: statements
default:         statements
}
```

`[Book-derived, B]` `switch` is a multi-way decision that tests an integer expression
against a set of **constant** integer values (each `case` label must be a compile-time
constant — this is *why* you can't `switch` on a `float` or on a runtime-computed value as
a case label). Execution jumps to the matching `case` and then **falls through** to
whatever comes next — cases are just labels, not separate scopes with implicit exits.

```c
switch (c) {
case '0': case '1': case '2': /* ... */ case '9':
    ndigit[c - '0']++;
    break;
case ' ': case '\n': case '\t':
    nwhite++;
    break;
default:
    nother++;
    break;
}
```

`[Book-derived, B]` `break` exits the `switch` immediately. Fall-through is a genuine
double-edged design: it lets several case labels share one action (as with the ten digit
labels above, or the three whitespace labels), but it also means **every** case normally
needs its own `break` to avoid unintentionally running into the next case's code — a
missing `break` is one of the most common real-world C bugs. Book B's own defensive habit:
put a `break` after the *last* case too (even the `default`), so that adding a new case
later doesn't silently fall into code that used to be safely terminal.

### ⚠️ Common bug — accidental fallthrough

Fallthrough is **not** undefined behavior and **not** a compiler bug when it happens — it's
exactly what the language specifies. The bug is almost always a *missing `break` the
programmer intended to write*. Because the compiler can't tell "intentional fallthrough"
from "forgot a break," modern compilers warn about it (`-Wimplicit-fallthrough`), and
C23/some compiler dialects support an explicit `[[fallthrough]]` annotation to mark
intentional cases. `[External]`

---

## 4. Loops — `while` and `for`

`[Book-derived, B]`

```c
while (expression)
    statement
```

`expression` is evaluated; nonzero → `statement` runs and `expression` is re-evaluated;
this repeats until `expression` is zero.

```c
for (expr1; expr2; expr3)
    statement
```

is equivalent to

```c
expr1;
while (expr2) {
    statement
    expr3;
}
```

**with one exception: `continue`'s behavior differs** — in the `for` form, `continue` still
runs `expr3` (the increment) before re-testing `expr2`; a naive hand-expansion to `while`
would skip `expr3` on a bare `continue`. This is a precise, easy-to-miss detail: `for` is
not *purely* syntactic sugar over `while`, it has one small semantic difference that only
shows up when `continue` is used (§6 below expands on this).

All three `for` clauses are optional (semicolons must remain); an absent `expr2` is treated
as permanently true, giving the idiomatic infinite loop:

```c
for (;;) {
    /* loop forever, presumably exited via break or return */
}
```

**When to reach for which:** `[Book-derived, B]` `for` is preferable when there's a simple
init+increment, because it keeps all the loop-control pieces visible together at the top
(`for (i = 0; i < n; i++)` is the idiomatic way to process the first `n` elements of an
array); `while` is more natural when there's no clean init/increment structure, e.g.
skipping whitespace: `while ((c = getchar()) == ' ') ;`.

---

## 5. Loops — `do`-`while`

```c
do
    statement
while (expression);
```

`[Book-derived, B]` Unlike `while`/`for`, which test **before** the first iteration,
`do`-`while` tests **after** — so the body always runs at least once. It's the least-used
of C's three loop forms, but it's the right tool whenever "run once, then maybe repeat" is
the actual logic, rather than "maybe run, then maybe repeat."

**Worked example** — `itoa` (integer to string) genuinely needs at-least-once semantics: it
must emit at least one digit even when converting `0`.

```c
/* itoa: convert n to characters in s (digits generated in reverse, then flipped) */
void itoa(int n, char s[])
{
    int i, sign;
    if ((sign = n) < 0)
        n = -n;
    i = 0;
    do {
        s[i++] = n % 10 + '0';   /* peel off the lowest digit each pass */
    } while ((n /= 10) > 0);
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);                  /* digits came out backward — flip before returning */
}
```

**Why `do`-`while` here specifically:** the digit-generation loop must run once even for
`n == 0` (producing the single digit `'0'`) — a `while`-based version (`while (n > 0)`)
would produce an *empty* string for `n == 0`, since the test-before-run semantics would
skip the body entirely.

### ⚠️ Common bug — mistaking `while (...)` for a new loop

Because `do`/`while` are two separate keywords with a statement in between, a reader
skimming code can misread the trailing `while (expression);` as the start of a *new*
`while` loop rather than the end of the `do`. This is exactly why Book B recommends bracing
even a single-statement `do`-`while` body — `do { ... } while (expr);` is visually
unambiguous in a way `do stmt; while (expr);` is not.

---

## 6. `break` and `continue`

`[Book-derived, B]` `break` exits the **innermost** enclosing loop or `switch` immediately
— it does not "see" outer loops. `continue` starts the **next iteration** of the innermost
enclosing loop; in `while`/`do`, that means jumping straight to the test; in `for`, it
means jumping to `expr3` (the increment step) and *then* the test. `continue` applies only
to loops, never to `switch` — a `continue` inside a `switch` that's itself inside a loop
continues the *loop*, not the `switch`.

```c
for (i = 0; i < n; i++)
    if (a[i] < 0)
        continue;           /* skip negative elements */
    /* ... process positive elements ... */
```

`continue` is most valuable exactly when the "rest of the loop body" is complex enough that
inverting the test and nesting another level would push the code too deep — it's a
readability tool, not just a control-flow primitive.

---

## 7. `goto` and labels

`[Book-derived, B]` C provides `goto` and labels (`identifier:`), whose scope is the entire
enclosing function. Book B is direct about this: `goto` is formally never *necessary* — any
`goto`-based control flow can be rewritten without one, typically at the cost of an extra
flag variable or repeated tests — and the book itself uses no `goto` anywhere in its
examples.

**The one situation where it earns its place:** breaking out of *multiple* nested loops at
once, which `break` alone cannot do (it only exits the innermost loop):

```c
for (/* ... */)
    for (/* ... */) {
        /* ... */
        if (disaster)
            goto error;
    }
/* ... */
error:
    /* cleanup */
```

The flag-variable alternative works but is arguably less clear:

```c
found = 0;
for (i = 0; i < n && !found; i++)
    for (j = 0; j < m && !found; j++)
        if (a[i] == b[j])
            found = 1;
```

`[Book-derived, B]` Book B's own conclusion, stated plainly: with a few exceptions like
multi-level loop exit, `goto`-based code is generally harder to understand and maintain
than code without it — use it rarely, if at all.

---

## 8. Cross-book synthesis: "truth" in C

`[Book-derived, A]` C has no dedicated boolean type in its classic form (C99's `_Bool` /
`<stdbool.h>` is a later, opt-in addition — see `fundamentals/02_types.md` §8). Instead,
**logic is arithmetic**: `0` means false, any nonzero value means true, and the relational/
logical operators themselves produce plain `int`s (`1` or `0`). Book A flags a specific
telltale sign of unfamiliarity with this model: code that `#define`s its own `TRUE`/`FALSE`
and assumes `TRUE` is always `1` — since *any* nonzero value is "true" in C, code that
compares against a hardcoded `TRUE` constant instead of just testing truthiness directly
can misbehave the moment a "true" value that isn't literally `1` shows up (e.g. from a
library function documented only to return "nonzero on success").

The classic `=`/`==` mixup (`if (c = 20)` silently assigns and is still "true") is covered
in depth in `03_expressions_and_operators.md` §10 — it belongs conceptually here too, since
its symptom always shows up as a control-flow statement doing the wrong thing.

---

## 9. ⚠️ Misconceptions to unlearn now

- **"Forgetting `break` in a `switch` is a compiler bug or undefined behavior."** It's
  neither — fallthrough is exactly what the standard specifies; the bug is always in the
  programmer's expectations, not the compiler's behavior (§3).
- **"`for` and `while` are 100% interchangeable, just different syntax."** Almost, but not
  quite — `continue`'s interaction with the increment step differs (§4/§6); a `for`-loop
  hand-expanded to `while` needs its increment step manually replicated *before* the
  re-test whenever a `continue` is reachable.
- **"`do { ... } while (0);` is a pointless, silly construct."** `[External]` It shows up
  constantly in real macro-heavy C precisely *because* it's a block that behaves like a
  single statement and correctly requires a trailing semicolon at the call site — this
  becomes very relevant in `preprocessing/01_macros.md` (Phase 2) when wrapping multi-
  statement macros safely.

---

## 10. 🎯 Interview questions

**Easy**
- What's the difference between `while` and `do`-`while`, in terms of when the condition
  is tested? → Before the first iteration vs. after (§5).
- Does a `switch` case automatically stop after its statements run? → No — execution falls
  through to the next case unless a `break` (or other jump) stops it (§3).

**Medium**
- Explain the dangling-`else` ambiguity and the rule C uses to resolve it. → `else` binds
  to the nearest preceding `else`-less `if`; braces are the only way to force a different
  association (§2).
- `for` is said to be "just sugar" over `while` — where does that equivalence actually
  break down? → `continue`'s effect on the increment step (§4).

**Hard**
- When, if ever, is `goto` the *right* tool in C, according to the source material, and why
  does that one case resist a clean rewrite with `break`/`continue` alone? → Breaking out
  of multiple nested loops at once; `break` only exits the innermost loop, and a
  `continue`/flag-variable rewrite (§7) trades clarity for avoiding `goto`, which is a
  real, debatable tradeoff rather than a strictly superior alternative.

**Systems-level**
- You're reviewing a PR that replaced a nested-loop `goto error;` cleanup pattern with a
  boolean flag and compound loop conditions (`while (!found && ...)`). What's actually
  gained and lost by this change? → Gained: no `goto`, arguably more "structured" control
  flow; lost: the flag variable and compound conditions run on every iteration of every
  loop level (a minor but real runtime cost) and can make the "why are we still looping"
  logic harder to read than a single `disaster`/`error:` jump, especially as nesting depth
  grows — this is a genuine style tradeoff, not a strict correctness improvement either way.

---

## 11. External references

- [cppreference — Selection statements](https://en.cppreference.com/w/c) `[External]` —
  current, precise grammar for `if`/`switch` including C23-era additions this file's
  book sources predate.
- [cppreference — Iteration statements](https://en.cppreference.com/w/c) `[External]` —
  same, for `while`/`do`/`for`.

## 12. 🛠 Practice

1. `[Book-derived, B, Exercise 3-2]` Write `escape(s, t)`, converting characters like
   newline and tab into visible escape sequences (`\n`, `\t`) as it copies `t` into `s`,
   using `switch`. Then write the inverse function, converting escape sequences back into
   real characters.
2. Reproduce the dangling-`else` bug from §2 verbatim, compile it, and confirm with `-Wall`
   that your compiler warns about the misleading indentation — then fix it with explicit
   braces and confirm the warning disappears.
3. `[Book-derived, B, Exercise 3-3]` Write `expand(s1, s2)`, expanding shorthand ranges like
   `a-z` in `s1` into the full `abc...xyz` in `s2`, handling both cases and digits, and
   edge cases like a leading/trailing `-` being taken literally rather than as a range.
