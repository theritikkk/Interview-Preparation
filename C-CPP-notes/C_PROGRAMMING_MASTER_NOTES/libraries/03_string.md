# `<string.h>` — Three Families, One Accretion History

> **Phase 7 · The Standard Library, By Design.** Depends on: `memory/04_arrays_and_strings.md`
> (the `sizeof`/`strlen` distinction and the pointer-based `strcpy` idioms this file
> formalizes into the real library's contracts), `memory/02_pointers.md` §4 (why
> overlapping-pointer operations need special care).
> Feeds into: `advanced/03_aliasing_and_optimization.md` (Phase 11 — `restrict` and why
> `memcpy` may not handle overlap).
> Sources: **Book C** ch.14 (primary — including Plauger's own candid critique of the
> header's design), **Book A** §27.2 (practical `<string.h>` tour), **Book B** Appendix
> B.3 (quick reference).

---

## 1. Three families, by naming convention

`[Book-derived, C]` Every function in this header falls into one of three groups, and the
group is encoded directly in the function's name prefix:

| Prefix | Operates on | Length determined by |
|---|---|---|
| `mem*` | arbitrary bytes (not necessarily text) | an explicit count `n`, always |
| `strn*` | text, but bounded | the smaller of an explicit `n` or the string's own `'\0'` |
| `str*` (no `n`) | null-terminated text only | the string's own `'\0'`, exclusively |

`[Synthesis]` Knowing this convention lets you predict a function's basic contract from its
name alone, before ever looking it up — `memmove` will take an explicit count and not care
about `'\0'` at all; `strncpy` will stop early at a `'\0'` even if `n` hasn't been reached
yet; `strcat` will trust its input is properly terminated and has no built-in bound at all.

---

## 2. 💡 A library designer's own critique — genuinely worth reading

`[Book-derived, C]` Plauger, who helped standardize this header, is unusually candid that
it is **not** the product of a single coherent design — it accreted from many contributors
over years, and by standardization time it was too late to fix real warts:

- Many search functions return `NULL` on failure — forcing every caller to test the return
  value before using it further, when returning a pointer to the string's end would often
  have been just as valid a "not found" signal *and* more directly usable in further
  expressions.
- The copying functions return a pointer to the **start** of the destination, when the
  **end** of the copy is frequently more useful (it lets you chain multiple copies
  efficiently without a separate `strlen` call to find where the previous copy ended).
- Some names are simply opaque (`strcspn`, `strpbrk`) — you cannot guess their behavior
  from their name the way you often can elsewhere in this header.
- The set is inconsistent: `strnlen` and `memrchr` (later, sensible additions in some
  implementations) contrast with `strncat`'s genuinely surprising bounding behavior (§4).

`[Synthesis]` This is worth internalizing as a general lesson, not just trivia about one
header: **a widely-used API's warts often reflect its real history**, not a deliberate
design choice you're missing the rationale for — and being able to recognize "this is
probably historical accretion, not deep wisdom" is itself a useful engineering instinct.

---

## 3. `memcpy` vs. `memmove` — the overlap distinction, precisely

`[Book-derived, C]`

```c
void *memcpy(void *s1, const void *s2, size_t n);   /* UNDEFINED if s1/s2 overlap */
void *memmove(void *s1, const void *s2, size_t n);  /* SAFE even if s1/s2 overlap */
```

**Why the difference exists, precisely:** `[Book-derived, C]` `memmove`'s standard-mandated
behavior is defined *as if* the source bytes were first copied into a temporary buffer, and
*then* copied from there into the destination — guaranteeing correctness regardless of
overlap direction, at the cost of that conceptual (and often literal) extra step.
`memcpy` carries no such guarantee specifically so an implementation is free to copy in
whichever direction (or however many bytes at a time) is fastest on the target hardware,
without needing to detect or handle overlap at all. `[Synthesis, tying directly to
`fundamentals/02_types.md` §6's `restrict` introduction]` This is exactly why `memcpy`'s
real prototype (in the standard) qualifies both pointer parameters `restrict` — the
programmer is explicitly promising non-overlap, which is what licenses the implementation's
freedom to optimize without an overlap check.

### 🎯 Interview point
"When would you ever choose `memcpy` over the always-safe `memmove`?" — Whenever you can
guarantee non-overlap yourself (e.g., copying between two independently-allocated buffers)
and performance matters — `memcpy` is potentially faster precisely because it's allowed to
skip work `memmove` cannot skip.

---

## 4. `strcpy`/`strncpy`/`strcat`/`strncat` — a real, commonly-tested asymmetry

`[Book-derived, C]`

```c
char *strcpy(char *s1, const char *s2);              /* copies through s2's '\0', inclusive */
char *strncpy(char *s1, const char *s2, size_t n);     /* copies AT MOST n chars; if s2 is
                                                           shorter than n, PADS s1 with
                                                           '\0' out to n total */
char *strcat(char *s1, const char *s2);                /* appends s2 (through its '\0')
                                                           onto the end of s1 */
char *strncat(char *s1, const char *s2, size_t n);      /* appends AT MOST n chars of s2,
                                                           and ALWAYS appends a terminating
                                                           '\0' afterward */
```

### ⚠️ The trap `memory/04_arrays_and_strings.md` §8 already warned about — now with the exact reason

**`strncpy` does *not* guarantee a null-terminated result** if `s2` (the source) is `n`
characters long or longer — it simply stops after copying `n` characters, terminator or
not. **`strncat`, by contrast, *always* appends a terminating `'\0'`** after copying at
most `n` characters from its source — a genuinely inconsistent guarantee between two
functions with parallel-looking names and signatures, exactly the kind of wart §2 already
warned about. This is precisely why `memory/04_arrays_and_strings.md` §8's defensive
pattern force-terminates manually after `strncpy` (`buffer[sizeof(buffer)-1] = '\0';`) —
it's compensating for a real, standard-mandated gap, not being needlessly cautious.

### 💡 Why `strncpy` pads with extra `'\0'`s at all
`[Book-derived, C]` This looks strange until you know its origin: `strncpy` was designed
for early UNIX's fixed-width directory-entry filename fields, where padding a short name
out to the field's full width with zero bytes was exactly the desired behavior — it was
never primarily designed as a "safe `strcpy`" at all, and using it as one (as many
programmers do today) is a repurposing of a function built for a narrower original job.

---

## 5. `strtok` — powerful, stateful, and not reentrant

`[Book-derived, C]` `strtok` splits a string into tokens across a **sequence of calls**:
the first call passes the real string; every subsequent call in the same sequence passes
`NULL`, and `strtok` resumes from an internally-saved position.

```c
char str[] = "?a???b,,,#c";
char *t = strtok(str, " ?");     /* t -> "a"    */
t = strtok(NULL, ",");            /* t -> "??b"  */
t = strtok(NULL, ",#");           /* t -> "c"    */
t = strtok(NULL, " ?");           /* t -> NULL (no tokens left) */
```

### ⚠️ Two real hazards, both book-derived

`[Book-derived, C]` First: **`strtok` writes into its input string** — it overwrites each
delimiter it finds with `'\0'` to terminate the current token in place. Calling it on a
string literal (`memory/04_arrays_and_strings.md` §4's read-only-storage warning) is
undefined behavior; it must be called on a genuinely writable `char` array.

Second: the standard itself states implementations must behave **as if no library function
calls `strtok`** — a direct acknowledgment that it keeps its "where was I" state
**internally, typically in a single shared location** — meaning **`strtok` is not safe to
use from more than one logical tokenization "in flight" at once** (nested calls, or use
across multiple threads, corrupt each other's progress). `[External]` This is precisely why
POSIX provides `strtok_r`, a re-entrant version taking an explicit "where am I" pointer the
caller manages — worth knowing exists, even though it's POSIX rather than ISO C.

---

## 6. Comparison functions, briefly

`[Book-derived, C]` `strcmp`/`strncmp` compare null-terminated text lexicographically,
returning negative/zero/positive (matching the convention this vault's own `my_strcmp`
practice exercise in `memory/02_pointers.md` §13 already used); `memcmp` does the same for
`n` raw bytes, with no `'\0'`-awareness at all — comparing binary data (not necessarily
text) is `memcmp`'s actual job.

---

## 7. ⚠️ Misconceptions to unlearn now

- **"`strncpy` is just a safe, bounded version of `strcpy`."** It doesn't guarantee
  termination the way that framing implies (§4) — it was designed for a different,
  narrower original purpose (fixed-width padded fields), and using it as a safety net
  requires manually force-terminating afterward.
- **"`strtok` can be called safely from two places at once (e.g., nested loops each
  tokenizing a different string) as long as neither modifies the other's string."** It
  cannot — its "current position" state is shared internally, not tied to which string
  you're tokenizing (§5).
- **"`memcpy` and `memmove` are interchangeable; `memmove` is just the safe default with no
  downside."** They can differ in performance specifically because `memcpy` is allowed to
  skip overlap-handling work `memmove` cannot skip (§3) — not a pure strict improvement,
  a genuine tradeoff.

---

## 8. 🎯 Interview questions

**Easy**
- What's the naming-convention difference between `mem*`, `strn*`, and `str*` functions? →
  Explicit count only; the smaller of an explicit count or `'\0'`; `'\0'`-terminated only,
  respectively (§1).
- Does `strncpy` always null-terminate its destination? → No — only if the source is
  shorter than `n`; otherwise it simply stops after `n` characters (§4).

**Medium**
- Why is `memcpy` undefined behavior on overlapping buffers, while `memmove` is safe? →
  `memmove`'s contract is defined as if copying through a temporary buffer first,
  guaranteeing correctness regardless of overlap; `memcpy` makes no such promise, which is
  exactly what lets an implementation optimize it more aggressively for the non-overlapping
  case it's meant for (§3).
- What's the actual difference in null-termination behavior between `strncpy` and
  `strncat`? → `strncpy` may leave the result unterminated if the source is `>= n`
  characters; `strncat` always appends a terminator regardless (§4).

**Hard**
- Why does the standard require implementations to behave "as if no library function calls
  `strtok`"? → It's an explicit acknowledgment that `strtok` keeps its tokenization state
  internally (commonly in a single shared/static location), making it unsafe to have more
  than one tokenization sequence "in flight" at once — nested or concurrent use corrupts
  each other's progress (§5).
- `memcpy`'s real standard prototype qualifies both pointers `restrict`. What does that
  actually license the implementation to do, and why doesn't `memmove`'s prototype do the
  same? → `restrict` is the caller's promise of non-overlap, letting `memcpy`'s
  implementation choose whichever copy direction/strategy is fastest without checking for
  overlap; `memmove` explicitly supports overlap, so making the same promise would be a
  lie the implementation couldn't rely on (§3, `fundamentals/02_types.md` §6).

**Systems-level**
- A library function you're writing needs to tokenize a caller-supplied string using
  `strtok`, but the caller's code also happens to call another function that internally
  uses `strtok` on a different string, nested inside your function's tokenization loop.
  What breaks, and what's the fix? → Both tokenization sequences share the same internal
  "current position" state, so the nested call corrupts your own in-progress tokenization
  the moment it runs; the fix is `strtok_r` (POSIX) or an equivalent explicit-state
  alternative, never plain `strtok` in code that might be nested or called concurrently (§5).

---

## 9. External references

- [cppreference — `<cstring>`/`<string.h>`](https://en.cppreference.com/w/c) `[External]` —
  current, complete function reference.
- [man7.org — `strtok(3)`](https://man7.org/linux/man-pages/) `[External]` — includes
  `strtok_r`'s signature and the thread-safety discussion §5 previews.

## 10. 🛠 Practice — the phase project

1. `[Book-derived, C]` Re-implement `memmove` so it's correct regardless of overlap
   direction (hint: compare the pointers to decide whether to copy forward or backward),
   and confirm it — unlike a naive forward-only copy loop — handles both
   `memmove(buf, buf+1, n)` and `memmove(buf+1, buf, n)` correctly on the same buffer.
2. Re-implement `strncpy` faithfully (including its padding-to-`n` behavior) and
   `strncat` (including its always-terminate behavior) from scratch, deliberately
   preserving §4's asymmetry rather than "fixing" it — the point is understanding the real
   contracts, not designing a nicer replacement.
3. Write a small `printf`-lite formatter (supporting at minimum `%d`, `%s`, `%c`) built on
   this file's and `libraries/08_stdarg.md`'s material, to round out the phase project
   alongside your `<string.h>` slice.
4. Use `strtok` (deliberately) inside a loop that also calls a second, independent
   tokenizing function using `strtok` internally, and observe the corruption from §5's
   hazard directly — then fix it with `strtok_r`.
