# 19 — Filesystem and VFS

> **Where this fits in the stack**
> ```
> User  →  ...  →  Shell  →  Programs  →  Syscalls  →  [ VFS / FILESYSTEM ]  →  Kernel  →  Disk
>                                                        ^^^^^^^^^^^^^^^^^^^^^
>                                                        this file
> ```
> File 07's trace of `ls -lah /tmp` named `openat`, `getdents`, and `stat` as steps without opening any of them up. This file does exactly that, and answers a question worth sitting with: what *is* a directory, really, if not "a folder"?

---

## 1. Recap: what `ls` was actually doing

From file 07: `ls` calls `openat()` on the target directory, then `getdents()` repeatedly to read its entries, then `stat()`/`lstat()` on each entry individually to get the size/permission/timestamp data `-l` needs to display. This file explains precisely why it takes *three different syscalls* to do this, rather than one.

---

## 2. VFS — the Virtual Filesystem layer

**L1 — intuition:** the same `open`/`read`/`write`/`close` interface works identically no matter what's actually underneath — a local disk, a network share, or something the kernel is generating on the fly.

**L2 — systems detail:** the kernel defines a set of abstract filesystem operations — open, read, write, `readdir`, and others — as function pointers that **every filesystem type must implement**. When you call `open()` on a path, the VFS layer resolves the path (consulting the mount table to find which filesystem actually owns that location) and dispatches to *that filesystem's own* implementation — which might read real disk blocks (ext4), make a network request (NFS), or generate content on demand with no disk involvement at all (`procfs`, file 33).

This is exactly why the same `ls`, `cat`, and `cp` work identically whether you're browsing an ext4 partition, an NFS-mounted network share, or looking inside `/proc` — none of them contain any filesystem-specific code, because the VFS layer presents an identical interface regardless of what's really there.

---

## 3. Inode

Recap and deepen file 10: an inode holds one file's **metadata** and **data location** — permissions, owner, size, timestamps, and pointers to where its actual data blocks live on disk.

> **Remember**
> An inode does **not** contain the file's name. Names live entirely separately (Section 4). This is precisely why a single file's data can have **multiple names** — hard links (`ln`, without `-s`) — all pointing at the same inode, all equally "real," with none of them more the "true" name than any other. Deleting one name just removes one entry pointing at that inode; the inode and its data survive as long as at least one name, or one open file descriptor, still references it.

`ls -i` shows inode numbers directly.

---

## 4. Dentry (directory entry)

The piece connecting a **name** (a string, inside a specific directory) to an **inode number**. This is precisely what a directory *is*, mechanically: not a distinct kind of kernel object from an ordinary file, but a file whose content is a formatted list of `(name, inode number)` pairs.

The kernel keeps an in-memory cache of recently-used dentries (the "dentry cache," or dcache) specifically to speed up repeated path lookups — resolving `/usr/bin/ls` means looking up `usr` in `/`, then `bin` in `usr`, then `ls` in `bin`: three separate name-to-inode lookups, cached specifically so common paths don't re-read directory data from disk every time.

---

## 5. Why directories aren't simply "folders"

The GUI mental model — folders nested inside folders, like physical folders in a filing cabinet — is a useful metaphor, but not what's mechanically happening.

**What's actually happening:** a directory is a **file** (it genuinely has its own inode, like anything else) whose content is a list of `(name, inode number)` pairs. "Containment" is really just "this directory's content-list includes an entry pointing at that other file's or directory's inode." There's no literal nesting of *data* — a subdirectory is just another inode (which itself contains its own list of name-to-inode mappings) that happens to be referenced by one entry in its parent's list.

This is also the mechanical explanation for hard links: a file's data doesn't "live inside" any one directory in any deep sense — some directory's entry-list simply happens to include a mapping to that inode, and nothing prevents multiple different directories (or the same directory, under different names) from all having entries pointing at the identical inode simultaneously.

> **Interview answer**
> "A directory isn't a fundamentally different kind of object from a file — it's a file whose contents are a structured list of name-to-inode mappings, accessed through a dedicated syscall (`getdents`) rather than an ordinary `read()`. Hard links exist precisely because names and inodes are separate: multiple directory entries, even in different directories, can point at the same inode."

