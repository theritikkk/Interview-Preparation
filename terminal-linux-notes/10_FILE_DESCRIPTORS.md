# 10 — File Descriptors

> **Where this fits in the stack**
> ```
> User  →  Terminal Emulator  →  PTY/TTY  →  Shell  →  Processes  →  Syscalls  →  [ FILE DESCRIPTORS ]  →  Kernel
>                                                                                  ^^^^^^^^^^^^^^^^^^^^^
>                                                                                  this file
> ```
> File 08 already told you that file descriptors are *shared, not copied*, after `fork()` — without saying precisely what "shared" points at. This file makes that precise, because the distinction between a file descriptor, an open file description, and an inode is one of the most commonly blurred ideas in all of Unix systems programming, and every later file involving redirection (12) or pipes (13) depends on you actually having it straight.

---

## 1. What is a file descriptor?

**L1 — intuition:** a small number representing "a thing this process currently has open."

**L2 — systems detail:** a file descriptor is an **index into a per-process table** the kernel maintains for that process. It's not a file, not a pointer to a file, not the file's data — it's a slot number, meaningful only *within that one process's own table*. "fd 3" in one process and "fd 3" in a completely different process can point at entirely unrelated things; the number carries no meaning outside the table it indexes into.

---

## 2. Why 0, 1, 2 are stdin, stdout, stderr

```
process
 |
 +-- fd 0   (stdin)
 +-- fd 1   (stdout)
 +-- fd 2   (stderr)
```

This is **convention**, not something the kernel treats as inherently special. Nothing stops a program from reading input via fd 5 if it wanted to — but essentially all user-space tooling (libc, shells, every terminal-aware program) agrees these three specific numbers mean "standard input," "standard output," and "standard error," and the shell **deliberately arranges** for exactly these three to be connected appropriately — normally, to the PTY slave from file 02 — before it ever `exec`s a child. File 11 goes deep on why stderr exists as a genuinely separate channel from stdout.

---

## 3. The distinction that must be crystal clear: file descriptor vs open file description vs inode

Three separate layers, each meaning something different:

| Layer | What it is | Scope |
|---|---|---|
| **File descriptor** | A small integer — an index into one process's fd table | Per-process; meaningless outside its own table |
| **Open file description** | A kernel object holding the **current read/write offset** and **status flags** (`O_APPEND`, `O_NONBLOCK`, etc.) for one specific "opening" of something | Created fresh by each `open()` call; can be *shared* across multiple fds, even across different processes |
| **Inode** | The actual underlying file (or device) — its permissions, owner, size, and on-disk data location | One per real file; many independent open file descriptions can all refer to the same one |

### Scenario A — two independent `open()` calls on the same file

```
Process A                          Process B
fd table:                          fd table:
 fd 3 ----+                         fd 3 ----+
          |                                  |
          v                                  v
  Open File Description A           Open File Description B
  offset: 120, flags: O_RDONLY      offset: 0,   flags: O_RDONLY
          |                                  |
          +----------------+-----------------+
                            |
                            v
                          INODE
                    (the actual file on disk:
                     permissions, size, data blocks)
```

Two completely separate calls to `open()` — even on the identical path, even from unrelated processes — produce **two independent open file descriptions**, each with **its own offset**. Process A reading forward doesn't move Process B's position at all. Both descriptions ultimately point at the same one inode, because it's the same underlying file — but the *reading position* is not shared, because that lives one layer up, in the open file description, and each `open()` created its own.

### Scenario B — `fork()`, or `dup()`/`dup2()`

```
Process A (parent)                Process B (child, after fork())
fd table:                          fd table:
 fd 3 ----+                         fd 3 ----+
          |                                  |
          +----------------+-----------------+
                            |
                            v
                  Open File Description
                  offset: 120, flags: O_RDONLY   <- SHARED
                            |
                            v
                          INODE
```

This time, there's only **one** open file description, and *two* fd-table entries — in two different processes — pointing at it. Because the offset lives inside the shared open file description, **either process advancing it (by reading or writing) moves it for both.** This is the exact, precise mechanism file 08 was previewing when it said fd table entries are "shared, not copied" after `fork()` — now you know exactly *what* is shared, and exactly why it behaves the way it does.

> **Interview answer**
> "A file descriptor is a per-process integer index into that process's fd table. Each table entry points at an open file description — a kernel object holding the current offset and status flags — and multiple entries, even across different processes, can point at the same open file description, in which case they share that offset. The open file description in turn points at an inode, the actual underlying file, and many independent open file descriptions can all point at the same inode without sharing anything else."

---

## 4. `open()`, `close()`, `dup()`, `dup2()`, `dup3()`

```c
int fd = open(path, flags, mode);
```
Creates a **brand-new open file description** (fresh offset, typically 0) and a new fd-table entry in the calling process pointing at it.

```c
close(fd);
```
Removes that fd-table entry. If it was the *last* entry anywhere pointing at that open file description, the kernel destroys the open file description too — it's reference-counted.

```c
int newfd = dup(oldfd);
```
Creates a **new fd** (the lowest currently-unused number) in the *same* process, pointing at the *same* open file description as `oldfd` — Scenario B's sharing, deliberately triggered by a syscall rather than by `fork()`.

