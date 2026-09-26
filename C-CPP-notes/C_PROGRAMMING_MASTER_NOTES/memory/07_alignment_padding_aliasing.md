# Alignment, Padding & a First Look at Aliasing

> **Phase 4 · Arrays, Strings, Structs & Unions.** Depends on: `06_struct_union_layout.md`
> §4 (the "don't assume struct size is the sum of member sizes" warning this file expands
> in full).
> Feeds into: `memory/05_dynamic_memory.md` (Phase 6 — allocator alignment guarantees),
> `libraries/10_limits_float_stddef.md` (Phase 7 — `offsetof`), `advanced/03_aliasing_and_optimization.md`
> (Phase 11 — the full strict-aliasing rule and its optimization consequences).
> Sources: **Book B** §6.4/6.5/6.8 (the padding warning, the allocator-alignment mention,
> and the union-forces-alignment technique — three short, precise, book-derived facts this
> file builds a complete picture around). **Most of this file's mechanics (why alignment
> requirements exist, the exact padding/reordering rules) are `[External]`/`[Synthesis]`** —
> neither book explains alignment's *rationale* in depth, only its consequences.

---

## 1. What alignment is, and why it exists

`[External]/[Synthesis]` Many CPUs can only access (or can only access *efficiently*) a
multi-byte value when its address is a multiple of that value's size — a 4-byte `int` at
an address divisible by 4, an 8-byte `double` at an address divisible by 8, and so on. An
object's **alignment requirement** is the smallest such boundary its address must satisfy.
Access to a *misaligned* multi-byte value is, depending on the CPU architecture, either
disallowed outright (a fault), or allowed but noticeably slower (the CPU quietly performs
extra work — sometimes multiple memory accesses — to assemble the value). `[Book-derived, B]`
Book B itself flags this in passing while discussing storage allocation: *"integers often
must be located at even addresses"* — a direct, if brief, acknowledgment of exactly this
requirement.

Every type has an alignment requirement (`char` is universally 1 — it can start anywhere,
since it's the smallest addressable unit; wider types typically require alignment equal to
their size, though this is itself implementation-defined). A **struct's** alignment
requirement is (informally) the *strictest* (largest) alignment requirement among its
members — which is exactly the fact that forces padding to appear.

---

## 2. Padding — the exact mechanism, worked in full

`[Book-derived, B, quote paraphrased from §6.4]` Consider:

```c
struct example {
    char c;    /* 1 byte, alignment requirement 1 */
    int  i;    /* 4 bytes, alignment requirement 4 (typical) */
};
```

Naively, `1 + 4 = 5` bytes. **The real size is typically 8.** Here's exactly why, byte by
byte:

```
offset:   0     1     2     3     4     5     6     7
member:  [ c ] [ pad] [ pad] [ pad] [    i (4 bytes)    ]
          ↑                          ↑
          c starts at offset 0        i must start at an offset divisible by 4 —
          (any offset is fine          offset 1, 2, or 3 would violate int's
           for a char)                 alignment requirement, so the compiler
                                        inserts 3 padding bytes to push i to
                                        offset 4, the next multiple of 4.
```

**The compiler inserts invisible, unnamed padding bytes between `c` and `i`** so that `i`
lands on a valid address for its type — this happens *silently*; there's no way to see the
padding bytes in the struct's declaration, only in its `sizeof`. `[Book-derived, B]` This
is precisely why Book B insists `sizeof(struct)` — never manual addition of member sizes —
is the only reliable way to know a struct's true size.

### 💡 Struct member order changes total size — a real, practical lever

`[External]/[Synthesis, a direct and well-known consequence of the padding rule Book B
states but doesn't demonstrate this optimization from]` Reordering members can eliminate
padding entirely, for the *same* set of members:

```c
struct bad {           struct good {
    char c;                 int i;    /* widest-alignment member first */
    int  i;                 char c;   /* narrow members afterward */
    char c2;                char c2;
};  /* likely 12 bytes */ };  /* likely 8 bytes */
```

