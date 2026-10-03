# Processes: `fork`, `exec`, and `wait`

> **Phase 9 · UNIX/POSIX System Interface.** Depends on:
> `memory/01_memory_model.md` §2 (a process's virtual address space — this file is about
> *creating* one of those), `libraries/02_stdlib.md` §6 (`exit`/`EXIT_SUCCESS`/`atexit`),
> `04_system_calls_vs_libc_vs_posix.md` (this phase — the POSIX-layer framing).
> Feeds into: `06_pipes_and_signals.md` (this phase — connecting processes together).
> Sources: **entirely `[External]`** — none of the three books covers process creation at
> all; every fact here is drawn from and cross-checked against POSIX/man7.org directly.

---

## 1. What a process actually is, restated for this context

`[Cross-reference]` `memory/01_memory_model.md` §2 already established that each process
has its own independent virtual address space. This file is about the **only** way new
processes come into existence on a UNIX-family system: an existing process asks the kernel
to make a copy of itself.

---

## 2. `fork` — one call, two returns

`[External]`

```c
#include <unistd.h>
pid_t fork(void);
```

`[External, POSIX-defined semantics]` `fork` creates a new process — the **child** — as a
near-exact duplicate of the calling process — the **parent**. Both processes then continue
executing **independently, from the instruction right after the `fork` call** — and this is
the detail that trips up everyone the first time: **`fork` returns twice**, once in each
process, with different values:

| Return value | Which process | Meaning |
|---|---|---|
| The child's PID (a positive integer) | The **parent** | "My child's process ID is this" |
| `0` | The **child** | "I am the child" |
| `-1` | The parent only (no child was created) | An error occurred; `errno` set |

```c
pid_t pid = fork();
if (pid < 0) {
    /* fork failed — no child exists */
} else if (pid == 0) {
    /* this code runs in the CHILD */
} else {
    /* this code runs in the PARENT; pid holds the child's PID */
}
```

