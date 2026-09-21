# 13 — Pipes

> **Where this fits in the stack**
> ```
> User  →  Terminal Emulator  →  PTY/TTY  →  Shell  →  fork/exec  →  [ PIPES ]  →  Programs
>                                                                     ^^^^^^^^
>                                                                     this file
> ```
> File 12 showed process substitution leaning on a pipe without explaining one. This file is the full mechanism: the `pipe()` syscall, the exact `fork`+`dup2`+`close` choreography that wires two (or more) processes together, and — the genuine payoff — precisely why forgetting to close one unused file descriptor can make a program hang forever.

---

## 1. What `|` actually is

`cat file.txt | grep hello` connects `cat`'s stdout to `grep`'s stdin, through a kernel-managed, in-memory, bounded byte queue — **not** a file, and not a temporary anything on disk.

```c
int fds[2];
pipe(fds);
// fds[0] = the READ end
// fds[1] = the WRITE end
```

> **Remember**
> `fds[0]` reads, `fds[1]` writes. Easy to transpose from memory — get it backwards and everything downstream silently wires the wrong direction.

Anything written to `fds[1]` becomes readable, in order, from `fds[0]` — a first-in-first-out queue living entirely in kernel memory.

---

## 2. The wiring dance for `cat file.txt | grep hello`, step by step

```
Shell
 |
 +--------+
 |        |
cat      grep
 |        |
 +--> pipe ->+
```