```
struct bad:   [c][pad pad pad][    i    ][c2][pad pad pad]   → 12 bytes
struct good:  [    i    ][c][c2][pad pad]                    → 8 bytes
```

**The general rule of thumb:** ordering members from largest-alignment-requirement to
smallest tends to minimize total padding, because it front-loads the members with strict
requirements (which are satisfied "for free" right after another strictly-aligned member)
and clusters the narrow, loosely-aligned members (`char`, etc.) together at the end, where
their lack of alignment requirements doesn't force gaps. This matters in practice for
large arrays of small structs (network packet definitions, particle/vertex data in
performance-sensitive code, …), where per-element padding multiplies across every element.

### Trailing padding — so arrays of structs stay aligned too

`[External]/[Synthesis]` A struct's *total* size is itself padded up to a multiple of the
struct's own alignment requirement (the strictest requirement among its members) — not
just padding *between* members, but potentially trailing padding *after* the last member
too. This guarantees that if you have `struct example arr[10];`, **every** element
(`arr[1]`, `arr[2]`, …) lands on a correctly-aligned address automatically, simply because
each element is exactly one (padded) struct-size apart — consistent with the scaling rule
from `memory/02_pointers.md` §4 (`p + 1` advances by `sizeof(*p)`, and `sizeof(*p)` already
includes this trailing padding).

---

## 3. `offsetof` — measuring a member's actual position

`[Book-derived, B, mentioned via `<stddef.h>`; full library treatment is Phase 7]` The
standard macro `offsetof(type, member)` (`<stddef.h>`) computes a member's byte offset
within its struct, accounting for whatever padding the compiler actually inserted — the
only fully portable way to get this number, since it can vary by compiler/platform/
optimization settings.

```c
#include <stddef.h>
printf("%zu\n", offsetof(struct example, i));   /* prints 4 on a typical platform,
                                                     per the diagram in §2 */
```

---

## 4. Alignment and dynamic allocation — a preview of Phase 6

