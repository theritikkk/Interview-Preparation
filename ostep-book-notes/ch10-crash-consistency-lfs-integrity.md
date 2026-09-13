# OSTEP Notes 10 — Crash Consistency, Log-Structured File Systems, and Data Integrity

> Covers Ch42 (Crash Consistency: FSCK and Journaling), Ch43 (Log-structured File Systems), Ch44 (Data Integrity and Protection). See `00-index.md` for the full series map.

---

## Part A: Crash Consistency — FSCK and Journaling

### The Crash Consistency Problem
A single logical file-system update (e.g., appending a block) requires **multiple** physical disk writes -- say, an updated inode, an updated allocation bitmap, and the new data block itself. The disk only commits **one write at a time**, and a crash (power loss) can occur **between** any of these writes -- leaving on-disk structures inconsistent: a bitmap that disagrees with the inode, an inode pointing at garbage data, or a permanently "leaked" block marked allocated but referenced by nothing. This is the **crash-consistency problem** (or **consistent-update problem**).

### Solution #1: The File System Checker (fsck)
The lazy approach: let inconsistencies happen, then find and repair them **after the fact**, typically at boot, before the file system is mounted. `fsck` runs in phases:
| Phase | What it checks |
|---|---|
| **Superblock** | Sanity checks (e.g., FS size > allocated blocks); falls back to a backup superblock copy if corrupt |
| **Free blocks** | Scans all inodes/indirect blocks to rebuild correct allocation bitmaps, **trusting the inodes** over the bitmaps if they disagree |
| **Inode state** | Verifies each inode's type field and other validity; clears unrecoverable inodes |
| **Inode links** | Recomputes each file's reference count by walking the **entire directory tree** from root; a reachable-but-unreferenced inode is moved to `lost+found` |
| **Duplicates** | Detects two inodes pointing at the same block; clears the bad one or copies the block |
| **Bad blocks** | Detects/clears pointers to out-of-range block addresses |
| **Directory checks** | Verifies `.`/`..` presence/order and that no directory is linked more than once |

