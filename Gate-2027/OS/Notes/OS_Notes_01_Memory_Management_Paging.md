# OS Notes 01 — Memory Management & Paging (High-Yield Deep Dive)

> **Why this file exists:** "Memory Management" is on the high-yield list (~60-mark set of topics). The video's claim: *it's easy — if binary conversions are crystal clear*. This file builds the entire chain from first principles: fragmentation → paging → address translation → multi-level page tables → TLB → demand paging. Every formula is derived, not memorized. Ends with a 10-question practice set with full solutions.
>
> **Companion question banks:** `OS_05_Memory_Management_Paging.md`, `OS_06_Page_Tables.md`, `OS_07_TLB_and_EMAT.md`, `OS_08_Page_Replacement.md` (page replacement has its own notes file: `OS_Notes_02`).

---

## 0. The one mental model that carries this whole topic

The OS has one job here: **give every process the illusion of its own private, contiguous memory**, starting at address 0, arbitrarily large — while the hardware actually has limited, shared, physical RAM.

Everything else — paging, page tables, TLB, page faults — is machinery for maintaining that illusion cheaply.

```
  Process sees:                 Hardware uses:
  ┌──────────────┐              ┌──────────────┐
  │ logical addr │──MMU────────▶│ physical addr│
  │ 0 ........ N │  translation │ scattered    │
  └──────────────┘              │ frames in RAM│
                                └──────────────┘
```

The MMU (Memory Management Unit) translates on **every** memory access. The whole game of GATE questions is: *how fast can we do that translation, and how much memory does the translation machinery itself eat?*

---

## 1. Contiguous allocation & fragmentation

The naive approach: give each process one contiguous chunk of RAM.

| Scheme | How it works | Problem |
|---|---|---|
| Fixed partitions | RAM cut into fixed-size partitions; process fits into one | **Internal fragmentation** — wasted space *inside* the partition (the partition is bigger than the process) |
| Variable partitions | Each process gets exactly what it asks for | **External fragmentation** — enough total free space exists, but it's scattered in holes too small individually |

**Definitions to nail:**

- **Internal fragmentation** = allocated − requested, wasted *inside* your allocation. Only happens when allocation is in fixed-size units (partitions, **pages**, blocks).
- **External fragmentation** = free memory exists but is unusable because it's not contiguous. Happens with variable-size allocations (**segments**, files).

**Fit strategies (non-contiguous-free only):**

| Strategy | Rule | Classic exam result |
|---|---|---|
| First fit | First hole big enough | Fastest scan; generally good |
| Best fit | Smallest hole that fits | Leaves tiny unusable slivers → worst external fragmentation |
| Worst fit | Largest hole | Leaves a large remainder (often usable) but usually poor overall |

**Compaction** = shuffle processes to merge holes. Requires **relocatable** processes (addresses fixed at load time cannot move). Think of it as defragmenting a disk.

> GATE trap: "best fit minimizes wasted space per allocation" ≠ "best fit minimizes total wasted memory." Best fit creates the most unusable slivers.

---

## 2. Paging — the big idea

Cut **logical memory** into fixed-size **pages**, **physical memory** into same-size **frames**. Any page can go in any frame.

- Kills **external** fragmentation (any free frame works).
- Leaves **internal** fragmentation: only in the *last* page of each region (avg half a page wasted per region).
- Introduces the **page table**: per-process array mapping page number → frame number.

```
  Page table of process P (one entry per virtual page):

  page# | frame#
  ------+-------
     0  |   5
     1  |   2        virtual page 0 → frame 5
     2  |   9        virtual page 1 → frame 2
     3  |  --        (not in memory / invalid)
```

**Page size is always a power of 2.** This is not a style choice — it makes translation free in hardware: the offset needs no arithmetic, just bit slicing.

### 2.1 The address split (the "binary conversions" the video means)

For a logical address of width `m` bits and page size `P = 2^d` bytes:

```
   m-bit logical address
  ┌─────────────────┬──────────────┐
  │  page number p  │  offset d    │
  └─────────────────┴──────────────┘
     m − d bits        d = log2(P)

  p  = address >> d          (integer division by P)
  off= address & (P − 1)     (remainder mod P)
```

**Bootcamp table — know these cold:**

