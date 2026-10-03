# Pipes & Process-Control Signals

> **Phase 9 · UNIX/POSIX System Interface.** Depends on: `05_processes_fork_exec_wait.md`
> (`fork`, `exec`, `wait` — pipes exist to connect the processes those calls create),
> `04_system_calls_vs_libc_vs_posix.md` §2 (`dup2` — the glue between a pipe and a
> program's standard streams), `libraries/07_setjmp_and_signal.md` §3 (`<signal.h>` as the
> *ISO C* view of signals — this file is the POSIX view).
> Feeds into: `07_sockets_and_networking.md` (this phase — sockets are, in effect, pipes
> that can span machines), `projects/03_systems_projects.md`.
> Sources: **entirely `[External]`** — none of the three books covers pipes or POSIX signal
> handling. Pipe semantics below were checked directly against the Linux `pipe(7)` manual
> page (man7.org); signal semantics against POSIX and man7.org.

---

## 1. Pipes: a kernel-buffered, one-way byte channel

`[External]`

```c
#include <unistd.h>
int pipe(int fd[2]);      /* on success: fd[0] = READ end, fd[1] = WRITE end */
```

A pipe is a **unidirectional** channel: bytes written to `fd[1]` come out of `fd[0]`, in
order. Both ends are ordinary **file descriptors** (`systems/03_file_descriptors_and_unix_io.md`
§2) — you use plain `read`/`write`/`close` on them, no special API. The data lives in a
**kernel-managed buffer**, not in either process's memory — which is precisely why a pipe
can carry data between two processes whose memory (`05_processes_fork_exec_wait.md` §2) is
otherwise completely separate. `[External, checked against `pipe(7)`]` On modern Linux that
buffer holds 65,536 bytes by default (16 pages), but the manual is explicit that
**applications should not rely on any particular capacity** — design the reader to consume
promptly instead.

### The properties that matter, all from `pipe(7)`

