# Structures, Unions & Self-Referential Types

> **Phase 4 · Arrays, Strings, Structs & Unions.** Depends on: `memory/02_pointers.md`
> (the `->` operator is pointer dereference plus member access, fused), `04_arrays_and_strings.md`
> §3 (arrays of structs follow the same layout logic as arrays of anything else).
> Feeds into: `07_alignment_padding_aliasing.md` (this phase — struct padding), `memory/05_dynamic_memory.md`
> (Phase 6 — `malloc`-backed tree/list nodes), `projects/02_intermediate_projects.md`.
> Sources: **Book B** ch.6, in full (primary — this chapter is the definitive treatment
> among the three books), **Book A** §28.1 (the anonymous-struct-`typedef` idiom and its
> self-reference gotcha).

---

## 1. Struct basics

`[Book-derived, B]`

```c
struct point {
    int x;
    int y;
};
```

`struct` introduces a structure declaration — a list of member declarations in braces. The
optional name after `struct` (`point` here) is a **tag**, usable afterward as shorthand for
the braced member list. A struct declaration **defines a type**; it can be followed by a
list of variable names exactly like any basic type (`struct { ... } x, y, z;` parallels
`int x, y, z;`). A struct declaration with no tag *and* no variable list reserves no
storage — it's purely a template. Given the tag, `struct point pt;` defines an actual
variable.

**Members** are accessed with `.`: `pt.x`, `pt.y`. A struct member and an ordinary
variable can share a name without conflict (context disambiguates); different structs can
reuse the same member names too. Structs can **nest**:

```c
struct rect { struct point pt1, pt2; };
struct rect screen;
screen.pt1.x        /* the x member of screen's pt1 member */
```

**Initialization** mirrors arrays — a brace list of constant expressions, one per member,
in declaration order: `struct point maxpt = {320, 200};`.

### ⚠️ The only legal operations on a struct as a whole
`[Book-derived, B]` Copying/assigning it as a unit, taking its address with `&`, and
accessing its members. **Structs cannot be compared** (`==`/`!=` are not defined for
struct operands) — there's no language-level "are these two structs equal" check; you
write your own member-by-member comparison function if you need one.

---

## 2. Structs and functions

`[Book-derived, B]` Structures may be passed to functions and returned from them **by
value** — the same call-by-value rule from `functions/01_functions_and_stack_frames.md`
§2 applies to structs exactly as to scalars: the *entire struct is copied*. For a large
struct, this copy has a real, measurable cost — which is exactly why passing a **pointer**
to a struct is the common alternative for anything beyond a small, cheap-to-copy struct
like `point`.

```c
struct point *pp;
pp = &origin;
printf("origin is (%d,%d)\n", (*pp).x, (*pp).y);   /* parens REQUIRED — see below */
printf("origin is (%d,%d)\n", pp->x, pp->y);        /* the idiomatic shorthand */
```

### ⚠️ Precedence trap — `(*pp).x` vs. `*pp.x`

`[Book-derived, B]` The structure member operator `.` binds **tighter** than the unary `*`
— so `*pp.x` parses as `*(pp.x)`, which is illegal here (`x` is a plain `int`, not a
pointer, so dereferencing it makes no sense). The parentheses in `(*pp).x` are mandatory
to get the intended meaning ("dereference `pp`, then access member `x`"). This is exactly
why the `->` shorthand exists: `pp->x` says the same thing without the parenthesization
trap, and is what real code uses essentially universally once a pointer-to-struct is
involved.

### More precedence: `->`, postfix, and prefix operators together

`[Book-derived, B]` `.` and `->` sit at the very top of the precedence hierarchy (alongside
function-call `()` and subscript `[]`), binding even tighter than postfix `++`/`--`. Given
`struct { int len; char *str; } *p;`:

