# The Memory Model

> **Phase 3 · Pointers & Memory — the brief's single most-emphasized phase.** Depends on:
> `functions/01_functions_and_stack_frames.md` §6 (the stack-frame model) and §5/§8
> (storage classes and initialization).
> Feeds into: `02_pointers.md` (this phase), `memory/05_dynamic_memory.md` (Phase 6 — the
> heap gets its full treatment there), `systems/` (Phase 8–9 — virtual memory is revisited
> with real OS mechanics once file descriptors and processes are in view).
> Sources: **Book B** §5.1's opening framing (primary, but brief — neither book diagrams
> process memory layout). **Most of this file is `[External]`/`[Synthesis]`**, built on top
> of the storage-class vocabulary Phase 2 already established, because none of the three
> books draws the memory-layout picture the brief explicitly asks for.

---

## 1. The machine model: memory as an addressable array

`[Book-derived, B]` Start from the simplest accurate picture: a computer has an array of
consecutively numbered memory cells (bytes), each independently addressable. A `char`
occupies one cell; wider types (`short`, `int`, `long`, …) occupy several consecutive
cells, treated as one unit. A **pointer** is itself a value — typically the same size on
a given machine regardless of what it points to — that holds one of these addresses.

```
address:   0x1000  0x1001  0x1002  0x1003  0x1004  0x1005  ...
contents:  [ byte ][ byte ][ byte ][ byte ][ byte ][ byte ] ...
                    └──────── one int (4 bytes) ────────┘
                    starting at address 0x1001
```

This is the entire physical picture pointers operate on: **an address is just a number
that names a byte**, and a pointer variable is ordinary storage that happens to hold one of
those numbers, tagged (by its declared type) with how many bytes to treat as one unit and
how to interpret them.

---

## 2. 🧠 Mental model: a process's address space has regions

`[External]/[Synthesis]` Neither source book draws this picture, but it's essential for
connecting "where does a variable live" (Phase 2's storage-class vocabulary) to actual
memory addresses. A running program's memory is conventionally divided into regions, each
serving one storage class:

```
   High addresses
  ┌─────────────────────────────┐
  │   Stack                       │  ← grows downward (on most common platforms)
  │   (automatic variables,       │     function parameters, return addresses —
  │    one frame per active call) │     see functions/01 §6
  ├─────────────────────────────┤
  │        ↓  (unused space)  ↑   │
  ├─────────────────────────────┤
  │   Heap                        │  ← grows upward
  │   (malloc/calloc/realloc —    │     memory/05_dynamic_memory.md, Phase 6
  │    explicitly requested,      │
  │    explicitly freed)          │
  ├─────────────────────────────┤
  │   BSS                         │  ← zero-initialized static/external variables
  │   (uninitialized globals,     │     (functions/01 §8: zero by default)
  │    static locals)             │
  ├─────────────────────────────┤
  │   Data                        │  ← explicitly-initialized static/external variables
  │   (initialized globals,       │
  │    static locals)             │
  ├─────────────────────────────┤
  │   Text / Code                 │  ← the program's compiled instructions
  │   (typically read-only)       │     (this is what the linker/loader chain from
  │                                │      fundamentals/01_program_structure.md §1 builds)
  └─────────────────────────────┘
   Low addresses
```

**Mapping storage classes onto regions, connecting directly back to Phase 2:**

| Storage class (from `functions/01…md` §5/§8) | Region | Lifetime |
|---|---|---|
| Automatic (ordinary locals, parameters) | Stack | One function call — created on entry, destroyed on return |
| `static` local, or external/global | Data (if initialized) or BSS (if not) | Entire program run |
| `malloc`/`calloc`/`realloc` result | Heap | From allocation until explicitly `free`d (Phase 6) |
| String literals, function code | Text/read-only data | Entire program run, typically not writable |

### ⚠️ Common misconception
"The stack and the heap are the same kind of memory, just used differently." They're
managed completely differently: the stack's allocation/deallocation is automatic and tied
precisely to function call/return (§6 of the functions file — pure LIFO discipline built
into the calling convention), while the heap is *explicitly* managed by your code calling
`malloc`/`free` in whatever order you choose, with no inherent structure at all — this
difference is *why* stack allocation is essentially free and heap allocation has real,
variable cost (Phase 6 covers this in depth).

### 🔬 Under the hood — this is a simplification
`[External]` Real operating systems don't hand a process *physical* memory laid out this
neatly — each process sees its own **virtual address space**, and the OS/CPU's memory-
management unit (MMU) maps virtual addresses to wherever the underlying physical memory
(or swap/disk) actually lives, via page tables. This is *why* two different processes can
both have a variable at "address 0x7ffee0" without conflict — each process's 0x7ffee0 maps
to different physical memory entirely. Full treatment belongs to
`systems/` (Phase 8–9), once file descriptors and processes are in view; this file only
needs you to know virtual memory exists so "address" doesn't silently mean "physical RAM
location" in your head.

---

## 3. Zero is special: the meaning of a null address

`[Book-derived, B]` C guarantees that the constant `0`, used in a pointer context, is never
a valid address for real data — which is exactly why `0` (conventionally written as the
symbolic constant `NULL`) can safely be used as a sentinel meaning "this pointer doesn't
point to anything." `alloc`'s convention of returning `0` on failure (a rudimentary
allocator Book B builds as a precursor to `malloc`, covered fully in Phase 6) relies
directly on this guarantee. **Full pointer-safety implications** (NULL checks, dangling
vs. null vs. wild pointers) belong to `02_pointers.md` §7 — this section is only the
memory-model fact underlying that discussion.

