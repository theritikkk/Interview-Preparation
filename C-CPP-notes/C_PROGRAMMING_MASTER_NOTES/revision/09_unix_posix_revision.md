# Revision — Phase 9: UNIX/POSIX System Interface

> Quick-reference companion to `systems/04_…` → `07_…`. **This is the brief's heaviest
> external-synthesis phase** (none of the three books cover process creation, pipes, or
> most of POSIX). **Phase 9 is now complete**: layers, processes, pipes/signals, sockets.

---

## C vs. libc vs. POSIX vs. Linux syscalls — 1-minute summary

Four distinct layers, four different authorities: the C language and C standard library
(ISO C, portable to any hosted implementation, including Windows) vs. POSIX (a separate
standard — `fork`, `pipe`, `dup2`, `read`/`write`/`open` — portable to POSIX-conformant
systems only, not native Windows) vs. raw Linux kernel syscalls (not standardized at all
outside Linux itself). "Lives in `libc.so`" is a packaging detail, not proof something is
part of the C standard — glibc implements POSIX's `fork` right alongside ISO C's `printf`.
`dup2(a, b)` makes two file descriptors share one underlying open file — exactly what a
shell's `2>&1` redirection compiles down to.

### 5-minute revision
- `main`'s optional third parameter, `envp`, and the POSIX-guaranteed global `environ`
  variable both expose the same `"NAME=value"`-array environment `getenv` searches.
- The reliable test for "is this part of ISO C" is which *standard* documents it, never
  which library file it happens to ship in.

**Worth memorizing:** the four-layer table itself — this is asked constantly, in exactly
this shape, in systems-programming interviews.

---

## Processes: `fork`/`exec`/`wait` — 1-minute summary

`fork()` returns **twice** — the child's PID in the parent, `0` in the child, `-1` on
failure in the parent only — the single most important fact about it. The child is a
copy-on-write duplicate of the parent at the moment of the call; changes after that point
are invisible across the fork in either direction. `exec` never returns on success — it
replaces the calling process's own program entirely; it only returns (with `-1`) on
failure. The classic `fork`-then-`exec` split (rather than one combined "spawn" call) lets
the child adjust its own environment (redirect fds, change directory) *before* becoming a
different program, with no special support needed from that program. `wait`/`waitpid`
reap a finished child and decode its exit status via `WIFEXITED`/`WEXITSTATUS`/
`WIFSIGNALED`/`WTERMSIG`.

### 5-minute revision
- A **zombie** has exited but hasn't been `wait`-ed for yet — costs a process-table slot,
  not CPU/memory. An **orphan** is still running with its original parent gone — re-parented
  to `init`/a subreaper specifically so it's never left permanently unreapable.
- Open file descriptors survive `fork` (shared, not copied) *and* `exec` (unless marked
  close-on-exec) — exactly what makes the redirect-then-exec shell pattern work.
- The minimal-shell project is this file's whole content in one loop: `fork` → (child)
  `execvp` → (parent) `waitpid`.

**Worth memorizing:** zombie vs. orphan as two genuinely different (and independent)
failure-to-reap situations — a very common interview pairing.
**Worth understanding, not memorizing:** *why* `fork`+`exec` beats one combined spawn call
(processes_fork_exec_wait.md §4) — this is a real design-philosophy point, not trivia.

---

## Pipes & process-control signals — 1-minute summary

`pipe(fd)` gives a unidirectional, kernel-buffered **byte stream** (no message boundaries):
`fd[0]` reads, `fd[1]` writes. `read` returns `0` (EOF) only when **every** write-end
descriptor is closed — including inherited copies in the reader itself and in the parent
shell. Writing with no readers raises `SIGPIPE` (or `EPIPE` if ignored). A shell pipeline
`a | b` is `pipe` + two `fork`s, each child `dup2`-ing its end onto fd 1 or 0 and closing
both originals *before* `exec`, with the parent closing both ends too. POSIX signals add
`kill`, `sigaction` (well-defined, portable handler semantics — prefer it over `signal`),
and `sigprocmask`. `SIGKILL`/`SIGSTOP` cannot be caught. `SIGCHLD` handlers must reap in a
`waitpid(-1, ..., WNOHANG)` loop because standard signals coalesce.

### 5-minute revision
- `PIPE_BUF`: writes of at most this many bytes are atomic (POSIX minimum 512; 4096 on
  Linux); larger writes may interleave between writers.
- Async-signal-safe functions (`write`, `_exit`, `waitpid`, `kill`, ...) are a documented
  list (`signal-safety(7)`); `printf`/`malloc` are not on it, for the same re-entrancy
  reason as Phase 7's ISO-C rule.
- Slow syscalls interrupted by a signal return `-1`/`EINTR` unless `SA_RESTART`.
- `fork` preserves signal dispositions; `exec` resets caught signals to default but leaves
  ignored ones ignored.

**Worth memorizing:** "unclosed write end => reader hangs forever" and the
`SIGCHLD` + `WNOHANG`-loop reaping idiom.
**Worth understanding, not memorizing:** why `ls | wc` needs the parent to close its pipe
ends (pipes_and_signals.md §3) — trace EOF delivery yourself.

---

## Sockets & networking (bonus) — 1-minute summary

A socket is a file descriptor that can span machines. Client: `socket` -> `connect` ->
`send`/`recv` -> `close`. Server: `socket` -> `bind` -> `listen` -> `accept` (returns a
**new** session descriptor per client) -> `send`/`recv`. TCP is a byte stream like a pipe:
`send`/`recv` may move fewer bytes than asked, `recv` returning `0` means the peer closed,
and message boundaries are your protocol's job (length prefix or delimiter). Ports and
addresses cross the wire in big-endian **network byte order** — always `htons`/`htonl`
(and `ntohs`/`ntohl` on receipt). `gethostbyname` is obsolete (removed from POSIX in 2008;
returns static data); use `getaddrinfo`.

**Worth memorizing:** the server call order and that `accept` returns a new descriptor.
**Worth understanding, not memorizing:** the five hidden bugs in Book A's bare-bones example
(sockets_and_networking.md §5) — unchecked `recv` giving `buffer[-1]`, a `while (consocket)`
loop that spins on failure, leaked session descriptors — they show *why* error checks are
not optional.

---

## Cross-topic traps worth drilling before an interview

1. Assuming a function is ISO C just because it's declared in a header and lives in
   `libc` — check which *standard* documents it instead.
2. Treating `fork()`'s single call as returning once — it returns twice, in two processes.
3. Expecting `exec` to return on success — it doesn't; only failure returns.
4. `fork`-ing many children without ever `wait`-ing — zombies accumulate silently until the
   process table fills.
5. Assuming two processes can communicate via an ordinary shared global variable after
   `fork` — they can't; that needs explicit IPC (pipes, next in this phase).
6. Forgetting to close the write end of a pipe in the reader (or the parent) — the reader
   blocks forever waiting for an EOF that can never arrive.
7. A `SIGCHLD` handler that calls `waitpid` once instead of looping with `WNOHANG` — leaves
   zombies when several children exit together (signals coalesce).
8. Calling `printf` (or `malloc`) inside a signal handler — not async-signal-safe.
9. Assigning `dest.sin_port = 80` without `htons` — works only on big-endian hosts.
10. `buffer[len] = '\0'` after an unchecked `recv` — `recv` returns `-1` on error, writing
    `buffer[-1]`.
11. Treating one `send` as one `recv` — TCP has no message boundaries.
