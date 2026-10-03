# Interview Questions — UNIX/POSIX System Interface

> Consolidated from `systems/04_…` → `07_…`. **Phase 9 is now complete**: layers,
> processes, pipes/signals, sockets.

---

## Easy

1. **Is `fork` part of the ISO C standard?** No — it's a POSIX function.
   (`04_system_calls_vs_libc_vs_posix.md` §1)
2. **What does `dup2(a, b)` do, at a high level?** Makes file descriptor `b` refer to the
   same open file as `a`. (§2)
3. **What does `fork()` return in the parent versus the child?** The child's PID in the
   parent; `0` in the child. (`05_processes_fork_exec_wait.md` §2)
4. **Does `exec` create a new process?** No — it replaces the current process's program;
   `fork` creates new processes. (§3)

## Medium

1. **Why can ISO-C-only code run natively on Windows while code calling `fork` cannot?**
   ISO C's requirements are platform-agnostic; `fork` is POSIX-specific, and Windows isn't
   natively POSIX-conformant. (`04_system_calls_vs_libc_vs_posix.md` §1)
2. **How does a shell implement `prog > output.txt` in terms of file descriptors?** Opens
   the file, then `dup2`s fd 1 to refer to it, before calling `exec`. (§2)
3. **Why does the classic shell pattern use `fork` then `exec` in the child, rather than
   one combined call?** Everything the child does between the two calls (redirecting fds,
   changing directory) still applies once `exec` loads the new program, with no special
   support needed from it. (`05_processes_fork_exec_wait.md` §4)
4. **What's a zombie process, precisely?** A child that has exited but whose exit status
   hasn't yet been collected by its parent via `wait`/`waitpid`. (§6)

## Hard

1. **Explain why "this function lives in `libc.so`" isn't a reliable test for "this is
   part of the C standard."** glibc implements POSIX's `fork` in the same shared library
   as ISO C's `printf`; some POSIX functionality has even moved between separate and
   merged libraries over time — packaging is a distribution decision, not a standards
   classification. (`04_system_calls_vs_libc_vs_posix.md` §1)
2. **Why does `dup2` atomically close its target descriptor rather than requiring a
   separate `close` first?** A manual close-then-dup sequence has a race window where the
   descriptor number is free and could be reused by unrelated code before the intended dup
   claims it. (§2)
3. **Explain why global variable changes before `fork` are visible to the child, but
   changes after are not.** Before `fork` there's one process and one memory copy; `fork`
   copies memory at that instant, and the two processes have entirely separate physical
   memory (via copy-on-write) from then on. (`05_processes_fork_exec_wait.md` §2)
4. **Why does re-parenting orphans to `init`/a subreaper matter, rather than leaving them
   parentless?** A parentless process that exits would have no one able to `wait` for it,
   becoming a permanent, unreapable zombie. (§6)

## Systems-level

1. **A program fails to compile on Windows because it calls `fork`. Where's the
   portability boundary, and what's the fix?** The POSIX layer specifically; fix via a
   POSIX compatibility layer (WSL, Cygwin) or a Windows-native process-creation API
   entirely. (`04_system_calls_vs_libc_vs_posix.md` §1)
2. **A long-running server `fork`s a child per request but never calls `wait`/`waitpid`.
   What happens over time?** Every finished child becomes a zombie; the process table
   accumulates these indefinitely and can eventually be exhausted, blocking new process
   creation entirely. (`05_processes_fork_exec_wait.md` §6)
3. **Trace what happens, system-call by system-call, when a minimal shell runs `ls -l`.**
   `fork()` creates a child; the child's `execvp("ls", ["ls","-l",NULL])` replaces its
   image, `ls` runs and exits; the parent's `waitpid` (blocked since the fork) returns,
   decoding the status via `WIFEXITED`/`WEXITSTATUS` before looping to prompt again. (§7)

## Easy (pipes, signals, sockets)

1. **What do `fd[0]` and `fd[1]` mean after `pipe(fd)`?** Read end and write end.
   (`06_pipes_and_signals.md` §1)
2. **Can a pipe carry data in both directions?** No — unidirectional; use two pipes.
   (§1)
3. **What does `accept` return?** A new file descriptor for the accepted connection.
   (`07_sockets_and_networking.md` §4)
4. **What does `recv` return when the peer has closed the connection?** `0`. (§6)

## Medium (pipes, signals, sockets)

1. **What happens if you `write` to a pipe with no remaining readers?** `SIGPIPE` is
   delivered (default: terminate); if ignored, `write` fails with `EPIPE`.
   (`06_pipes_and_signals.md` §1)
2. **Why does a `SIGCHLD` handler need to loop over `waitpid(-1, ..., WNOHANG)`?**
   Standard signals aren't queued; several child exits can collapse into one delivery, so
   a single `waitpid` would leave zombies. (§4)
3. **Why is `sigaction` preferred over `signal` in POSIX code?** Well-defined, portable
   semantics: the handler stays installed, and you control blocking and syscall restart;
   `signal`'s behavior varied historically. (§4)
4. **Why must you call `htons` on a port number?** The wire format is big-endian; on
   little-endian hosts the bytes must be swapped, and `htons` does that portably.
   (`07_sockets_and_networking.md` §3)
5. **Why might `send(fd, buf, 1000, 0)` return 600?** `send` can transfer fewer bytes than
   requested; the caller must loop. (§6)

## Hard (pipes, signals, sockets)

1. **A `fork`-based pipeline hangs after the writer finishes. Diagnose.** An unclosed
   write-end descriptor somewhere (often the reader's own inherited copy or the parent's)
   keeps EOF from being delivered. (`06_pipes_and_signals.md` §2)
2. **Why is `printf` unsafe inside a signal handler while `write` is fine?** `printf` uses
   non-reentrant `stdio` buffer state the interrupted code may be mid-update on; `write` is
   a thin syscall wrapper on the POSIX async-signal-safe list. (§4)
3. **Explain why `buffer[len] = '\0'` after an unchecked `recv` is undefined behavior.**
   On error `recv` returns `-1`, so it writes `buffer[-1]`, an out-of-bounds store.
   (`07_sockets_and_networking.md` §5)
4. **How would you design a message protocol over TCP, given no message boundaries?**
   Add framing — a length prefix (in network byte order) or a delimiter — and loop on
   `recv` until a whole frame has arrived. (§6)

## Systems-level (pipes, signals, sockets)

1. **Walk through, at the file-descriptor level, how a shell sets up `ls | wc -l`.** One
   `pipe`; `fork` child 1: `dup2(fd[1], 1)`, close both originals, `exec ls`; `fork` child
   2: `dup2(fd[0], 0)`, close both originals, `exec wc`; the parent closes both ends and
   `waitpid`s both. Each command is oblivious to the pipe. (`06_pipes_and_signals.md` §3)
2. **A worker is killed by `SIGPIPE` when its consumer crashes, with no log message. How to
   degrade gracefully?** Ignore `SIGPIPE` so `write` returns `-1`/`EPIPE`, then handle that
   error path explicitly. (§1, §4)
3. **Your `fork`-per-connection server slowly stops accepting clients after days, with no
   crash. What leaks are worth checking?** Un-reaped children (zombies), un-closed session
   descriptors in the parent after `fork`, and un-closed listening descriptors in children.
   (`07_sockets_and_networking.md` §4–5)
4. **A little-endian client reads a 4-byte length field from a socket and gets absurdly
   large values. What went wrong?** The field arrived in network (big-endian) order and
   was used as a host `int` without `ntohl`. (§3)