`[Book-derived, B]` Two facts Book B states explicitly, both worth having in mind well
before Phase 6's full treatment: **`malloc` guarantees whatever alignment is needed for
*any* type** — its returned pointer is safe to use for a `double`, a struct, or anything
else, without you needing to reason about alignment yourself. And, historically, **a union
can be used to force a variable (or an allocator's internal block header) onto a
particular alignment boundary** — by including a member of the strictest type the
allocator needs to support (a `double`, or a pointer), the union's own alignment
requirement becomes at least as strict as that member's, guaranteeing any address correctly
aligned for the union is also correctly aligned for that member. `[External]` C11 added
`_Alignas`/`_Alignof` and `<stdalign.h>` as a direct, purpose-built language feature for
this — the union trick is the pre-C11 technique both source books' era relied on, and it's
still common to see in older or portability-conscious codebases.

---

## 5. Aliasing — a first look (full treatment in Phase 11)

`[Synthesis, expanding the preview from `memory/02_pointers.md` §9 and `06_struct_union_layout.md`
§7]` **Aliasing** is when two different names/expressions refer to the *same* memory. It's
usually just how pointers and references normally work (§9 of `02_pointers.md`) — but it
becomes a distinct hazard specifically when you access the *same bytes* through **two
different types**, which is exactly what a union does when used for "reinterpret these
bytes as something else" rather than the disciplined tagged-union pattern:

```c
union { float f; unsigned int u; } converter;
converter.f = 3.14f;
printf("%x\n", converter.u);   /* "type punning": read the float's bit pattern as
                                   an unsigned int */
```

`[External]` Whether this specific technique is even legal is a genuinely contested,
compiler- and standard-version-dependent question — some compilers explicitly support
exactly this pattern as an extension; the ISO standard's own position on reading a union
member other than the one last written is murkier than either source book lets on (Book B
calls it simply "implementation-dependent," §7 of `06_struct_union_layout.md`). **Don't
treat this file's mention as an endorsement** — it's flagged here only so the vocabulary
("aliasing," "type punning") is in place before `advanced/03_aliasing_and_optimization.md`
(Phase 11) covers the full strict-aliasing rule, why optimizing compilers care about it,
and what the actually-safe alternatives (`memcpy`-based reinterpretation, mainly) look
like.

---

## 6. ⚠️ Misconceptions to unlearn now

- **"A struct's size is the sum of its members' sizes."** Directly contradicted by Book B
  itself (§2) — padding for alignment routinely makes a struct larger than that sum.
- **"Struct member order is purely stylistic."** It can change the struct's total size,
  sometimes substantially, across many instances/array elements (§2's reordering example).
- **"Padding bytes are visible or accessible in the struct declaration."** They're
  compiler-inserted and invisible at the source level — `sizeof` and `offsetof` (§2–3) are
  the only reliable ways to observe their effect.

---

## 7. 🎯 Interview questions

**Easy**
- Why might `sizeof(struct { char c; int i; })` be 8 instead of 5? → Padding inserted so
  `i` lands on a 4-byte-aligned offset (§2).
- What does `offsetof` compute? → A member's actual byte offset within its struct,
  accounting for real (compiler-inserted) padding (§3).

**Medium**
- Why does reordering a struct's members sometimes reduce its total size? → Grouping
  wide/strictly-aligned members first and narrow ones last minimizes the gaps the compiler
  needs to insert between them (§2).
- Why is a struct's *own* alignment requirement equal to its strictest member's
  requirement, and why does that matter for arrays of that struct? → It guarantees every
  element of an array of that struct is correctly aligned, since each element is exactly
  one (possibly trailing-padded) struct-size apart (§2's trailing-padding discussion).

**Hard**
- Explain, precisely, why `malloc`'s alignment guarantee matters even for a program that
  never explicitly reasons about alignment. → Without it, a `malloc`-returned block
  couldn't safely be cast to point at an arbitrary type (a `double`, a struct with strict
  alignment needs) — the guarantee is what makes `malloc`'s `void *` return type usable
  generically at all (§4).
- What's risky about using a union to "reinterpret" a `float`'s bits as an `unsigned int`,
  beyond just "it's unusual style"? → Reading a union member other than the one most
  recently written is implementation-dependent per the standard as both source books
  describe it, and is exactly the kind of aliasing pattern optimizing compilers may assume
  doesn't happen — full treatment of why that assumption can silently break such code is
  `advanced/03_aliasing_and_optimization.md`, Phase 11 (§5 above).

**Systems-level**
- You're optimizing a large array of small structs for cache/memory-footprint reasons.
  What's the first thing worth checking, and why? → Member ordering and resulting padding
  (§2) — the *same* logical fields, reordered, can shrink each element noticeably, and that
  saving multiplies directly across every array element, often mattering more than
  algorithmic micro-optimizations for large, tightly-packed data.

---

## 8. External references

- [cppreference — Object alignment](https://en.cppreference.com/w/c) `[External]` — current
  rules including `_Alignas`/`_Alignof`/`<stdalign.h>` (C11), which neither source book
  covers.
- [CERT C — `DCL` and memory-layout-adjacent rules](https://wiki.sei.cmu.edu/confluence/display/c) `[External]`
  — for a systematic list of alignment/padding-related pitfalls beyond this file's core set.

## 9. 🛠 Practice

1. Reproduce §2's `struct example { char c; int i; }` and print `sizeof` and both members'
   `offsetof` values — confirm they match the diagram.
2. Take a struct with several `char`/`int`/`double` members in a "bad" (naive declaration)
   order, print its `sizeof`, then manually reorder the members from widest-alignment to
   narrowest and compare the new `sizeof` — quantify the savings for yourself rather than
   taking §2's claim on faith.
3. Write the `union { float f; unsigned int u; }` type-punning example from §5, print both
   interpretations, and research (via the external references above) what your specific
   compiler's documented stance is on this pattern's legality — don't assume it's safe
   just because it compiles and "looks right" on your machine.
