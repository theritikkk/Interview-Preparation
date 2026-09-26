# Pointers

> **Phase 3 · Pointers & Memory.** Depends on: `01_memory_model.md` (addresses, regions),
> `functions/01_functions_and_stack_frames.md` §2 (call-by-value — the reason pointers
> exist as a language necessity, not a convenience).
> Feeds into: `03_pointer_declarations_parsing.md` (this phase), `memory/04_arrays_and_strings.md`
> (Phase 4), `memory/05_dynamic_memory.md` (Phase 6), `advanced/01_undefined_behavior.md`
> and `03_aliasing_and_optimization.md` (Phase 11).
> Sources: **Book B** §5.1–5.6 (primary — this is the definitive treatment among the three
> books), **Book A** ch.25 (extra worked examples and the dangling-pointer caution in §7).

---

## 1. What a pointer is, restated precisely

`[Book-derived, B]` A pointer is a variable that holds an address (`01_memory_model.md`
§1). Two operators do all the fundamental work:

- **`&` (address-of):** applied to an object *in memory* (a variable or array element —
  never to an expression, a constant, or a `register` variable, since none of those
  necessarily have an addressable memory location), it yields that object's address.
- **`*` (indirection/dereference):** applied to a pointer, it accesses the object the
  pointer points to — a dereferenced pointer expression can appear anywhere the pointed-to
  object itself could.

### Worked example — the canonical walkthrough

`[Book-derived, B]`

```c
int x = 1, y = 2, z[10];
int *ip;          /* ip is a pointer to int */

ip = &x;           /* ip now points to x */
y = *ip;            /* y is now 1 */
*ip = 0;            /* x is now 0 */
ip = &z[0];          /* ip now points to z[0] */
```

```
Before "ip = &x;":                After "ip = &x;":

  x: [ 1 ]  @0x1000                x: [ 1 ]  @0x1000
  ip: [ ? ]  @0x2000                ip: [0x1000]  @0x2000
                                          │
                                          └── points to x
```

**Line by line:** `ip = &x;` stores `x`'s address into `ip` — `ip` now "points to" `x`.
`y = *ip;` dereferences `ip` (follows the arrow) and reads the `int` there, copying `1` into
`y`. `*ip = 0;` dereferences `ip` on the *left* side of an assignment — this writes through
the pointer, setting `x` itself (not `ip`) to `0`. `ip = &z[0];` reassigns `ip` to point
somewhere else entirely — `ip` itself is just an ordinary variable, freely reassignable.

### 🧠 Mental model: declarations mimic use

`[Book-derived, B]` The declaration `int *ip;` is deliberately written to *mnemonically*
echo how `ip` is used: it says "`*ip` is an `int`" — read the declaration as a statement
about what the **dereferenced** expression's type will be, not about `ip`'s own type
directly. This single mental habit is the seed of the entire declaration-parsing skill
built out fully in `03_pointer_declarations_parsing.md`. It applies to function
declarations too: `double *dp, atof(char *);` says "`*dp` and `atof(s)` both have type
`double`, and `atof`'s argument is a pointer to `char`."

### Precedence: `*` and `++`/`--` bind right-to-left

`[Book-derived, B]` `*` and `&` (as unary operators) bind **tighter** than arithmetic
operators but, critically, `++`/`--` associate with `*` right-to-left, which makes
parenthesization matter:

| Expression | Meaning |
|---|---|
| `y = *ip + 1` | take what `ip` points to, add 1, assign to `y` — `*ip` binds first |
| `*ip += 1` | increment *what `ip` points to* by 1 |
| `++*ip` | same — increment what `ip` points to |
| `*ip++` | increment **`ip` itself** (then dereference the *old* address) — ⚠️ almost
  certainly not what a beginner means when reaching for this |
| `(*ip)++` | increment what `ip` points to — the parentheses are **required** to get this
  meaning instead of the row above |