| Expression | Means | Effect |
|---|---|---|
| `++p->len` | `++(p->len)` | increments `len`, not `p` |
| `(++p)->len` | explicit parens override | increments `p` first, *then* accesses `len` |
| `(p++)->len` | explicit parens override | accesses `len`, *then* increments `p` |
| `*p->str` | `*(p->str)` | fetches whatever `str` points to |
| `*p->str++` | `*((p->str)++)` | increments `str` *after* fetching what it pointed to |
| `(*p->str)++` | explicit parens override | increments the pointed-to *character*, not `str` |

`[Synthesis]` This table is the struct-pointer sibling of `memory/02_pointers.md` §1's
`*ip++` vs. `(*ip)++` trap — same underlying lesson (postfix binds to the nearest operand,
not to "the thing you meant"), now combined with `->`'s own very high precedence.

---

## 3. Arrays of structures

`[Book-derived, B]` A struct type and an array combine exactly as you'd expect:

```c
struct key { char *word; int count; } keytab[NKEYS];
```

An idiom worth internalizing: computing an array's element count safely at compile time,
which works for *any* array (not just struct arrays), so you never need to hardcode (and
risk desyncing) a separate count constant:

```c
#define NKEYS (sizeof keytab / sizeof(keytab[0]))
```

`[Book-derived, B]` This divides the whole array's total byte size by one element's byte
size — safe because both are known at compile time, and it automatically stays correct if
`keytab`'s element type or entry count ever changes. `[Cross-reference]` This is exactly
the `sizeof`-on-an-array fact from `memory/02_pointers.md` §3 and `04_arrays_and_strings.md`
§5, applied usefully: it only works where `keytab` is still a real array in scope, never on
a decayed pointer/array-function-parameter.

---

## 4. Pointers to structures, and the "one past the end" rule in practice

`[Book-derived, B]` Rewriting a struct-array-processing loop with pointers instead of
indices:

```c
struct key *low = &tab[0];
struct key *high = &tab[n];       /* one past the last real element */

while (low < high) {
    mid = low + (high - low) / 2;   /* NOT (low+high)/2 — pointer ADDITION is illegal
                                        (memory/02_pointers.md §4); pointer SUBTRACTION
                                        is legal and gives an element count, so this is
                                        the correct way to find the pointer "midpoint" */
    /* ... */
}
```

`[Book-derived, B]` `&tab[n]` (one past the last real element) is legal to *compute and
compare against* — exactly the exception called out in `memory/02_pointers.md` §4 — but
`&tab[-1]` is not, and dereferencing `&tab[n]` itself is not. `p++` on a struct pointer
advances by `sizeof(struct key)`, correctly landing on the next array element regardless of
that struct's actual size — the same scaling rule as any other pointer arithmetic, just
with a (usually) larger element size.

### 💡 The line that sets up the entire next section
`[Book-derived, B, quoted here because it's exactly the fact `07_alignment_padding_aliasing.md`
builds on]` *"Don't assume... that the size of a structure is the sum of the sizes of its
members. Because of alignment requirements for different objects, there may be unnamed
'holes' in a structure."* Book B's own example: a `struct { char c; int i; };` with a
1-byte `char` and a 4-byte `int` **might well require 8 bytes, not 5** — full explanation
in `07_alignment_padding_aliasing.md`.

---

## 5. Self-referential structures — the phase's core project material

`[Book-derived, B]` A struct **cannot contain an instance of itself** (that would require
infinite storage), but it **can** legally contain a *pointer* to its own type — a pointer
is a fixed size regardless of what it points to (`memory/01_memory_model.md` §1), so no
infinite regress occurs:

```c
struct tnode {
    char *word;          /* pointer to the text */
    int count;            /* occurrence count */
    struct tnode *left;    /* left child — a POINTER to a tnode, not a tnode itself */
    struct tnode *right;    /* right child */
};
```

This single pattern is how C builds every classic linked/recursive data structure —
linked lists, binary trees, graphs. `[Book-derived, B]` For **mutual** self-reference
(two struct types each needing to point to the other), forward-declare one struct's tag
before it's fully defined:

```c
struct t { struct s *p; };    /* p points to an s */
struct s { struct t *q; };    /* q points to a t */
```

### Worked example — a binary tree built entirely from this pattern

