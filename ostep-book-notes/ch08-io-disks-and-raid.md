# OSTEP Notes 8 — I/O Devices, Hard Disk Drives, and RAID

> Covers Ch36 (I/O Devices), Ch37 (Hard Disk Drives), Ch38 (RAID). Opens Part III (Persistence). See `00-index.md` for the full series map.

---

## Part A: I/O Devices

### A Canonical Device
A simplified device exposes three hardware registers, accessed by the OS via reads/writes:
| Register | Purpose |
|---|---|
| **Status** | Read to check the device's current state |
| **Command** | Write to tell the device to perform a task |
| **Data** | Read/write to pass data to or from the device |

### The Canonical Protocol
```c
While (STATUS == BUSY) ;           // 1. poll until device is ready
Write data to DATA register        // 2. programmed I/O (PIO): CPU moves the data itself
Write command to COMMAND register  // 3. this starts the operation
While (STATUS == BUSY) ;           // 4. poll until device is done
```
⭐ **The problem:** steps 1 and 4 **poll** — the CPU busy-waits, wasting cycles it could spend running another process.

### Lowering CPU Overhead With Interrupts
Instead of polling, the OS issues the request, puts the caller to **sleep**, and switches to another process. When the device finishes, it raises a **hardware interrupt**, jumping the CPU into a pre-registered **interrupt service routine (ISR)**, which finishes the request and wakes the waiting process.

```
Without interrupts:  CPU: 1 1 1 1 1 p p p p p 1 1 1 1 1     (wasted polling)
With interrupts:     CPU: 1 1 1 1 1 2 2 2 2 2 1 1 1 1 1     (Process 2 runs while disk works)
                     Disk:          1 1 1 1 1
```
> **Tip: interrupts aren't always better than PIO.** For a **fast** device, polling can actually win — the overhead of a full interrupt + context switch may exceed just waiting a moment. The right choice depends on device speed; some systems use a **hybrid**, polling briefly before falling back to interrupts. ⭐ **A specific failure mode of pure interrupts: livelock** — a flood of incoming interrupts (e.g., a packet storm hitting a web server) can leave the OS doing *nothing but* servicing interrupts, starving user-level processes entirely; occasional polling gives the OS back some control in this case. **Interrupt coalescing** (batching several near-simultaneous completions into one interrupt delivery) is a common mitigation, trading a bit of latency for lower overhead.

### More Efficient Data Movement With DMA
⭐ **The remaining problem:** PIO still burns CPU cycles copying data word-by-word to/from the device. **Direct Memory Access (DMA)** offloads this: the OS tells the DMA engine the source, size, and destination device; the DMA engine copies the data **independently**, raising an interrupt only once the whole transfer is done — freeing the CPU for other work during the copy itself.
```
CPU: 1 1 1 1 1 2 2 2 2 2 2 2 2 1 1     (Process 2 runs during the copy)
DMA:           c c c                    (DMA engine does the copying)
Disk:                    1 1 1 1 1
```

### Fitting Into the OS: The Device Driver
⭐ The OS is structured to hide device-specific details behind a **generic block interface** (`read`/`write`); a lower **device driver** layer translates generic requests into the specific register-level protocol each device type requires. This is a direct instance of the recurring **abstraction** theme — a file system doesn't need to know whether it's talking to a SCSI or ATA disk. The downside: driver code makes up a **huge fraction of kernel code** (and hence bugs) in modern OSes, since each new device needs its own driver.

---

## Part B: Hard Disk Drives

### The Interface
A disk presents a simple abstraction: an array of **sectors** (512-byte blocks), numbered 0 to N−1 — a **logical block address (LBA)**. Internally, physical geometry is far more complex.

### Basic Geometry
- **Platters** — circular, magnetic-coated surfaces holding data; a **spindle**, connected to a motor, spins them at a constant RPM.
- **Tracks** — concentric circles of sectors on a surface; thousands fit within the width of a human hair.
- **Disk heads** (one per surface, attached to a single **disk arm**) read/write bits by sensing/inducing magnetic field changes.