This exact ambiguity — "am I incrementing the pointer or the pointee?" — is one of the
most common real C bugs involving pointers, and it's purely a precedence/associativity
issue, not a conceptual one, once you know the rule.

---

## 2. Pointers and function arguments — the payoff, revisited

`[Book-derived, B, cross-referencing `functions/01_functions_and_stack_frames.md` §2]`
This vault already showed *why* pointers are a language necessity: C is always
call-by-value, so the only way a function can modify a caller's variable is to be handed
that variable's **address** and dereference it. Nothing new mechanically happens here —
this section exists to make explicit that "pass a pointer as an output parameter" is a
named, idiomatic C pattern, not a one-off trick:

```c
void getnext(int *out) { *out = compute_something(); }
/* called as: int result; getnext(&result); */
```

This is the standard way C functions return more than one value, or communicate success/
failure separately from a computed result (a common pairing: return an `int` status code,
write the real result through an output pointer parameter).

---

## 3. Pointers and arrays — the central duality

`[Book-derived, B]` This is arguably the single most important relationship in the
language's memory model, and the brief specifically warns against the lazy "an array is
just a pointer" shortcut — so let's be precise about what's actually true.

`int a[10];` defines a block of 10 consecutive `int`-sized objects, `a[0]` through `a[9]`.
By definition, **the value of an array-typed expression is the address of its element
zero** — this conversion (called **array decay**, `[Synthesis]` naming a concept Book B
describes but doesn't name with this exact term) is what makes the following all true
simultaneously:

```c
int a[10];
int *pa;

pa = &a[0];      /* explicit address-of element zero */
pa = a;          /* IDENTICAL to the line above — "a" decays to &a[0] */

*(pa + 1)  ==  a[1]  ==  pa[1]     /* three equivalent ways to reach element 1 */
&a[i]      ==  a + i                /* address-of-element-i, two equivalent ways */
```

`[Book-derived, B]` More precisely: `a[i]` is **defined** to mean `*(a+i)` — the compiler
converts every subscript expression to this pointer form. Applying `&` to both sides of
that equivalence shows `&a[i]` and `a+i` are identical too. As the mirror image, if `pa` is
a pointer, `pa[i]` is identical to `*(pa+i)` — so array-and-index notation and
pointer-and-offset notation are two spellings of the exact same operation, for *both*
arrays and pointers.

### ⚠️ Common misconception (the one the brief explicitly flags)

**"An array is just a pointer."** It is not, and the distinction is observable:

```c
int a[10];
int *pa = a;

pa = a;       /* LEGAL — pa is a variable, can be reassigned */
a = pa;       /* ILLEGAL — a is not a variable; its "value" (element 0's address)
                 isn't something you can overwrite */
pa++;         /* LEGAL */
a++;          /* ILLEGAL, for the same reason */

sizeof(a);    /* the size of the WHOLE ARRAY — 10 * sizeof(int) */
sizeof(pa);   /* the size of ONE POINTER — typically 4 or 8 bytes, regardless of
                 what it points to or how many elements the underlying array has */
```

`[Synthesis]` The rule that resolves this cleanly: **an array is an object that decays to
a pointer to its first element in most expression contexts** — but the array itself is
still a distinct, fixed-size, non-reassignable object, and `sizeof` is one of the specific
contexts where decay does **not** happen (the others are `&array` and using a string
literal to initialize a `char` array — full treatment, including *why* `sizeof` is
special-cased, is `memory/04_arrays_and_strings.md`, Phase 4). This `sizeof` difference is
also *why* "passing an array to a function" is a common trap: the parameter receives a
decayed pointer, so `sizeof` *inside* the function gives the pointer's size, not the
original array's — a function can never recover an array's element count from the array
alone; it must be told separately (an explicit length parameter, or a sentinel value like
the string terminator covered in Phase 4).

### 🎯 Interview point
"Is `sizeof(array_name)` the same inside the function it's passed to?" — **No.** At the
call site, `a` still names the actual array, so `sizeof(a)` is the whole array's size. Once
passed as a function parameter, the parameter is a pointer (the array decayed at the call),
so `sizeof` on the parameter gives the pointer's size instead. This single fact explains a
large fraction of real "why is my buffer-size calculation wrong" bugs.

---

## 4. Pointer arithmetic — precisely, not just "p+1 moves along"

`[Book-derived, B]` If `pa` points to some element of an array, `pa+1` points to the
**next** element — regardless of that element's size. The brief specifically asks for more
than "p + 1 moves by one"; here's the precise mechanism:

> **`p + n` is defined as the address of the `n`-th object beyond the one `p` currently
> points to.** The compiler scales `n` by `sizeof(*p)` — the size of whatever type `p`
> points to — to compute the actual byte offset. If `p` points to a 4-byte `int`, `p + 1`
> adds 4 bytes to `p`'s underlying address; if `p` points to a 1-byte `char`, `p + 1` adds
> just 1 byte.

```
int arr[4] = {10, 20, 30, 40};    int *p = arr;   (assume 4-byte int, addresses shown)

address:   0x1000    0x1004    0x1008    0x100C
value:     [  10  ]  [  20  ]  [  30  ]  [  40  ]
             ↑ p
                        ↑ p+1                        p+1 is 0x1004, not 0x1001 —
                                  ↑ p+2               the offset is 1 * sizeof(int) = 4 bytes,
                                            ↑ p+3      because pointer arithmetic is defined
                                                       in terms of ELEMENTS, not raw bytes.
```

`[Book-derived, B]` This scaling is precisely why the identical `alloc`/`afree` code from
`fundamentals`/`functions`-adjacent examples works unchanged whether the pointer type is
`char *` or `float *` — "add 1" always means "advance by one object of whatever type is
pointed to," automatically, regardless of that type's size.

### Legal vs. illegal pointer operations

`[Book-derived, B]` A precise, complete list — memorize this one, it's tested constantly:

| Operation | Legal? |
|---|---|
| pointer `+`/`-` an integer | ✅ (scaled per the rule above) |
| pointer `-` pointer (**same array only**) | ✅ — yields a `ptrdiff_t` (`<stddef.h>`), the count of elements between them |
| pointer `<` `>` `<=` `>=` `==` `!=` pointer (**same array only**, or either side `== NULL`) | ✅ |
| pointer `+` pointer | ❌ — meaningless; not permitted |
| pointer `*`/`/`/`<<`/`>>`/`&`/`\|` (multiply, divide, shift, mask) | ❌ |
| assigning a pointer of one type to another type, without a cast | ❌ (except `void *`, §7) |

`[Book-derived, B]` **One narrow, explicitly-permitted exception** to the "same array
only" rule: a pointer to *one past the end* of an array is valid to compute and compare
(though not to dereference) — this is exactly the idiom behind `for (p = a; p < a + n; p++)`
style loops, where `a + n` never gets dereferenced but is a legal comparison target.
Comparing or subtracting pointers into two **different, unrelated** arrays is undefined
behavior, even if it happens to "look like it works" — full catalogue in
`advanced/01_undefined_behavior.md` (Phase 11).

---

## 5. Pointers to pointers

`[Book-derived, B]` A pointer can itself point to another pointer — `char **argv` (a very
common real example: `main`'s second parameter) declares `argv` as "pointer to (pointer to
`char`)": `*argv` is a `char *` (a string), and `**argv` is a `char` (that string's first
character).