`[Book-derived, B, condensed and lightly adapted for teaching — see the source book for its
own exact listing]` A word-frequency binary tree: each node holds a word, a count, and left/
right child pointers; insertion is naturally recursive.

```c
struct tnode *addtree(struct tnode *p, char *w)
{
    int cond;
    if (p == NULL) {                 /* base case: empty slot found — insert here */
        p = talloc();                  /* allocate a new node (Phase 6: malloc-backed) */
        p->word = strdup(w);
        p->count = 1;
        p->left = p->right = NULL;
    } else if ((cond = strcmp(w, p->word)) == 0) {
        p->count++;                    /* word already present — just bump the count */
    } else if (cond < 0) {
        p->left = addtree(p->left, w);   /* recurse left */
    } else {
        p->right = addtree(p->right, w);  /* recurse right */
    }
    return p;                          /* hand the (possibly new) node back up */
}
```

**Execution flow, tying together Phase 2's recursion and Phase 3's pointers:** each call
either plants a new node at a `NULL` slot it was just handed, or recurses into whichever
child subtree the comparison points toward — and crucially **returns `p`** every time, so
that the *caller* (whether `main` or a parent call) can re-link that slot to whatever node
now lives there. This return-and-relink pattern is precisely how a tree "grows" via a
purely recursive, no-explicit-parent-pointer algorithm — each level of recursion is
responsible only for its own child pointer, per `functions/02_recursion.md` §1's "each
call gets an independent frame" model.

`[Book-derived, B]` Printing the tree in sorted order is the natural mirror-image
recursion:

```c
void treeprint(struct tnode *p)
{
    if (p != NULL) {
        treeprint(p->left);                        /* everything smaller, first */
        printf("%4d %s\n", p->count, p->word);       /* this node */
        treeprint(p->right);                          /* everything larger, last */
    }
}
```

`[Book-derived, B]` A practical caveat worth flagging now (full complexity treatment is
out of scope for this vault): if words arrive already sorted, this tree degenerates into
what's effectively a linked list, and search/insert cost grows linearly rather than
staying near-logarithmic — a real, known weakness of unbalanced binary search trees.

---

## 6. `typedef` — naming struct types

`[Book-derived, B]` `typedef` creates a new **name** for an existing type — it does not
create a new type, and adds no new semantics; a `typedef`'d variable behaves identically to
one declared with the type spelled out in full. Syntactically, `typedef` behaves like a
storage-class keyword (`extern`, `static`, …), with the *new type name* appearing in the
position a variable name would normally go:

```c
typedef struct tnode *Treeptr;
typedef struct tnode {
    char *word;
    int count;
    struct tnode *left;
    struct tnode *right;
} Treenode;

Treeptr talloc(void) { return (Treeptr) malloc(sizeof(Treenode)); }
```

### ⚠️ The self-reference gotcha with anonymous structs
`[Book-derived, A]` It's tempting to write a `typedef`'d struct with **no tag at all**:

```c
typedef struct {
    int value;
    /* struct Node *next;   -- ERROR: "struct Node" doesn't exist -- the tag was omitted */
} Node;
```

For a **self-referential** struct, this doesn't work — a member needs to reference the
struct type *before* the `typedef` name (`Node`) has been fully defined, and an anonymous
struct has no other name to use at that point. The fix, matching Book A's own guidance, is
to give the struct an explicit tag even though you also `typedef` it:

```c
typedef struct Node {
    int value;
    struct Node *next;   /* refers to the TAG, which exists immediately, not the
                             typedef name, which isn't complete until the semicolon
                             at the very end of this declaration */
} Node;
```

### 💡 Two real reasons to use `typedef` (not just aesthetics)
`[Book-derived, B]` (1) **Portability**: parameterizing machine-dependent choices (is a
particular quantity best as `int`, `long`, …?) behind one name that only needs updating in
one place per platform — `size_t` and `ptrdiff_t` from the standard library are exactly
this kind of `typedef`. (2) **Documentation**: `Treeptr` communicates intent more clearly
than a bare `struct tnode *` scattered through the code.

---

## 7. Unions