---

## 4. Why this matters *before* learning pointer syntax

`[Synthesis]` It's tempting to treat pointers as pure syntax (`*`, `&`, `->`) to memorize.
The syntax is trivial next to the mental model: a pointer is a number that names a byte in
one of the regions above, and **everything** about pointer bugs — dangling pointers, stack-
vs-heap confusion, use-after-free, buffer overflows walking off the end of an array — is a
consequence of that address either (a) not being valid anymore for the region it named, or
(b) walking outside the bounds of the object it was meant to point into. Keep this
region-and-lifetime picture in mind through `02_pointers.md`, and later through
`memory/05_dynamic_memory.md` (Phase 6) and `advanced/01_undefined_behavior.md` (Phase 11)
— it's the single mental model that makes all of those topics click instead of feeling like
an arbitrary list of rules to memorize.

---

## 5. 🎯 Interview questions

**Easy**
- Name the typical regions of a process's memory layout. → Text/code, data, BSS, heap,
  stack (§2).
- Which region does an ordinary local variable live in? → The stack, as part of its
  function's stack frame (§2, `functions/01…md` §6).

**Medium**
- What's the fundamental management difference between the stack and the heap? → Stack
  allocation/deallocation is automatic and tied to function call/return (LIFO, built into
  the calling convention); heap allocation is explicit and unordered — you call `malloc`/
  `free` whenever and in whatever order your code chooses (§2).
- Why is address `0` special in C? → The standard guarantees it's never a valid address
  for real data, making it safe to use as a "points to nothing" sentinel (§3).

**Hard**
- Two unrelated processes both have a local variable that happens to print as address
  `0x7ffee0`. Is that a bug? → No — each process has its own virtual address space; the
  same virtual address in two processes maps (via the OS/MMU) to entirely different
  physical memory, so this is expected, not a collision (§2's "under the hood" box).

**Systems-level**
- A program crashes with a segmentation fault when it dereferences a pointer that "looks
  valid" (non-zero, plausible-looking address). What memory-model-level explanations are
  worth checking first? → The address may be valid in a *different* mapping/region than
  the one being assumed (e.g. a stale stack address after the frame that owned it has
  returned — a dangling pointer, `02_pointers.md` §7), or it may fall outside any region
  actually mapped into this process's virtual address space at all, which the OS reports
  as a fault rather than silently allowing (full diagnostic workflow in
  `debugging/04_debugging_workflows_labs.md`, Phase 10).

---

## 6. External references

- [cppreference — Object and storage duration](https://en.cppreference.com/w/c) `[External]`
  — the standard's own (region-agnostic) vocabulary for how long an object's storage lasts,
  which this file maps onto the conventional physical-layout picture.
- [man7.org — `proc(5)`](https://man7.org/linux/man-pages/) `[External]` — on Linux,
  `/proc/<pid>/maps` shows a real running process's actual memory regions; a concrete way
  to see this file's diagram made real, once `systems/` (Phase 8–9) covers processes.

## 7. 🛠 Practice

1. Write a program that prints the address (`&variable`) of a local variable, a `static`
   local variable, a global variable, and (if you're comfortable jumping ahead) a
   `malloc`-returned pointer, all in one run. Compare the relative addresses — do they fall
   into the ordering this file's diagram predicts (stack high, heap and static/global
   lower)? *(Exact addresses and ordering are platform/ASLR-dependent — this is an
   observational exercise, not a portability guarantee.)*
2. On Linux, run a small long-sleeping program and inspect `/proc/<pid>/maps` while it
   runs, to see a real memory map instead of just this file's idealized diagram.