```
argv:  [addr of argv[0]] ──► argv[0]: [addr of 'h'] ──► 'h' 'e' 'l' 'l' 'o' '\0'
       [addr of argv[1]] ──► argv[1]: [addr of 'w'] ──► 'w' 'o' 'r' 'l' 'd' '\0'
       [addr of argv[2]] ──► argv[2]: [   NULL    ]        (conventional end marker)

  argv        is a pointer to the first element of an array of char* (double indirection)
  *argv       is argv[0]   — a char* (the first string's address)
  **argv      is argv[0][0] — a char (the first string's first character)
```

`[Book-derived, B]` This double-indirection pattern is exactly how an *array of strings*
works in C (there's no native "string array" type — it's an array of `char *`, each
pointing to a separately-stored string), and it's how a function can sort or rearrange a
collection of variable-length strings efficiently: swap the *pointers* in the array (cheap,
fixed-size) rather than moving the strings themselves (expensive, variable-size) — this is
precisely the technique behind a `char *lineptr[]`-based line-sorting utility, and the same
technique behind `argv` itself, which is why command-line argument handling and "array of
strings" processing feel so similar once this mental model clicks.

---

## 6. `const` and pointers — four combinations, one common trap

`[Synthesis, applying `fundamentals/02_types.md` §6's `const` rules specifically to
pointer declarations — neither source book works through all four combinations explicitly]`
Because a pointer declaration has two "layers" (the pointer itself, and what it points to),
`const` can qualify either, both, or neither:

```c
int x = 1, y = 2;

int       *p1 = &x;   /* plain pointer to plain int: can reassign p1, can modify *p1 */
const int *p2 = &x;   /* pointer to CONST int: can reassign p2 (p2 = &y is fine),
                          but cannot modify *p2 (*p2 = 5 is a compile error) */
int * const p3 = &x;  /* CONST pointer to plain int: cannot reassign p3 (p3 = &y is an
                          error), but CAN modify *p3 (*p3 = 5 is fine) */
const int * const p4 = &x; /* both: cannot reassign p4, cannot modify *p4 */
```

### 🧠 Reading rule
Read right to left from the identifier, exactly as `03_pointer_declarations_parsing.md`
will formalize: `p2` is "a pointer to a `const int`" (the `const` binds to `int`, closer to
it than to `*`); `p3` is "a `const` pointer to an `int`" (the `const` binds to the `*p3`
declarator itself, i.e. to `p3`). `const int *` and `int const *` are equivalent — a
frequently-surprising bit of trivia, since `const`'s position relative to the base type
(before or after) doesn't matter, but its position relative to `*` does.

### 🎯 Interview point
The overwhelmingly common real use is `p2`'s form — a pointer parameter meaning "I promise
not to modify what you handed me," used constantly in library function signatures
(`strlen(const char *s)`, for instance): the function can read through the pointer freely
but the compiler will flag any attempt to write through it, giving the *caller* a
compile-time guarantee about the callee's behavior.

---

## 7. `void *` — the generic pointer

`[Book-derived, B]` A "pointer to `void`" holds an address without committing to what type
of object lives there — it can hold any object pointer type and be assigned back to any
object pointer type, but **cannot be dereferenced directly** (there's no way to know how
many bytes to read, or how to interpret them, without a type) and no pointer arithmetic is
defined on it (arithmetic requires knowing `sizeof(*p)`, and `void` has no size). It exists
specifically for writing genuinely type-generic code — `malloc`'s return type (Phase 6),
and generic comparator-based sorting like the `qsort`-style pattern in
`03_pointer_declarations_parsing.md` §6, both depend on `void *`.

---

## 8. NULL, dangling, and wild pointers — three different failure modes

`[Book-derived, A + B]` These three terms get casually conflated; they describe genuinely
different situations, and knowing which one you're looking at changes how you debug it.

| Term | What it means | How it typically arises |
|---|---|---|
| **NULL pointer** | Explicitly set to `0`/`NULL` — "points to nothing," by the guarantee in `01_memory_model.md` §3 | Deliberate initialization, or a function like `malloc` signaling failure |
| **Dangling pointer** | Still holds an address that *used to* be valid, but the object there no longer exists | Returning the address of a local (automatic) variable after its function returns (§ below); using a pointer after `free`ing what it pointed to (Phase 6) |
| **Wild pointer** | Never initialized at all — holds whatever garbage bit pattern happened to be in that memory | Declaring `int *p;` and dereferencing it before ever assigning it a real address |

