# Revision — Phase 4: Arrays, Strings, Structs & Unions

> Quick-reference companion to `memory/04_arrays_and_strings.md`, `06_struct_union_layout.md`,
> and `07_alignment_padding_aliasing.md`.

---

## Arrays & strings — 1-minute summary

Arrays are stored contiguous and row-major (rightmost subscript varies fastest in a
multi-dimensional array); a function taking a multi-dimensional array parameter must
specify every dimension except the outermost, since the compiler needs them to compute
row offsets. `char arr[] = "..."` gives you your own writable copy; `char *p = "..."`
points at a (commonly read-only) string literal — writing through the latter is undefined
behavior. `sizeof` is a compile-time array-size fact; `strlen` is a runtime scan for the
terminator — they answer different questions and diverge the moment an array isn't fully
"the string" (e.g., a partially-filled buffer).

### 5-minute revision
- `int a[10][20]` (one contiguous 200-int block) vs. `int *b[10]` (10 independently-
  assignable pointers, rows can differ in length) — genuinely different memory layouts,
  not two spellings of the same thing.
- `#define NKEYS (sizeof keytab / sizeof(keytab[0]))` — the standard, self-correcting way
  to count an in-scope array's elements; breaks silently if applied to a decayed pointer.
- `while (*s++ = *t++);` — the canonical `strcpy` idiom; the assignment's value doubles as
  the loop test (nonzero == true), so it stops right after copying `'\0'`.
- `argv[argc]` is guaranteed `NULL` — a built-in end-of-arguments sentinel, no separate
  length tracking needed.
- `strncpy` does **not** guarantee a terminator if the source is `>= n` characters long —
  force one explicitly (`buffer[sizeof(buffer)-1] = '\0';`) after calling it.

**Worth memorizing:** the `sizeof`-vs-`strlen` distinction, cold — it's asked constantly
and confused constantly.
**Worth understanding, not memorizing:** *why* `int a[10][20]` and `int *b[10]` differ
(arrays_and_strings.md §3) — you should be able to draw both memory layouts from scratch.

---

## Structs, unions & self-reference — 1-minute summary

`.` binds tighter than `*`, so `*pp.x` is illegal — use `(*pp).x` or, idiomatically,
`pp->x`. Structs are call-by-value like anything else (§2 of the functions file, revisited)
— pass a pointer for anything beyond a small, cheap struct. A struct's `sizeof` is **not**
the sum of its members' sizes, because of alignment padding. A struct can hold a *pointer*
to its own type (not an instance) — this is the entire mechanism behind linked lists and
trees in C. `typedef` names a type; it never creates one. A union's members share the
*same* storage; reading a member other than the one last written is implementation-
dependent, so real code pairs a union with an explicit tag field.

### 5-minute revision
- `++p->len` means `++(p->len)` — `->` binds even tighter than prefix `++`; parenthesize
  `(++p)->len` explicitly if you actually mean to increment the pointer first.
- `addtree`'s recursive insert always returns `p` — that's how each level re-links its own
  child pointer to whatever node now correctly occupies that slot.
- A self-referential `typedef`'d struct needs an explicit tag (`typedef struct Node {...
  struct Node *next; } Node;`) — the member reference needs a name that exists *before*
  the `typedef` name is complete.
- Bit-field packing order is implementation-defined — never rely on it for an externally-
  defined binary format; use explicit shift/mask on a plain unsigned type instead.

**Worth memorizing:** `p->m` ≡ `(*p).m`, cold, and *why* the shorthand exists (the `.`
precedence trap).
**Worth understanding, not memorizing:** working through `addtree`/`treeprint` by hand on
paper for a small input — the recursion-plus-self-reference combination is worth being able
to trace, not just recognize.

---

## Alignment, padding & aliasing — 1-minute summary

Multi-byte types generally need addresses that are multiples of their size; the compiler
inserts invisible padding bytes inside a struct so each member satisfies its own alignment
requirement, and pads the struct's *total* size up to a multiple of its own (strictest-
member) alignment so arrays of that struct stay aligned element to element. Reordering
struct members from widest-alignment to narrowest can shrink total size for the *same*
fields. `malloc` guarantees alignment suitable for any type — you never need to reason
about it yourself when using heap memory normally.

### 5-minute revision
- `struct { char c; int i; }` is commonly 8 bytes, not 5 — 3 padding bytes push `i` to a
  4-byte-aligned offset.
- `offsetof(type, member)` (`<stddef.h>`) is the portable way to measure a member's real
  offset, padding included.
- Type punning through a union (reading a different member than was last written) is
  implementation-dependent, not a guaranteed-portable reinterpretation — treat it as
  advanced/risky territory, covered properly in `advanced/03_aliasing_and_optimization.md`.

**Worth memorizing:** the general reordering heuristic (widest-alignment members first).
**Worth understanding, not memorizing:** *why* padding exists at the hardware level
(alignment_padding_aliasing.md §1) — this is what makes the reordering trick make sense
rather than feeling like an arbitrary rule.

---

## Cross-topic traps worth drilling before an interview

1. `sizeof`/`strlen` confusion on a string buffer that isn't fully "the string" (extra
   trailing garbage past the terminator) — two genuinely different measurements
   (arrays_and_strings.md §5).
2. `char *p = "literal"; p[0] = 'X';` — undefined behavior, sometimes a hard crash
   (arrays_and_strings.md §4).
3. `*pp.x` instead of `pp->x` or `(*pp).x` — a `.`-vs-`*` precedence compile error
   (struct_union_layout.md §2).
4. Assuming `sizeof(struct { char; int; char; })` equals the sum of member sizes — padding
   makes this false almost universally (alignment_padding_aliasing.md §2).
5. A self-referential `typedef`'d struct with no tag — fails to compile; the tag has to be
   there even though `typedef` is also used (struct_union_layout.md §6).