### 🧠 Mental model: copy, then diverge
`[External]` Immediately after `fork` returns, the child's memory (code, data, heap, stack
— `memory/01_memory_model.md` §2's whole picture) is a **copy** of the parent's at that
exact moment — same variable values, same open file descriptors (referring to the *same*
underlying open files, sharing position — exactly `04_system_calls_vs_libc_vs_posix.md`
§2's `dup`/`dup2` sharing model, now arising from `fork` instead), same everything. From
that instant on, the two processes are **completely independent** — one modifying a
variable has no effect on the other's copy, because they now occupy genuinely separate
physical memory (real implementations use **copy-on-write** to make this cheap: pages are
only actually duplicated in physical memory at the moment either process writes to one,
`[External]`, an optimization detail that doesn't change the observable behavior).

### ⚠️ Common bug — assuming shared state after `fork`
`[Synthesis]` A parent setting a global variable *before* `fork` is seen by the child (it
was copied); a parent setting a global variable *after* `fork` is **not** seen by the
child (the copies have already diverged) — and vice versa. Two processes wanting to
communicate after `fork` need an explicit IPC mechanism (a pipe, `06_pipes_and_signals.md`,
this phase) — ordinary variables cannot do it, precisely because "same address" no longer
means "same physical memory" once two independent processes are involved.

---

## 3. `exec` — replacing a process's program, not creating a new process

`[External]` A family of related functions (`execl`, `execlp`, `execv`, `execvp`, `execve`,
`execle` — differing in argument-passing style and whether `$PATH` is searched), all doing
the same fundamental thing: **replace the calling process's own memory image** (code, data,
heap, stack — everything) **with a fresh copy of a different program**, then start
executing that new program from its own beginning.

```c
execvp("ls", (char *[]){"ls", "-l", NULL});
/* if this line returns AT ALL, exec failed — there is no "process after exec"
   to return into on success, because that process's image no longer exists */
```

`[External]` **Open file descriptors are preserved across `exec`** by default (unless
individually marked `close-on-exec`) — this is precisely what makes the fork-then-`dup2`-
then-`exec` redirection pattern from `04_system_calls_vs_libc_vs_posix.md` §2 work: set up
the file descriptors you want the new program to inherit *before* calling `exec`, since
`exec` itself doesn't disturb them.

### 🎯 Interview point
"What does `exec` return on success?" — Trick question: **it never returns on success** —
success means the calling process's own code is now gone, replaced entirely; there's
nothing left to return *to*. It only returns (with `-1`) if the exec itself failed (e.g.,
the program named doesn't exist).

---

## 4. Why two separate calls, `fork` and `exec`, instead of one?

`[External, a genuinely important design-rationale point]` A single combined "spawn a new
program" call would be simpler for the common case, but would lose real flexibility that
the two-call split provides for free: **everything a program does between `fork` and
`exec`, in the child, still affects the new program once `exec` runs** — because it's still
the *same* process, just about to load different code. This is exactly how shells
implement I/O redirection (§2's `dup2` pattern), and how a child process can drop
privileges, change its working directory, or otherwise adjust its own environment before
becoming an entirely different program — all without the `exec`'d program needing any
special support for any of it. `[Synthesis]` This is a genuinely elegant instance of UNIX's
general "small, composable primitives" philosophy (the same spirit already seen in
`libraries/01_stdio.md` §1's device-independent I/O history) — two simple operations
combine to do more than either could alone.

---

## 5. `wait`/`waitpid` — reaping a child, and reading its exit status

`[External]`

```c
#include <sys/wait.h>
pid_t wait(int *status);                       /* blocks until ANY child exits */
pid_t waitpid(pid_t pid, int *status, int options);  /* can wait for a SPECIFIC child,
                                                          or poll non-blockingly (WNOHANG) */
```

`status` (if non-`NULL`) is filled in with encoded information about *how* the child
ended, decoded via standard macros:

| Macro | Meaning |
|---|---|
| `WIFEXITED(status)` | True if the child ended via a normal `return`/`exit` |
| `WEXITSTATUS(status)` | The child's actual exit code (only meaningful if `WIFEXITED` is true) |
| `WIFSIGNALED(status)` | True if the child was killed by an unhandled signal |
| `WTERMSIG(status)` | Which signal killed it (only meaningful if `WIFSIGNALED` is true) |

```c
int status;
pid_t child = fork();
if (child == 0) {
    execvp("some_program", argv);
    _exit(127);                        /* only reached if exec itself failed */
} else {
    waitpid(child, &status, 0);
    if (WIFEXITED(status))
        printf("child exited with code %d\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        printf("child killed by signal %d\n", WTERMSIG(status));
}
```

---

## 6. ⚠️ Two failure-to-reap problems, and why they matter

`[External]`

### Zombie processes
When a child exits, it does **not** immediately vanish — the kernel keeps a small record
(its PID and exit status) until the **parent** calls `wait`/`waitpid` to retrieve it. A
child that has exited but hasn't yet been `wait`-ed for is a **zombie**: it consumes no
memory or CPU, but it does occupy a slot in the kernel's process table, and a program that
`fork`s many children without ever reaping them can — in principle — exhaust that table.
**A parent should always eventually `wait` for every child it creates.**

### Orphan processes
If a **parent** exits *before* its child does, the child becomes an **orphan** — rather
than being left with no parent at all, it's automatically **re-parented** to a special
system process (traditionally PID 1, `init`, or a designated subreaper on modern Linux),
which exists in part specifically to `wait` for and clean up orphaned children so they
don't linger as permanent zombies with no one left to reap them.

### 🎯 Interview point
"What's the difference between a zombie and an orphan?" — A zombie has exited but not been
reaped by its parent; an orphan is still running but its original parent is gone (and it's
been re-parented). A process can even be *both* in sequence: an orphan that later exits and
is promptly reaped by its new parent never lingers as a zombie at all — the two problems
are independent, not two names for the same thing.

---

## 7. 🛠 Practice — the phase project: a minimal shell

`[External, synthesizing this file's material into the phase's stated project]` The
canonical structure, tying together every section above:

```
loop forever:
    print a prompt, read a line of input
    parse the line into a command and its arguments
    pid = fork()
    if pid == 0:                          (child)
        [optionally: set up redirection via dup2, §4-referenced pattern]
        execvp(command, arguments)
        _exit(127)                          # only reached if exec failed
    else:                                    (parent)
        waitpid(pid, &status, 0)             # wait for THIS command before prompting again
```

**Every piece of this file is represented in that loop:** `fork` creates the worker
process; the child sets up its environment and calls `exec` to become the real command;
the parent calls `waitpid` to avoid leaving a zombie and to know when it's safe to print
the next prompt. Building this end to end is the single best way to make this entire
phase's material concrete rather than diagrammatic.

---

## 8. ⚠️ Misconceptions to unlearn now

- **"`fork` returns once, like a normal function call."** It returns **twice** — once in
  each process, with different values — this is the single most important fact about it (§2).
- **"A successful `exec` call returns, just like any other function."** It never returns on
  success — the calling process's own code is gone, replaced entirely (§3).
- **"A zombie process is still 'running' in some sense, just wasting resources."** It's
  already finished — it consumes no CPU or memory, only a small kernel bookkeeping slot,
  until its parent reaps it (§6).

---

## 9. 🎯 Interview questions

**Easy**
- What does `fork()` return in the parent versus the child? → The child's PID in the
  parent; `0` in the child (§2).
- Does `exec` create a new process? → No — it replaces the *current* process's program;
  `fork` is what creates new processes (§3).

**Medium**
- Why does the classic shell pattern use `fork` followed by `exec` in the child, rather
  than one combined call? → Everything the child does between `fork` and `exec` (redirecting
  file descriptors, changing directory, dropping privileges) still applies to the program
  `exec` loads, without that program needing any special support — a flexible, composable
  design (§4).
- What's a zombie process, precisely? → A child that has exited but whose exit status
  hasn't yet been collected by its parent via `wait`/`waitpid` (§6).

**Hard**
- Explain why a parent process's global variable changes made *before* `fork` are visible
  to the child, but changes made *after* `fork` are not. → Before `fork`, there's only one
  process and one copy of memory; `fork` copies that memory at the moment it's called, and
  from then on the two processes have entirely separate physical memory (via copy-on-write)
  — so changes in either after that point are invisible to the other (§2).
- Why does re-parenting orphaned processes to `init`/a subreaper matter, rather than just
  leaving them parentless? → A parentless process that later exits would have no one able
  to call `wait` for it, making it a permanent, unreapable zombie; re-parenting guarantees
  some process is always responsible for eventually reaping it (§6).

**Systems-level**
- A long-running server process `fork`s a new child for every incoming request but never
  calls `wait`/`waitpid` on any of them. What happens over time, and why? → Every child that
  finishes becomes a zombie, since nothing ever reaps it; the kernel's process table
  accumulates these zombie entries indefinitely, and on a sufficiently busy server this can
  eventually exhaust the table, preventing new processes (including new children of this
  same server) from being created at all (§6).
- Trace exactly what a minimal shell does, system-call by system-call, when the user types
  `ls -l` and presses enter. → `fork()` creates a child; in the child, `execvp("ls", ["ls",
  "-l", NULL])` replaces its image with `ls`'s program, which runs and eventually calls
  `exit`; in the parent, `waitpid` (blocked since the fork) returns once that exit happens,
  decoding the status via `WIFEXITED`/`WEXITSTATUS` before the shell loops back to prompt
  again (§7).

---

## 10. External references

- [man7.org — `fork(2)`](https://man7.org/linux/man-pages/) `[External]` — current,
  precise semantics, including copy-on-write implementation notes.
- [man7.org — `execve(2)`](https://man7.org/linux/man-pages/) `[External]` — the underlying
  system call behind the whole `exec` family, with the exact differences between
  `execl`/`execlp`/`execv`/`execvp`/`execve`/`execle` spelled out.
- [man7.org — `wait(2)`](https://man7.org/linux/man-pages/) `[External]` — `wait`/`waitpid`
  in full, including every `WIF*`/`W*STATUS` macro.
- [The Open Group Base Specifications, Issue 8](https://pubs.opengroup.org/onlinepubs/9799919799/)
  `[External]` — the formal POSIX text underlying every function in this file.

## 11. 🛠 Practice

1. Build the minimal shell from §7 end to end: read a line, `fork`, `execvp` in the child,
   `waitpid` in the parent — test it with a handful of real commands (`ls`, `echo`, `pwd`).
2. Deliberately omit the `waitpid` call and run your shell against many quick commands in a
   row (or in a loop) — use a process-listing tool (`ps`, previewed here, full coverage in
   `debugging/`, Phase 10) to observe the resulting zombie processes accumulate, then add
   the `waitpid` call back and confirm they're gone.
3. Add basic output redirection (`cmd > file`) to your shell, using the `fork`-then-`dup2`-
   then-`exec` pattern from `04_system_calls_vs_libc_vs_posix.md` §2 in the child, before
   the `execvp` call.