### ⚠️ Worked example — the classic dangling-pointer bug

`[Book-derived, A]`

```c
int *make_dangling(void)
{
    int local = 42;
    return &local;      /* WRONG — local's storage is a stack frame slot that's
                            gone the instant this function returns */
}
```

`local` is an automatic variable — per `01_memory_model.md` §2, its storage is part of
`make_dangling`'s stack frame, which is deallocated the moment the function returns
(`functions/01_functions_and_stack_frames.md` §6). The returned address still *looks*
like a valid-shaped pointer — it may even, by coincidence, still hold `42` if nothing has
overwritten that stack location yet — but it points into memory the language no longer
guarantees means anything, and reading or writing through it is undefined behavior even
when it "happens to work" in testing.

**The fix, matching Book A's own guidance:** return a pointer to something with a longer
lifetime instead — a `static` local (persists for the program's duration,
`functions/01_functions_and_stack_frames.md` §5), a heap allocation the caller is
responsible for freeing (Phase 6), or restructure so the caller supplies the storage (an
output pointer parameter, §2 above).

### ⚠️ Common bug — the uninitialized (wild) pointer

```c
int *p;
*p = 5;      /* WRONG — p was never assigned an address; this writes to
                whatever garbage address happened to be in p's storage */
```

Unlike a NULL pointer (which at least *reliably* crashes or is checkable), a wild pointer's
garbage value might, by chance, be a "valid-looking" address, making this bug intermittent
and platform/run dependent — one of the nastier categories of pointer bug precisely because
it doesn't fail consistently. **Always initialize a pointer** — to a real address, or
explicitly to `NULL` if it has no target yet (checkable with `if (p != NULL)` before any
use).

---

## 9. Aliasing — a preview

`[Synthesis]` Because two differently-named pointers can point to the *same* object
(`ip` and `iq` both set to `&x`, for instance), reads/writes through one can affect what
the other sees — this is **aliasing**, and it's mostly a helpful tool (it's exactly how
"pass a pointer to modify a caller's variable," §2, and pointer-to-pointer sharing, §5,
work). It becomes a hazard specifically when the *compiler* can't tell two pointers might
alias and optimizes based on an incorrect assumption that they don't — full treatment,
including the `restrict` qualifier's role (`fundamentals/02_types.md` §6) in letting a
programmer promise "no aliasing here," is `advanced/03_aliasing_and_optimization.md`
(Phase 11).

---

## 10. ⚠️ Misconceptions to unlearn now

- **"An array and a pointer are the same type."** They're not (§3) — an array decays to a
  pointer in most expression contexts, but remains a distinct, non-reassignable object with
  its own, different `sizeof`.
