# CPU Memory Notes 3 — Virtual Memory and NUMA

> Detailed notes on Ulrich Drepper's *What Every Programmer Should Know About Memory*, Ch4 (Virtual Memory) and Ch5 (NUMA Support). See `00-index.md` for the full series map.

---

## Part A: Virtual Memory

⭐ **Framing:** this paper doesn't re-argue *why* virtual memory is good (covered thoroughly in the OSTEP-based notes already in this workspace) -- it goes straight to **implementation details and their costs**, since those costs are exactly what a performance-conscious programmer needs to understand.

### Simplest Address Translation -- One-Level Page Table
```
Virtual Address:  | Directory | Offset |
                        |
                        v
                 Page Directory ---> Directory Entry ---> Physical Page
```
The MMU splits the virtual address, uses the top part to index into a **Page Directory**, and each directory entry (settable individually by the OS) points to a physical page -- combined with the low (offset) bits of the virtual address to form the full physical address.

⭐ **Worked example -- x86, 4MB pages:** the Offset is 22 bits (enough to address every byte in a 4MB page); the remaining 10 bits select one of 1024 page-directory entries, each holding a 10-bit page base address.

### Multi-Level Page Tables -- Why One Level Isn't Enough
⭐ **The problem with small (4kB) pages and one flat table:** the Offset shrinks to 12 bits, leaving **20 bits** to select a page-directory entry -- a table with **2^20 entries**, even at just 4 bytes each, would be **4MB**, and every process potentially needing its own such table would tie up enormous amounts of physical memory just for translation bookkeeping.

⭐ **The fix, identical in spirit to the OSTEP notes' multi-level page tables: chain several directory levels together**, forming a sparse tree -- unused address-space regions simply don't need any directory allocated for them.

```
Virtual Address: | L4 Index | L3 Index | L2 Index | L1 Index | Offset |
                      |           |          |          |
                      v           v          v          v
                  L4 Dir  ---> L3 Dir ---> L2 Dir ---> L1 Dir ---> Physical Page
```
- **Up to 4 levels** in real-world designs at the time of writing. The **level 4 directory's address** is held in a special CPU register; each level's entry points to the next lower directory, until the **level 1** entry finally yields the high bits of the physical address.
- This process -- walking down through each directory level -- is called a **page tree walk.** Some CPUs (x86, x86-64) perform it entirely in **hardware**; others need OS assistance.
- ⭐ **A subtle, practical scheduling consequence:** modern OSes deliberately **randomize** the load addresses of code/heap/stack/DSOs (ASLR, for security) -- which means a process's memory footprint is typically **spread across the virtual address space**, needing **more** distinct level-2/3 directories than a compact, non-randomized layout would. ⭐ **Performance vs. security trade-off, stated directly by the paper:** if performance genuinely matters more than ASLR's security benefit for a given deployment, randomization can be disabled -- the OS will then load DSOs contiguously, reducing the page-table tree's footprint.

### Optimizing Page Table Access -- Why We Need a TLB

⭐ **Quantifying why simply caching directory entries in L1d isn't enough:** a full page-tree walk on a 4-level system needs **up to 4 sequential, dependent** memory accesses -- each waiting on the result of the previous one, so they **cannot be parallelized.** Even at pure L1d-hit speed, that's a minimum of roughly **12 cycles**, before accounting for any L1d miss probability -- a cost the instruction pipeline cannot hide, and one that also consumes precious L1d bandwidth on every single memory reference.

⭐ **The actual fix: cache the *entire computed translation result***, not just the raw directory entries -- this dedicated, small, fast cache is the **Translation Look-Aside Buffer (TLB).**
- Since the page **offset** never participates in the physical-page computation, only the rest of the virtual address serves as the TLB's tag -- meaning **every byte within one page shares the same TLB entry**, exactly the same locality principle that makes ordinary data/instruction caching effective.
- Modern CPUs implement **multi-level TLBs** too (a small, fast L1TLB and a larger, slower L2TLB), mirroring the general cache hierarchy -- and small L1TLBs are often made **fully associative** (feasible precisely because TLBs are so much smaller than data/instruction caches) with LRU eviction; as TLBs have grown, some have shifted to set-associative designs instead.
- **A TLB hit** is what makes ordinary memory access fast; a **TLB miss** forces the full, expensive multi-level page-tree walk described above.

### Caveats of Using a TLB -- Context Switches
⭐ **The core problem:** TLB entries encode a translation valid *only* for the address space that was active when they were cached -- a naive TLB would give **wrong answers** after a context switch to a different process, or after the page table tree itself changes.

**Two solutions:**
| Approach | Mechanism | Cost |
|---|---|---|
| **Flush on every context switch** | Invalidate the whole TLB whenever the page-table-tree register changes | Correctness guaranteed, but every switch (including entering/leaving the kernel or a hypervisor) pays a burst of TLB misses afterward, even for addresses that would otherwise still be valid |
| **Extended (tagged) TLB entries -- ASID** | Tag each TLB entry with an **Address Space ID**, distinguishing which address space it belongs to | Entries from multiple address spaces can coexist; no flush needed at all when switching **between threads of the same process**, and the calling process's entries survive a short excursion into kernel/VMM code |

