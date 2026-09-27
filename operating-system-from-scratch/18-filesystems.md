# 18 — Filesystems

## Layer map for this file

```
┌────────────────────────────────────────────────┐
│ 1. BOOK: none. §6.6 "File System" is a         │
│    section heading with zero body text —       │
│    the closest the draft gets is files 15-17's │
│    raw sector reads, with no structure at all  │
├────────────────────────────────────────────────┤
│ 2. BEYOND THE BOOK: FAT12 — the natural        │
│    historical partner for a floppy-boot toy    │
│    OS, built from general filesystem theory    │
├────────────────────────────────────────────────┤
│ 3. MODERN: inodes, VFS, journaling             │
└────────────────────────────────────────────────┘
```

**Book coverage: Stub.** Chapter 1 (file `01`) names "file systems" as
one of five course goals; the Chapter 6 table of contents lists §6.6
"File System" directly after the disk-driver stub — and, like §6.3
and §6.5, it has no body text at all. Files `15`–`17` leave you able
to read and write raw, numbered 512-byte sectors — a filesystem is
everything layered on top of that to turn "sector 4,217" into "the
file named `kernel.bin`."

## Prerequisites

Files `01`–`17`.

---

## [BOOK] The Complete Inventory

Nothing. File `17`'s disk driver is as far as the book's own content
goes — raw sectors, addressed by number, with no naming, no
directories, no concept of "a file" at all. Everything below is
constructed specifically to fill the gap Chapter 1 promised and
Chapter 6 never delivered.

---

## [BEYOND THE BOOK] What a Filesystem Actually Adds

A raw disk (file `17`) is *N* addressable blocks with no meaning
attached to any of them. A filesystem imposes four things on top:
**naming** (paths instead of block numbers), **organization**
(directories), **allocation tracking** (which blocks belong to which
file, which are free), and **metadata** (size, timestamps,
permissions). None of this is free — it's real structure someone has
to design and someone has to implement.

## [BEYOND THE BOOK] FAT12: the Natural Fit for This Book's World

Given a floppy-disk-boot toy OS, FAT12 is the obvious real-world
choice — designed for exactly this size of medium, simple enough to
implement in a few hundred lines, and thoroughly documented.

![FAT12 disk layout: boot sector plus BIOS Parameter Block, two FAT copies, a fixed-size root directory, and the data region, followed by a diagram showing a file's clusters chained together through the FAT table itself, from a directory entry's single stored first-cluster number](images/18-fat12-layout.svg)

One genuinely satisfying connection back to file `06`: a *real*
FAT-formatted floppy's boot sector doesn't just carry the jump
instruction and the magic number — it also embeds a **BIOS Parameter
Block (BPB)**, describing sector size, cluster size, and FAT layout,
sitting in exactly the space Blundell's own minimal boot sector leaves
as unused padding. Same 512 bytes, a real filesystem asks more of them.

A file's contents live in a chain of **clusters**, threaded through
the **File Allocation Table** itself: a directory entry stores only
the *first* cluster number; that cluster's own entry in the FAT holds
the *next* cluster number, and so on until a reserved end-of-chain
value. Reading a file end to end means walking this chain one link at
a time — there's no separate index structure at all.

---

## [RELATED KNOWLEDGE] FAT vs. Inode-Based Design

Unix-family filesystems (ext4 and its predecessors) split things FAT
keeps together: an **inode** holds a file's metadata and block
locations, addressed by number; a directory entry is nothing more
than a name mapped to an inode number. Multiple names can point at the
same inode (**hard links**) — meaningless in FAT's design, where a
directory entry *is* the file's only record. Two genuinely different
answers to "how do I find a file's data," not a superficial naming
difference.

---

## [MODERN SYSTEM] VFS: The Same Split Idea, at Filesystem Scale

File `16`'s `screen.h`/`screen.c` split let the kernel call `print()`
without knowing it's VGA underneath. Linux's **Virtual File System
(VFS)** does the identical thing for storage: every filesystem — ext4,
btrfs, even FAT12 itself, still supported for USB drives — implements
a common interface (`open`, `read`, `write`, `close`), so the rest of
the kernel and every application call the same syscalls regardless of
what's actually formatting the disk underneath.

## [MODERN SYSTEM] Journaling: The Crash-Consistency Problem FAT Never Answers

