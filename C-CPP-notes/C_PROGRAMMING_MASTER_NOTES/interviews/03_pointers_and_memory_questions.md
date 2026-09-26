# Interview Questions — Pointers & Memory

> Consolidated from `memory/01_memory_model.md`, `02_pointers.md`, and
> `03_pointer_declarations_parsing.md`. This phase is the brief's single most-emphasized
> topic — expect these to come up in essentially any C or systems interview.

---

## Easy

1. **Name the typical regions of a process's memory layout.** Text/code, data, BSS, heap,
   stack. (`memory_model.md` §2)
2. **Which region does an ordinary local variable live in?** The stack, as part of its
   function's stack frame. (§2)
3. **What does `int *p;` declare?** `p` is a pointer to `int`.
   (`03_pointer_declarations_parsing.md` §2)
4. **What's the difference between a NULL pointer and a dangling pointer?** NULL
   deliberately points to nothing; a dangling pointer holds a stale address to an object
   that no longer exists. (`02_pointers.md` §8)
5. **What does the `&` operator require of its operand?** An addressable object in memory
   (a variable or array element) — never an expression, constant, or `register` variable. (§1)

## Medium

1. **What's the fundamental management difference between the stack and the heap?** Stack
   allocation/deallocation is automatic and LIFO, tied to function call/return; heap
   allocation is explicit and unordered, entirely under the program's control.
   (`memory_model.md` §2)
2. **Why does `p + 1` advance by 4 bytes for an `int *` but 1 byte for a `char *`?** Pointer
   arithmetic is defined in terms of elements of the pointed-to type; the offset is scaled
   by `sizeof(*p)`. (`02_pointers.md` §4)
3. **Is `sizeof` on an array function-parameter the same as `sizeof` on the original array
   at the call site?** No — the parameter has decayed to a pointer, so `sizeof` gives the
   pointer's size. (§3)
4. **What's the difference between `const int *p` and `int *const p`?** The former forbids
   modifying `*p` but allows reassigning `p`; the latter is the reverse. (§6)
5. **What's the difference between `int *daytab[13]` and `int (*daytab)[13]`?** Array of 13
   pointers to `int`, versus a pointer to one 13-element array of `int`.
   (`03_pointer_declarations_parsing.md` §3)

## Hard

1. **Why is returning the address of a local variable undefined behavior, even if the
   returned pointer "still works" in a quick test?** The local's stack-frame storage is
   deallocated on return; nothing guarantees what (if anything) is still there afterward —
   apparent correctness is an accident of a specific call pattern, not a guarantee.
   (`02_pointers.md` §8)
2. **Why can't `void *` be dereferenced directly?** Dereferencing needs to know how many
   bytes to read and how to interpret them, both of which depend on a concrete pointed-to
   type that `void` doesn't have. (§7)
3. **Parse `char (*(*x())[])()` and narrate each step.** `x`: function returning a pointer
   to an array of pointers to functions returning `char` — read outward from `x`,
   alternating right (`()`/`[]`) then left (`*`), with explicit parens marking where each
   "pointer to" binds before the next `()`/`[]` applies.
   (`03_pointer_declarations_parsing.md` §4)
4. **Two unrelated processes both show a local variable at the same-looking address. Bug or
   not?** Not a bug — each process has its own independently-mapped virtual address space;
   identical virtual addresses across processes map to different physical memory.
   (`memory_model.md` §2)

## Systems-level

1. **A function takes `int arr[]` and internally computes
   `sizeof(arr) / sizeof(arr[0])` hoping to count elements. What's wrong, and what's the
   fix?** `arr` decayed to a pointer as a parameter, so this computes a meaningless
   (typically small/wrong) count; pass the element count explicitly instead.
   (`02_pointers.md` §3)
2. **Explain, in terms of stack frames, why a dangling pointer to a local might "work"
   immediately after the function returns but break later.** The frame's memory isn't
   cleared on return, just marked reusable — the old data may coincidentally survive until
   another call's frame overwrites that region, which is why dangling-pointer bugs are
   notoriously intermittent. (§8)
3. **A colleague writes `int *comp(void*, void*);` intending a comparator pointer to pass
   into a generic sort routine. What's actually declared, and what happens when they try to
   use it as intended?** It declares a *function* named `comp` returning `int *`, not a
   pointer variable — passing it where a `int (*)(void*, void*)` is expected is a type
   mismatch the compiler catches at compile time, not a silent runtime bug.
   (`03_pointer_declarations_parsing.md` §5)
4. **A program crashes with a segfault dereferencing a pointer that "looks valid" (non-
   zero, plausible address). What memory-model-level explanations are worth checking
   first?** A stale (dangling) address from a mapping/region that's no longer valid for it
   (e.g. a returned stack address), or an address outside any region mapped into this
   process's virtual address space at all. (`memory_model.md` §5)
