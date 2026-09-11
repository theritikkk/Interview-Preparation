# OSTEP Notes 5 — Beyond Physical Memory: Mechanisms and Policies

> Covers Ch21 (Beyond Physical Memory: Mechanisms) and Ch22 (Beyond Physical Memory: Policies). Ch23 (VAX/VMS case study) is summarized briefly at the end. See `00-index.md` for the full series map.

---

## Part A: Mechanisms — Swapping Pages to Disk

So far, address spaces have been assumed to fully fit in physical memory. Real systems need to support **address spaces larger than physical memory** — this requires a place to stash pages that don't currently fit: **disk.**

### Swap Space
The OS reserves a portion of disk — **swap space** — for moving pages back and forth, read/written in page-sized units. The OS tracks each swapped-out page's **disk address.** Swap space size effectively **caps the maximum total memory in use** across all processes at once (assumed unbounded for simplicity in this chapter).

```
Physical Memory:  [Proc0:VPN0] [Proc1:VPN2] [Proc1:VPN3] [Proc2:VPN0]
Swap Space:       [Proc0:VPN1] [Proc0:VPN2] [free] [Proc1:VPN0] [Proc1:VPN1] [Proc3:VPN0] [Proc2:VPN1] [Proc3:VPN1]
```
⭐ A fourth process with **all** its pages swapped out simply isn't runnable right now — this is exactly how swap space lets the system pretend memory is larger than it physically is. Note swap isn't the *only* on-disk source for pages — a program's **code pages** are also loaded on-demand straight from its on-disk binary, and can be safely discarded and re-loaded later (no need to write them to swap first, since the original is still on disk unchanged).

### The Present Bit and Page Faults
Recall the PTE's **present bit** (previous notes file). If set, the page is in physical memory and translation proceeds normally. If **not** set, the page lives on disk — accessing it triggers a **page fault**, transferring control to the OS's **page-fault handler.**

