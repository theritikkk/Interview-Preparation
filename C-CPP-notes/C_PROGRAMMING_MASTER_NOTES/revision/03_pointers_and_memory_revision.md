# Revision — Phase 3: Pointers & Memory

> Quick-reference companion to `memory/01_memory_model.md`, `02_pointers.md`, and
> `03_pointer_declarations_parsing.md`. This is the brief's single most-emphasized phase —
> spend real time here before moving on.

---

## The memory model — 1-minute summary

A process's memory is conventionally divided into regions: text/code (instructions),
data/BSS (static/external variables), heap (`malloc`-managed, explicit lifetime), and stack
(automatic variables, one frame per active call, lifetime tied to call/return). A pointer is
just a variable holding an address — a number naming a byte. Zero is guaranteed never a
valid data address, which is what makes `NULL` a safe "points to nothing" sentinel.

### 5-minute revision
- Stack allocation is automatic and LIFO (tied to the calling convention); heap allocation
  is explicit and unordered (you call `malloc`/`free` whenever your code chooses) —
  genuinely different management, not just different regions.
- Virtual memory means two processes can have identical-looking addresses without
  collision — each process's address space is independently mapped by the OS/MMU.

**Worth memorizing:** the five-region layout (text/data/BSS/heap/stack) and which storage
class maps to which.
**Worth understanding, not memorizing:** *why* stack allocation is cheap and heap
allocation isn't (memory_model.md §2) — this explains real performance advice you'll get
later, not just a diagram to recall.

---

## Pointers — 1-minute summary

`&` takes an addressable object's address; `*` dereferences a pointer to reach the object.
Declarations are written to mirror use: `int *p;` means "`*p` is an `int`." An array
decays to a pointer to its first element in most expression contexts, but remains a
distinct, non-reassignable object with a different `sizeof` — "an array is just a pointer"
is false. Pointer arithmetic is scaled by `sizeof(*p)`, not raw bytes. Legal pointer
arithmetic/comparison is restricted to the same array (plus one-past-the-end); anything
else is undefined behavior even when it "looks fine."

### 5-minute revision
- `*ip++` increments `ip`, not `*ip` — parenthesize as `(*ip)++` for the other meaning.
- `sizeof` on an array parameter gives the pointer's size, not the original array's — the
  array decayed at the call; a function can never recover an element count from the array
  alone.
- `const int *p` = pointer to const int (can reassign `p`, can't modify `*p`);
  `int *const p` = const pointer to int (reverse). Read right-to-left from the identifier.
- Three distinct pointer failure modes: NULL (deliberately points to nothing), dangling
  (used to be valid, isn't anymore — e.g. returning `&local`), wild (never initialized at
  all). NULL-checking alone doesn't catch the other two.
- Returning the address of an automatic (local, non-`static`) variable is a classic
  dangling-pointer bug — its stack-frame storage is gone the instant the function returns.

**Worth memorizing:** the legal-vs-illegal pointer operation table (memory/02_pointers.md
§4) — cold, for interviews.
**Worth understanding, not memorizing:** why `a[i]` and `*(a+i)` are the same operation
(pointers.md §3) — you should be able to *derive* one from the other, not just recall that
they're equivalent.

---

## Parsing complicated declarations — 1-minute summary

`()` and `[]` bind tighter than the prefix `*`, so declarations aren't read strictly left
to right. The reading algorithm: find the identifier, look right first (function/array),
then left (pointer), repeating outward, with parentheses forcing a sub-part to resolve as
a unit before continuing. `int (*daytab)[13]` (pointer to a 13-element array) and
`int *daytab[13]` (13-element array of pointers) are the canonical pair showing why the
parenthesis placement matters.

### 5-minute revision
- Algorithm, condensed: identifier → right (`()`/`[]`) → left (`*`) → repeat → base type;
  a parenthesized group resolves before continuing outward.
- `int *comp(void*, void*);` declares a *function* returning `int *` — not a function-
  pointer variable. The intended function-pointer form needs parens around `*comp`.
- `typedef` doesn't change a type, only its spelling for later declarations — a genuine
  readability tool for function-pointer-heavy code, used constantly in real codebases.

**Worth memorizing:** the right-then-left reading algorithm itself, well enough to apply it
cold under interview pressure.
**Worth understanding, not memorizing:** working through one or two of the genuinely
complex examples (`char (*(*x())[])()`) yourself, rather than memorizing their readings —
the goal is transferable parsing skill, not recall of specific example answers.

---

## Cross-topic traps worth drilling before an interview

1. `sizeof` on a function's array parameter — gives the pointer's size, a classic silent
   buffer-size miscalculation (pointers.md §3).
2. Returning `&local_variable` from a function — dangling pointer, sometimes appears to
   work by accident (pointers.md §8).
3. `int *comp(void*, void*);` vs. `int (*comp)(void*, void*);` — function vs. pointer to
   function; the missing parens change the type entirely (declarations_parsing.md §5).
4. Comparing/subtracting pointers into two unrelated arrays — undefined behavior even when
   the types match and the code "runs fine" (pointers.md §4).
5. `int (*daytab)[13]` vs. `int *daytab[13]` — the single example most worth being able to
   parse instantly, cold (declarations_parsing.md §3).