If power is lost mid-write, FAT12 has no mechanism at all to guarantee
the FAT and the actual data stay consistent — a corrupted filesystem
is a real, common outcome. **Journaling filesystems** (ext4 and
others) write a description of an intended change to a dedicated
journal *before* applying it, so an interrupted operation can be
replayed or cleanly rolled back on the next mount — a problem this
book's world never has reason to raise, since it never writes to a
filesystem it built at all.

---

## [ADVANCED / BEYOND THE BOOK] Beyond Chains and Inodes

**Extent-based** allocation (used by ext4, NTFS, and others) stores a
*range* of contiguous blocks in one metadata entry instead of a
pointer per block — far more efficient for large, mostly-contiguous
files. **Copy-on-write** filesystems (btrfs, ZFS) never overwrite data
in place at all, enabling cheap snapshots and stronger crash
consistency, at real complexity cost.

---

## Practical Experiment

1. Create a small FAT12 image: `mkfs.fat -F 12 floppy.img` (sized to a
   floppy, e.g. 1.44 MB). `xxd floppy.img | head -5` — find the BPB
   fields (bytes per sector, sectors per cluster) sitting exactly where
   file `06`'s boot sector left blank padding.
2. Mount it, copy a small file in, unmount, then `xxd` the image again
   — find the new directory entry and its first-cluster number, and
   the corresponding FAT entry.
3. Add a second file large enough to need two clusters; find its FAT
   chain — two linked entries instead of one.
4. **Capstone-scale project:** write a minimal FAT12 reader for your
   own kernel (file `13`) — parse the BPB, read the root directory,
   find a file by name, follow its cluster chain, print its contents.

## Debugging

A "file not found" bug in a from-scratch FAT reader is almost always
one of: reading the root directory from the wrong sector (miscounting
past the boot sector and both FAT copies), comparing filenames without
accounting for FAT's fixed 8.3 padding format, or an off-by-one
walking the cluster chain. Verify each stage independently — can you
read *any* directory entry correctly before worrying about matching a
specific name?

## Source Code

Your FAT12 image, plus a minimal reader — the first genuinely
from-scratch subsystem in this course with no book original at all to
check line-by-line against.

---

## Common Misconceptions

- **"A filesystem is just how files are named."** Naming is one of
  four things it provides — allocation tracking and metadata matter
  just as much, and are what actually make "delete" and "how much
  space is free" possible.
- **"FAT and inode-based filesystems just use different terminology
  for the same idea."** Genuinely different designs — FAT's directory
  entry *is* the file's only record; inode-based designs separate name
  from metadata entirely, enabling things (hard links) FAT can't express.
- **"Any filesystem protects you from power loss mid-write."** FAT12
  has no journal and no such guarantee at all — corruption is a real,
  common outcome.

## Interview Questions

**Basic:** What four things does a filesystem add on top of raw
sectors?
**Intermediate:** How does a FAT filesystem find the second cluster of
a file that spans more than one?
**Advanced:** What's the architectural difference between FAT's design
and an inode-based filesystem's?
**Systems Design:** Why does Linux's VFS matter — what would application
code look like without it?
**Debugging:** A hand-written FAT12 reader can't find a file that
definitely exists on the image — what three things would you check first?
**Explain what happens when...:** you run `cat somefile.txt` on an
ext4-formatted Linux system, from the syscall to bytes returned?

## Active Recall

1. What does the book actually deliver toward "file systems"?
2. What four things does a filesystem provide beyond raw sectors?
3. How is a file's cluster chain actually stored in FAT?
4. What problem does journaling solve that FAT12 never addresses?

## Answers

1. Nothing — §6.6 is an empty section heading; files `15`–`17` leave
   you with raw, numbered sectors only.
2. Naming, organization (directories), allocation tracking, and metadata.
3. A directory entry stores only the first cluster number; each
   cluster's own FAT entry stores the next cluster's number, forming a
   chain ending in a reserved end-of-chain marker.
4. Crash consistency — recovering cleanly from a write interrupted by
   power loss, which FAT12 has no mechanism to guarantee at all.

---

## What You Should Now Understand

- Exactly how much (nothing) the book delivers here, and why files
  `15`–`17` are the honest limit of its own filesystem-adjacent content.
- FAT12's structure, chosen specifically because it's what this book's
  own floppy-boot world would naturally have used.
- The real architectural difference between FAT and inode-based design,
  and where Linux's VFS and journaling pick up from there.

---

**Next:** file `19-processes-and-threads.md` — another Stub file,
§7.1 has zero body text.