1. **Shell calls `pipe()` once**, obtaining `fds[0]` (read) and `fds[1]` (write) — at this instant, both ends exist only in the **shell's own process**.
2. **Shell `fork()`s** — the first child (destined to become `cat`) inherits *copies* of both pipe fds (file 08's fd-sharing-after-fork mechanics: both fd-table entries point at the same underlying pipe object).
3. **In that first child:** `dup2(fds[1], STDOUT_FILENO)` — its stdout now points at the pipe's write end. Then it `close()`s **both** of its own original `fds[0]` and `fds[1]` numbers — no longer needed once `dup2` made the copy at fd 1 (why this matters: Section 5). Then `execve("cat", ...)`.
4. **Shell `fork()`s again** — the second child (destined to become `grep`) *also* inherits copies of both original pipe fds, because the shell — the process this fork happened from — still has them open at this point.
5. **In that second child:** `dup2(fds[0], STDIN_FILENO)` — its stdin now points at the pipe's read end. It closes its own copies of both original fds. Then `execve("grep", ["grep", "hello"])`.
6. **Back in the shell (the parent):** it closes its own original `fds[0]` and `fds[1]` too — it doesn't need them once both children have their own `dup2`'d copies — and calls `waitpid()` for both children.

---

## 3. Why both programs genuinely run concurrently

This falls directly out of file 08's model: the shell created **two separate processes** via two separate `fork()` calls, and the kernel's scheduler (file 22) runs them independently and interleaved, with no rule that one must finish before the other starts. For nontrivial input, they run as a genuine producer/consumer pair in real time: `cat` writes some bytes, fills the pipe's buffer, blocks once it's full (Section 5) — meanwhile `grep` is *already* reading and processing what's been written so far, draining buffer space, which unblocks `cat` to continue.

> **Common mistake**
> "`cat` runs to completion, *then* `grep` runs on the finished result." That's how `$(...)` command substitution behaves (files 04, 06) — the subshell runs fully to completion before its captured output is used at all. A pipe is the opposite: genuinely streaming, with both sides alive and making progress simultaneously.

---

## 4. Pipe buffering

Each pipe gets a fixed-size kernel-managed buffer — commonly **64 KB** by default on modern Linux (tunable via `fcntl(fd, F_SETPIPE_SZ, size)`), a real, memory-backed circular buffer, not a promise or an abstraction.

> **Common mistake**
> "The pipe buffer size and `PIPE_BUF` are the same thing." `PIPE_BUF` (512 bytes, POSIX-guaranteed) is the maximum write size the kernel promises to perform **atomically** — without interleaving bytes from a concurrent writer. The actual buffer *capacity* (64 KB, by default) is a separate, much larger number controlling how much unread data can accumulate before a writer blocks.

This buffer is exactly what lets a fast producer and a slow consumer (or vice versa) coexist without either data loss or lockstep waiting on every single byte — the buffer absorbs short-term speed mismatches.

---

## 5. Blocking behavior

- **`write()` to a full pipe** blocks — the writing process sleeps (state `S`/`D`, file 08) until the reader drains enough space to fit at least part of the write.
- **`read()` from an empty pipe, with the write end still open somewhere**, blocks until either more data arrives or every write end is closed.
- **`read()` from an empty pipe where every write end has been closed** returns **0 immediately.** This is EOF — and it's not a special "pipe closed" signal, it's the exact same mechanism as reaching the end of an ordinary file: `read()` succeeding with zero bytes transferred.

---

## 6. Why closing unused pipe fds matters — the critical insight

**A pipe's read end only sees EOF once *every* process holding any copy of the write end has closed it.** If even one stray, forgotten copy of the write end remains open *anywhere* — even in a process that isn't actually writing to it, like the shell forgetting to close its own original copies after handing `dup2`'d copies to both children — the reader will **never see EOF**. It will block forever, waiting for data that's never coming, because the kernel has no way to know the "real" writer is finished as long as *some* process, somewhere, still technically holds that write end open and could theoretically write to it.

This is precisely why every `close()` call in Section 2's dance matters, not just the ones that seem obviously necessary. Skipping any of them — the shell forgetting its own originals, or either child forgetting the copy it didn't `dup2` over — is a real, common bug whose symptom is exactly: *"my program hangs forever after a fork/pipe setup, for no visible reason."* File 37 revisits this precise failure mode as a debugging scenario.

> **Interview answer**
> "A pipe's reader sees EOF only when every writable file descriptor referring to it, across every process, has been closed. Forgetting to close an unused copy — commonly, the parent's own original pipe fds after handing duplicates to its children — leaves the kernel unable to conclude writing is finished, and the reader blocks indefinitely. This is a correctness bug, not just a resource leak."

---

## 7. The full pipeline: `cat a.txt | grep hello | sort | uniq -c`

Generalizing Section 2 to `k` commands needs `k - 1` pipes and `k` child processes:

```
Shell
 |
 +--> cat
 |      |
 |      v
 |    pipe1
 |      |
 +--> grep
        |
        v
      pipe2
        |
      sort
        |
      pipe3
        |
      uniq
```

| Process | stdin | stdout |
|---|---|---|
| `cat` | (its file argument) | pipe1's write end |
| `grep` | pipe1's read end | pipe2's write end |
| `sort` | pipe2's read end | pipe3's write end |
| `uniq -c` | pipe3's read end | **unchanged** — the original terminal, since it's last in the pipeline |

**The real subtlety in a multi-pipe pipeline:** each child must close not only the pipe fds it isn't using from *its own* pair, but **every fd from every other pipe** in the pipeline that it may have inherited along the way. In a naive implementation that forks children sequentially without careful cleanup between each fork, `sort`'s child could end up holding an inherited, unused copy of `pipe1`'s fds too — and per Section 6, any such straggler is a genuine correctness risk, not just untidiness. This is exactly why real shell implementations are meticulous about closing *every* pipe fd not immediately needed, in *every* child, right after each fork — and it's precisely what the mini-shell project (file 34) will have you get right by hand.

The shell itself, once all four children are launched, closes every pipe fd it was still holding and `waitpid()`s for all four.

---

## 8. Common misconceptions

| Misconception | Reality |
|---|---|
| "`cat` finishes, then `grep` starts." | Both run concurrently, streaming — contrast with `$(...)`'s run-to-completion-then-capture behavior. |
| "A pipe is basically a temporary file." | An in-memory kernel object with genuine blocking semantics and a bounded buffer — no disk involved anywhere. |
| "EOF on a pipe is some special closed-pipe error." | It's an ordinary `read()` returning 0 — the same mechanism as end-of-file on a real file. |
| "Forgetting to close an unused pipe fd is just a minor leak." | It can make the reader hang **forever**, waiting for an EOF that structurally cannot arrive while that fd stays open. |

---

## 9. Try this

> **Try this**
> ```
> yes | head -5
> ```
> `yes` produces output forever; `head` only wants 5 lines and then exits, closing its read end. `yes`'s next `write()` now fails — its reader is gone — and the kernel delivers it `SIGPIPE`, terminating it. A direct, observable preview of file 14's signal coverage, and proof that both processes really were running concurrently the whole time.

---

## 10. Summary

- `pipe()` creates a kernel-managed, in-memory FIFO with a read end and a write end — `fds[0]` reads, `fds[1]` writes.
- Wiring a pipeline is `fork` + `dup2` + `close`, repeated per command, with **every** unused pipe fd closed in **every** process — the parent included.
- Both sides of a pipe genuinely run concurrently, streaming — not "producer finishes, then consumer runs," which is what command substitution does instead.
- EOF on a pipe's read end requires *every* copy of the write end, everywhere, to be closed — one forgotten copy anywhere causes the reader to hang forever, a correctness bug, not a cosmetic one.

---

## What's next

`14_SIGNALS.md` picks up exactly where this file's `SIGPIPE` preview left off: the full signal set, precisely what happens when you press `Ctrl+C`, and why `SIGKILL` can never be caught.
