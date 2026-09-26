# Arrays & Strings

> **Phase 4 · Arrays, Strings, Structs & Unions.** Depends on: `memory/02_pointers.md`
> §3–4 (array decay, pointer arithmetic — this file is where that theory becomes daily
> practice).
> Feeds into: `06_struct_union_layout.md` (this phase — arrays of structs), `libraries/03_string.md`
> (Phase 7 — the `<string.h>` API built on the idioms here), `advanced/01_undefined_behavior.md`
> (Phase 11 — buffer overflow as a UB category).
> Sources: **Book B** §1.6, §5.5, §5.7–5.10 (primary), **Book A** ch.24/27 (string-literal
> framing, `<string.h>` overview).

---

## 1. Array layout: the concrete picture

`[Book-derived, B]` `int ndigit[10];` declares 10 consecutive `int`-sized objects,
subscripted `ndigit[0]` through `ndigit[9]` — **C array subscripts always start at 0**, and
a subscript can be any integer expression. Combined with `memory/02_pointers.md` §4's
scaling rule, the physical picture is exact and simple:

```
int a[4] = {10, 20, 30, 40};

address:   0x1000    0x1004    0x1008    0x100C
value:     [  10  ]  [  20  ]  [  30  ]  [  40  ]
             a[0]       a[1]      a[2]      a[3]

each element starts sizeof(int) = 4 bytes after the previous one — no gaps, no
reordering; array storage is guaranteed contiguous, in subscript order.
```

---

## 2. Multi-dimensional arrays — row-major order

`[Book-derived, B]` C's multi-dimensional arrays are "rectangular": `static char daytab[2][13]`
is genuinely a **2-row, 13-column table**, subscripted `daytab[row][col]` (never
`daytab[row,col]` — that's a comma expression, not a 2D subscript, and means something
different and almost certainly wrong). Internally, a 2D array is a one-dimensional array
whose elements are themselves arrays — **stored row by row**, so the *rightmost* subscript
varies fastest in memory order:

```
int a[2][3];      /* 2 rows, 3 columns */

address:  0x1000  0x1004  0x1008  0x100C  0x1010  0x1014
element:  a[0][0] a[0][1] a[0][2] a[1][0] a[1][1] a[1][2]
          └──────── row 0 ────────┘└──────── row 1 ────────┘
```

### ⚠️ Passing a 2D array to a function
`[Book-derived, B]` The parameter declaration **must** specify the number of columns — the
number of rows is irrelevant, because what's actually passed is a pointer to an array of
rows (each row itself an array of a fixed width). All three of these are equivalent
parameter declarations for a `2×13` table:

```c
void f(int daytab[2][13])   /* row count is decorative here — ignored */
void f(int daytab[][13])    /* idiomatic — only the column count matters */
void f(int (*daytab)[13])   /* the same thing spelled as an explicit pointer-to-array;
                                the parentheses are required (§3 of
                                memory/03_pointer_declarations_parsing.md) — without them,
                                int *daytab[13] is a completely different type: an array
                                of 13 pointers, not a pointer to a 13-element array */
```

**Only the first dimension of a multi-dimensional array parameter is ever "free"** — every
dimension after the first must be given, because the compiler needs every dimension but the
outermost to compute each row's byte offset via the scaling rule from `02_pointers.md` §4.

---

## 3. Arrays of pointers vs. true multi-dimensional arrays — a real distinction

`[Book-derived, B]` Given:

```c
int a[10][20];   /* a TRUE 2D array: 200 contiguous ints, one block */
int *b[10];       /* an array of 10 POINTERS: 10 pointer-sized slots, NOT initialized,
                      and NOT necessarily pointing at anything contiguous at all */
```

`a[3][4]` and `b[3][4]` are both syntactically legal — but structurally very different.
`a` sets aside exactly 200 `int`-sized slots in one contiguous block, computed by the
rectangular formula `20 * row + col`. `b` sets aside only 10 pointers; **each must be
separately assigned** to point somewhere (statically or in code) before use, and — this is
the real payoff — **the rows `b` points to don't have to be the same length**, or even
contiguous with each other at all. This is exactly the mechanism behind an array of
strings (`char *name[]`, `memory/02_pointers.md` §5): each string can be a different
length, because each `name[i]` is an independently-allocated/stored block, not a fixed-
width row of one giant rectangular array.

