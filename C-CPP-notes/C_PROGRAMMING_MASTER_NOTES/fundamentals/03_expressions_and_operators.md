# Expressions, Operators & Conversions

> **Phase 1 · Fundamentals.** Depends on: `02_types.md` (conversions below assume you know
> the type-size story).
> Feeds into: `04_control_flow.md` (control flow tests these expressions' truthiness),
> `advanced/01_undefined_behavior.md` (order-of-evaluation and overflow traps introduced
> here get their full catalogue there).
> Sources: **Book B** §2.5–2.12 (primary — this chapter is one of K&R's most precise), a
> little **Book A** ch.14/16.1 for beginner framing.

---

## 1. Arithmetic operators

`[Book-derived, B]` The binary arithmetic operators are `+ - * / %`. Integer `/` truncates
any fractional part; `%` (modulus) gives the remainder, so `x % y` is zero exactly when `y`
divides `x`. `%` cannot be applied to `float`/`double`. For **negative** operands, the
direction of truncation for `/` and the sign of the result for `%` were machine-dependent
under classic K&R-era C — `[External]` this was tightened by C99, which mandates truncation
toward zero (so `-7 / 2 == -3` and `-7 % 2 == -1`, defined exactly), removing what used to
be a real portability trap. If you're reading code written against the older assumption,
know that its "it works on my machine" negative-division behavior may not be guaranteed on
whatever standard it was actually compiled under.

`+`/`-` share one precedence level, lower than `* / %`, which is in turn lower than the
*unary* `+`/`-`. All of these associate left-to-right.

---

## 2. Relational & logical operators — and why short-circuiting matters

`[Book-derived, B]` The relational operators `> >= < <=` all share one precedence level;
just below them (still lower than arithmetic operators) are the equality operators `== !=`.
So `i < lim-1` parses as `i < (lim-1)`, as expected.

`&&` and `||` are evaluated **strictly left to right, and evaluation stops the instant the
overall truth value is known** — this is *short-circuit evaluation*, and it is not just an
optimization detail; real code depends on it for correctness. Book B's own example, from
its `getline` function:

```c
for (i = 0; i < lim-1 && (c = getchar()) != '\n' && c != EOF; ++i)
    s[i] = c;
```

This **must** check `i < lim-1` first — if the buffer is already full, the code must not go
on to call `getchar()` and consume a character it has nowhere to store. Reordering these
three conditions changes program behavior, not just style.

### 💡 Key insight
`!` converts a nonzero operand to `0` and a zero operand to `1`. `if (!valid)` and
`if (valid == 0)` are equivalent; which reads better is a judgment call, not a correctness
one.

---

## 3. Type conversions — the "usual arithmetic conversions"

`[Book-derived, B]` This is the single most important section in this file — it's the
formal rule behind everything you already do informally, like mixing `int` and `float` in
one expression. When an operator has operands of different types, they're converted to a
common type. The *general* principle: only "narrower → wider" conversions happen
automatically (e.g. `int` → `float` in `f + i`); conversions that could lose information
(a wider type into a narrower one) are legal but may draw a compiler warning — they are not
forbidden, just risky.

For operands with **no unsigned types involved**, the informal conversion ladder is:

```
long double  ─┐
              ├─ if either operand is this type, convert the other operand to match
   double     │
              │
   float      ┘
              │
 (char, short are first promoted to int — always, regardless of the other operand)
              │
    long      ─── if either operand is long, convert the other to long
```

`[Book-derived, B]` More precisely, stated as a checklist applied top to bottom:

1. If either operand is `long double`, convert the other to `long double`.
2. Else if either operand is `double`, convert the other to `double`.
3. Else if either operand is `float`, convert the other to `float`.
4. Else, convert any `char`/`short` operands to `int` (this step, called **integer
   promotion**, happens even in expressions with no floating-point type at all).
5. Then, if either operand is `long`, convert the other to `long`.

`[Book-derived, B]` **Important nuance:** unlike some other languages, a `float` is *not*
automatically widened to `double` just for being an operand — that conversion only happens
via the rules above when the *other* operand forces it. Math library functions
(`<math.h>`) generally expect `double`, which is why C code so often declares floating
variables as `double` by default and reaches for `float` specifically to save space in
large arrays.

When *unsigned* types are involved, the rules get materially trickier — comparisons between
signed and unsigned values become machine-dependent because they depend on the relative
sizes of the integer types in play (e.g., whether `int` and `long` are the same width).
Book B gives a concrete example: with a 16-bit `int` and 32-bit `long`, `-1L < 1U` is true
(because the `unsigned int` `1U` gets promoted to a *signed* `long`), but `-1L > 1UL` is
also true (because now `-1L` gets promoted to `unsigned long`, becoming a huge positive
number). **Mixing signed and unsigned types in a comparison is a real, live bug source, not
a theoretical one** — see the ⚠️ box below.

### 🧠 Mental model: why `char`/`short` always promote to `int`

`[Synthesis]` This rule — called *integer promotion* — exists because most CPUs don't have
efficient native instructions for arithmetic on sub-word-sized integers; it's simpler and
usually faster for the compiler to widen a `char`/`short` to the machine's natural `int`
size, do the arithmetic there, and only narrow back down if you explicitly assign the
result into a smaller variable. It's also *why* `s[i] - '0'` (subtracting two `char`s)
produces an `int` result, not a `char` — and why functions that conceptually take a
"character" (`getchar`, `isdigit`, …) are prototyped to take/return `int`.

### Worked example — `char` → `int` conversion in practice

`[Book-derived, B]`

```c
/* atoi: convert a digit string to its integer value */
int atoi(char s[])
{
    int i, n;
    n = 0;
    for (i = 0; s[i] >= '0' && s[i] <= '9'; ++i)
        n = 10 * n + (s[i] - '0');
    return n;
}
```

**Why this works:** `s[i] - '0'` relies on two facts guaranteed by the standard's execution
character set: the digit characters `'0'`…`'9'` are a *contiguous, increasing* sequence, so
subtracting the character code of `'0'` from any digit character yields exactly that
digit's numeric value (0–9) — and because both operands are promoted to `int` for the
subtraction (per the promotion rule above), the result is an ordinary integer, ready to
feed into `10 * n + ...`.

### ⚠️ Common bug — signed/unsigned comparison

```c
int i = -1;
unsigned int u = 1;
if (i < u)          /* you probably expect true — it is FALSE */
    printf("i is smaller\n");
```

Because `i` gets converted to `unsigned int` before the comparison (per the rules above),
`-1` becomes a huge positive number (`UINT_MAX`), so `i < u` is false. This is a genuine,
frequently-shipped bug pattern — it's *why* good compilers warn on `-Wsign-compare`, and
*why* loop conditions like `for (int i = n - 1; i >= 0; i--)` where `n` is `size_t`
(unsigned!) can infinite-loop: `n - 1` when `n == 0` wraps to a huge unsigned value instead
of going negative.

### Casts

`[Book-derived, B]` A cast, `(type) expression`, forces an explicit conversion using the
same rules as above, as if the expression had been assigned to a variable of that type.
Casts have the same high precedence as other unary operators. Typical legitimate use: `sqrt`
expects a `double`, so passing an `int n` requires `sqrt((double) n)` under a pre-prototype
mental model — with modern prototyped functions the compiler inserts this conversion for
you automatically at the call site, but the cast is still needed whenever *you* need to
force a conversion the compiler wouldn't otherwise perform (e.g. forcing integer division
to produce a fractional result: `(double) a / b`).

---

## 4. Increment & decrement (`++`, `--`) — prefix vs. postfix

`[Book-derived, B]` `++`/`--` may be prefix (`++n`) or postfix (`n++`); both increment `n`
by 1, but differ in the *value the expression produces*: `++n` increments first and yields
the new value; `n++` yields the old value, then increments. If `n` is 5, `x = n++` sets `x`
to 5 (and `n` becomes 6); `x = ++n` sets `x` to 6. Where only the side effect matters (not
the value), prefix and postfix are equivalent — but where the value *is* used, the choice
changes behavior, not just style.

**Worked example** — `squeeze`, which deletes all occurrences of a character `c` from a
string `s`, relies on postfix `++` to combine "use the current index" and "advance to the
next slot" in one expression:

```c
void squeeze(char s[], int c)
{
    int i, j;
    for (i = j = 0; s[i] != '\0'; i++)
        if (s[i] != c)
            s[j++] = s[i];   /* write at the OLD j, then advance j */
    s[j] = '\0';
}
```

`s[j++] = s[i]` is exactly equivalent to the more verbose `s[j] = s[i]; j++;` — this
"assign, then step" idiom (also seen in `strcat`-style copy loops) is one of the most
common real C idioms, and it only works because postfix `++` yields the pre-increment value.

⚠️ The increment/decrement operators can only be applied to an actual variable (an
*lvalue* — see `memory/01_memory_model.md`, Phase 3, for the precise definition); an
expression like `(i + j)++` is illegal.

---

## 5. Bitwise operators

`[Book-derived, B]` Six operators, applicable only to integral operands:

| Operator | Meaning |
|---|---|
| `&` | bitwise AND — commonly used to *mask off* bits |
| `\|` | bitwise inclusive OR — commonly used to *turn on* bits |
| `^` | bitwise exclusive OR |
| `<<` | left shift |
| `>>` | right shift |
| `~` | one's complement (unary — flips every bit) |

`x << n` shifts left by `n` positions, filling vacated bits with 0 — equivalent to
multiplying by `2ⁿ`. Right-shifting an **unsigned** quantity always fills vacated bits with
0. Right-shifting a **signed** quantity is implementation-defined: some machines fill with
copies of the sign bit ("arithmetic shift"), others with 0 ("logical shift") — so `x >> n`
on a negative signed `x` is not portable.

### ⚠️ Common bug — `&`/`|` are not `&&`/`||`

`x & y` and `x && y` are entirely different operators: the former does bitwise AND on the
full value, the latter does short-circuit logical AND, yielding only 0 or 1. If `x == 1`
and `y == 2`: `x & y` is `0` (no shared bits), but `x && y` is `1` (both nonzero, so
"true"). Confusing these is a classic beginner (and not-only-beginner) bug.

**Worked example** — extracting an `n`-bit field starting at bit position `p`:

```c
/* getbits: get n bits from position p, right-adjusted */
unsigned getbits(unsigned x, int p, int n)
{
    return (x >> (p + 1 - n)) & ~(~0 << n);
}
```

`x >> (p+1-n)` shifts the desired field down to the low end of the word. `~0` is all
one-bits; `~0 << n` puts `n` zero bits at the low end (ones everywhere else);
complementing that with the outer `~` produces a mask with exactly `n` one-bits at the low
end. AND-ing with that mask discards everything outside the field.

---

## 6. Assignment operators and expressions

`[Book-derived, B]` `i = i + 2` can be written `i += 2`; every binary operator
`+ - * / % << >> & ^ |` has a corresponding `op=` form. `expr1 op= expr2` is equivalent to
`expr1 = (expr1) op (expr2)` **except that `expr1` is evaluated only once** — which matters
when `expr1` has a side effect (e.g. `a[i++] += 1` increments `i` only once, not twice).

⚠️ Note the implied parentheses: `x *= y + 1` means `x = x * (y + 1)`, **not**
`x = x * y + 1` — the entire right-hand side is treated as one parenthesized operand.

Assignment is itself an expression with a value (the value assigned), which is why
`while ((c = getchar()) != EOF)` works: the assignment's value (the character just read) is
what gets compared against `EOF`. The parentheses around `c = getchar()` are required — `!=`
binds *tighter* than `=`, so without them the expression would parse as
`c = (getchar() != EOF)`, assigning `0` or `1` to `c` instead of the character read.

---

## 7. The conditional (ternary) operator

`[Book-derived, B]` `expr1 ? expr2 : expr3` evaluates `expr1`; if nonzero, evaluates and
yields `expr2` (and `expr3` is **not** evaluated at all); otherwise evaluates and yields
`expr3` (and `expr2` is skipped). Exactly one branch ever executes — this is not just style
sugar for `if`/`else`, the "only one side runs" guarantee is itself sometimes load-bearing
(e.g. `x != 0 ? 1/x : 0` safely avoids the division when `x` is zero). `?:` has very low
precedence, just above assignment.

---

## 8. Precedence and associativity (the full table)

`[Book-derived, B, reformatted]` Operators on the same row share precedence; rows are
listed from **highest to lowest** precedence.

| Operators | Associativity |
|---|---|
| `()` `[]` `->` `.` | left to right |
| `!` `~` `++` `--` `+` `-` `*` (deref) `&` (addr-of) `(type)` `sizeof` | **right to left** |
| `*` `/` `%` | left to right |
| `+` `-` | left to right |
| `<<` `>>` | left to right |
| `<` `<=` `>` `>=` | left to right |
| `==` `!=` | left to right |
| `&` | left to right |
| `^` | left to right |
| `\|` | left to right |
| `&&` | left to right |
| `\|\|` | left to right |
| `?:` | right to left |
| `=` `+=` `-=` `*=` `/=` `%=` `&=` `^=` `\|=` `<<=` `>>=` | **right to left** |
| `,` | left to right |

### ⚠️ Common bug — bitwise vs. equality precedence

Note that `&`, `^`, `|` sit **below** `==`/`!=` in precedence — a fact that regularly
surprises people. `if (x & MASK == 0)` does **not** do what it looks like; it parses as
`if (x & (MASK == 0))` because `==` binds tighter than `&`. Bit-testing code must be fully
parenthesized: `if ((x & MASK) == 0)`.

---

## 9. Order of evaluation is (mostly) unspecified

`[Book-derived, B]` C does **not** specify the order in which an operator's operands are
evaluated, with exactly four exceptions: `&&`, `||`, `?:`, and the comma operator `,` all
guarantee left-to-right evaluation (that's precisely what makes short-circuiting in §2
reliable). Everywhere else — including function-call argument lists — the order is up to
the compiler.

```c
x = f() + g();                          /* f() and g() may run in either order */

printf("%d %d\n", ++n, power(2, n));    /* WRONG — n's value at the power() call
                                            depends on unspecified evaluation order */
```

The fix for the second example is to force a sequence explicitly:

```c
++n;
printf("%d %d\n", n, power(2, n));
```

`[Synthesis]` This is your first direct exposure to a theme that becomes central in
`advanced/01_undefined_behavior.md`: **the standard deliberately leaves some things
unspecified so compilers can pick whatever order is fastest on their target machine** — and
code that silently depends on one particular order is not "usually fine," it's already
broken, just not always visibly. A stricter, more dangerous relative of this same idea is
`a[i] = i++;` — whether the write happens to the *old* or *new* value of `i` as the array
index is left to the compiler's discretion, and different compilers really do disagree.

---

## 10. ⚠️ Misconceptions to unlearn now

- **"`=` and `==` are basically interchangeable typos that the compiler will catch."**
  `[Book-derived, A]` They are not, and it will *not*, because `if (c = 20)` is perfectly
  legal C: it assigns 20 to `c` and the whole expression evaluates to 20 (nonzero → "true").
  A cheap defensive habit: writing the constant first (`if (20 == c)`) turns an accidental
  `=` into a compile error, since you can't assign to a literal.
- **"Operands are evaluated left to right, like in most other statements."** True for `&&`,
  `||`, `?:`, and `,` — **not** true in general (§9). Never write an expression whose
  correctness depends on argument-evaluation order.
- **"Signed and unsigned comparisons just compare the numbers."** They compare the numbers
  *after* the usual arithmetic conversions have silently converted the signed value to
  unsigned — see the worked example in §3.

---

## 11. 🎯 Interview questions

**Easy**
- What's the difference between `x++` and `++x` as *expressions* (not as statements)? → §4.
- What does `x % y` do for negative `x`? → Implementation-defined under classic C;
  truncation toward zero is mandated from C99 onward.

**Medium**
- Why does `if (x & MASK == 0)` not do what it looks like? → `==` binds tighter than `&`
  (§8); it parses as `x & (MASK == 0)`.
- Explain why `-1 < 1u` is false in C. → The usual arithmetic conversions convert `-1` to
  `unsigned int` before comparing, turning it into `UINT_MAX` (§3).

**Hard**
- Is `printf("%d %d\n", ++n, power(2, n));` well-defined? Why or why not? → No — function
  argument evaluation order is unspecified, so whether `power` sees the incremented or
  un-incremented `n` is compiler-dependent (§9).
- `a[i] = i++;` — what's actually wrong with this line, precisely? → Whether the array
  index uses the pre- or post-increment value of `i` is unspecified by the standard;
  different compilers can and do produce different results (§9), which is why this pattern
  should never appear in real code regardless of what any one compiler currently does with it.

**Systems-level**
- A junior engineer writes `for (size_t i = n - 1; i >= 0; i--)` to iterate an array
  backward, where `n` is a `size_t`. What happens when `n == 0`, and why? → `size_t` is
  unsigned, so `n - 1` when `n == 0` wraps around to `SIZE_MAX` rather than going negative
  (well-defined unsigned wraparound, §3 + `02_types.md`), and `i >= 0` is then *always*
  true for an unsigned `i` — the loop runs effectively forever (or until it walks off
  valid memory), a real and common bug in array-backward-iteration code.

---

## 12. External references

- [cppreference — Operator precedence](https://en.cppreference.com/w/c) `[External]` — a
  complete, current precedence/associativity table including operators outside this file's
  scope (member access, `sizeof`, casts covered elsewhere in this vault).
- [cppreference — Usual arithmetic conversions](https://en.cppreference.com/w/c) `[External]`
  — the exact, current standard wording of the conversion ladder in §3, including the
  unsigned-involved cases this file only summarizes.

## 13. 🛠 Practice

1. `[Book-derived, B, Exercise 2-9]` In two's-complement, `x &= (x - 1)` clears the
   rightmost 1-bit of `x`. Work out *why* on paper, then use it to write a faster
   population-count (`bitcount`) function than the naive shift-and-test loop.
2. Compile and run `printf("%d %d\n", ++n, power(2, n));` with two different compilers (or
   two optimization levels of the same compiler) and see whether the output actually
   differs — this makes §9's warning concrete rather than theoretical.
3. Write `if (i < u)` with `int i = -1; unsigned u = 1;`, print the branch taken, and then
   explain the result using the conversion ladder in §3 rather than just observing it.