⭐ **Real hardware example the paper cites:** AMD introduced a **1-bit ASID tag** with its Pacifica virtualization extensions specifically to distinguish the VMM's address space from a guest domain's -- avoiding unnecessary TLB flushes on every VM-exit/VM-entry round trip, which would otherwise be a significant, entirely avoidable performance tax on virtualized workloads.

### Influencing TLB Performance -- What the Programmer Can Actually Do
⭐ **Lever 1 -- larger page sizes.** A bigger page covers more instructions/data per TLB entry, directly reducing the number of distinct translations (and hence TLB entries) a given working set needs. x86/x86-64 support 4kB (normal), and 2MB/4MB ("huge") pages; other architectures (IA-64, PowerPC) support other sizes like 64kB.

⭐ **The practical cost of huge pages: physical contiguity is hard to guarantee.** Allocating a 2MB huge page with a 4kB unit size requires finding **512 physically contiguous** free small pages -- increasingly difficult (or outright impossible) as a long-running system's physical memory becomes fragmented. **Linux's practical answer:** the `hugetlbfs` special filesystem, which **reserves** a fixed pool of huge pages **at boot time** -- a real resource commitment (memory that's tied down whether or not it's actively used, and a pool that typically requires a reboot to resize). ⭐ **Recommended use case, stated directly:** situations where performance is paramount, resources are plentiful, and a somewhat cumbersome setup is acceptable -- the paper's own example is **database servers.**

⭐ **A second, easily-overlooked benefit of huge pages:** since the offset field grows, **fewer page-directory levels** are needed to cover the same address range -- meaning a TLB miss, when it does happen, requires **less work** (a shorter tree walk) to resolve, independent of the primary "fewer TLB entries needed" benefit.

⭐ **Lever 2 (without changing page size at all): improve data locality.** Since a small number of TLB entries covers a page, deliberately grouping data that's used together onto **fewer distinct pages** reduces the number of TLB entries the working set actually needs -- this is conceptually the exact same "spatial-locality-for-performance" idea already covered for ordinary data/instruction caches, just operating at the coarser page-size granularity, and correspondingly requiring alignment discipline at a much larger scale.

> **Practical note the paper includes:** an ELF binary's required alignment is encoded directly in its program header (viewable via `eu-readelf -l`), and increasing the effective page size beyond what a binary was built to expect can **break loading entirely** -- a real, concrete constraint on how freely huge pages can be adopted for existing binaries.

### Impact of Virtualization
⭐ Under a hypervisor, guest OSes do **not** get unrestricted access to raw physical memory -- the VMM constructs and owns a **separate page table tree per guest domain**, handing out memory through that indirection rather than letting guests address physical pages directly. This is precisely what provides isolation against a faulty or malicious guest touching memory it shouldn't -- but it also means an extra layer of translation (and hence extra potential TLB-miss cost) sits between a guest's virtual addresses and the true underlying physical memory, on top of everything already described above.

---

## Part B: NUMA Support

### NUMA Hardware, Concretely
⭐ **Recall from notes file 1:** integrated, per-CPU memory controllers avoid a single congested Northbridge, at the cost of non-uniform access times. This section goes deeper into the actual interconnect topologies used to realize this in practice.

