# Revision — Phase 6: Dynamic Memory

> Quick-reference companion to `memory/05_dynamic_memory.md`.

---

## 1-minute summary

`malloc` returns uninitialized memory; `calloc` zero-initializes and safely checks for
`count * size` overflow. `realloc` leaves the original block untouched on failure —
always assign its result to a temporary pointer first, or a failed call leaks the original
block. Real allocators (both K&R's and Plauger's designs, independently) manage the heap as
an address-ordered linked list of blocks, each with a header (aligned via a union with a
wide type, or an equivalent boundary macro) recording size and a next-pointer; they search
for a fit (first-fit, resuming from the last search position to spread out fragmentation),
request more memory from the OS via `sbrk` only when necessary, and coalesce adjacent free
blocks on `free` to fight external fragmentation. `free` only releases the one block it's
given — freeing a linked structure requires walking it manually, one node at a time.

### 5-minute revision
- `malloc(n * sizeof(*ptr))`, not `sizeof(type)` — tracks the pointer's actual type
  automatically if it ever changes.
- `p = realloc(p, n);` is a bug on the failure path — use a temporary pointer.
- Heap corruption from a buffer overflow often crashes *later*, in unrelated code, once a
  corrupted block header finally gets used — "where it crashes" ≠ "where the bug is."
- `free(node)` on a tree/list node frees only that one block — freeing the whole structure
  needs an explicit (often recursive, children-before-parent) traversal.
- `free(NULL)` is a safe no-op — setting a pointer to `NULL` right after freeing it turns
  an accidental double-free into a harmless one.
- Internal fragmentation (a block bigger than requested, due to rounding) and external
  fragmentation (free space scattered in unusably small pieces) are different costs; no
  allocator design eliminates both at once.

**Worth memorizing:** the `realloc`-into-a-temporary idiom, cold — it's asked constantly
and the failure-path bug is easy to miss in casual code review.
**Worth understanding, not memorizing:** *why* both source books' allocators converge on
almost the same design (free list + alignment-forcing header + resume-scanning search +
coalescing) — dynamic_memory.md §5 — this is what lets you reason about an allocator you've
never seen before, not just recall these two.

---

## Cross-topic traps worth drilling before an interview

1. `p = realloc(p, n);` with no temporary — silent leak on the (rarely-tested) failure path.
2. `free(root)` on a tree — leaks every node except the root; `free` isn't recursive.
3. Assuming `malloc`'d memory starts zeroed — only `calloc` guarantees that.
4. A buffer overflow that corrupts an adjacent block's header, surfacing as a crash in a
   later, unrelated `malloc`/`free` call rather than at the actual overflow site.
5. Double-free after a pointer has already been `free`d and not reset to `NULL` — compare
   against the `free(NULL)`-is-safe defensive idiom.