> ⭐ **Aside — the "fault" naming oddity:** despite being a perfectly *legal* access (to a page that genuinely belongs to the process's address space, just not currently resident), it's called a "fault" because the hardware handles it identically to any other exceptional condition — raise an exception, let the OS sort it out. Really, "page miss" would be a more accurate name.

### Page Fault Control Flow (Hardware)
```c
VPN = (VirtualAddress & VPN_MASK) >> SHIFT;
(Success, TlbEntry) = TLB_Lookup(VPN);
if (Success == True) {                              // TLB Hit
    ... // as before
} else {                                              // TLB Miss
    PTEAddr = PTBR + (VPN * sizeof(PTE));
    PTE = AccessMemory(PTEAddr);
    if (PTE.Valid == False) RaiseException(SEGMENTATION_FAULT);
    else if (!CanAccess(PTE.ProtectBits)) RaiseException(PROTECTION_FAULT);
    else if (PTE.Present == True) {
        TLB_Insert(VPN, PTE.PFN, PTE.ProtectBits);
        RetryInstruction();
    } else if (PTE.Present == False)
        RaiseException(PAGE_FAULT);                  // <-- OS takes over
}
```
⭐ **Three distinct outcomes on a TLB miss**, worth keeping straight:
1. **Valid + Present** → normal case: fetch the PFN, insert into TLB, retry (→ TLB hit).
2. **Valid + NOT Present** → legitimate page, but currently on disk → **page fault handler runs.**
3. **NOT Valid** → illegal access (e.g., a program bug) → trap, likely **process termination.**

### Page Fault Control Flow (Software / OS Page-Fault Handler)
```c
PFN = FindFreePhysicalPage();
if (PFN == -1)                 // no free page found
    PFN = EvictPage();         // run replacement algorithm (Part B)
DiskRead(PTE.DiskAddr, PFN);   // sleep (waiting for slow I/O)
PTE.present = True;            // update page table: now present...
PTE.PFN     = PFN;             // ...and here's the translation
RetryInstruction();
```
⭐ **Crucially, disk I/O is slow** (milliseconds vs. nanoseconds) — so while a page fault is being serviced, the OS **context-switches** to run some other ready process, exactly the same way it overlaps CPU work with I/O for any other blocking operation (recall the scheduling notes file's "incorporating I/O" discussion). This is yet another instance of **overlap enabling higher utilization.**

### What If Memory Is Full?
If no free physical frame exists, the OS must first **evict** an existing page to make room — deciding **which** page to evict is the **page-replacement policy**, the subject of Part B. ⭐ Getting this decision wrong is extremely costly: a bad choice can make a program run at **disk speed instead of memory speed** — a slowdown of **10,000× to 100,000×** with modern hardware gaps between memory and disk latency.

---

## Part B: Policies — Page Replacement

> **The Crux:** how should the OS decide which page(s) to evict when memory is full?

### Cache Management and AMAT
Physical memory, holding only a subset of all pages, is really acting as a **cache** for virtual memory. The goal: **minimize cache misses** (page faults requiring a disk fetch).

**Average Memory Access Time (AMAT):**
```
AMAT = (P_Hit · T_M) + (P_Miss · T_D)
```
where `T_M` = memory access cost, `T_D` = disk access cost, `P_Hit`/`P_Miss` = probabilities (summing to 1).

⭐ **Worked example** — a 4KB address space, 256-byte pages (4-bit VPN, 8-bit offset), accessing the first byte of pages 0-9, with every page except VPN 3 already resident: hit rate = 90% (`P_Hit = 0.9`). With `T_M = 100ns`, `T_D = 10ms`:
```
AMAT = 0.9 × 100ns + 0.1 × 10ms = 90ns + 1ms ≈ 1.00009 ms
```
At a 99.9% hit rate instead: AMAT ≈ **10.1 microseconds** — roughly **100× faster.** ⭐ **The core lesson:** because disk is so astronomically slower than memory, even a **tiny** miss rate dominates overall performance — this is exactly why a *smart* replacement policy matters so much.

### The Optimal Replacement Policy (Belady's Algorithm / MIN)
⭐ **Definition:** evict the page that will be accessed **furthest in the future** (or never again). Provably yields the **fewest possible misses** — but requires knowing the future, making it **impossible to implement in practice.** Its value is purely as a **comparison baseline**: "our policy gets 80% hit rate, optimal gets 82%" is far more meaningful than "our policy gets 80%" in isolation.

**Worked trace** — access stream `0,1,2,0,1,3,0,3,1,2,1`, 3-page cache:
- First 3 accesses: **cold-start (compulsory) misses**, filling the empty cache with `{0,1,2}`.
- Accesses to `0`, `1` → hits.
- Access to `3` → miss, cache full → must evict. Optimal looks ahead: `0` is needed almost immediately, `1` a bit later, `2` **furthest** in the future → **evict 2.**

### FIFO
Evict whichever page has been in memory **longest**, regardless of how recently/often it's been *used.* ⭐ **Simple to implement**, but performs notably worse than optimal (36.4% hit rate vs. optimal in the book's benchmark) — it has **no notion of importance/popularity**, so it can evict a heavily-used page purely because it happened to arrive first.
- ⭐ **Belady's Anomaly:** FIFO can, counter-intuitively, get a **worse** hit rate as the cache **grows larger** — a genuinely surprising result, first observed by its namesake. This happens because FIFO (and Random) don't obey the **stack property**: a bigger cache under a stack-property-respecting policy (like LRU) is *guaranteed* to contain everything a smaller cache would (plus more), so hit rate can never get worse as cache size grows. FIFO offers no such guarantee.

### Random
Evict a randomly-chosen page. Similarly simple, no real intelligence — performance depends entirely on luck, generally falling between FIFO and optimal (in one study, matching optimal about 40% of the time, sometimes doing much worse).

### LRU (Least-Recently-Used)
⭐ **Principle: history predicts the future.** If a page hasn't been used in a long time, it's a good candidate for eviction (leaning on the observed **locality** that programs generally re-reference recently-used data — the same principle underlying TLBs and hardware caches generally).

**Worked trace** — same stream, LRU maintains a recency-ordered list, evicting the **least-recently-used** entry on a miss:
```
0 Miss  -> LRU: [0]
1 Miss  -> LRU: [0,1]
2 Miss  -> LRU: [0,1,2]
0 Hit   -> LRU: [1,2,0]
1 Hit   -> LRU: [2,0,1]
3 Miss, evict 2 -> LRU: [0,1,3]
...
```
⭐ LRU has the **stack property** — larger caches are guaranteed to never do worse than smaller ones under LRU. **Related policy — LFU (Least-Frequently-Used):** evict the page used *least often*, rather than least recently — a different, sometimes complementary, notion of "importance."

### Approximating LRU (the Clock Algorithm)
⭐ **The problem:** implementing *perfect* LRU requires tracking exact access-time ordering for every page — on a machine with, say, 1 million 4KB pages (4GB RAM), scanning to find the true least-recently-used page is **prohibitively expensive.**

**Hardware support: the use bit (reference bit)** — one bit per page; hardware sets it to **1** whenever the page is referenced (read or written); the hardware **never clears it** — that's the OS's job.

⭐ **The Clock Algorithm (Corbato):** arrange all pages in a circular list with a **clock hand** pointing at one. On eviction:
1. Check the current page's use bit. If **1** ("recently used" — not a good eviction candidate): **clear it to 0**, advance the hand, repeat.
2. If **0**: **evict this page** (the hand doesn't need to have made a full circle first — it stops at the first zero it finds).

This gives an **approximation** of LRU — worse than perfect LRU, but far better than FIFO/Random, and crucially **avoids repeatedly scanning all of memory** the way true LRU tracking would require.

### Considering Dirty Pages
⭐ **Refinement:** incorporate the **dirty (modified) bit** into the eviction decision. Evicting a **clean** page is "free" (the physical frame can just be reused — the original data is still safely on disk, unmodified). Evicting a **dirty** page requires an expensive **write-back to disk first.** The clock algorithm can be extended to **prefer evicting clean-and-unused pages first**, falling back to dirty-and-unused pages only if no clean candidates exist.

### Other VM Policies
- **Page selection policy** — *when* to bring a page into memory:
  - **Demand paging** — the default: bring a page in only when it's actually accessed ("on demand").
  - **Prefetching** — speculatively bring in a page *before* it's needed, when there's good reason to expect it'll be used soon (e.g., code page `P+1` right after `P` was demand-paged in).
- **Write clustering/grouping** — batch several pending page writes together into one larger, more efficient disk write, rather than issuing many small individual writes — exploits the fact that disks perform one big sequential write far more efficiently than many scattered small ones (a theme returned to in depth in the Persistence notes files).

### Thrashing
⭐ **Definition:** when the combined memory demands of all running processes **exceed** available physical memory, the system may end up **constantly paging** — spending nearly all its time servicing page faults rather than doing useful work.

Two broad responses:
| Approach | Mechanism | Trade-off |
|---|---|---|
| **Admission control** | Decide *not* to run a subset of processes, so the remaining ones' **working sets** (actively-used pages) fit comfortably in memory | "Better to do less work well than everything poorly" — a recurring systems-design wisdom |
| **Out-of-memory (OOM) killer** | (Some Linux versions) forcibly kill a memory-intensive process when oversubscribed | Effective at relieving pressure, but blunt — can kill something critical (e.g., the X display server), breaking dependent applications |

⭐ **The book's own pragmatic closing take:** modern replacement algorithms (e.g., **ARC**, adding **scan resistance** to avoid LRU's own worst-case pathologies on looping-sequential access patterns) continue to improve — but as the gap between memory and disk speed has only widened over time, the cost of *any* significant amount of paging has become so severe that the most effective real-world fix is often simply: **buy more memory.**

---

## Part C: The VAX/VMS Virtual Memory System (Ch23 — brief overview)

A real-world **case study** chapter illustrating how these general mechanisms/policies came together in a real, influential system (DEC's VAX architecture running VMS, late 1970s/early 80s):
- **Hybrid segmentation + paging**, with a small number of hardware-recognized page sizes.
- Used a **clock-like FIFO-with-second-chance** replacement approach, with specific real-world engineering tweaks (like clustering pages into larger "clusters" for more efficient disk transfer, and a **secondary cache of recently evicted pages** to cheaply recover from a slightly-too-eager eviction) to work around some of the weaknesses of pure FIFO discussed above.
- ⭐ **Overall lesson from this case study:** real systems rarely implement the "clean" textbook algorithm exactly — they layer **pragmatic engineering compromises** (working around known weaknesses like Belady's Anomaly, minimizing disk I/O granularity, reusing recently-evicted pages cheaply) on top of the conceptual foundations covered in Parts A and B.

---

## Summary Table

| Concept | Purpose |
|---|---|
| **Swap space** | Disk-backed storage for pages that don't currently fit in physical memory |
| **Present bit / page fault** | Detects when a valid page is on disk rather than in memory, triggering the OS page-fault handler |
| **AMAT** | Quantifies why even a small miss rate can dominate overall performance, given the memory/disk speed gap |
| **Optimal (Belady/MIN)** | Theoretical best-case baseline (evict what's needed furthest in the future) — not implementable, but useful for comparison |
| **FIFO / Random** | Simple, cheap, but no notion of page importance; FIFO can suffer Belady's Anomaly |
| **LRU** | Exploits locality — approximates "the future looks like the recent past"; obeys the stack property |
| **Clock algorithm** | Hardware-feasible LRU approximation using a single use/reference bit per page |
| **Thrashing** | System-wide paging overload; addressed via admission control or (bluntly) an OOM killer |

## Interview/Exam-Ready Talking Points

- Walk through the **full page-fault control flow**, hardware side and OS side, and precisely distinguish the three TLB-miss outcomes (valid+present, valid+not-present, invalid).
- Compute an **AMAT example** from given hit rate and memory/disk latencies, and explain why even a 1% miss rate can dominate runtime.
- Explain **why the Optimal policy is only useful as a baseline**, and describe how it decides what to evict.
- Precisely describe **Belady's Anomaly** and why it demonstrates FIFO's lack of the "stack property" that LRU has.
- Explain **why perfect LRU is impractical at scale**, and walk through the **clock algorithm** as its hardware-feasible approximation, including the dirty-bit refinement.
- Define **thrashing** and describe the two broad classes of system response (admission control vs. OOM killing), including their respective trade-offs.