- **"`*ip++` increments what `ip` points to."** It increments `ip` itself, then dereferences
  the *original* address (postfix `++` binds tighter than the immediately-following
  dereference in execution order here — precisely the trap in §1's precedence table).
- **"A pointer that isn't NULL is safe to dereference."** Dangling and wild pointers are
  both non-NULL and both unsafe (§8) — NULL-checking alone catches only one of three
  distinct failure modes.
- **"Pointer subtraction/comparison always works between any two pointers of the same
  type."** Only within the same array (§4) — comparing or subtracting pointers into
  unrelated objects is undefined behavior regardless of whether the types match.

---

## 11. 🎯 Interview questions

**Easy**
- What does the `&` operator require of its operand? → An addressable object in memory
  (a variable or array element) — never an expression, constant, or `register` variable (§1).
- What's the difference between a NULL pointer and a dangling pointer? → NULL deliberately
  points to nothing; a dangling pointer holds a stale address to an object that no longer
  exists (§8).

**Medium**
- Why does `p + 1` advance by 4 bytes for an `int *` but 1 byte for a `char *`? → Pointer
  arithmetic is defined in terms of *elements* of the pointed-to type; the compiler scales
  the offset by `sizeof(*p)` (§4).
- Is `sizeof` on an array parameter the same as `sizeof` on the original array at the call
  site? → No — the parameter has decayed to a pointer, so `sizeof` gives the pointer's
  size, not the original array's total size (§3).
- What's the difference between `const int *p` and `int *const p`? → The former forbids
  modifying `*p` but allows reassigning `p`; the latter is the reverse (§6).

**Hard**
- Why is returning the address of a local variable from a function undefined behavior, even
  if the returned pointer "still works" in a quick test? → The local's stack-frame storage
  is deallocated on return; the standard gives no guarantee about what (if anything) is
  still there afterward — "happens to still hold the value" is an accident of a specific
  compiler/optimization level/call pattern, not a guarantee (§8).
- Why can't `void *` be dereferenced directly? → Dereferencing requires knowing how many
  bytes to read and how to interpret them, both of which depend on a concrete pointed-to
  type; `void` has neither (§7).

**Systems-level**
- A function takes `int arr[]` as a parameter and internally does `sizeof(arr) / sizeof(arr[0])`
  hoping to count the array's elements. What's wrong, and what's the fix? → `arr` has
  decayed to a plain pointer as a parameter, so `sizeof(arr)` is the pointer's size, not
  the original array's — this computes a meaningless (and typically wrong, small) count;
  the fix is to pass the element count explicitly as a separate parameter (§3, full array-
  decay treatment in `memory/04_arrays_and_strings.md`, Phase 4).
- Explain, in terms of stack frames, exactly why a dangling pointer to a local variable
  might still "work" immediately after the function returns but break later. → The stack
  frame's memory isn't cleared on return, just deallocated (marked reusable); until another
  function call overwrites that region with its own frame, the old data may coincidentally
  still be there — so the bug's symptom depends entirely on what happens to run next, which
  is why dangling-pointer bugs are notoriously intermittent (§8, `01_memory_model.md` §2).

---

## 12. External references

- [cppreference — Pointer declaration](https://en.cppreference.com/w/c) `[External]` — the
  current, precise grammar and rules, including `const`-qualification interactions this
  file's §6 only summarizes.
- [CERT C — Pointer-related rules](https://wiki.sei.cmu.edu/confluence/display/c) `[External]`
  — a systematic catalogue of pointer pitfalls (dangling pointers, invalid pointer
  arithmetic, etc.), useful once you want the exhaustive list beyond this file's core set.

## 13. 🛠 Practice — the phase project

`[Book-derived, B, following the exact style of B's own strlen re-implementations in §5.3–5.4]`
Implement `strlen`, `strcpy`, and `strcmp` from scratch, **using only pointer arithmetic**
(no array-indexing syntax):

```c
int my_strlen(char *s)
{
    char *p = s;
    while (*p != '\0')
        p++;
    return p - s;             /* pointer subtraction — §4 */
}
```

1. Write `my_strlen` as above, then a second version using `*s++` in the loop test itself
   (`while (*s++)`) — confirm both give the same answer, and explain why the second version
   needs an adjustment to the final subtraction (it advances one step *past* the
   terminator before the loop test fails).
2. Write `my_strcpy(char *dst, char *src)` copying character by character via pointer
   increments, stopping at (and including) the `'\0'`.
3. Write `my_strcmp(char *a, char *b)`, returning negative/zero/positive per the standard
   `strcmp` convention, again using only pointer arithmetic.
4. Deliberately write the dangling-pointer bug from §8, compile with a sanitizer if
   available (`debugging/02_sanitizers_and_valgrind.md`, Phase 10, gets to this properly),
   and observe how it's reported.
