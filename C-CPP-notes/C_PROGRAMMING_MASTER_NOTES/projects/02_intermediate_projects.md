# Intermediate Projects

> **Phase 12 · Projects & Interview Preparation.** These seven projects need Phases 3–6
> (pointers, arrays/structs, dynamic memory) — they're where this vault's memory-management
> material turns into real, reusable data structures. Two (linked list, custom allocator)
> already have full practice exercises earlier in the vault; they're given a shorter
> entry here with pointers back, so this file can focus its depth on the five that don't.

---

## 1. Dynamic Array (a minimal `vector`)

**Requirements:** an array that grows automatically as elements are appended, with
`push`, `get`, `set`, and `length` operations.

**Architecture:** a struct holding a `malloc`'d buffer pointer, the current element count,
and the buffer's current *capacity* (distinct from count — the whole point of this
structure).

```c
typedef struct {
    int *data;
    size_t count, capacity;
} IntVector;
```

**Concepts exercised:** `memory/05_dynamic_memory.md`'s `realloc` (§4, including its
pointer-assignment trap — this project is exactly where that trap gets hit for real if
you're not careful); the `sizeof(*ptr)` idiom (`memory/05_dynamic_memory.md` §3).

**Implementation strategy:** when `count == capacity` and a push arrives, **double** the
capacity (not increment by 1) and `realloc`. Doubling is the standard strategy specifically
because it gives *amortized* constant-time appends — each element gets copied at most
`O(log n)` times total across all the doublings, not once per push.

