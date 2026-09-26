# Parsing Complicated Declarations

> **Phase 3 · Pointers & Memory.** Depends on: `02_pointers.md` §1 ("declarations mimic
> use") and §6 (`void *`).
> Feeds into: `functions/03_function_pointers_and_variadics.md` (usage patterns beyond
> syntax — callbacks, comparator tables), `memory/06_struct_union_layout.md` (Phase 4 —
> `typedef` is the practical escape hatch from declarations like these).
> Sources: **Book B** §5.11–5.12 (primary — the canonical treatment, including the exact
> grammar), **Book A** §25.9 (the graduated example ladder).

---

## 1. Why C declaration syntax gets hard

`[Book-derived, B]` Book B is candid about this: C's declaration syntax is an attempt to
make a declaration's *shape* mirror how the declared name is *used* in an expression
(§1 of `02_pointers.md`) — this works cleanly for simple cases and gets genuinely confusing
for compound ones, because declarations can't always be read strictly left to right, and
parentheses end up doing a lot of structural work. The canonical example of exactly how
easy this is to get wrong:

```c
int *f();     /* f: a FUNCTION returning a pointer to int */
int (*pf)();  /* pf: a POINTER to a function returning int */
```

These look almost identical and mean completely different things. The reason: `()` (the
"is a function" marker) and `[]` (the "is an array" marker) both bind **tighter** than the
prefix `*` ("is a pointer to"). Without parentheses forcing a different grouping, `*` always
ends up applying to whatever the `()`/`[]` already produced, not to the bare name — so
`int *f()` groups as `int *(f())` ("`f` is a function; call it, then treat the result as
`int *`"), while `int (*pf)()` uses explicit parentheses to force `*` to bind to `pf`
*before* the `()` is applied, giving "`pf` is a pointer; the thing it points to is a
function returning `int`."

---

## 2. The one principle underlying everything below

`[Book-derived, B]` Every technique in this file is one consequence of a single fact,
already introduced in `02_pointers.md` §1: **a declaration is written to look like the
expression that would produce the base type**, given valid data. `int *p;` looks like
`*p`, which is an `int` — so `p` is a pointer to `int`. Everything else is just this
principle applied recursively, with parentheses managing precedence exactly as they do in
ordinary expressions.

---

## 3. A practical reading algorithm ("inside-out" / right-left)

`[External]/[Synthesis] — this specific step-by-step algorithm is a widely-taught practical
technique, not verbatim from any of the three source books; it's built to produce results
consistent with Book B's own grammar-based description in §5.12, which this section
paraphrases rather than reproduces in full.` Book B's own approach (§5.12) is to define a
small formal grammar and parse declarations against it with a recursive-descent program;
that's precise but heavyweight to apply by hand. The following manual procedure reaches the
same answers faster:

1. **Find the identifier being declared.** Start there.
2. **Look immediately to its right first.** If you see `()`, it's "a function taking
   \[those arguments\]"; if you see `[N]`, it's "an array of `N` \[elements\]." Consume
   that.
3. **Then look immediately to its left.** If you see `*`, it's "a pointer to \[whatever
   comes next\]." Consume that.
4. **Repeat, alternating right then left**, until you hit either the base type or a
   parenthesis.
5. **A parenthesis around part of the declarator forces that part to be resolved as a
   unit** before continuing outward — exactly the same role parentheses play in an
   ordinary arithmetic expression.
6. Read the pieces you collected in the order you collected them, substituting the base
   type at the end.

### Worked example 1 — `int *p;`

`p` → nothing to its right → look left: `*` → "pointer to" → base type `int`.
**Read: `p` is a pointer to `int`.**

### Worked example 2 — `int **p;`

`p` → nothing right → left: `*` → "pointer to" → left again: `*` → "pointer to" → `int`.
**Read: `p` is a pointer to a pointer to `int`.**

### Worked example 3 — `int (*p)[10];`

`p` is inside `(*p)`, so resolve that parenthesized part first: right of `p` inside the
parens is nothing, left is `*` → "pointer to." Now step *outside* the parens: to the right
is `[10]` → "array of 10 \[of whatever the parenthesized part was a pointer to\]." Base
type `int`. **Read: `p` is a pointer to an array of 10 `int`s.**

Contrast this immediately against the *unparenthesized* sibling:

### Worked example 4 — `int *p[10];` (no parentheses this time)

`p` → right first (higher precedence than the left-side `*`): `[10]` → "array of 10" →
then left: `*` → "pointer to \[each element\]" → `int`. **Read: `p` is an array of 10
pointers to `int`.**

`[Book-derived, B]` This exact pair — `int (*daytab)[13]` ("pointer to array[13] of int")
versus `int *daytab[13]` ("array[13] of pointer to int") — is Book B's own headline example
of why the parentheses matter so much: swapping which side of `*` the parenthesis falls on
completely changes the type.

### Worked example 5 — `int *(*p)(int);`

Resolve the innermost parens first: `p` → right: nothing inside `(*p)` → left: `*` →
"pointer to." Step outside: right is `(int)` → "function taking an `int` argument,
returning \[the pointer-to thing from the inner parens\]." Then continue left of that whole
`(*p)(int)` unit: `*` → "pointer to" → base type `int`.
**Read: `p` is a pointer to a function that takes an `int` and returns a pointer to `int`.**

### Worked example 6 — `char *(*fp)(char *);`

Same shape as example 5, with `char` as the base type and an explicit `char *` argument
instead of `int`: **`fp` is a pointer to a function that takes a `char *` and returns a
`char *`.** (This is, not coincidentally, a plausible real signature — a string-
transforming function pointer, e.g. for a callback that takes and returns a string.)

---

## 4. More worked examples, book-derived

`[Book-derived, B, §5.12]` Book B's own `dcl` utility (a small recursive-descent parser it
builds specifically to automate the algorithm in §3) is demonstrated against these:

| Declaration | Reading |
|---|---|
| `char **argv` | `argv`: pointer to (pointer to `char`) |
| `int (*daytab)[13]` | `daytab`: pointer to (array[13] of `int`) |
| `int *daytab[13]` | `daytab`: array[13] of (pointer to `int`) |
| `void *comp()` | `comp`: function returning (pointer to `void`) |
| `void (*comp)()` | `comp`: pointer to (function returning `void`) |
| `char (*(*x())[])()` | `x`: function returning (pointer to (array[] of (pointer to (function returning `char`)))) |
| `char (*(*x[3])())[5]` | `x`: array[3] of (pointer to (function returning (pointer to (array[5] of `char`)))) |

`[Synthesis]` The last two rows are exactly the kind of "truly complicated" declaration
Book B itself calls rare in practice — included here so the algorithm in §3 is proven
against the hardest cases the source material offers, not because you should expect to
write (or even often read) anything this nested in real code.

`[Book-derived, A]` For more graduated practice, Book A's own ladder builds up the same way,
one layer of indirection/array/function at a time:

```c
int i;            /* plain int */
int *p;            /* pointer to int */
int a[];            /* array of int */
int f();             /* function returning int */
int **pp;             /* pointer to pointer to int */
int (*pa)[];           /* pointer to array of int */
int (*pf)();             /* pointer to function returning int */
int *ap[];                 /* array of pointer to int */
int *fp();                  /* function returning pointer to int */
int ***ppp;                   /* pointer to pointer to pointer to int */
```

Work through each line with the §3 algorithm yourself before checking the pattern — that
repetition is what actually builds the reading skill; recognizing the shape from having
seen it once is not the same as being able to parse a *new* declaration under exam or
interview pressure.

---

## 5. Function pointers in practice, not just in syntax

`[Book-derived, B]` Syntax alone doesn't show *why* function pointers matter. Book B's own
motivating case: a generic sort routine that can sort by *any* criterion, by taking the
comparison logic as a parameter instead of hard-coding it:

```c
int (*comp)(void *, void *);   /* comp: pointer to a function taking two void*
                                    arguments and returning int */
```

Reading this with §3: `comp` → left: `*` → "pointer to" → right (stepping back out past the
parens): `(void *, void *)` → "function taking two `void *` arguments" → returning `int`
(the base type). **A generic sort function accepts one of these as a parameter**, and calls
`(*comp)(a, b)` internally wherever it needs to compare two elements — meaning the *same*
sort routine can sort strings lexicographically, strings numerically, or any other data
type entirely, just by being handed a different comparison function. This is precisely the
mechanism behind the standard library's own `qsort` (`libraries/02_stdlib.md`, Phase 7) and
is the general **callback pattern**: passing behavior, not just data, as a function
argument. `[Synthesis]`

### ⚠️ Common bug — forgetting the parentheses on a function-pointer declaration

`[Book-derived, B]`

```c
int *comp(void *, void *);   /* WRONG (probably) — this declares comp as a FUNCTION
                                 returning int *, not a pointer to a function */
int (*comp)(void *, void *);  /* what was almost certainly intended */
```

This is exactly example 4 from §1 recurring in a function-pointer-specific context — worth
internalizing precisely because it's such an easy typo to make and the compiler will
happily accept the wrong version without complaint (it's syntactically valid C, just a
different type than intended).

---

## 6. `typedef` — the practical escape hatch

`[Book-derived, B]` Book B's own advice for genuinely gnarly declarations: build them up in
small steps using `typedef` (full `typedef` treatment belongs to
`memory/06_struct_union_layout.md`, Phase 4, where it's used constantly for struct types —
this is a forward pointer, not the complete story). A quick preview relevant here:

```c
typedef int (*CompareFn)(void *, void *);

CompareFn comp;              /* now reads as "comp is a CompareFn" — far more legible
                                 than the raw declarator form in §5 */
```