`[Book-derived, B]` A union holds, **at different times**, values of different types and
sizes — all members share the *same* storage, rather than each getting its own
(contrast a struct, where every member gets independent storage):

```c
union u_tag { int ival; float fval; char *sval; } u;
```

`u` is large enough to hold its **largest** member (the exact size, including any padding
for alignment, is implementation-dependent). Any member can be assigned; the language
places the burden of **tracking which type is currently valid** entirely on the
programmer — reading a union through a *different* member than the one most recently
written is implementation-dependent behavior, not a defined "reinterpret the bytes"
operation you can safely rely on portably. The idiomatic pattern is a **tagged union**:
pair the union with an explicit "which member is active" field, typically inside an
enclosing struct:

```c
struct {
    int utype;               /* which member of u is currently valid */
    union { int ival; float fval; char *sval; } u;
} entry;

if (entry.utype == INT)      printf("%d\n", entry.u.ival);
else if (entry.utype == FLOAT) printf("%f\n", entry.u.fval);
```

`[Book-derived, B]` Members are accessed with `.`/`->` exactly as for structs; unions
support the same limited operation set (assignment/copy as a unit, `&`, member access) and
can only be **initialized** with a value for their *first* member. `[Cross-reference]` A
union used to interpret the same bytes as two different types (rather than the disciplined
tagged-union pattern above) is exactly the kind of type-punning `advanced/03_aliasing_and_optimization.md`
(Phase 11) treats as a genuine aliasing hazard — mentioned here, treated in full there.

---

## 8. Bit-fields

`[Book-derived, B]` For packing several small flags/values into one machine word (compiler
symbol tables and hardware-register-mirroring structs are the classic use cases), C offers
**bit-fields**: named, adjacent groups of bits within a struct, an alternative to hand-
rolled masking with `<<`/`&`/`|`:

```c
struct {
    unsigned int is_keyword : 1;
    unsigned int is_extern  : 1;
    unsigned int is_static  : 1;
} flags;

flags.is_extern = flags.is_static = 1;    /* turn bits on — reads like ordinary members */
```

### ⚠️ Bit-fields are almost entirely implementation-defined — real portability hazards
`[Book-derived, B]` A direct, important list:
- Whether a field may straddle a storage-unit ("word") boundary is implementation-defined.
- **Field packing order (left-to-right vs. right-to-left) is implementation-defined** —
  code that depends on a specific order (e.g. matching an externally-defined binary format,
  a network protocol, or hardware register layout) is **not portable**.
- Fields must be declared as some form of `int`; **always specify `signed` or `unsigned`
  explicitly** for portability (plain `int` bit-field signedness is as implementation-
  defined as plain `char` signedness was in `fundamentals/02_types.md` §2).
- Bit-fields are **not arrays and have no address** — `&flags.is_keyword` is illegal,
  because a bit-field generally doesn't start at a byte boundary.
- An unnamed field (just a colon and a width) pads without naming anything; a zero-width
  unnamed field forces alignment to the next storage-unit boundary.

### 🎯 Interview point
"When would you use a bit-field instead of manual bitmask macros?" — Bit-fields read more
naturally as ordinary struct member access (`flags.is_static = 1;` vs.
`flags |= STATIC;`), at the cost of genuine portability risk the moment the exact bit
layout matters (an external format/protocol/hardware register) — manual masking with
explicit shift/mask constants remains the *portable* choice whenever the precise bit
positions must be guaranteed, which is why both techniques still coexist in real code.

---

## 9. ⚠️ Misconceptions to unlearn now

- **"`.` and `->` do fundamentally different things."** `p->m` is exactly `(*p).m` —
  dereference-then-access, spelled as one operator purely for convenience and to sidestep
  the `.`-binds-tighter-than-`*` precedence trap in §2.
- **"A struct can contain a member of its own type."** It cannot (only a *pointer* to its
  own type) — a direct instance would require infinite storage (§5).
- **"`typedef` creates a genuinely new, distinct type."** It creates a new *name* for an
  existing type — no new type, no new semantics (§6).
