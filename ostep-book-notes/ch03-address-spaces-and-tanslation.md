# OSTEP Notes 3 — Address Spaces, the Memory API, and Address Translation

> Covers Ch13 (Address Spaces), Ch14 (Memory API), Ch15 (Address Translation / base-and-bounds), Ch16 (Segmentation). See `00-index.md` for the full series map.

---

## 1. The Abstraction: Address Spaces

**Early systems** gave a running program *direct* access to physical memory — simple, but offered no protection (a buggy program could corrupt the OS or another program) and no way to run more than one program at once.

⭐ **The address space** is the OS's abstraction of memory: **the running program's own view of memory**, separate from where it actually sits in physical RAM.

### What's in an address space
| Region | Contents | Growth |
|---|---|---|
| **Code** | Program instructions | Static — placed once, doesn't grow |
| **Heap** | Dynamically-allocated data (`malloc()`) | Grows **downward** from just after code |
| **Stack** | Local variables, function params, return addresses | Grows **upward** from the top |

```
   0KB ┌─────────────────┐
       │  Program Code   │
   1KB ├─────────────────┤
       │      Heap       │  ← grows downward
       │       ↓         │
       │                 │
       │   (free space)  │
       │                 │
       │       ↑         │
       │      Stack      │  ← grows upward
  16KB └─────────────────┘
```
⭐ Code and heap/stack are placed at **opposite ends** of the address space specifically so each can grow toward the middle without a fixed boundary between them. (This convention breaks down once multiple **threads** share one address space — each thread needs its own stack, and there's no longer a clean "opposite ends" layout — foreshadowing the Concurrency notes file.)

### The Crux: How to Virtualize Memory
> Every process believes it's loaded at address 0 with a large, private address space — the *reality* is that physical memory is shared, and each process is actually loaded at some arbitrary physical offset. When process A loads from virtual address 0, the OS + hardware must transparently redirect that to A's **actual** physical location (e.g. 320KB).

⭐ **Tip: The Principle of Isolation.** Properly isolating two entities means one can fail without affecting the other. Memory isolation between processes (and between processes and the OS) is a direct application of this principle — some OSes (**microkernels**) even wall off pieces of the OS from *other* pieces of the OS for extra reliability.

### Goals of Memory Virtualization
- **Transparency** — the program shouldn't notice it's being virtualized at all.
- **Efficiency** — both in time (minimal added instructions) and space (minimal extra memory for OS structures).
- **Protection** — a process must not be able to read/write memory outside its own address space (this simultaneously protects **other processes AND the OS itself**).

---

## 2. Interlude: The Memory API

### `malloc()` and `free()`
```c
double *d = (double *) malloc(sizeof(double));
```
- `malloc()` is a **library call**, not a system call (built on top of lower-level OS calls like `brk`/`mmap` that actually request memory from the kernel). It takes a `size_t` byte count and returns `void*` — cast to the appropriate type by convention (readability, not strict necessity).
- `free()` takes **only** the pointer previously returned by `malloc()` — the allocation size is **not** passed back in; the malloc library itself must track it internally (typically via a small header just before the returned pointer).

### Common Memory Errors (a very testable list)
| Error | Example | Consequence |
|---|---|---|
| **Forgetting to allocate** | `char *dst; strcpy(dst, src);` | Segmentation fault |
| **Not allocating enough** (buffer overflow) | `malloc(strlen(src))` instead of `strlen(src)+1` | May "work" by luck, may silently corrupt adjacent data, may crash — nondeterministic and dangerous |
| **Forgetting to initialize** | Reading before writing | Undefined behavior — reads whatever garbage was already on the heap |
| **Memory leak** (forgetting `free()`) | — | Fine for short-lived programs (OS reclaims everything at exit); fatal for long-running servers/the OS kernel itself, which will eventually run out of memory |
| **Dangling pointer** (using memory after `free()`) | Using `x` after `free(x)`, especially after a subsequent `malloc()` reuses that space | Crashes, or silent corruption of unrelated data |
| **Double free** | Calling `free()` twice on the same pointer | Undefined behavior, often a crash |
| **Invalid free** | Passing `free()` something *not* returned by `malloc()` | Undefined, dangerous |

⭐ **Tip: "It compiled or it ran" ≠ "It is correct."** All the above examples compile cleanly and often *appear* to run correctly — memory bugs are famously **nondeterministic**, which is exactly what makes them so hard to track down. Tools like **Purify** and **Valgrind** exist specifically because these bugs are so pervasive and hard to spot by inspection alone.

⭐ **Aside — why no memory is truly "leaked" once a process exits:** there are **two levels** of memory management: the **OS** hands memory to processes and reclaims *all* of it (code, stack, heap pages) unconditionally when a process exits; the **process itself** manages its own heap via `malloc`/`free`. So a `malloc()` without a matching `free()` in a short program is harmless in practice (the OS cleans it up at exit) — but it's still bad *habit*, and becomes a real, eventually-fatal problem in any **long-running** process (a server, a database, or the OS kernel itself).

### Underlying OS Support
`malloc`/`free` are library calls that internally invoke real system calls — most notably `brk` (or `sbrk`), which changes the location of the program's **break** (the end of the heap) — but application code should essentially never call `brk` directly; use `malloc`/`free` instead. `mmap()` can also be used to create an anonymous memory region usable as heap-like space.

---

## 3. Mechanism: Address Translation

> **Tip: use limited direct execution, just as with the CPU.** For memory, the goal is the same as for CPU virtualization (previous notes file) — let the program run mostly directly on the hardware, with the OS stepping in only where needed for protection.

⭐ **Hardware-based address translation:** on every memory reference, hardware **transforms the virtual address** the program generates into a **physical address** where the data actually resides — transparently and efficiently, without the OS having to intervene on every single memory access.

### Dynamic (Hardware-Based) Relocation — Base and Bounds
The oldest scheme (late 1950s), also called **dynamic relocation.** Requires **two hardware registers per CPU**: a **base register** and a **bounds (limit) register.**

- Every program is **compiled as if loaded at address 0.** At load time, the OS picks a physical location for it and sets the **base register** to that value.
- On every memory access:
```
physical_address = virtual_address + base
```
- The **bounds register** enforces protection: the hardware checks the virtual address against it *before* translation; if it's negative or ≥ the bounds, the CPU raises an **exception**, and the OS typically terminates the offending process.

**Worked example** — process's 4KB address space loaded at physical 16KB:
| Virtual Address | Physical Address |
|---|---|
| 0 | 16 KB |
| 1 KB | 17 KB |
| 3000 | 19384 |
| 4400 | **Fault** (out of bounds — exceeds the 4KB bound) |

- ⭐ Base and bounds registers together form the **memory management unit (MMU)** — the piece of the CPU that helps with address translation.
- **Software-based static relocation** (an older, pre-hardware-support alternative) had a **loader** physically rewrite instruction addresses at load time (e.g. `movl 1000, %eax` becomes `movl 4000, %eax` if loaded at offset 3000). ⚠️ Its critical flaw: **no protection at all** — a process can still generate a bad address and access anyone else's memory, and once placed, an address space is very hard to relocate again. This is precisely why hardware support (dynamic relocation) replaced it.

### Hardware Support Summary
| Requirement | Purpose |
|---|---|
| **User/kernel mode bit** | Determines what the currently running code is allowed to do (from the previous notes file) |
| **Base and bounds registers** | Perform translation (base) and protection (bounds), one pair per CPU |
| **Privileged instructions to modify base/bounds** | Only the OS (kernel mode) may change where a process's address space is relocated |
| **Exception-raising ability** | CPU must trap to the OS on an out-of-bounds access, or on an attempt to modify base/bounds from user mode |

### OS Responsibilities Under Dynamic Relocation
1. **On process creation:** find free space for the new address space in physical memory (tracked via a **free list** — a simple list of not-currently-in-use physical memory ranges).
2. **On process termination:** reclaim its memory, adding it back onto the free list.
3. **On context switch:** save the outgoing process's base/bounds values (into its process structure) and load the incoming process's values into the actual hardware registers — this is why base/bounds are sometimes described as **part of each process's context**, alongside the general-purpose registers covered in the previous notes file.
4. **Exception handlers:** must be installed at boot time, exactly like the trap handlers for system calls (previous notes file) — one for "out-of-bounds access," one for "attempted privileged instruction in user mode."

⭐ **Aside — the term "segmentation fault":** it originates from exactly this kind of illegal-memory-access exception on a segmented machine (see Section 4 below) — the term stuck around even on modern machines that don't literally implement "segments" anymore.

---

## 4. Segmentation

### The Problem With Plain Base-and-Bounds
Base-and-bounds relocates the **entire** address space as one contiguous unit — but recall the address space diagram above: there's often a **big chunk of unused space** between the heap and stack. Relocating the whole thing (including that gap) wastes physical memory, and makes it impossible to run a program whose *full* address space (unused middle included) doesn't fit in available physical memory — e.g., a 32-bit (4GB) address space where the program only actually *uses* a few megabytes.

> **The Crux:** how do we support a large (sparse) address space without wasting physical memory on space the process isn't actually using?

⭐ **Segmentation: generalize base-and-bounds to one base/bounds pair *per logical segment*** (code, heap, stack), rather than one pair for the whole address space. Each segment can be placed **independently** in physical memory.

```
Segment   Base    Size
Code      32K     2K
Heap      34K     2K
Stack     28K     2K
```
Now only the memory a process **actually uses** occupies physical space — this supports **sparse address spaces** efficiently.

### Which Segment Does an Address Belong To?
**Explicit approach (VAX/VMS):** use the **top few bits** of the virtual address to select a segment. With 3 segments, 2 bits suffice:
```
13 12 | 11 ... 0
Segment | Offset
```
```c
Segment = (VirtualAddress & SEG_MASK) >> SEG_SHIFT;
Offset  = VirtualAddress & OFFSET_MASK;
if (Offset >= Bounds[Segment])
    RaiseException(PROTECTION_FAULT);
else
    PhysAddr = Base[Segment] + Offset;
    Register = AccessMemory(PhysAddr);
```
⭐ **Worked example:** virtual address 4200 (`0100 0000 0110 1000` in binary, using a 14-bit address) → top 2 bits `01` select the **heap** segment; the remaining 12 bits give an offset of 104 (`4200 − 4096`); physical address = `34K + 104 = 34920`.
- Note: since 2 bits can address 4 segments but only 3 are used, some systems fold code and heap into a single segment to save a bit.
- **Implicit approach:** infer the segment from *how* the address was generated — an instruction fetch (via the PC) → code segment; an address based on the stack/frame pointer → stack segment; anything else → heap.

### The Stack Problem — Negative Growth
The stack grows **backward** (from high to low addresses). The hardware needs one extra bit per segment: a **"grows positive?"** flag.
```
Segment   Base   Size   Grows Positive?
Code      32K    2K     1
Heap      34K    2K     1
Stack     28K    2K     0
```
⭐ **Worked example:** translate virtual address 15KB (stack), which should map to physical 27KB. Top 2 bits select the stack segment, leaving an offset of 3KB — but since the segment grows *negatively*, subtract the max segment size (4KB) to get the true offset: `3KB − 4KB = −1KB`. Add that to the base: `28KB + (−1KB) = 27KB`. ✓ Bounds checking uses the **absolute value** of the negative offset against the segment size.

### Support for Sharing
Adding **protection bits** per segment (read/write/execute) enables **code sharing**: mark a code segment read-execute-only, and the *same physical segment* can be safely mapped into **multiple** processes' address spaces simultaneously — each process still believes it has its own private memory, but the OS is secretly sharing the read-only code pages underneath, saving real physical memory.
```
Segment   Base   Size   Grows Positive?   Protection
Code      32K    2K     1                 Read-Execute
Heap      34K    2K     1                 Read-Write
Stack     28K    2K     0                 Read-Write
```

### Fine-Grained vs. Coarse-Grained Segmentation
- **Coarse-grained** — just a few big segments (code/heap/stack), as above.
- **Fine-grained** — many smaller segments (e.g., one per logical data structure or module), requiring a **segment table** in memory rather than a small fixed hardware register set — more flexible, but needs more sophisticated hardware/software support to manage.

### OS Support and the New Problem: External Fragmentation
- On process creation, the OS must find free physical space for **each segment independently** (harder than finding one contiguous block for a whole address space).
- On process exit, reclaim each segment.
- ⚠️ **External fragmentation:** as segments of *varying sizes* are allocated and freed over time, physical memory develops many small, scattered free "holes" — even if the *total* free space is enough for a new request, it may not be **contiguous**, so the allocation fails.

```
Not Compacted                       Compacted
0KB  ┌──────────┐                  0KB ┌──────────┐
     │    OS    │                      │    OS    │
8KB  ├──────────┤                  8KB ├──────────┤
     │ (free)   │                      │ Allocated│
16KB ├──────────┤                 24KB ├──────────┤
     │ Allocated│                      │ (free)   │
24KB ├──────────┤                 64KB └──────────┘
     │ (free)   │
32KB ├──────────┤
     │ Allocated│
40KB ├──────────┤
     │ (free)   │
48KB ├──────────┤
     │ Allocated│
56KB └──────────┘
```
**Two families of solutions:**
1. **Compaction** — stop running processes, physically copy their segments together into one contiguous region, update their segment registers accordingly, freeing up one large contiguous extent. ⚠️ **Expensive** — memory-intensive and burns real CPU time.
2. **Smarter free-list management** — algorithms like **best-fit** (pick the smallest free chunk that still satisfies the request), **worst-fit**, **first-fit**, and more elaborate schemes like the **buddy algorithm**, aiming to keep large contiguous extents available. (Explored in depth in the next notes file, on free-space management.)

⭐ **Note the recurring tension:** segmentation solves the *sparse address space* problem cleanly, but at the cost of *reintroducing* a variable-sized-allocation problem (external fragmentation) that a fixed-size scheme wouldn't have. This tension is exactly what motivates **paging** — the next major mechanism, covered in the following notes file, which uses **fixed-size** units instead of variable-sized segments.

---

## Summary Table

| Mechanism | Unit of relocation | Solves | Introduces |
|---|---|---|---|
| **Base & bounds (dynamic relocation)** | Whole address space | Simple protection + relocation | Wastes physical memory on the unused gap between heap and stack |
| **Segmentation** | Per-segment (code/heap/stack, or finer) | Sparse address spaces — only used memory is actually allocated | External fragmentation (variable-sized holes) |

## Interview/Exam-Ready Talking Points

- Explain **why code, heap, and stack are placed at opposite ends of the address space** and derive the translation formula for base-and-bounds from first principles.
- Walk through the **full base-and-bounds worked example** (virtual → physical, plus the out-of-bounds fault case) and be ready to do the same for segmentation, including the **negative-growth stack case.**
- Precisely explain the **two levels of memory management** (OS-level reclaim-at-exit vs. process-level `malloc`/`free`) and why this makes leaks harmless in short programs but fatal in long-running ones.
- List the **6 classic `malloc`/`free` bugs** from memory — this is one of the most commonly tested practical C/OS topics.
- Explain **why segmentation causes external fragmentation** and name the two families of mitigation (compaction vs. smarter allocation policy) — and be ready to explain *why* this specific problem motivates moving to paging.