| Property | Behavior |
|---|---|
| **Byte stream** | No message boundaries. Two 10-byte `write`s may arrive as one 20-byte `read`, or a 20-byte write as two reads. |
| **Read on empty pipe** | Blocks until data arrives — **unless** every write end is closed, in which case `read` returns `0` (end-of-file). |
| **Write to a full pipe** | Blocks until the reader makes room. |
| **Write when every read end is closed** | The kernel sends the writer a **`SIGPIPE`** signal (default action: terminate); if `SIGPIPE` is ignored, `write` fails with `EPIPE` instead. |
| **`lseek`** | Not possible — a pipe has no position to seek. |
| **Atomicity** | POSIX guarantees `write`s of **at most `PIPE_BUF` bytes** are atomic (never interleaved with another writer's data). POSIX requires `PIPE_BUF` ≥ 512; on Linux it is 4096. Larger writes may interleave. |

### 🧠 Mental model: a pipe is a `FILE*`-free file descriptor pair
`[Synthesis]` Everything from Phase 8 carries over unchanged: a pipe end is a small integer,
`read`/`write`/`close` work on it, and `fork` duplicates it (both processes now hold both
ends, sharing the same underlying pipe — the same shared-open-file behavior as `dup`). The
only new idea is that the two descriptors point at the *two ends of one buffer* instead of
at a disk file.

---

## 2. Using a pipe across `fork` — the canonical pattern

`[External]` A pipe is created **before** `fork`, so both processes inherit both ends;
each process then **closes the ends it doesn't use**:

```c
int fd[2];
pipe(fd);
pid_t pid = fork();
if (pid == 0) {           /* child: will READ */
    close(fd[1]);          /* close unused write end */
    char buf[100];
    ssize_t n = read(fd[0], buf, sizeof buf);
    /* ... use buf[0..n) ... */
    close(fd[0]);
    _exit(0);
} else {                   /* parent: will WRITE */
    close(fd[0]);          /* close unused read end */
    write(fd[1], "hello", 5);
    close(fd[1]);          /* <-- lets the child's read eventually see EOF */
    waitpid(pid, NULL, 0);
}
```

### ⚠️ The #1 pipe bug: forgetting to close the write end
`[External, straight from the `pipe(7)` text quoted above]` A reader only sees end-of-file
when **every** descriptor referring to the write end is closed — *including the reader's
own inherited copy*. If the reading process never closes its copy of `fd[1]`, `read` on an
empty pipe **blocks forever**, waiting for data that no one is ever going to write (because
the only remaining writer is the reader itself). The manual says it directly: applications
using `pipe` and `fork` "should use suitable `close` calls to close unnecessary duplicate
file descriptors; this ensures that end-of-file and `SIGPIPE`/`EPIPE` are delivered when
appropriate." This is the pipe equivalent of a missing `waitpid`: nothing crashes, the
program simply hangs, which makes it disproportionately painful to debug
(`debugging/04_debugging_workflows_labs.md`, Phase 10, treats "it hangs" as its own symptom
class).

---

## 3. Building a shell pipeline: `ls | wc -l`

`[External, synthesizing `dup2`, `fork`, `exec`, and `pipe` — this is the phase's second
stated project]` This is where every earlier piece of Phase 8–9 finally combines:

```
pipe(fd)                          # one pipe for the two commands
fork() → child 1  (runs "ls"):
    dup2(fd[1], 1)                # its stdout now goes INTO the pipe
    close(fd[0]); close(fd[1])     # close BOTH originals (dup2 made a copy at fd 1)
    execvp("ls", ...)
fork() → child 2  (runs "wc -l"):
    dup2(fd[0], 0)                # its stdin now comes FROM the pipe
    close(fd[0]); close(fd[1])
    execvp("wc", ...)
parent:
    close(fd[0]); close(fd[1])     # parent must close both too — or wc never sees EOF
    waitpid(child1); waitpid(child2)
```

**Why it works:** neither `ls` nor `wc` knows a pipe is involved — `ls` writes to fd 1 as it
always does, `wc` reads fd 0 as it always does. The shell rewired those descriptors
(`dup2`) *between* `fork` and `exec` — exactly the flexibility `05_processes_fork_exec_wait.md`
§4 identified as the reason `fork` and `exec` are separate calls. `wc` finishes only after
`ls` exits and **all** write ends (the parent's included) are closed, delivering EOF.

### 🎯 Interview point
"Why must the parent shell close its own copies of the pipe's file descriptors?" — Because
EOF is delivered only when *every* write-end descriptor system-wide is closed. If the
parent keeps `fd[1]` open, `wc` will wait forever for more input even after `ls` has
exited. This single question separates people who've built a pipeline from people who've
only read about one.

---

## 4. Signals as process control (POSIX view)

`[Cross-reference]` `libraries/07_setjmp_and_signal.md` §3 covered `<signal.h>` from
**ISO C's** view: a deliberately weak, portable subset (`signal`, `raise`,
`volatile sig_atomic_t`, six signal names). POSIX adds the real process-control machinery:

```c
#include <signal.h>
int kill(pid_t pid, int sig);                            /* send a signal to a process */
int sigaction(int sig, const struct sigaction *act,
              struct sigaction *old);                     /* the reliable way to install a handler */
int sigprocmask(int how, const sigset_t *set, sigset_t *old);  /* block/unblock signals */
```

| Signal | Typical role | Catchable? |
|---|---|---|
| `SIGINT` | Ctrl-C from the terminal | Yes |
| `SIGTERM` | Polite "please terminate" (what `kill pid` sends by default) | Yes |
| `SIGKILL` | Unconditional kill | **No** — cannot be caught, blocked, or ignored |
| `SIGSTOP` | Unconditional pause | **No** |
| `SIGCHLD` | A child process terminated or stopped — sent to its parent | Yes (default: ignored) |
| `SIGPIPE` | Write to a pipe/socket with no readers (§1) | Yes |
| `SIGSEGV` | Invalid memory access | Yes, but see `libraries/07_setjmp_and_signal.md` §3 — must not return |

### `sigaction` over `signal` — why the upgrade exists
`[External, connecting directly to Book C's account in `libraries/07_setjmp_and_signal.md` §3]`
Book C described the historical UNIX flaw: handling reverts to default *before* the handler
runs, forcing a racy re-arm inside the handler. `signal()`'s behavior varied across UNIX
lineages precisely because of this history. `sigaction` is POSIX's fix: it gives
**well-defined, portable semantics** (the handler stays installed; you choose whether the
signal is blocked during its own handler; you choose whether interrupted system calls
restart). `[Synthesis]` Rule of thumb for real POSIX code: use `sigaction`, keep `signal`
for ISO-C-only portability.

### `SIGCHLD` — the tie-back to zombies
`[External, closing the loop with `05_processes_fork_exec_wait.md` §6]` A server that
`fork`s many children can avoid zombies without blocking in `wait`: install a `SIGCHLD`
handler that reaps finished children in a non-blocking loop:

```c
static void reap(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;                    /* reap EVERY finished child — one SIGCHLD may cover several */
}
```

The **loop** matters: signals of the same kind are not queued, so if three children exit
close together, you may receive just one `SIGCHLD` — a single `waitpid` would reap one child
and strand two zombies. `[Book-derived, C, Phase 7]` This is exactly the "signals can be
coalesced/lost" property Plauger described, appearing here as a concrete, common bug.

### What a handler may safely do — the POSIX extension of ISO C's short list
`[External]` ISO C (`libraries/07_setjmp_and_signal.md` §3) allows almost nothing inside an
async handler. POSIX widens this to a documented list of **async-signal-safe** functions —
notably including `write`, `_exit`, `waitpid`, and `kill`, but **excluding** `printf`,
`malloc`, and most of `<stdio.h>`. Full list: `signal-safety(7)`. `[Synthesis]` The
underlying reason is the same as in Phase 7: a signal can interrupt the program *anywhere*,
including in the middle of `malloc`'s free-list update
(`memory/05_dynamic_memory.md` §5) or `printf`'s buffer manipulation
(`libraries/01_stdio.md` §2) — calling the same non-reentrant function from the handler
re-enters half-updated internal state.

### Two more facts that interact with Phase 9's other files
- **`EINTR`:** a signal arriving while a process is blocked in a "slow" system call
  (`read` on a pipe, `waitpid`) can make the call return `-1` with `errno == EINTR`, unless
  the handler was installed with `SA_RESTART`. Robust code either restarts or loops.
- **Across `fork` and `exec`:** a child inherits its parent's signal dispositions across
  `fork`. Across `exec`, handlers are reset to default (the old handler's code no longer
  exists in the new program image) — but signals set to *ignored* stay ignored.

---

## 5. ⚠️ Misconceptions to unlearn now

- **"A pipe preserves message boundaries."** It's a byte stream — two writes can merge into
  one read and vice versa (§1). Protocols on top must add their own framing.
- **"If the writer exits, the reader always sees EOF."** Only if *every* write-end
  descriptor is closed everywhere — including inherited copies in the reader itself or in
  the shell (§2, §3).
- **"`kill` always kills."** It *sends a signal*; `SIGTERM` can be caught or ignored. Only
  `SIGKILL`/`SIGSTOP` are uncatchable (§4).
- **"One `SIGCHLD` means one child exited."** Standard signals are not queued; one delivery
  can represent several exits, hence the `waitpid(-1, …, WNOHANG)` loop (§4).

---

## 6. 🎯 Interview questions

**Easy**
- What do `fd[0]` and `fd[1]` mean after `pipe(fd)`? → Read end and write end (§1).
- Can a pipe carry data in both directions? → No — unidirectional; use two pipes for
  bidirectional communication (§1).

**Medium**
- What does `read` return on a pipe whose write ends are all closed? → `0` (EOF) (§1).
- What happens if you `write` to a pipe with no remaining readers? → `SIGPIPE` is
  delivered (default: terminate); if ignored, `write` fails with `EPIPE` (§1).
- Why can't you `lseek` on a pipe? → It's a stream through a kernel buffer, with no
  positioned storage behind it (§1).

**Hard**
- A `fork`-based pipeline hangs after the writer finishes. Diagnose. → Almost certainly an
  unclosed write-end descriptor somewhere (often the reader's own inherited copy, or the
  parent's); EOF requires *all* copies closed (§2).
- Why does a `SIGCHLD` handler need to loop over `waitpid(-1, …, WNOHANG)` rather than call
  it once? → Standard signals aren't queued; multiple near-simultaneous child exits can
  collapse into a single signal delivery, so one `waitpid` would leave zombies (§4).
- Why is `printf` unsafe inside a signal handler while `write` is fine? → `printf` uses
  non-reentrant `stdio` buffer state that the interrupted code may be mid-update on;
  `write` is a thin syscall wrapper on the POSIX async-signal-safe list (§4).

**Systems-level**
- Walk through, at the file-descriptor level, exactly how `ls | wc -l` is set up by a
  shell. → One `pipe`; `fork` child 1, `dup2(fd[1], 1)`, close both originals, `exec ls`;
  `fork` child 2, `dup2(fd[0], 0)`, close both originals, `exec wc`; parent closes both
  ends and `waitpid`s both — each command is oblivious to the pipe because the shell
  rewired stdout/stdin *between* `fork` and `exec` (§3).
- A worker process is killed by `SIGPIPE` when its consumer crashes, with no log message.
  How would you make it degrade gracefully instead? → Ignore `SIGPIPE`
  (`signal(SIGPIPE, SIG_IGN)` or via `sigaction`) so `write` returns `-1`/`EPIPE`, then
  handle the error path explicitly (log, clean up, exit deliberately) (§1, §4).

---

## 7. External references

- [man7.org — `pipe(7)`](https://www.man7.org/linux/man-pages/man7/pipe.7.html)
  `[External]` — the source this file's pipe semantics were checked against: capacity,
  `PIPE_BUF` atomicity, EOF and `SIGPIPE`/`EPIPE` rules.
- [man7.org — `pipe(2)`](https://man7.org/linux/man-pages/) `[External]` — the system call
  itself, with a complete `fork`-plus-pipe example program.
- [man7.org — `sigaction(2)`](https://man7.org/linux/man-pages/) `[External]` — full
  flags (`SA_RESTART`, `SA_NOCLDSTOP`, …) and semantics.
- [man7.org — `signal-safety(7)`](https://man7.org/linux/man-pages/) `[External]` — the
  authoritative async-signal-safe function list.
- [The Open Group Base Specifications, Issue 8](https://pubs.opengroup.org/onlinepubs/9799919799/)
  `[External]` — the formal POSIX text for `pipe`, `kill`, `sigaction`, `sigprocmask`.

## 8. 🛠 Practice — completing the phase projects

1. Build the `parent writes, child reads` pipe program from §2; then deliberately *omit* the
   child's `close(fd[1])` and observe the hang — then use `ps`/`strace` (previewed;
   `debugging/`, Phase 10) to see the child stuck in `read`.
2. Implement `ls | wc -l` from §3 in C, end to end, using `pipe`/`fork`/`dup2`/`execvp`.
   Then generalize to a shell that parses `cmd1 | cmd2` from a line of input, extending the
   minimal shell from `05_processes_fork_exec_wait.md` §7.
3. Write a server-style loop that `fork`s a child per "request" and reaps children only
   via a `SIGCHLD` handler using the `waitpid(-1, …, WNOHANG)` loop; fire many short-lived
   children at once and confirm (with `ps`) that no zombies accumulate.
4. Install a `SIGINT` handler with `sigaction` that sets a `volatile sig_atomic_t` flag
   (the safe pattern from `libraries/07_setjmp_and_signal.md` §3) and shuts your shell
   down cleanly at the next prompt instead of dying mid-command.