```c
dup2(oldfd, newfd);
```
Like `dup()`, but lets you specify the **exact** fd number to reuse, atomically closing `newfd` first if it was already open. **This is the precise mechanism behind shell redirection and pipe wiring** — `dup2(file_fd, STDOUT_FILENO)` makes fd 1 point at the same open file description as `file_fd`, so anything subsequently written to "stdout" actually lands wherever `file_fd` pointed. Full application: files 12 and 13.

```c
dup3(oldfd, newfd, flags);
```
Like `dup2()`, but additionally accepts flags — most notably `O_CLOEXEC` — applied **atomically** in the same call. This exists specifically to close a race: doing `dup2()` followed by a separate `fcntl()` call to set close-on-exec leaves a window where a signal handler, or another thread, could `fork`+`exec` in between, leaking the fd into a child process you never intended to hand it to.

> **Common mistake**
> "Duplicating a file descriptor with `dup`/`dup2` preserves its close-on-exec flag." It does not — per POSIX and Linux's own documented behavior, the duplicate's close-on-exec flag is always **off** after plain `dup()`/`dup2()`, regardless of the original's setting. Only `dup3()` with `O_CLOEXEC` explicitly passed can create a duplicate that starts with it set.

---

## 5. The fd table, precisely

Conceptually an array inside the process's `task_struct` (file 08): index = fd number, and each occupied slot holds a pointer to an open file description **plus a small amount of per-slot state of its own** — most importantly, the close-on-exec flag.

> **Remember**
> Close-on-exec lives in the **fd table entry**, not the open file description. Two different fds, in two different processes, sharing the exact same open file description (Scenario B) can have **different** close-on-exec settings — one might vanish across that process's next `execve()`, the other might survive, even though they're pointing at the identical underlying offset and file.

---

## 6. `/dev/null` and `/dev/zero` — the special files redirection often actually points at

Both are real inodes — you can `open()` them, get a perfectly ordinary fd and open file description for them — but they're backed by **kernel device-driver code**, not actual disk blocks. `read()`/`write()` on them is intercepted by that driver logic instead of touching a filesystem.

- **`/dev/null`** — writes to it are silently discarded (successfully, no error); reads from it return end-of-file immediately (zero bytes). This is the literal destination behind `command > /dev/null` (throw output away) and the literal source behind `command < /dev/null` (give a background command an input that's instantly exhausted, so it never blocks waiting for someone to type something).
- **`/dev/zero`** — reads return an endless stream of zero bytes. Used for zeroing out a region of a file or disk, and as a source for anonymous zeroed memory via `mmap` (file 21).

---

## 7. Common misconceptions

| Misconception | Reality |
|---|---|
| "A file descriptor *is* the file." | It's an index pointing, via an open file description, at an inode — three separate layers, not one. |
| "Two processes both having 'fd 3' open means they're looking at the same thing." | fd numbers are per-process and carry no meaning across process boundaries without knowing which table you mean. |
| "If two processes have the same file open, they always share a read position." | Only if they share the same *open file description* (via `fork`/`dup`) — two independent `open()` calls on the same path get two independent offsets. |
| "`dup2()` preserves the original fd's close-on-exec setting." | It always clears it on the duplicate; only `dup3()` with `O_CLOEXEC` can set it explicitly. |

---

## 8. Try this

> **Try this**
> ```
> ls -l /proc/$$/fd
> ```
> `$$` is your shell's own PID, expanded by the shell itself before anything runs (file 04) — so this shows every fd your *current shell process* has open right now, each a symlink to what it actually points at. Expect to see fd 0/1/2 pointing at your PTY slave. (`/proc/self` would instead show *this `ls` process's own* fds, not your shell's — file 33 explains precisely why.) Full `/proc` exploration: file 33.

> **Try this**
> Run `cat somefile.txt` in one terminal, and while it's still resolvable (or use `sleep 100 < somefile.txt` to keep it open longer), inspect it from another:
> ```
> ls -l /proc/<pid>/fd
> ```
> Fds 0/1/2 point at a terminal device; a higher-numbered fd (typically 3) points at the actual file — a direct, observable instance of Scenario A/B above.

---

## 9. macOS note

Darwin's VFS layer (BSD-derived) uses the identical three-layer model — fd table, open file description (there called, in BSD terminology, a "file table entry" or "file glob," depending on which layer of documentation you're reading, but conceptually the same object), and vnode/inode underneath. Everything in this file transfers directly; only some tool-level details (`/proc` doesn't exist the same way on macOS — file 33 covers the difference) change.

---

## 10. Summary

- A file descriptor is a per-process integer index — meaningless in isolation, outside its own process's table.
- It points at an **open file description** — a kernel object holding the current offset and status flags — which can be shared across multiple fds, even across processes, precisely when `fork()` or `dup`/`dup2`/`dup3` created that sharing.
- The open file description points at an **inode** — the actual file — and many independent open file descriptions (each with their own separate offset) can point at the same one.
- `dup2()` is the exact mechanism behind redirection and pipes: point a well-known fd number (0, 1, 2) at a different open file description without the program being redirected ever needing to know.
- `/dev/null` and `/dev/zero` are real inodes backed by kernel driver code rather than disk blocks — ordinary fd mechanics apply to them all the same way.

---

## What's next

`11_STDIN_STDOUT_STDERR.md` takes fd 0/1/2 specifically and goes deep on why stderr exists as a genuinely separate channel, and exactly what `2>&1` is doing in terms of the machinery this file just established.