### 🎯 Interview point
"When would you use `char *name[]` over `char name[][N]`?" — Whenever the strings vary
meaningfully in length: the pointer-array form only pays for the space each string
actually needs, while the fixed-width 2D-array form wastes `N` bytes per row regardless of
how short most strings are, and hard-caps every string at `N-1` characters.

---

## 4. String literals vs. `char` arrays — a distinction with real teeth

`[Book-derived, B]` A string constant like `"I am a string"` is itself an array of
characters, automatically terminated with `'\0'` — its storage length is always one more
than the number of characters between the quotes. But **how you use it matters
enormously**:

```c
char amessage[] = "now is the time";   /* an ARRAY, initialized from the literal */
char *pmessage  = "now is the time";   /* a POINTER, pointing AT the literal */
```

`amessage` is its own, freshly-allocated array — sized to exactly fit the string plus its
terminator — and its characters may be freely modified; `amessage` itself always refers to
that same storage. `pmessage` is a plain pointer, initialized to point at the string
constant's storage — the pointer itself may later be reassigned to point elsewhere, **but
the result of modifying the pointed-to characters through `pmessage` is undefined**.

### ⚠️ Common bug — writing through a pointer to a string literal
`[Synthesis, connecting directly to `memory/01_memory_model.md` §2's text/read-only-data
region]` String literals are commonly placed in read-only memory by real implementations —
this is precisely *why* `pmessage[0] = 'N';` is undefined behavior rather than just "bad
style": on many systems it will crash outright (a write fault against a read-only page);
on others it may silently succeed and corrupt a literal that other code in the program
still expects to read unchanged (since identical string literals are sometimes shared/
pooled by the compiler — modifying one can corrupt every other use of that same text
elsewhere in the program). **If you need a modifiable string, always declare it as a
`char` array** (`char buf[] = "..."`, which copies into fresh, writable storage), never as
a plain pointer to a literal.

---

## 5. `sizeof` vs. `strlen` — genuinely different questions

`[Synthesis, directly answering the master brief's explicit request for this contrast]`
These are easy to conflate because both "measure" a string, but they measure fundamentally
different things:

```c
char s[] = "hello";

sizeof(s);     /* 6 — the ARRAY'S total storage: 5 letters + 1 terminator byte.
                  Computed entirely at COMPILE TIME (fundamentals/02_types.md §3);
                  costs nothing at run time. */
strlen(s);      /* 5 — the number of characters BEFORE the terminator, found by a
                   RUNTIME scan that walks memory one byte at a time until it hits
                   '\0' (libraries/03_string.md, Phase 7, covers its implementation). */
```

```
's[]' in memory:      'h'   'e'   'l'   'l'   'o'   '\0'
                        [0]   [1]   [2]   [3]   [4]   [5]
                                                       ↑
                              strlen stops counting HERE (5 characters counted)
                        └──────────── sizeof(s) counts all 6 bytes ────────────┘
```

**Why they diverge:** `sizeof` is a compile-time property of the *array's declared type* —
it has no idea, and doesn't care, what's actually stored in the array right now. `strlen`
is a runtime scan of the *actual contents* — it doesn't know or care how big the
surrounding array is, only where the first `'\0'` happens to be. This has a dangerous
consequence: **`strlen` on a `char` array with no `'\0'` anywhere inside it keeps reading
past the array's end** until it happens to hit a zero byte somewhere in adjacent memory —
undefined behavior, and a real, common bug source (`advanced/01_undefined_behavior.md`,
Phase 11, covers this as a specific UB category: "missing string terminator").

### ⚠️ Common bug — `sizeof` on a pointer, revisited
`[Cross-reference]` `memory/02_pointers.md` §3 already covered this, but it bears repeating
in the strings context specifically, since it's where it bites hardest: `sizeof` on a
`char *` parameter (a string passed into a function) gives the *pointer's* size, not the
string's length — you cannot recover a string's length via `sizeof` inside a function it
was passed into; only `strlen` (a runtime scan) or an explicitly-passed length works there.