- **AMD's approach (HyperTransport):** an interconnect fabric letting a processor reach memory attached to a *different* CPU that it isn't directly wired to. The achievable topology size is constrained by the number of interconnect links per node -- going beyond that requires increasing the **diameter** (max hops between any two nodes), directly increasing worst-case latency.
- ⭐ **The Hypercube topology** is highlighted as an efficient way to connect nodes: with `C` interconnect interfaces per node, up to `2^C` nodes can be connected while keeping the diameter minimal (the smallest possible diameter for any topology with `2^n` nodes and `n` links per node). The paper notes AMD's next generation (4 links/CPU) would enable a **diameter-1**, 4-socket all-to-all topology -- but scaling to 8 sockets would reintroduce a diameter of 3, "with a vengeance."
- **Bigger, specialized NUMA systems** (IBM x445, SGI Altix) use dedicated, expensive interconnect fabrics (SGI's **NUMAlink**) to keep the NUMA factor low even at very large scale (SGI Altix: thousands of CPUs) -- though the paper notes even these can hit a **dynamic, workload-dependent** NUMA factor once contention on the interconnect itself becomes the bottleneck, not just hop count.
- ⭐ **Important distinction: clusters are not NUMA.** A collection of commodity machines connected via ordinary high-speed networking, without a **shared address space**, doesn't qualify as NUMA at all -- NUMA specifically requires a single, coherent, shared physical address space spanning the nodes.

### OS Support for NUMA
⭐ **The core placement principle:** when a process runs on a given CPU, its physical memory should ideally be allocated **locally** to that CPU -- otherwise every instruction/data fetch pays the remote-access NUMA penalty.

- **Shared library/DSO handling is a special, harder case:** a library like `libc` used by processes on *every* CPU ideally should be **mirrored** into each node's local physical memory -- but this is a genuine optimization, not a baseline requirement, and it's "generally hard to implement," so support may be partial or absent depending on the OS/version.
- ⭐ **The OS deliberately avoids migrating processes/threads across NUMA nodes** -- beyond the ordinary cache-locality cost of migration (already true on any multi-processor system, NUMA or not), a NUMA migration additionally **strands** the process's already-allocated local memory on the *old* node, turning every subsequent access into an expensive remote one. ⭐ **The paper calls this out explicitly as a deliberate "pessimization"**: the OS accepts somewhat worse load-balancing flexibility as the price for predictable, non-catastrophic NUMA behavior -- and exposes this trade-off to applications, since Linux lets a process select its own memory-allocation policy (covered in the programmer-optimizations notes file later in this series).

### Published NUMA Information (Linux specifics)
- **`/sys/devices/system/cpu/cpu*/cache`** -- cache topology info the kernel publishes via `sysfs`.
- ⭐ **`/proc/PID/numa_maps`** -- shows, per memory mapping, how many pages are allocated on each NUMA node (`N0=...`, `N1=...`, etc.) for a running process -- directly answering "is this process's memory actually local to where it's running?" The paper's own worked example shows a process's own dirtied/heap pages correctly landing on its execution node (`N3`), while shared **read-only** mappings (`ld.so`, `libc`, a shared locale archive) are scattered across *other* nodes -- exactly the "shared library mirroring is hard" problem described above, made visible in real diagnostic output.

### Remote Access Costs -- Real Measured Numbers
⭐ **AMD's own published data (cited directly), for a four-socket machine:**
| Access type | Slowdown vs. 0-hop |
|---|---|
| 2-hop read | **+30%** |
| 2-hop write | **+49%** |
| 1-hop write vs. 0-hop write | (a smaller but real gap) |
| 2-hop write vs. 1-hop write | **+17%** |

⭐ **What this means in practice, stated directly:** for any workload whose working set exceeds cache (forcing genuine memory accesses, not just cache hits), being scheduled on a NUMA node **1 or 2 hops away** from its data means paying an **across-the-board 9-30% latency tax** (the paper gives 9%/30% for 1-hop/2-hop reads specifically) on **every single cache-line miss** for the remainder of that memory-bound computation. ⭐ **The programmer/administrator-level takeaway:** on a NUMA machine, *where* a process is scheduled relative to *where* its memory was allocated is not a minor implementation detail -- it's a first-order performance factor, on the same order of magnitude as many of the cache-optimization techniques covered elsewhere in this paper, and it's directly why Linux exposes explicit NUMA policy controls to applications (detailed further in the programmer-optimizations notes file).

---

## Summary Table

| Concept | Key fact |
|---|---|
| **Multi-level page tables** | Chain several directory levels into a sparse tree; avoids allocating tables for unused address-space regions, at the cost of multiple sequential (dependent) lookups per translation |
| **TLB** | Caches the *fully computed* physical-page translation, not just raw directory entries -- a plain L1d cache of directory entries would still be far too slow given the sequential dependency chain |
| **TLB + context switches** | Flush-on-switch (simple, costs a burst of misses) vs. ASID tagging (entries from multiple address spaces coexist, no unnecessary flushing) |
| **Huge pages** | Fewer TLB entries needed + shorter tree walks on a miss, at the cost of needing genuinely contiguous physical memory (Linux: reserved via `hugetlbfs` at boot) |
| **NUMA hardware** | Integrated per-CPU memory controllers + interconnect fabric (HyperTransport, NUMAlink); hypercube topology minimizes diameter for a given link count |
| **NUMA factor** | The extra latency cost of remote vs. local memory access, growing with hop count -- AMD's own numbers: +30%/+49% for 2-hop read/write |
| **OS NUMA policy** | Avoids cross-node migration (would strand already-allocated local memory); exposes explicit allocation-policy controls to applications |

## Key Talking Points

- Walk through a **4-level page table translation** from a virtual address, explaining why the sequential dependency between levels rules out simple parallelization of the walk.
- Explain **precisely why a TLB caches the computed physical-page translation rather than just directory entries** -- tie this back to the 12-cycle-minimum calculation for a naive L1d-cached-directory-entries approach.
- Compare **TLB-flush-on-switch vs. ASID tagging**, and explain the two specific scenarios (kernel/VMM excursions, same-process thread switches) where ASID tagging pays off.
- Explain the **huge-pages trade-off**: fewer TLB entries and shorter tree walks, versus the real difficulty of finding physically contiguous memory, and why Linux handles this via upfront reservation (`hugetlbfs`) rather than on-demand allocation.
- Explain **why NUMA migration is worse than ordinary cache-losing migration** -- the stranded-local-memory effect -- and why the OS treats NUMA migration as a deliberate, accepted pessimization.
- Quote the **concrete AMD NUMA remote-access numbers** (30%/49% for 2-hop read/write) as a memorable, specific illustration of why NUMA-awareness matters for real workloads, not just as an abstract concern.