# Interview Questions — Arrays, Strings, Structs & Unions

> Consolidated from `memory/04_arrays_and_strings.md`, `06_struct_union_layout.md`, and
> `07_alignment_padding_aliasing.md`.

---

## Easy

1. **Do C array subscripts start at 0 or 1?** 0, always.
   (`04_arrays_and_strings.md` §1)
2. **What's the difference between `sizeof(s)` and `strlen(s)` for `char s[] = "hi"`?**
   3 (compile-time array size, terminator included) vs. 2 (runtime character count,
   terminator excluded). (§5)
3. **What are the only legal operations on a struct as a whole?** Copy/assign as a unit,
   take its address, access members — never `==`/`!=`. (`06_struct_union_layout.md` §1)
4. **What's the size of a union relative to its members?** At least the size of its
   largest member. (§7)
5. **Why might `sizeof(struct { char c; int i; })` be 8, not 5?** Padding inserted so `i`
   lands on a properly aligned offset. (`07_alignment_padding_aliasing.md` §2)

## Medium

1. **Why must a 2D array function parameter specify the column count but not the row
   count?** The compiler needs every dimension but the outermost to compute row offsets
   via pointer-arithmetic scaling. (`04_arrays_and_strings.md` §2)
2. **Why is modifying a string reached through `char *p = "literal";` undefined
   behavior?** String literals are commonly stored in read-only, possibly shared/pooled
   memory. (§4)
3. **Why does `*pp.x` fail to compile for `struct point *pp;`?** `.` binds tighter than
   `*`, so it parses as `*(pp.x)`, illegal since `x` isn't a pointer; use `(*pp).x` or
   `pp->x`. (`06_struct_union_layout.md` §2)
4. **Why can a struct hold a pointer to its own type but not an instance of its own
   type?** A pointer has fixed size regardless of target; a direct instance would need
   infinite storage. (§5)
5. **Why does reordering struct members sometimes reduce total size?** Grouping wide/
   strictly-aligned members first and narrow ones last minimizes inserted padding.
   (`07_alignment_padding_aliasing.md` §2)

## Hard

1. **Explain why `int a[10][20]` and `int *b[10]` both accept `[3][4]`-style subscripting
   but have completely different memory layouts.** `a` is one contiguous 200-int block
   addressed by `20*row + col`; `b` is 10 independent pointer slots, each potentially
   pointing to differently-sized, non-contiguous storage. (`04_arrays_and_strings.md` §3)
2. **Walk through why `while (*s++ = *t++);` correctly copies a string including its
   terminator.** Postfix `++` yields the old value before incrementing; each iteration
   fetches the old `*t`, stores it at the old `s` position, advances both pointers, and
   the stored value (doubling as the loop test) makes the loop stop right after copying
   `'\0'`. (§6)
3. **Walk through `addtree`'s recursive insertion and explain why it returns `p` on every
   path.** The return value is how each recursion level re-links its own child pointer
   to whatever node now occupies that slot — the existing node on a match, a freshly
   allocated one on a new insert. (`06_struct_union_layout.md` §5)
4. **Why must a self-referential `typedef`'d struct still carry an explicit tag?** The
   self-reference needs a name available *before* the `typedef` name is complete; only
   the tag exists at that point. (§6)

## Systems-level

1. **A function receives `char *buf` (originally a fixed-size array at the call site) and
   does `strncpy(buf, src, sizeof(buf))`. What's wrong?** `buf` is a pointer parameter —
   `sizeof(buf)` gives the pointer's size, not the original buffer's real capacity; pass
   the capacity explicitly. (`04_arrays_and_strings.md` §5, §8)
2. **A binary/network format specifies exact bit positions for several flags. Why is a C
   bit-field struct risky here, and what's safer?** Bit-field packing order and boundary-
   straddling behavior are implementation-defined; explicit shift/mask on a plain unsigned
   type guarantees exact, portable bit positions. (`06_struct_union_layout.md` §8)
3. **You're optimizing a large array of small structs for memory footprint. What's the
   first thing worth checking?** Member ordering and resulting padding — the same logical
   fields, reordered, can shrink each element, and the saving multiplies across every
   array element. (`07_alignment_padding_aliasing.md` §7)
4. **Why does `malloc`'s alignment guarantee matter even for code that never explicitly
   reasons about alignment?** Without it, a `malloc`-returned block couldn't safely be
   cast to point at an arbitrary type with strict alignment needs (a `double`, certain
   structs) — the guarantee is what makes `void *`-returning generic allocation usable at
   all. (§4)