---

## 6. `openat()`, `getdents()`, `stat()`, precisely

- **`openat(dirfd, path, flags)`** (file 09) — opens a file or directory, returns an fd.
- **`getdents(fd, ...)`** (modern systems typically use `getdents64`) — reads raw directory entries from an **open directory fd**. This is a dedicated syscall specifically for this purpose — you cannot meaningfully `read()` a directory fd the way you would an ordinary file, precisely because its content has this specific structured `(name, inode)` format that needs its own accessor, not an undifferentiated byte stream. Returns a batch of `dirent` structures, each roughly containing an inode number, a name, and a type hint.
- **`stat()`/`lstat()`/`fstat()`** — given a path (`stat`, `lstat`) or an already-open fd (`fstat`), retrieves the target inode's metadata into a `struct stat`.

> **Common mistake**
> "`stat()` and `lstat()` are interchangeable." `stat()` follows a final symlink and reports the *target's* metadata; `lstat()` does **not** follow it, and reports the *link itself*. Getting this backwards on a symlink-heavy path gives you the wrong file's size, permissions, and type entirely.

---

## 7. The full `ls -lah /tmp` walk, now completely connected

```
openat("/tmp", ...)                     → fd
loop: getdents64(fd, ...)               → batches of (inode_number, name) until exhausted
for each name:  lstat(full_path, ...)   → size, permissions, owner, timestamps
                                            (getdents alone gives name + inode number,
                                             NOT the size/permission detail -lah needs —
                                             that's exactly why a SEPARATE stat call per
                                             entry is required)
format, write(1, ...)                    → the formatted listing, to stdout
```

---

## 8. Common misconceptions

| Misconception | Reality |
|---|---|
| "A directory is a genuinely different kind of kernel object from a file." | It's a file — one with specially-structured content, accessed through a dedicated syscall. |
| "An inode contains the file's name." | Names live in directory entries; an inode doesn't know its own name(s) at all. |
| "Deleting a file's last name instantly frees its data." | Only once **both** its link count (names) reaches zero **and** no process still has it open via an fd — this is exactly why `rm`-ing a file a running process still has open doesn't immediately reclaim disk space. |
| "You can `read()` a directory's raw bytes like an ordinary file." | You need `getdents()` specifically — directories aren't exposed as a plain byte stream. |

---

## 9. Try this

> **Try this**
> ```bash
> ls -i file.txt
> ln file.txt file2.txt
> ls -i file.txt file2.txt      # IDENTICAL inode number for both names
> ```
> A direct, hands-on hard-link demonstration.

> **Try this**
> ```
> stat file.txt
> ```
> A full inode metadata dump — every field this file described, in one place.

> **Try this**
> ```
> strace -e openat,getdents64,newfstatat ls /tmp
> ```
> Direct, observable confirmation of Section 7's trace, tying straight back to file 09's `strace` preview.

---

## 10. macOS note

Darwin's VFS layer, BSD-derived, uses the equivalent inode/dentry concepts — even though APFS's actual on-disk structure differs substantially from ext4's, it still exposes the same POSIX-compatible inode-based interface at the syscall level. Full comparison: file 43.

---

## 11. Summary

- VFS is the abstraction that lets identical syscalls work across wildly different underlying filesystems — local disk, network shares, and kernel-generated pseudo-filesystems alike.
- An inode holds a file's metadata and data location, but never its name.
- A dentry connects a name to an inode number — and a directory is simply a file whose content is a list of these mappings.
- Hard links exist because names and inodes are genuinely separate concepts; multiple names can reference one inode.
- `getdents()` gives you names and inode numbers; it takes a separate `stat`/`lstat` per entry to get the detailed metadata `ls -l` displays.

---

## What's next

`20_PERMISSIONS_AND_SUDO.md` builds directly on the inode metadata this file introduced — `rwx`, ownership, `chmod`/`chown`, and exactly how the kernel enforces permission checks before it lets any of these syscalls succeed.
