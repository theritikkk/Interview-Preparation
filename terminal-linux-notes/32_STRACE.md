# 32 — strace Deep Dive

> **Where this fits in the stack**
> ```
> You  →  [ strace ]  →  every syscall a traced process makes  →  the conceptual model, verified
>          ^^^^^^^^^^
>          this file
> ```
> This is the payoff file. Everything conceptual across files 07-13 gets made **literally, empirically observable** here — culminating in `strace -f` on a full pipeline, which is the file 13 wiring dance, verified line by line rather than taken on faith.

---

## 1. How `strace` actually works

`strace` uses **`ptrace()`** — itself a syscall, whose entire job is letting one process observe and control another. It intercepts every syscall entry and exit of the traced process, **pausing it at each kernel-boundary crossing** (file 09) to print what's happening before letting it continue.

> **Remember**
> This is exactly why `strace` carries real, measurable overhead (file 31's `perf` comparison) — it's pausing the target program at *every single syscall*. It's a correctness/behavior tool, not a performance-measurement one; for that, reach for `perf` instead.

---

## 2. `strace ls` — annotated

A representative trace (exact output varies by system, but the shape and order are accurate):

```
execve("/usr/bin/ls", ["ls"], 0x7ffde...) = 0
```
`execve` — file 08, in full. This is where the new process image begins.

```
mmap(NULL, ..., PROT_READ, MAP_PRIVATE, 3, 0) = 0x7f...
```
Early `mmap` calls — file 08's dynamic linking section: `ld.so` mapping in shared libraries (`libc.so.6`, and others) *before* `ls`'s own `main()` ever runs.

```
mprotect(0x7f..., ..., PROT_READ) = 0
```
`mprotect` changes the protection flags of an already-`mmap`'d region — commonly seen right after loading a shared library, marking code pages read-and-execute-only once any relocation-fixup writes `ld.so` needed have been applied. A security hardening measure: making code regions non-writable (file 21) blocks certain classes of memory-corruption exploits from overwriting executable code.

```
openat(AT_FDCWD, "/tmp", O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_DIRECTORY) = 3
```
`openat` — files 07 and 19's trace, now literal: the target directory, opened.

```
getdents64(3, ..., 32768) = 176
```
`getdents64` — file 19: reading the directory's actual `(name, inode)` entries.

```
close(3) = 0
```
`close` — file 10: releasing that fd once it's no longer needed.

```
write(1, "file1.txt  file2.txt\n", 21) = 21
```
`write` — files 09, 11: the actual formatted output, landing on fd 1.

(Trailing `munmap` calls near the end are cleanup — not strictly necessary since the process is about to exit anyway, but some libraries clean up their own mappings regardless.)

---

## 3. `strace cat file.txt` — the read loop, precisely

```
openat(AT_FDCWD, "file.txt", O_RDONLY) = 3
fstat(3, {st_mode=S_IFREG|0644, st_size=1234, ...}) = 0
read(3, "...", 131072) = 1234
write(1, "...", 1234) = 1234
read(3, "", 131072) = 0
close(3) = 0
```

`fstat` — file 19's `stat`/`lstat`/`fstat` trio: `cat` checks the file's size and type first, partly to choose an efficient read buffer size, partly to confirm it's a regular file rather than something unexpected.

> **Remember**
> The read loop itself is the same unifying mechanism from files 10, 13, and 23, paying off yet again: `cat` keeps reading (and writing what it got) until `read()` returns **0** — end of file. The exact same "successful read of zero bytes signals EOF" mechanism as a closed pipe's read end (file 13) or `Ctrl+D` on an empty line (file 23), here shown for an ordinary file instead.

---

## 4. `strace -f` on `cat file.txt | grep hello` — the full payoff

`-f` is required specifically to **follow child processes** — the shell forks two children, and plain `strace` on the shell itself only traces the one process it directly launched, showing nothing of what either child actually does. With `-f`, every line is prefixed with the PID it belongs to.

```
[pid 1234] pipe([3, 4])                     = 0      <- SHELL creates the pipe (file 13)
[pid 1234] fork()                           = 1235   <- first child: cat
[pid 1235] dup2(4, 1)                       = 1      <- cat: stdout -> pipe write end
[pid 1235] close(3)                         = 0      <- cat: close unused read end
[pid 1235] close(4)                         = 0      <- cat: close original fd 4 (now dup'd to 1)
[pid 1235] execve("/bin/cat", ...)          = 0
[pid 1234] fork()                           = 1236   <- second child: grep
[pid 1236] dup2(3, 0)                       = 0      <- grep: stdin -> pipe read end
[pid 1236] close(3)                         = 0
[pid 1236] close(4)                         = 0      <- grep: close unused write end
[pid 1236] execve("/bin/grep", ...)         = 0
[pid 1234] close(3)                         = 0      <- SHELL closes its own originals
[pid 1234] close(4)                         = 0
[pid 1235] read(...) ...  write(1, ...) ...          <- cat reading the file, writing to the pipe
[pid 1236] read(0, ...) ...  write(1, ...) ...       <- grep reading the pipe, writing matches to its OWN stdout (still the terminal)
```

> **Interview answer**
> "This trace is file 13's fork/dup2/close wiring dance, made literally observable — every line maps directly onto a specific step: the shell's `pipe()` call, each child's `dup2` repointing its stdin or stdout, every close of an fd that isn't needed, and finally the shell closing its own original copies. Nothing here is inferred or assumed — it's the actual sequence of syscalls, in order, with PIDs attached."

This is the entire handbook's "here's the conceptual model, now here's how to verify it yourself" promise, delivered as literally as it gets.

---

> **What you will actually see — a correction to the trace above**
> The trace is idealised. On a real Linux system, `fork()` appears as **`clone(...)`** (glibc implements `fork` with `clone`, on x86-64 and arm64 alike); on **arm64** — for example a Linux VM on an Apple Silicon Mac — `dup2(4, 1)` appears as **`dup3(4, 1, 0)`** and `pipe([3, 4])` as **`pipe2([3, 4], 0)`**, because that architecture's syscall table has no `dup2` or `pipe`. The *structure* is unchanged: the pipe is created, each child repoints its stdin or stdout and closes the unused ends, then `execve`s. I could not run `strace` in my sandbox, so this correction comes from the syscall tables and glibc's behaviour, not from a captured trace. Full architecture notes: file 49.

---


## 5. Common misconceptions

| Misconception | Reality |
|---|---|
| "`strace` shows library-level calls too." | Syscalls only — `ltrace` (file 31) is the separate, complementary library-level tool. |
| "`strace` has no effect on the traced program's behavior or timing." | Real, measurable overhead from pausing at every syscall — can genuinely mask or change timing-sensitive bugs, a real "observer effect." |

---

## 6. Try this

> **Try this**
> Run all three of this file's examples yourself — `strace ls`, `strace cat file.txt`, `strace -f bash -c 'cat file.txt | grep hello'` — and compare your actual output line by line against this file's annotated version.

> **Try this**
> ```
> strace -T ls        # show time spent in each individual syscall
> strace -c ls        # summary mode, recap file 09's preview
> ```

---

## 7. Summary

- `strace` works via `ptrace()`, pausing the traced process at every syscall boundary — real overhead, a genuine tool for behavior, not performance.
- `execve`/`mmap`/`mprotect` at the start of any trace are file 08's loader and dynamic-linking sequence, made literal.
- `cat`'s read loop terminates on `read()` returning 0 — the same EOF mechanism as a pipe or `Ctrl+D`, now shown for an ordinary file.
- `strace -f` on a pipeline is file 13's entire wiring dance, verifiable line by line rather than taken on faith.

---

## What's next

`33_PROC_FILESYSTEM.md` covers the other major observation tool this handbook keeps previewing — `/proc` — which complements `strace`'s syscall-by-syscall view with a direct window into a process's *current state* at any moment, no tracing required.