**⚠️ Pitfalls:**
- `realloc`'s failure-path trap (`memory/05_dynamic_memory.md` §4): `vec->data =
  realloc(vec->data, new_cap);` loses the original block on failure. Use a temporary.
- Growing by a fixed increment (`capacity += 10`) instead of doubling — technically
  correct, but turns amortized-constant-time appends into quadratic-time ones for large
  `n`, a genuine, measurable performance bug.
- Forgetting that `data` may move on `realloc` — any previously-taken pointer/address into
  the array (`&vec->data[3]`, say) is invalidated the moment a `realloc` actually moves the
  block; never hold such a pointer across a push.

**Testing:** push enough elements to force several growths; confirm `get` returns correct
values after growth; test growth from zero capacity.

**Extension ideas:** a generic version via `void *` + element size (the design `qsort`/
`bsearch` use, `libraries/02_stdlib.md` §3); a `remove`/`shrink` operation.

---

## 2. Stack and Queue

**Requirements:** a LIFO stack (`push`/`pop`/`peek`) and a FIFO queue (`enqueue`/`dequeue`),
each over `int`s (or generalized via `void *`).

**Architecture:** the stack is almost identical to the dynamic array above (push/pop from
one end); the queue is more interesting — implement it as a **circular buffer** (fixed-size
array with wrap-around head/tail indices) or as a **linked list** (§3 below) for unbounded
growth.

**Concepts exercised:** the stack project directly reuses §1; the circular-buffer queue
exercises modular arithmetic (`fundamentals/03_expressions_and_operators.md` §1's `%`) for
wrap-around indexing, and is a good small example of a data structure where an off-by-one
in the "is it full or empty" check is the single most common bug.

**⚠️ Pitfalls (queue, circular-buffer version):**
- Distinguishing "empty" from "full" when `head == tail` is genuinely ambiguous for the
  naive version — the standard fixes are keeping an explicit count, or deliberately wasting
  one slot so the two states are never both representable as `head == tail`.

**Testing:** push/pop (or enqueue/dequeue) past the point where one circular-buffer
instance has fully wrapped around, to directly exercise the wrap-around indexing.

**Extension ideas:** a stack-based expression evaluator (connects directly back to
`projects/01_beginner_projects.md`'s Calculator); a priority queue (needs a heap — a good
next step beyond this vault's core scope).

---

## 3. Linked List — already built; extend it here

`[Cross-reference]` A singly linked list's core (`struct node`, `push_front`, traversal,
`free`) was this vault's own practice exercise in `memory/06_struct_union_layout.md` §12
and revisited in `memory/05_dynamic_memory.md` §14. **If you haven't built it yet, do that
first** — this entry is for extending it:

- Add `push_back` (requires either a tail pointer kept up to date, or a full traversal per
  call — a good, concrete demonstration of why maintaining extra state, like a tail
  pointer, can be a genuine algorithmic improvement, not just convenience).
- Add `remove(value)`, handling the tricky case of removing the **head** node specially
  (the caller's head pointer itself must change, which means `remove` needs a
  `struct node **` or must return the new head — a direct, practical use of
  `memory/03_pointer_declarations_parsing.md`'s pointer-to-pointer material).
- Convert it to **doubly linked** (add a `prev` pointer) and support `remove` without
  needing to search for the predecessor first.

---

## 4. Hash Table

**Requirements:** a string-keyed, integer-valued hash table supporting `insert`, `lookup`,
and `delete`.

**Architecture:** an array of **buckets**, each a linked list (§3) of key/value pairs —
"separate chaining" collision resolution, the most approachable strategy to implement
correctly first.

```c
typedef struct Entry { char *key; int value; struct Entry *next; } Entry;
typedef struct { Entry **buckets; size_t nbuckets; } HashTable;
```

**Concepts exercised:** this project is a direct, practical combination of three earlier
projects/phases at once — a hash function (`libraries/02_stdlib.md`-adjacent "why do we
need a good distribution" reasoning), array indexing, and the linked-list chaining from §3.
It's a genuinely good "have I actually internalized pointers" checkpoint, because getting
the double-pointer insert/delete logic right (same issue as §3's head-removal case, now at
every bucket) requires real fluency, not memorized syntax.

**Implementation strategy:** a simple string hash (e.g., djb2: `hash = hash * 33 + c` for
each character) is sufficient for this project — the focus is the table's structure, not
hash-function design. `hash % nbuckets` picks the bucket; insert/lookup/delete all start by
computing this and then operate on that bucket's linked list.

**⚠️ Pitfalls:**
- Not copying the key string on insert (`strdup`, or your own `malloc`+`strcpy`) — storing
  a pointer to a caller-owned buffer that may change or be freed later is a dangling-pointer
  bug waiting to happen (`memory/02_pointers.md` §8).
- Forgetting to free **both** the entry struct and its copied key string on delete — a
  two-part leak if only one is freed (`memory/05_dynamic_memory.md` §7-adjacent reasoning).
- A poor hash function or too few buckets degrading every lookup to the linked list's
  O(n) worst case — directly echoing `memory/06_struct_union_layout.md` §10's
  already-sorted-input binary-tree degradation, same underlying lesson in a different
  structure.

**Testing:** insert many keys that collide into the same bucket deliberately (by using a
deliberately bad hash for testing) to confirm chaining actually works; delete a key from
the middle of a chain and confirm the chain is still correctly linked afterward.

**Extension ideas:** automatic resizing (rehashing) once the load factor crosses a
threshold — a direct conceptual cousin of the dynamic array's doubling strategy in §1.

---

## 5. Custom String Library

**Requirements:** re-implement a meaningful slice of `<string.h>` from scratch:
`my_strlen`, `my_strcpy`, `my_strcmp`, `my_strcat`, `my_strchr`, `my_memcpy`, `my_memmove`.

**Architecture:** one function per `<string.h>` function, matching the real contracts
exactly — this project's whole value is faithfulness to the real semantics, not inventing
simpler ones.

**Concepts exercised:** this project directly completes practice exercises already seeded
in `memory/02_pointers.md` §13 (`my_strlen`/`my_strcpy`/`my_strcmp`) and
`libraries/03_string.md` §10 (`my_memmove`'s overlap-safety, `my_strncpy`/`my_strncat`'s
asymmetric termination behavior) — if you've done those, this project is assembling them
into one coherent library plus the remaining functions.

**⚠️ Pitfalls:** every one of `libraries/03_string.md`'s documented real-library warts
(§2, §4) applies here directly — faithfully reproducing `strncpy`'s non-termination and
`strncat`'s always-termination, rather than "fixing" them, is the actual point of the
exercise.

**Testing:** test each function against the real `<string.h>` version's behavior on the
same inputs (including edge cases: empty strings, `my_memmove` with genuinely overlapping
regions in both directions).

**Extension ideas:** `my_strtok` (requires genuinely understanding the "state kept
between calls" design from `libraries/03_string.md` §5 well enough to reproduce it).

---

## 6. Custom Memory Allocator — already built; extend it here

`[Cross-reference]` A fixed-block free-list allocator (`my_malloc`/`my_free`) was this
vault's own practice exercise in `memory/05_dynamic_memory.md` §14, directly modeling both
K&R's and Plauger's real designs from that file's §5. **If you haven't built it yet, do
that first.** Extensions once it exists:

- Add **coalescing** on free (merge a freed block with an adjacent free neighbor) if your
  first version didn't — this is the single biggest fragmentation-fighting improvement,
  per `memory/05_dynamic_memory.md` §5's description of both books' real designs.
- Add the defensive sanity checks Plauger's implementation performs (§6 of that file) —
  verifying a freed block's size field is plausible before trusting it, to catch some
  corruption/double-free cases defensively.
- Run it under AddressSanitizer deliberately with injected bugs (`debugging/02_sanitizers_and_valgrind.md`)
  to confirm your allocator's own bugs are catchable the same way real ones are.

---

## 7. 🎯 What these seven projects have in common

`[Synthesis]` Every one is a **data structure with an ownership story**: who allocates,
who's responsible for freeing, and what happens when the structure needs to grow or shrink.
That ownership discipline (`memory/05_dynamic_memory.md` §10) — not the specific algorithm
— is the transferable skill, and it's exactly what `projects/03_systems_projects.md`'s
larger tools assume you now have solid.