| P | offset bits | #bytes |
|---|---|---|
| 512 B | 9 | 2⁹ |
| 1 KB | 10 | 2¹⁰ |
| 4 KB | 12 | 2¹² |
| 8 KB | 13 | 2¹³ |
| 16 KB | 14 | 2¹⁴ |
| 1 MB | 20 | 2²⁰ |

**Hex shortcut:** 1 hex digit = 4 bits.
- 4 KB pages → offset = last **3** hex digits (12 bits).
- 1 KB pages → last 2.5 digits — careful! 10 bits ≈ 2.5 hex digits. For non-multiples of 4, convert to binary.

**Worked example (do this one by hand now):** page size 1 KB, logical address `0x3A7E`, `PT[14] = 0x2C`. Physical address?

1. 1 KB = 2¹⁰ → offset = low 10 bits of `0x3A7E`.
2. `0x3A7E` = `0011 1010 0111 1110` → offset = `10 0111 1110` = `0x27E`; page number = remaining bits `001110` = 14.
3. Frame 0x2C shifted: `0x2C × 0x400 = 0xB000`. Add offset: `0xB000 + 0x27E = 0xB27E`. ✔

> The whole video claim in one sentence: if you can do step 2 fast in your head (binary/hex slicing), memory management questions become nearly free marks.

---

## 3. Page table size — the formula family

The single most repeated GATE calculation:

```
  #virtual pages      = 2^(bits of logical address − offset bits)
  page table size     = #entries × PTE size
                      = #virtual pages × PTE size     (single level!)
```

**Worked example A (the canonical one):** 32-bit logical address, 4 KB page, 4-byte PTE.

- Offset = 12 bits → 2²⁰ virtual pages.
- Page table = 2²⁰ × 4 B = **4 MB** — *per process*, just for translation!