### Single-Track Latency: Rotational Delay
On a single track, if the desired sector has just passed the head, the disk must **wait almost a full rotation** — this wait is **rotational delay.**

### Multiple Tracks: Seek Time
For a request on a *different* track, the disk arm must physically **seek** to the correct track — comprising: acceleration, coasting, deceleration, and **settling** (fine positioning — often the single largest component, since it must be precise). Only **after** settling can the (now-continuing) rotation carry the desired sector under the head — meaning a seek is typically followed by **some** rotational delay too.
- ⭐ **Track skew:** sectors are deliberately offset between adjacent tracks so that a sequential read crossing a track boundary doesn't have to wait almost a full rotation for the next sector — the seek time to the adjacent track is accounted for in the skew.
- ⭐ **Multi-zoned disks:** outer tracks are physically longer, so they're built to hold **more sectors** than inner tracks (constant sectors-per-track would waste the outer tracks' extra space) — a "zone" is a contiguous set of tracks sharing the same sectors/track count.
- **Track buffer / cache:** a small amount of on-drive memory (often a full track's worth) lets the drive **read ahead** in anticipation of nearby follow-up requests.

### I/O Time: Doing the Math
```
T_I/O = T_seek + T_rotation + T_transfer
```
⭐ **Worked reasoning example:** for a 10MB transfer with a 7ms seek + 3ms rotation + 200ms transfer (at 50MB/s) → total 210ms → **effective bandwidth (S) ≈ 47.6MB/s**, close to peak, because the large transfer **amortizes** the fixed seek/rotation cost. For a 10KB transfer with the same seek/rotation but only 0.195ms transfer time → **effective bandwidth (R) ≈ 0.98MB/s** — nearly 50× worse, because the fixed overhead now dominates. ⭐ **General lesson:** the ratio between sequential (S) and random (R) I/O performance on a disk can be enormous — this is precisely why so much of file-system and database design (LSM-trees, log-structured writes, batching) is obsessed with converting random I/O into sequential I/O.

### Disk Scheduling
Because the OS *can* estimate how long each pending I/O will take (unlike CPU job lengths), it can **greedily approximate SJF** for disk requests.

| Algorithm | Rule | Weakness |
|---|---|---|
| **SSTF** (Shortest Seek Time First) / NBF (nearest block first) | Service whichever pending request is on the **nearest track** | Doesn't account for rotation; and if a **steady stream** of requests keeps arriving near the head's current position, far-away requests can **starve** |
| **SCAN / Elevator** | Sweep across tracks in one direction, servicing requests in order, then reverse direction ("like an elevator") | Favors middle tracks slightly (visited twice as often as the outer/inner extremes per full cycle) |
| **C-SCAN** | Like SCAN but only sweeps one direction (e.g., outer→inner), then jumps back to the start | More fair between inner/outer tracks than plain SCAN |
| **SPTF** (Shortest Positioning Time First) | Accounts for **both** seek *and* rotational delay when picking the next request | Best approximation of SJF, but requires low-level disk geometry knowledge often only the **disk's own internal controller** has — so it's typically implemented **inside the disk**, not the OS |

---

## Part C: RAID (Redundant Arrays of Inexpensive Disks)

⭐ **Motivation:** a single disk is limited in capacity, performance, *and* reliability (any one disk can fail and lose your data). RAID combines multiple physical disks into one logical unit that's faster, bigger, and/or more fault-tolerant — and does so **transparently.**

> **Tip: transparency enables deployment.** A RAID looks exactly like one big disk to the OS/file system above it — no software changes are needed to benefit from it. This transparency was a direct contributor to RAID's real-world success: an admin could simply swap in a RAID box in place of a single disk.

### Fault Model
The standard, simplifying **fail-stop fault model**: a disk is either fully **working** or **completely and detectably failed** (no silent corruption, no partial/latent errors — those harder, more realistic faults are addressed later, in the data-integrity notes file).

### How to Evaluate a RAID — Three Axes
| Axis | Question |
|---|---|
| **Capacity** | Given N disks of B blocks each, how much *usable* space results? |
| **Reliability** | How many disk failures can be tolerated before data is lost? |
| **Performance** | How does it perform across different workloads (sequential/random, read/write)? |

### RAID Level 0: Striping (no redundancy)
Spread blocks **round-robin** across disks in fixed-size **chunks**; a set of blocks spanning all disks at one "row" is a **stripe**.
```
Disk0  Disk1  Disk2  Disk3
  0      1      2      3
  4      5      6      7
```
**Mapping formula** (chunk size = 1 block): `Disk = A % num_disks`, `Offset = A / num_disks` (integer division).
- **Capacity:** full — `N · B` (no redundancy at all — RAID-0 "is actually not a RAID level," since it adds no fault tolerance, only a performance/capacity baseline).
- **Reliability:** **0** — any single disk failure loses data.
- **Performance:** best case for everything — `N · S` sequential, `N · R` random (uses all disks in parallel).
- ⭐ **Chunk size trade-off:** small chunks spread a single file across more disks (more intra-file parallelism, but positioning time = the *max* of all disks' positioning times); large chunks reduce that parallelism but improve positioning cost per request, relying instead on many concurrent requests to reach full throughput.

### RAID Level 1: Mirroring
Keep 2+ full copies of every block, each on a different disk.
```
Disk0  Disk1  Disk2  Disk3
  0      0      1      1
  2      2      3      3
```
- **Capacity:** `(N·B)/2` (mirroring level 2) — expensive.
- **Reliability:** guaranteed to tolerate **1** disk failure (can sometimes survive more, if the failed disks happen not to be a mirrored pair — but this is generally treated as a happy accident, not a design guarantee).
- **Performance:**
  - Single-read/single-write latency ≈ same as one disk (reads pick either copy; writes go to both copies **in parallel**, so latency ≈ the slower of the two, not the sum).
  - ⭐ **Sequential reads are surprisingly NOT 2× faster** despite having 2 copies — because interleaving reads across mirrored pairs means each individual disk only delivers **every other block**, wasting half its rotational bandwidth on "skip-over" — net sequential bandwidth is `(N/2)·S`, not `N·S`.
  - Random reads: best case, `N·R` (fully parallel, arbitrary distribution across all disks).
  - Random writes: `(N/2)·R` — each logical write is 2 physical writes, so throughput is effectively halved versus striping, even though all disks stay busy.

### RAID Level 4: Parity
Add **one parity block per stripe** (not a full mirror) — much cheaper in space.
```
Disk0  Disk1  Disk2  Disk3  Disk4(parity)
  0      1      2      3      P0
  4      5      6      7      P1
```
⭐ **Parity computation: bitwise XOR** across all data blocks in a stripe. XOR's key property: the count of 1-bits in each row (across data + parity) is always **even** — this invariant is exactly what enables reconstruction: if any one block in a row is lost, XOR-ing together everything else that's left recovers the missing value.
```
C0  C1  C2  C3   P
0   0   1   1    0     (even number of 1s: C2,C3 → P=0)
0   1   0   0    1     (odd count without P → P=1 makes it even)
```
**Performance:**
- **Sequential writes:** use the **full-stripe write** optimization — compute the new parity from the full set of blocks being written, then write all blocks (data + parity) to all disks **in parallel** in one shot → `(N−1)·S` MB/s.
- **Random reads:** spread across data disks only (parity disk isn't touched) → `(N−1)·R`.
- ⭐ **Random writes — the "small-write problem," parity-RAID's Achilles' heel:** updating a single block requires updating its stripe's parity too. **Subtractive parity** is the efficient method:
```
P_new = (C_old ⊕ C_new) ⊕ P_old
```
  — read the old data block and old parity (2 reads), compute the new parity, then write new data and new parity (2 writes) = **4 I/Os per logical write.** ⭐ **The bottleneck:** *every* random write, regardless of which data disk it targets, must also touch the **single shared parity disk** — serializing all writes through it. Net random-write throughput: just `R/2` — **does not improve as you add more disks**, a severe limitation.

### RAID Level 5: Rotating Parity
Identical to RAID-4, except the parity block's disk **rotates** stripe by stripe:
```
Disk0 Disk1 Disk2 Disk3 Disk4
  0     1     2     3    P0
  5     6     7    P1     4
 10    11    P2     8     9
```
⭐ **Fix:** since parity blocks are now spread across *all* disks rather than concentrated on one, concurrent small writes to *different* stripes touch **different** parity disks and can proceed **in parallel** — directly resolving RAID-4's bottleneck. Capacity, reliability, sequential performance, and single-request latency are all **identical to RAID-4**; the gain is specifically in **random write throughput**, which scales with disk count again (`N/4 · R` for random writes, roughly). ⭐ **As a result, RAID-5 has almost entirely replaced RAID-4 in practice** — RAID-4 only remains useful in niche cases guaranteed to never do small random writes (avoiding the problem RAID-5 was built to fix, while staying slightly simpler to implement).

### RAID Comparison Summary

| | RAID-0 | RAID-1 | RAID-4 | RAID-5 |
|---|---|---|---|---|
| **Capacity** | `N·B` | `(N·B)/2` | `(N−1)·B` | `(N−1)·B` |
| **Reliability (failures tolerated)** | 0 | 1 (guaranteed) | 1 | 1 |
| **Sequential Read/Write** | `N·S` | `(N/2)·S` | `(N−1)·S` | `(N−1)·S` |
| **Random Read** | `N·R` | `N·R` | `(N−1)·R` | `N·R` |
| **Random Write** | `N·R` | `(N/2)·R` | `~R/2` | `~(N/4)·R` |
| **Latency (read/write)** | T / T | T / T | T / 2T | T / 2T |

⭐ **The single-sentence summary of the whole RAID design space:** mirroring (RAID-1) trades **capacity** for good all-around performance and simplicity; parity (RAID-4/5) trades **random-write performance** for much better capacity efficiency — and RAID-5's rotation is a targeted fix for parity's single worst weakness.

---

## Summary Table

| Concept | Purpose | Key trade-off |
|---|---|---|
| **Polling vs. interrupts** | Detecting device completion | Interrupts overlap CPU/IO but cost more per-event overhead — best for slow devices; polling wins for fast ones or under interrupt storms |
| **DMA** | Offloading bulk data copying from the CPU | Requires a dedicated DMA engine; frees the CPU during large transfers |
| **Seek + rotation + transfer** | Disk I/O time model | Random I/O pays fixed seek/rotation cost per request; sequential I/O amortizes it — explains the S vs. R gap |
| **SSTF / SCAN / SPTF** | Disk request ordering | SSTF can starve; SCAN/elevator fixes starvation at some fairness cost; SPTF is most accurate but needs disk-internal geometry |
| **RAID-0/1/4/5** | Combine disks for capacity/performance/reliability | Striping = no redundancy; mirroring = costly but simple/fast; parity = capacity-efficient but has a random-write bottleneck (fixed by rotating it in RAID-5) |

## Interview/Exam-Ready Talking Points

- Explain **why interrupts aren't strictly better than polling**, and describe the livelock failure mode and interrupt coalescing as its mitigation.
- Walk through the **I/O time formula** and the S vs. R worked example numerically — this crisply explains *why* sequential access is so much faster than random access on disk, a foundational fact underlying LSM-trees, log-structured file systems, and database design generally.
- Explain **SSTF's starvation problem** and how the elevator/SCAN algorithm fixes it, using the elevator analogy.
- Derive **RAID-4's XOR parity reconstruction** from first principles (the "even number of 1s" invariant), and walk through the subtractive-parity update formula.
- Precisely explain **the small-write problem** in RAID-4, and why RAID-5's rotating parity fixes it — this is one of the most commonly tested RAID interview questions.
- Recite the **capacity/reliability/performance trade-offs** across RAID-0/1/4/5 from the comparison table, and be ready to justify each cell's value from the underlying mechanism, not just from memory.