⭐ **The fundamental weakness (the book's own analogy):** *fsck is like dropping your keys in the bedroom, then searching the entire house -- basement to attic -- to find them.* It **must scan the entire disk**, even though only a handful of blocks were involved in the crash -- prohibitively **slow** as disks and RAID arrays grew, sometimes taking hours.

### Solution #2: Journaling (Write-Ahead Logging)
Borrowed directly from database systems. Core idea: **before** overwriting real on-disk structures, first write a small **note** -- describing the pending update -- to a separate, well-known **log (journal)**. If a crash occurs mid-update, recovery just **reads the log** to know exactly what to redo -- no full-disk scan needed. Used by ext3/ext4, ReiserFS, IBM JFS, SGI XFS, Windows NTFS.

Layout: `[Super][Journal][Group 0][Group 1] ... [Group N]`

#### Data Journaling (full protocol)
For an update touching inode `I[v2]`, bitmap `B[v2]`, and data block `Db`:
```
Journal:  [TxB][ I[v2] ][ B[v2] ][ Db ][TxE]
```
- **TxB (transaction begin)** -- records the pending update's final target addresses plus a **transaction ID (TID)**.
- Middle blocks are the **physical** contents being written (physical logging; **logical logging** stores a more compact description instead).
- **TxE (transaction end)** -- marks completion, tagged with the same TID.

**Full protocol, in order:**
1. **Journal write:** write TxB + all pending data/metadata to the log; wait for completion.
2. **Journal commit:** write TxE; wait -- the transaction is now **committed.**
3. **Checkpoint:** write the actual updates to their real, final on-disk locations.
4. **Free:** later, mark the transaction free in the **journal superblock**, allowing reuse (a **circular log**).

⭐ **Why the two-step journal write matters -- a subtle ordering hazard:** if all 5 blocks were issued as one big write, the disk could complete them **out of order** internally -- e.g., TxB/I[v2]/B[v2]/TxE landing but `Db` not, before a crash. The transaction would then **look complete** (matching TIDs) even though `Db` is garbage -- recovery would blindly replay it. **Fix:** write everything **except TxE first**, wait, then write TxE -- so finding TxE on disk guarantees everything before it made it too.

> **Aside:** disk write buffering can report a write complete once merely cached, not on the physical surface -- breaking ordering unless the OS uses explicit **write barriers.** Some disk manufacturers have reportedly ignored barrier requests to appear faster, undermining this guarantee invisibly until a crash occurs.

⭐ **Optimization (later adopted into ext4):** embed a **checksum** of the transaction in TxB/TxE, allowing the **entire** transaction to be written in one fast write with no wait between steps -- on recovery, a checksum mismatch means a crash interrupted the write, so that transaction is simply discarded.

#### Metadata (Ordered) Journaling -- avoiding the double-write cost
Full data journaling writes every data block **twice** -- roughly halving effective write bandwidth. The fix: journal only metadata, write data directly -- but **ordering matters**:
```
Journal:  [TxB][ I[v2] ][ B[v2] ][TxE]      (Db is NOT logged)
```
⭐ **Why data must be written *before* the journal commit:** if `Db` were written *after* commit, and a crash hit in between, recovery would replay `I[v2]`/`B[v2]` -- producing a metadata-consistent file system whose inode now points at **garbage.** The rule: **"write the pointed-to object before the pointer."**

**Corrected protocol:**
1. **Data write:** write `Db` to its final location; wait.
2. **Journal metadata write:** write TxB + metadata to the log; wait.
3. **Journal commit:** write TxE; wait -- transaction (including already-written data) is committed.
4. **Checkpoint metadata:** write `I[v2]`/`B[v2]` to final locations.
5. **Free:** mark free later.

⭐ Net effect: metadata journaling recovers just as fast, while writing ordinary data only once -- the dominant mode in practice (ext3/ext4 default).

---

## Part B: Log-Structured File Systems (LFS)

> **The Crux: how can a file system transform (almost) all writes into sequential writes?** Reads can go anywhere; but for writes, the file system has a choice of placement -- LFS exploits this fully.

### The Basic Idea
Rather than updating structures **in place**, buffer all pending updates -- data blocks, inodes, everything -- in memory, and periodically flush the whole batch as **one large, sequential write** to a fresh region of disk. LFS **never overwrites in place.**

### Segments and Effective Buffering
Simply writing sequentially isn't enough -- writes spaced apart in time still incur near-full rotational delay even if sequential in address. **LFS batches updates into a large in-memory segment (a few MB)** before flushing -- amortizing fixed positioning cost across a large write, the same amortization principle seen elsewhere (disk I/O time, context-switch overhead).
```
T_write = T_position + D / R_peak
R_effective = D / T_write
```
Solving for `D` given a target fraction `F` of peak bandwidth gives a concrete buffer-size answer (typically several MB to reach 90%+ of peak).

### Problem: Finding Inodes
Old file systems (and FFS) place inodes at fixed, computable locations. **LFS scatters inodes** wherever they were last written, and since nothing updates in place, **the current version of an inode keeps moving.**

### Solution: The Inode Map (imap)
A level of indirection mapping inode number -> current disk address. Pieces of the imap are written **alongside** whatever else is currently being flushed, rather than at one fixed (hot, seek-inducing) location.

> **Tip: use a level of indirection.** *"All problems in computer science can be solved by another level of indirection, except of course for the problem of too many indirections"* (Wheeler).

⭐ **The recursive update problem, solved by the imap:** without indirection, moving an inode would force updating whatever directory entry points at it -- cascading updates all the way to the root. The imap breaks this: a directory entry refers to a stable **inode number**, never a raw address; only the imap (itself updated via the same batched mechanism) tracks where that number currently lives.

### The Checkpoint Region (CR)
A small, fixed-location region holding pointers to the latest imap pieces -- the one "must be found without indirection" bootstrap point, analogous to a superblock.

**Full lookup path:** CR -> latest imap pieces -> imap[inode #] -> inode's current address -> inode -> data block address -> data.

### A New Problem: Garbage Collection
Since updates never overwrite in place, old versions are left scattered as **garbage.** LFS's cleaner: (1) read in several old, partially-live segments; (2) determine live vs. dead blocks (via a segment summary cross-checked against the current imap/inode); (3) compact live blocks from `M` old segments into `N < N` new segments, freeing the originals.

⭐ **Why cleaning works on whole segments, not individual blocks:** freeing scattered single blocks would fragment free space -- undermining LFS's whole point (guaranteed large contiguous regions for future writes). The remaining open question is **policy** -- which segments to clean and when -- balanced against how "dead" and how old each segment is.

⭐ **Modern relevance:** SSDs are internally log-structured for similar reasons (flash's erase-before-rewrite constraint) and typically include an analogous indirection layer to solve their own version of the recursive-update problem.

---

## Part C: Data Integrity and Protection

> **The Crux: how should systems ensure data written to storage remains correct, with low space/time overhead?**

### Disk Failure Modes -- Beyond Fail-Stop
Large-scale studies (Bairavasundaram et al., ~1.5M drives, 3 years) found disks fail in more nuanced ways than "working or dead":

| Failure type | What it is | Detectable? |
|---|---|---|
| **Latent sector errors (LSEs)** | A sector becomes physically unreadable | **Yes** -- drive's own ECC detects and reports an error |
| **Block corruption** | A block silently returns the **wrong** contents | **No** -- a **silent** fault |

⭐ This is the **fail-partial disk failure model.** Cheap (SATA) drives saw LSEs on ~9.4% and corruption on ~0.5% of drives studied; costlier drives were roughly an order of magnitude better on both, but not immune. Error rates rose with disk size and age, and errors showed real spatial/temporal locality. Notably, **most LSEs were found via background scrubbing**, not ordinary access.

### Handling Latent Sector Errors
Comparatively easy -- the drive reports the error explicitly, so the system falls back on existing **redundancy**: read the mirror (RAID-1) or reconstruct from parity (RAID-4/5). **NetApp's RAID-DP** keeps two parity disks specifically to survive an LSE hit *during* a full-disk reconstruction after a separate whole-disk failure.

### Detecting Corruption: The Checksum
The harder problem, since corruption is silent. A **checksum** -- a small summary computed from a block and stored alongside it -- is the primary tool: recompute on read and compare.

Common functions, increasing in cost/strength: **XOR** (cheap, blind to some error patterns), **addition**, and stronger functions like **Fletcher checksum or CRC.**

> **Tip: there's no free lunch.** Stronger detection always costs more CPU/time.

**Checksum layout trade-off:**
| Layout | How | Cost |
|---|---|---|
| One checksum per (520-byte) sector | Drive natively supports oversized sectors | Clean 1-write updates; needs special hardware |
| Checksums batched separately from data | A block of `n` checksums followed by `n` data blocks | Works on any drive; updating one data block needs 1 read + 2 writes |

**Using checksums:** on read, fetch block `D` and stored checksum `Cs(D)`; compute `Cc(D)` fresh; compare. Mismatch -> corruption detected, fall back to redundancy if available. ⭐ Checksums **detect**; they don't fix -- recovery still needs redundancy somewhere.

### Misdirected and Lost Writes
- **Misdirected writes** (block written to the wrong location): fixed by embedding the **intended disk number and block offset** inside the checksummed block itself -- a purely-content checksum can't catch a correctly-written block landing in the wrong place.
- **Lost writes** (drive reports success but never persists): the stale old block's checksum still matches its own old contents, so **ordinary checksums cannot detect this at all.** Fixes: (1) **write-verify/read-after-write** -- doubles I/O; (2) **ZFS's approach** -- store a checksum of each data block in its **parent** inode/indirect block, so a lost data write leaves the parent's recorded checksum mismatched against the stale child (fails only if both writes are lost simultaneously).

### Scrubbing
A periodic **background** process that proactively reads every block and verifies its checksum, catching problems before an application actually needs that data -- consistent with the finding that most real LSEs were caught this way, not through ordinary access. Typically scheduled during low-activity periods to limit overhead.

⭐ **Overall theme:** just as transactions and consensus build strong guarantees from weaker primitives, **data integrity is built entirely from redundancy plus verification** -- checksums to detect, mirrors/parity/backups to actually recover.

---

## Summary Table

| Concept | Core idea | Cost/trade-off |
|---|---|---|
| **fsck** | Scan the whole disk after a crash, rebuild consistent metadata | Correct but far too slow for large modern disks |
| **Data journaling** | Log full data + metadata before checkpointing | Fast, safe recovery; doubles write traffic |
| **Metadata (ordered) journaling** | Log only metadata; write data first, in order | Nearly as fast to recover, without doubling data writes |
| **LFS segments** | Buffer all updates, write sequentially in large batches | Excellent write throughput; needs inode-location + GC solutions |
| **Inode map (imap)** | Indirection: inode number -> current disk address | Solves inode lookup and the recursive-update problem |
| **Segment cleaning** | Garbage-collect dead blocks in whole-segment units | Keeps free space usable in large chunks; policy is separate/open |
| **Checksums** | Detect (not fix) silent corruption | Needs redundancy elsewhere to recover |
| **Scrubbing** | Proactively verify data at rest | Background I/O cost, but catches most real LSEs early |

## Interview/Exam-Ready Talking Points

- Explain the **crash-consistency problem** with a concrete 3-block example (inode/bitmap/data), and why single-write-at-a-time disks make it unavoidable without extra machinery.
- Walk through **fsck's "search the whole house for your keys" weakness** as the motivation for journaling.
- Recite the **full data-journaling protocol** and explain precisely why **TxE must be written after** the rest of the transaction.
- Explain **why metadata journaling requires data written before the journal commit**, and what fails if that order is reversed.
- Explain **LFS's imap** and how it solves both inode lookup and the recursive update problem via indirection.
- Explain **why LFS garbage collection works on whole segments**, tying it back to LFS's reason for existing.
- Distinguish **latent sector errors from silent corruption**, and explain why checksums matter only for the latter.
- Explain why **lost writes evade ordinary checksums** and how ZFS's parent-checksum trick catches them.