**PTE contents (what's in each entry):**
- Frame number (`log2(#frames)` bits, where `#frames = physical memory / page size`)
- Valid/invalid bit, dirty bit, reference bit, protection bits

> GATE trap: PTE size is often given as "frame number + k control bits, rounded UP to a whole number of bytes". Round up, don't truncate. Frame bits = log₂(#physical frames), not log₂(physical bytes).

### 3.1 Multi-level page tables — *why* they exist

Think about *where* the 4 MB single-level table came from: dense coverage of 2²⁰ pages, even though a real process uses a few MB around a few regions (text, heap, stack). Most entries are **invalid** and wasted.

Fix: **page the page table itself.** Split the page number into two indexes:

```
  32-bit address, 4 KB pages, 4 B PTE, two levels (10 | 10 | 12):

  ┌────────┬────────┬────────────┐
  │ L1 idx │ L2 idx │  offset    │
  │ 10 bits│ 10 bits│  12 bits   │
  └────┬───┴────┬───┴────────────┘
       │        │
   L1 table (4 KB)
  [entry for each 4 MB region]
       │ points to
       ▼
  L2 table (4 KB)  ← only exists if that 4 MB region is used
  [1024 entries, each covering 4 KB]
```

- L1 has 2¹⁰ entries × 4 B = 4 KB (fits in one page — the usual design constraint).
- One L2 table covers 1024 × 4 KB = **4 MB** of virtual space.
- A process using 1 MB needs: L1 (4 KB) + one L2 (4 KB) = **8 KB** vs 4 MB single-level. *This* is why multi-level paging exists: **sparse allocation**.

**Translation cost:** with a TLB miss, an `L`-level table needs `L` extra memory accesses before the data access (see §5).

**Two-level walk formula:** if the split is `| a | b | d |`:
- number of L2 tables needed for a region of size `S` = `S / (2^b × 2^d)`.

**Worked example B:** 32-bit VA, 4 KB pages, 4 B PTEs, split 10|10|12. How many second-level tables to cover a 1 GB region?
Each L2 covers 2¹⁰ × 4 KB = 4 MB. `1 GB = 2³⁰ B`, `2³⁰ / 2²² = 2⁸ = 256`. ✔

### 3.2 Inverted page table — the other design

One **global** table with **one entry per physical frame** (not per virtual page!):

```
  entry = <pid, virtual page number, control bits>   for each frame
```

- Size = `#frames × entry size` — independent of how many processes or how big the address space.
- Translation = *search* the table for `(pid, VPN)` → slow; real systems use a hash table.
- Sharing a page between processes is hard (one entry can name only one pid,vpn).

| | Hierarchical (multi-level) | Inverted |
|---|---|---|
| Entries | one per **virtual** page | one per **physical** frame |
| Per process? | yes | no, system-wide |
| Size scales with | virtual address space | physical memory |
| Translation | walk levels | hash/linear search |

**Worked example C:** physical memory 2 GB = 2³¹ B, page 4 KB → 2¹⁹ frames. Entry = PID (16 bits) + VPN (36 bits) + 4 control = 56 bits = 7 B. Table = 2¹⁹ × 7 B = 3.5 MB. Note: with 20 processes sharing one **virtual** page each, the inverted table doesn't grow at all — but the virtual space does.

---

## 4. Segmentation (and segmentation + paging)

Segmentation = variable-size logical units (code, data, stack, a whole function table…). Address = **(segment #, offset)**.

```
  segment table:  base | limit   (per segment)
  valid  ⇔  offset < limit      otherwise → trap (segmentation fault)
  physical address = base + offset
```

- Matches the programmer's view; easy sharing/protection per segment.
- Suffers **external** fragmentation (variable sizes).
- **Segmentation + paging** (what real x86 did): each segment is itself paged → no external fragmentation, segments still visible logically. Translation: segment table → page table → frame.

> GATE trap: an offset *equal to* the limit is a trap (`0 ≤ offset ≤ limit − 1`). A segment number beyond the table size is also a trap.

---

## 5. TLB — making translation fast

The page table lives in memory; translation on *every* access would double memory traffic. The **TLB (Translation Lookaside Buffer)** is a small, fast, associative cache of recent translations: `VPN → frame number`.

```
  logical address ──▶ ┌─ TLB ─┐
                      │ hit?  │── hit ──▶ frame, then access memory
                      └───┬───┘
                          │ miss
                          ▼
                 page table walk (1 access per level, in memory)
                          │
                          ▼
                    refill TLB, access memory
```

### 5.1 EMAT — the formulas GATE lives on

Let `h` = TLB hit rate, `t` = TLB access time, `m` = memory access time, `L` = page-table levels.

**Case 1 — TLB lookup is serial (lookup first, then decide):**
```
  hit path : t + m                      (TLB + data)
  miss path: t + L·m + m                (TLB + L page-table accesses + data)
  EMAT = h·(t + m) + (1 − h)·(t + L·m + m)
```

**Case 2 — TLB lookup is in parallel with cache access** (rare in GATE, read the wording):
```
  EMAT = h·m + (1 − h)·(t + L·m + m)      (no t added on the hit path)
```

**If a page fault term is added** (fault probability `p`, fault service time `F`):
```
  EAT = (1 − p) · EMAT + p · F        (p is tiny; F is milliseconds vs ns)
```

**Worked example D:** `t=20 ns`, `m=100 ns`, `h=95%`, single-level table (L=1).
- Hit: `20+100 = 120`; Miss: `20+100+100 = 220`.
- `EMAT = 0.95×120 + 0.05×220 = 114 + 11 = 125 ns`. ✔

**Worked example E (multi-level + TLB):** 48-bit VA, 4 KB pages, **4-level** table, `t=1 ns`, `m=100 ns`, `h=98%`.
- Hit: `1+100 = 101`. Miss: `1 + 4×100 + 100 = 501`.
- `EMAT = 0.98×101 + 0.02×501 = 98.98 + 10.02 = 109 ns`. ✔ (TLB saves us from 5 memory touches on 98% of accesses.)

**TLB reach** = `#entries × page size` (× number of ways for a fully associative view of effective reach). Increasing page size increases reach — a common MSQ option.

> Traps:
> - TLB miss ≠ page fault. TLB miss → walk the page table. Page fault → the page isn't in memory at all (valid bit = 0) → trap to OS.
> - On a context switch the TLB must be flushed unless entries carry an **ASID** (address-space ID) tag; stale `VPN→frame` entries from another process are catastrophic.
> - Real GATE phrasing: "show the number of memory accesses" — count page-table accesses + 1 for data; TLB access is not a memory access.

---

## 6. Demand paging, page faults, and EAT

**Demand paging:** load a page only when first touched. `valid = 0` means "not in memory".

**Page-fault flow (know the order):**
1. MMU raises fault (valid bit 0) → trap to OS.
2. OS checks the address is legal (else SIGSEGV).
3. Find a free frame, or **evict a victim** (replacement policy — next notes file).
4. If victim dirty → write it back to disk.
5. Read the faulting page from disk (process blocks).
6. Update page table (frame #, valid = 1), restart the instruction.

Note the faulting instruction **restarts** — the fault happened *before* execution.

**Effective access time with faults:** if the fault rate is `p`:

```
  EAT = (1 − p) × memory_time + p × fault_service_time
```
(on modern numbers the second term usually dominates above `p ≈ 10⁻⁵`!)

**Worked example F:** `m = 100 ns`, `F = 10 ms` (10⁷ ns), `p = 10⁻⁶`:
`EAT = 0.999999×100 + 10⁻⁶×10⁷ = 99.9999 + 10 ≈ 110 ns` — faults doubled your access time already.

**Copy-on-write (CoW):** `fork()` shares pages read-only. On a write, protection fault → OS copies the page. Quick way to remember: *share until someone writes*.

---

## 7. GATE problem patterns (the exact archetypes)

| Pattern | What they give | What they ask | Your move |
|---|---|---|---|
| Address split | VP size, page size | offset bits / VPN bits | subtract, watch word-size |
| Translation | hex VA + mini page table | hex PA | bit-slice + hex arithmetic |
| PTE size | PA width or #frames + control bits | PTE/table size | ceil bits to bytes |
| Multi-level size | VA, page, PTE, split | table sizes / #tables | per-level entries × size |
| Inverted | PM size, entry fields | table size | #frames × entry size |
| EMAT | h, t, m, levels | ns | hit/miss weighted sum |
| EAT + faults | p, m, F | ns | weighted sum |

**Trap checklist:**
- *Word-addressable* memory: convert everything to words first; offset = words per page.
- "Number of bits in the page offset" vs "bits of VPN": read twice.
- PTE "rounded up to a whole byte": 25 bits → 4 bytes.
- Rounding: answers often need "round to nearest integer" — do the arithmetic exactly first.
- Inverted table: exactly one entry per frame; hierarchical: one per page.

---

## 8. Practice set (verified by simulation)

### Q1 · MCQ · Fundamental
Why is the page size always a power of two?

- (A) It is a hardware tradition with no performance benefit.
- (B) It lets the hardware split a logical address into page number and offset by bit slicing instead of division/modulo.
- (C) It minimizes internal fragmentation.
- (D) It makes page tables smaller.

---

### Q2 · NAT · Fundamental
Logical addresses are 32 bits; the page size is 8 KB. The number of bits required to represent the page number is ______.

---

### Q3 · NAT · Fundamental
Physical memory is 2³⁶ bytes; the page size is 4 KB. A page-table entry stores the frame number plus control bits. The minimum number of bits needed for the frame-number field is ______.

---

### Q4 · MCQ · Standard
Page size is 4 KB. Logical address `0x1A2B`; the page table maps page 1 → frame `0x0F`. The physical address is

- (A) `0x0FA2B`
- (B) `0x1FA2B`
- (C) `0x0FA1B`
- (D) `0x1A2BF`

---

### Q5 · NAT · Standard
TLB access time 10 ns, memory access time 50 ns, TLB hit rate 90%, single-level page table, TLB lookup **serial** with the access. The effective memory access time in ns is ______.

---

### Q6 · MSQ · Standard
Which of the following statements about multi-level paging is/are TRUE?

- (A) It saves memory because page tables for unused regions of the address space need not exist.
- (B) On a TLB miss with a 2-level table, at least 3 memory accesses are needed (2 page-table levels + data).
- (C) By design, each level of the page table fits inside a single page.
- (D) It makes TLB misses faster because each table has fewer entries overall.

---

### Q7 · NAT · Moderate
32-bit logical addresses, 4 KB pages, 4-byte PTEs, two-level split 10 | 10 | 12. The number of second-level page tables required to cover a contiguous 1 GB region is ______.

---

### Q8 · MSQ · Moderate
Which of the following statements about TLBs is/are TRUE?

- (A) A TLB miss always implies a page fault.
- (B) Without ASID tagging, the TLB must be flushed on a context switch because entries are keyed only by virtual page number.
- (C) TLB reach is (number of entries) × (page size), so larger pages extend reach.
- (D) A fully associative TLB stores the full VPN as the tag.

---

### Q9 · NAT · Moderate
Memory access time 100 ns; page-fault service time 10 ms; 40% of evicted pages are dirty and cost an extra 5 ms to write back; page-fault rate is 10⁻⁶. The effective access time in ns (rounded to the nearest integer) is ______.

---

### Q10 · NAT · Killer
48-bit logical addresses, 4 KB pages, 4-level page table (9 | 9 | 9 | 9 | 12). TLB access 1 ns, hit rate 98%, memory access 100 ns. On a TLB miss, all 4 levels are read from memory, then the data. No page faults occur. The effective memory access time in ns is ______.

---

## Answer Key

| Q | Answer | Why (condensed) |
|---|---|---|
| 1 | B | Power-of-two sizes make split = shifts/masks |
| 2 | 19 | 32 − log₂(8K) = 32 − 13 |
| 3 | 24 | #frames = 2³⁶/2¹² = 2²⁴ |
| 4 | A | offset 0xA2B (low 12 bits), page 1 → frame 0x0F: `0x0F000 + 0xA2B` |
| 5 | 65 | 0.9(10+50) + 0.1(10+50+50) = 54 + 11 |
| 6 | A, B, C | (D) is backwards — multi-level *slows* TLB misses |
| 7 | 256 | L2 covers 2¹⁰×4 KB = 4 MB; 1 GB / 4 MB |
| 8 | B, C, D | (A) false: miss → walk table, not fault |
| 9 | 112 | 0.999999×100 + 10⁻⁶×(10ms + 0.4×5ms) = 99.9999 + 12 |
| 10 | 109 | 0.98(1+100) + 0.02(1+400+100) = 98.98 + 10.02 |

### Full solutions

**Q1.** B. Offset = `addr & (P−1)`, page = `addr >> d` — one AND and one shift in gates. Division is astronomically expensive in hardware.

**Q2.** 8 KB = 2¹³ → 13 offset bits. Page number = 32 − 13 = **19 bits**.

**Q3.** Frames = physical memory / page size = 2³⁶ / 2¹² = 2²⁴ → **24 bits**. (Distractor: "36 − log2(PTE)" or confusing bytes with frames.)

**Q4.** 4 KB = 2¹² → offset = low 12 bits of `0x1A2B` = `0xA2B`; page = `0x1A2B >> 12 = 0x1`. Frame `0x0F` → `0x0F000`. PA = `0x0F000 + 0xA2B = 0x0FA2B`. **A**.

**Q5.** Hit: `10+50 = 60`; miss: `10` (TLB) + `50` (PTE) + `50` (data) = 110. `EMAT = 0.9×60 + 0.1×110 = 54 + 11 = 65 ns`.

**Q6.** A ✔ — sparse tables are the whole point. B ✔ — 2 PT levels + 1 data = 3 memory accesses (TLB is separate). C ✔ — standard design (each table = one page, that's why 4 KB tables of 1024 × 4 B entries appear everywhere). D ✘ — more levels = more accesses on a miss.

**Q7.** Offset 12 bits → page covers 4 KB. L2 index 10 bits → one L2 entry covers 2¹² B, one L2 table covers 2¹⁰ × 2¹² = 2²² = 4 MB. 1 GB = 2³⁰ → `2³⁰/2²² = 2⁸ = 256`.

**Q8.** A ✘ — TLB miss → page-table walk; only valid=0 causes a fault. B ✔ — VPN keys collide across processes. C ✔ — definition. D ✔ — no index field exists since any entry can hold any VPN.

**Q9.** EAT = (1−10⁻⁶)·100 + 10⁻⁶·(10 ms + 0.4·5 ms) = 99.9999 + 10⁻⁶ × 12,000,000 ns = 99.9999 + 12 = 111.9999 ≈ **112 ns**.

**Q10.** Hit path: `1 + 100 = 101`. Miss path: `1 + 4×100 + 100 = 501`. `EMAT = 0.98×101 + 0.02×501 = 98.98 + 10.02 = 109 ns`.

---

## 9. What to do next

1. Re-derive §2.1 and §5.1 answers on paper without looking.
2. Do the 40 questions in `OS_05`, `OS_06`, `OS_07` — they are built to attack exactly the patterns in §7.
3. Move to `OS_Notes_02_Page_Replacement.md` (virtual memory, Belady's anomaly, thrashing, working set).