---

## 6. The canonical pointer-based string idioms

`[Book-derived, B]` These exact patterns appear constantly in real C code; being able to
read them fluently (not just write them) is worth deliberate practice.

```c
/* strcpy, built up in three stages of idiomatic compression */

/* stage 1 — explicit loop */
void my_strcpy_v1(char *s, char *t)
{
    int i = 0;
    while ((s[i] = t[i]) != '\0')
        i++;
}

/* stage 2 — pointer version */
void my_strcpy_v2(char *s, char *t)
{
    while ((*s = *t) != '\0') { s++; t++; }
}

/* stage 3 — the idiom experienced C programmers actually write */
void my_strcpy_v3(char *s, char *t)
{
    while (*s++ = *t++)
        ;
}
```

**Why stage 3 works, precisely:** postfix `++` yields the *old* value before incrementing
(`fundamentals/03_expressions_and_operators.md` §4), so `*t++` fetches the character `t`
currently points to, *then* advances `t`; the fetched character is stored through the old
`s` position (`*s = ...`), and *then* `s` advances too. The stored character is also the
whole expression's value, which is what the `while` tests — so the loop naturally copies
every character up to and including the terminating `'\0'`, and stops exactly when that
`'\0'` (numerically zero, hence "false") has just been copied. The comparison against
`'\0'` in stage 2 is redundant precisely because "nonzero is true" already does the job
(`fundamentals/04_control_flow.md` §8) — stage 3 simply omits it.

### 🎯 Interview point
Being asked to "implement `strcpy`" is really being asked whether you can read/write stage
3 fluently, not stage 1 — interviewers use this idiom specifically because it compresses
several concepts (postfix `++` semantics, pointer arithmetic, truthiness) into one line,
and fluency here is a reasonable proxy for general C fluency.

---

## 7. Command-line arguments: `argc`/`argv` as pointers-to-strings in practice

`[Book-derived, B]` `main`'s two conventional parameters: `argc` (argument count, always
≥1 — `argv[0]` is the program's own invocation name by convention) and `argv` (a pointer
to an array of `char *`, one string per argument; `argv[argc]` is guaranteed to be a null
pointer, a sentinel marking the end without needing `argc` at all).

```c
/* echo, pointer-marching version */
int main(int argc, char *argv[])
{
    while (--argc > 0)
        printf("%s%s", *++argv, (argc > 1) ? " " : "");
    printf("\n");
    return 0;
}
```

**Execution flow:** `argv` starts pointing at `argv[0]` (the program name). `++argv`
(prefix) advances it to point at `argv[1]` *before* dereferencing, so `*++argv` fetches
`argv[1]` on the first iteration — skipping the program name automatically, without any
separate index variable. `--argc` counts down in lockstep, stopping the loop once every
real argument has been printed. This is the same "pointer as its own loop variable" style
as the `strcpy` idiom above, applied to an array of pointers instead of an array of `char`.

---

## 8. ⚠️ Buffer sizing and overflow — where this theory becomes a security issue

`[Book-derived, A]` A fixed-size `char` buffer combined with a function that doesn't
respect that size is one of the most consequential bug classes in C's history (the
canonical example: copying unbounded user/network input into a fixed local buffer). The
defensive pattern:

```c
char buffer[10];
/* ... */
strncpy(buffer, argv[1], sizeof(buffer));   /* copies AT MOST sizeof(buffer) bytes */
buffer[sizeof(buffer) - 1] = '\0';           /* force-terminate: strncpy does NOT
                                                 guarantee a terminator if the source
                                                 is >= sizeof(buffer) long */
```