- **"A union lets you safely reinterpret the same bytes as two unrelated types."** Reading
  a union member other than the one most recently written is implementation-dependent, not
  a portable, well-defined reinterpretation (§7) — the disciplined use is a *tagged* union,
  tracking which member is actually valid.

---

## 10. 🎯 Interview questions

**Easy**
- What are the only legal operations on a struct as a whole? → Copy/assign as a unit,
  take its address, access members — never `==`/`!=` (§1).
- What's the size of a union, relative to its members? → At least the size of its largest
  member (§7).

**Medium**
- Why does `*pp.x` fail to compile for `struct point *pp;`, while `(*pp).x` and `pp->x`
  both work? → `.` binds tighter than `*`, so `*pp.x` parses as `*(pp.x)`, illegal since
  `x` isn't a pointer; the other two force the intended "dereference, then access member"
  grouping (§2).
- Why can a struct contain a pointer to its own type but not an instance of its own type? →
  A pointer has fixed size regardless of what it points to, so no infinite storage
  requirement arises; a direct instance would (§5).

**Hard**
- Walk through `addtree`'s recursive insertion logic and explain why it returns `p` on
  every path, including when nothing new was inserted. → The return value is how each
  level of recursion re-links its own child pointer (`p->left = addtree(p->left, w)`) to
  whatever node now correctly occupies that slot — on the "already present" path, that's
  just the existing node itself; on the "new node" path, it's the freshly allocated one
  (§5).
- Why must a self-referential `typedef`'d struct still carry an explicit tag, even though
  the whole point of the `typedef` is to avoid typing `struct X` elsewhere? → The member
  referencing the struct's own type needs a name to refer to *before* the `typedef` name is
  complete (the `typedef` isn't finished until its closing semicolon); the tag is available
  immediately, so it's the only name usable inside the struct's own member list (§6).

**Systems-level**
- A binary format (or hardware register layout) specifies exact bit positions for several
  flags. Why is a C bit-field struct a risky way to model this, and what's the safer
  alternative? → Bit-field packing order and boundary-straddling behavior are both
  implementation-defined, so the *same* bit-field struct can lay out differently across
  compilers/platforms; explicit shift/mask operations on a plain unsigned integer type
  guarantee the exact bit positions regardless of compiler (§8).
- Why does inserting already-sorted input into the `addtree` binary tree degrade its
  performance so badly? → Every new node becomes, say, always the right child of the
  previous one, degenerating the tree into a linked list in all but name — search/insert
  cost grows linearly with node count instead of staying near-logarithmic, a direct
  consequence of the tree never branching (§5).

---

## 11. External references

- [cppreference — Structure types](https://en.cppreference.com/w/c) `[External]` — current,
  precise rules for struct/union declaration, including anonymous struct/union support
  added after this vault's source books were written.
- [cppreference — Bit-field](https://en.cppreference.com/w/c) `[External]` — a complete,
  current list of what's implementation-defined about bit-fields, beyond §8's summary.

## 12. 🛠 Practice — the phase project

1. Implement a singly linked list of `int`s from scratch: a `struct node { int value;
   struct node *next; };`, plus `push_front`, a length-counting function, and a print
   function — all using the self-referential pattern from §5 (heap allocation via `malloc`
   is previewed here; full treatment is Phase 6's `memory/05_dynamic_memory.md`).
2. `[Book-derived, B]` Implement the word-frequency binary search tree from §5 end to end
   (`addtree` + `treeprint`, using `talloc`/`strdup` stubs if Phase 6 hasn't been reached
   yet), and run it against a real text file.
3. Reproduce the `(*pp).x` vs. `*pp.x` compile error from §2 yourself, and read the
   compiler's error message closely enough to explain it in terms of `.`'s precedence.
4. Write a tagged union (§7) for a small "value" type that can hold an `int`, a `double`,
   or a `char *`, plus a `print_value` function that switches on the tag — then
   deliberately read the wrong union member to observe what your platform actually does
   with implementation-dependent access (do this only as an observational exercise, not as
   a pattern to keep).