`[Synthesis]` This doesn't change what's actually declared — `comp` still has exactly the
type `int (*)(void *, void *)` — it just gives that type a name, so every *subsequent*
declaration, parameter, or return type using it can be written and read as an ordinary
identifier instead of re-deriving the full declarator shape each time. Real-world code with
non-trivial function-pointer types almost always reaches for a `typedef` exactly for this
readability reason.

---

## 7. ⚠️ Misconceptions to unlearn now

- **"Declarations read left to right, like most other C syntax."** They don't, in general
  — `()`/`[]` bind before the prefix `*`, which is why the right-then-left algorithm in §3
  (not a simple left-to-right scan) is necessary.
- **"Parentheses around a pointer declarator are just a style preference."** They change
  the type entirely — `int (*daytab)[13]` and `int *daytab[13]` are genuinely different
  types (§3), not two spellings of the same thing.
- **"Function pointer syntax is a rare, exotic corner of C I'll never actually need."**
  Callback-based generic code (`qsort`-style comparators, event handlers, dispatch tables)
  is common in real systems code; `typedef` (§6) is what makes it tolerable to work with day
  to day, but the underlying syntax in §5 is worth being genuinely fluent in.

---

## 8. 🎯 Interview questions

**Easy**
- What does `int *p;` declare? → `p` is a pointer to `int` (§2).
- What's the difference between `int *daytab[13]` and `int (*daytab)[13]`? → Array of 13
  pointers to `int`, versus pointer to a 13-element array of `int` (§3).

**Medium**
- Walk through parsing `int (*p)(int);` step by step. → `p` is inside parens with `*` to
  its left ("pointer to"); outside the parens, `(int)` says "function taking an `int`
  argument"; base type `int` is the return type. Read: `p` is a pointer to a function
  taking an `int` and returning `int` (§3, worked example 5's simpler cousin).
- Why does `int *f();` not declare a pointer to a function? → `()` binds tighter than the
  prefix `*`, so this groups as "`f` is a function returning `int *`," not "`f` is a
  pointer to a function" (§1).

**Hard**
- Parse `char (*(*x())[])()` fully, narrating each step. → `x` is a function (from the
  outer `()`) returning a pointer (from the `*` just inside) to an array (from `[]`) of
  pointers (the next `*`) to functions (the final `()`) returning `char` (§4) — reading
  outward from `x`, alternating right/left exactly per the §3 algorithm, with the explicit
  parens marking where each "pointer to" binds before the next `()`/`[]` is applied.
- Why does `typedef` not change a declaration's actual meaning, only its readability? →
  A `typedef` is purely a name for an existing type — `CompareFn comp;` and
  `int (*comp)(void *, void *);` declare `comp` with the identical underlying type; only
  the *spelling* of subsequent declarations changes (§6).

**Systems-level**
- You're reviewing code with `int *comp(void *, void *);` where the author clearly intended
  a comparator *pointer* to pass into a generic sort function. What's actually declared
  instead, and what breaks when this gets used as intended? → It declares a *function*
  named `comp` returning `int *` — not a pointer variable at all. Passing `comp` as a
  "function pointer argument" would actually pass the address of this (wrongly-typed)
  function, and the compiler would reject assigning/passing it wherever a
  `int (*)(void *, void *)` was expected, catching the bug at compile time rather than
  runtime — worth knowing precisely because it's a compile error, not a silent
  misbehavior, which is diagnostically useful once you recognize the pattern (§5).

---

## 9. External references

- [cppreference — Declarations](https://en.cppreference.com/w/c) `[External]` — the current,
  complete declarator grammar, for edge cases beyond this file's worked examples.
- [cdecl.org](https://en.cppreference.com/w/c) `[External, tool reference — verify the live
  URL before relying on it, as this is a third-party tool rather than a standards body]` —
  a well-known online "declaration ⇄ English" translator many C programmers use to check
  their own manual parsing; useful for building confidence in the §3 algorithm by checking
  your by-hand answers against it.

## 10. 🛠 Practice

1. Work through every row of Book A's ladder in §4 by hand with the §3 algorithm, writing
   out your reasoning for each — then check yourself against the given readings.
2. Declare a `typedef`'d function-pointer type for a comparator (`int (*)(const void *, const void *)`
   — the real signature the standard `qsort` uses, `[External]`, previewed here ahead of
   `libraries/02_stdlib.md` in Phase 7), and write a tiny generic `max_by(void *a, void *b, CompareFn cmp)`
   function using it.
3. Deliberately write the "missing parentheses" bug from §5 (`int *comp(...)` instead of
   `int (*comp)(...)`), attempt to use it where a function pointer variable was expected,
   and read the resulting compiler error closely — confirm you can explain *why* it fails
   in terms of the two different types involved.