`[Synthesis]` Note the idiom `sizeof(buffer)`, not a hardcoded `10` — this only works
because `buffer` is still a real array in the scope where `sizeof` is applied (§5/
`02_pointers.md` §3's decay warning: this exact pattern silently breaks if `buffer` were a
function parameter instead of a local array, since `sizeof` would then give the pointer's
size). Full treatment of `strcpy`-family safety and the standard library's own answers
(`strncpy`'s own footguns included) is `libraries/03_string.md`, Phase 7; full undefined-
behavior consequences of overflowing a buffer are `advanced/01_undefined_behavior.md`,
Phase 11.

---

## 9. ⚠️ Misconceptions to unlearn now

- **"`char *p = "hello";` and `char p[] = "hello";` are interchangeable."** They are not
  (§4) — one gives you a pointer to (likely read-only) storage; the other gives you your
  own writable array. Mixing them up is a real, common source of crashes.
- **"`sizeof` and `strlen` measure the same thing, more or less."** They measure genuinely
  different things by genuinely different means — one is a compile-time array-size fact,
  the other a runtime content scan (§5).
- **"A 2D array and an array of pointers are basically the same, just declared
  differently."** They have different memory layouts, different initialization
  requirements, and different flexibility (rows can vary in length for the pointer-array
  form, not for the true 2D form) (§3).

---

## 10. 🎯 Interview questions

**Easy**
- Do C array subscripts start at 0 or 1? → 0, always (§1).
- What's the difference between `sizeof(s)` and `strlen(s)` for `char s[] = "hi"`? →
  3 (compile-time array size, including the terminator) vs. 2 (runtime character count,
  excluding it) (§5).

**Medium**
- Why must a 2D array function parameter specify the column count but not the row count? →
  The compiler needs every dimension but the outermost to compute each row's byte offset
  via pointer-arithmetic scaling; the row count is never actually needed for that
  computation (§2).
- Why is modifying a string reached through `char *p = "literal";` undefined behavior? →
  String literals are commonly stored in read-only memory and may be shared/pooled across
  identical literals; writing through such a pointer can fault or silently corrupt
  unrelated code (§4).

**Hard**
- Explain precisely why `int a[10][20]` and `int *b[10]` both accept `a[3][4]`/`b[3][4]`
  syntax but have completely different memory layouts and initialization requirements. →
  `a` is one contiguous 200-`int` block addressed by the rectangular formula
  `20*row + col`; `b` is 10 independent pointer slots, uninitialized until explicitly
  assigned, each potentially pointing to a differently-sized (or entirely unrelated) block
  of memory (§3).
- Walk through exactly why `while (*s++ = *t++);` correctly copies a string including its
  terminator, using postfix-`++`'s precise semantics. → Each iteration fetches `*t` (the
  *old* value, before `t` advances), stores it through the *old* `s` position, then
  advances both pointers; the stored value is also the loop's test — the loop naturally
  stops the iteration *after* copying `'\0'`, since a stored `'\0'` makes the test false (§6).

**Systems-level**
- A function receives `char *buf` and its caller's `sizeof(original_array)` some other way
  (say, hardcoded). The function later does `strncpy(buf, src, sizeof(buf))`. What's wrong?
  → `buf` is a pointer parameter, not the original array — `sizeof(buf)` gives the
  pointer's size (4 or 8 bytes), not the real buffer's capacity, silently under- or
  over-sizing the copy; the real capacity must be passed in explicitly as a separate
  parameter (§5, §8).

---

## 11. External references

- [cppreference — Array declaration](https://en.cppreference.com/w/c) `[External]` — the
  current, precise grammar and decay rules.
- [CERT C — `STR` (strings) rules](https://wiki.sei.cmu.edu/confluence/display/c) `[External]`
  — a systematic catalogue of string-handling pitfalls, including buffer-overflow patterns
  beyond §8's single example.

## 12. 🛠 Practice

1. `[Book-derived, B, Exercise 5-8]` Add bounds checking to a `day_of_year`/`month_day`
   pair of functions built on a `static int daytab[2][13]` table (§2's worked example),
   using the leap-year test `year%4==0 && year%100!=0 || year%400==0`.
2. Write `my_strcpy` at all three stages of compression shown in §6 yourself, from a blank
   file, without looking — then compare against the shown versions.
3. Deliberately write the `char *p = "literal"; p[0] = 'X';` bug, and observe what your
   compiler/platform actually does with it (crash, silent corruption, or — on some
   permissive setups — apparent success, which is itself worth noting as *not* a safety
   guarantee).
4. Implement a small `argv`-driven command-line tool (e.g. a word/character counter taking
   a filename as `argv[1]`) using the pointer-marching `argc`/`argv` idiom from §7.
