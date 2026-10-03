# Interview Questions — Dynamic Memory

> Consolidated from `memory/05_dynamic_memory.md`.

---

## Easy

1. **What's the key difference between `malloc` and `calloc`?** `calloc` zero-initializes
   the memory and safely checks the `count * size` multiplication for overflow; `malloc`
   does neither. (§2–3)
2. **Is `free(NULL)` legal?** Yes — explicitly specified as a safe no-op. (§2)
3. **Does `malloc`'d memory start zero-initialized?** No — it contains whatever garbage was
   already there; only `calloc` zero-initializes. (§2)

## Medium

1. **What's wrong with `p = realloc(p, n);`?** On failure, `realloc` returns `NULL` without
   freeing the original block — this overwrites `p`, losing the only reference to still-
   valid memory and leaking it. Always assign to a temporary pointer first. (§4)
2. **Why does a real allocator's block header often use a `union` with an unused field of a
   wide type like `long`?** A union's alignment matches its strictest member, so including
   a wide type forces every block onto a boundary valid for any type — the mechanism behind
   "`malloc` guarantees alignment suitable for any type." (§5)
3. **Why is `malloc(n * sizeof(*ptr))` preferred over `malloc(n * sizeof(int))`?** It
   automatically tracks `ptr`'s actual type if it's ever changed during a refactor; the
   hardcoded-type version silently keeps computing the old size. (§3)
4. **What's the difference between internal and external fragmentation?** Internal: a
   block is larger than requested due to rounding, wasting space inside it. External:
   enough total free memory exists but no single block is big enough, because free space
   is scattered in small pieces. (§9)

## Hard

1. **Why doesn't freeing the root of a linked tree free the whole structure, and what's
   the fix?** `free` only knows the size of the one block it's given, nothing about struct
   members pointing elsewhere; the fix is an explicit recursive (or stack-based) traversal
   freeing every node, children before parent. (§7)
2. **Explain why a buffer overflow can cause a crash that appears to happen in completely
   unrelated code, much later.** Block headers sit adjacent to allocated space; an
   overflow corrupts the *next* block's header, and the corruption typically isn't
   "noticed" until the allocator later uses that now-garbled header during some unrelated
   `malloc`/`free` call. (§6)
3. **Both K&R's and Plauger's allocators resume scanning where the last search left off,
   rather than always restarting from the beginning. Why?** Always restarting clusters
   allocation activity (and resulting fragmentation) near the start of the list; resuming
   spreads usage — and fragmentation — more evenly across the whole list. (§5)

## Systems-level

1. **Trace what happens, at every level, when a program calls `malloc` in a tight loop
   with no matching `free` calls.** Each call either finds a free block (fast, no OS
   involvement) or, once the free list is exhausted, triggers a `sbrk`/`brk` (or `mmap`)
   system call for more virtual memory, backed by physical memory/swap as pages are
   touched; with no frees, the list never regains blocks to reuse, so memory usage grows
   roughly monotonically. (§1, §5)
2. **A defensive idiom sets a pointer to `NULL` immediately after `free`ing it. What does
   this actually protect against, and what does it not protect against?** Protects against
   an accidental *second* `free` on the same variable, since `free(NULL)` is a safe no-op;
   does not protect against a *different* pointer variable still aliasing the same
   now-freed memory. (§8)
3. **Why does `calloc` check for multiplication overflow but `malloc(n * size)` does
   not?** `calloc` takes `nmemb` and `size` as separate arguments specifically so the
   library can check their product for overflow before allocating; `malloc(n * size)`
   computes that multiplication as ordinary arithmetic *before* `malloc` is even called,
   with no built-in overflow protection. (§3)
