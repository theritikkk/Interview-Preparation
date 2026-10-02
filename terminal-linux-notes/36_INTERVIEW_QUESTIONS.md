# 36 — Interview Questions

Forty-one questions, each in four parts: a one-line answer you could give immediately, a deeper answer showing you actually understand the mechanism, the wrong answer people commonly give instead, and a natural follow-up an interviewer might ask next. Every answer here was fully derived earlier in this handbook — this file is revision, not new teaching.

---

## Terminal & TTY layer

### 1. What is a shell?
**Short:** A userspace program that reads commands and executes them.
**Deep:** An ordinary process (files 03-04) that parses command lines, resolves names to builtins or external programs, and uses `fork`/`exec`/`wait` to run the latter — no special kernel privilege of any kind.
**Misconception:** "The shell is part of the kernel." It's exactly as ordinary a userspace program as `ls`.
**Follow-up:** Why can't `cd` be an external program?

### 2. What is a terminal?
**Short:** Historically, physical hardware for interacting with a remote computer; today, the interface a terminal emulator reimplements in software.
**Deep:** Teletypes, then video terminals, connected by serial line to a shared computer (file 01) — the hardware is mostly gone, the interface contract it established survives.
**Misconception:** "Terminal" and "terminal emulator" name the same thing. One's history; the other's the modern software standing in for it.
**Follow-up:** What does a terminal emulator actually do, mechanically?

### 3. What is a terminal emulator?
**Short:** A userspace GUI program that presents a PTY to a shell and renders whatever comes out of it.
**Deep:** Allocates a PTY (file 02), forks a child that becomes your shell, reads the master side and interprets escape sequences (file 24), forwards your keystrokes in.
**Misconception:** "The terminal emulator is the shell." Two entirely separate processes — file 01's central point.
**Follow-up:** Is the terminal emulator the parent process of your shell?

### 4. What is a TTY?
**Short:** The abbreviation of "teletype," naming Linux's terminal device subsystem and its device files.
**Deep:** `/dev/tty1`-`6` (virtual consoles), `/dev/pts/N` (PTY slaves), `/dev/tty` (a per-process controlling-terminal alias) — all named after hardware that's largely disappeared (files 01-02).
**Misconception:** "TTY" and "PTY" are interchangeable. PTY is specifically the pseudo-terminal mechanism; TTY is the broader naming convention/subsystem.
**Follow-up:** What's the difference between `/dev/tty1` and `/dev/pts/3`?

### 5. What is a PTY?
**Short:** A pseudo-terminal — a kernel master/slave device pair with a genuine line discipline attached.
**Deep:** The terminal emulator holds the master; the shell holds the slave as its controlling terminal; the kernel's line discipline provides canonical-mode editing, echo, and signal generation across the pair (file 02).
**Misconception:** "A PTY is basically a pipe." A pipe has no line discipline — no echo, no signal generation at all (file 30).
**Follow-up:** What syscalls allocate a new PTY pair?

---

## Processes

### 6. What is a process?
**Short:** The kernel's unit of resource ownership and scheduling.
**Deep:** A `task_struct` (file 08) with its own address space, fd table, and execution context — two processes running identical code are still fully independent.
**Misconception:** A process and a program are the same thing. A program is a file; a process is a running instance of one, with its own PID and state.
**Follow-up:** What's the difference between a process and a process *image*?

### 7. What is `fork`?
**Short:** The syscall that creates a near-exact copy of the calling process.
**Deep:** Returns twice from one call — `0` in the new child, the child's PID in the parent — with memory shared copy-on-write until either side writes to it (file 08).
**Misconception:** `fork()` immediately duplicates all of memory. Copy-on-write defers the actual copying, often indefinitely if `exec` follows right away.
**Follow-up:** What exactly is shared versus copied after `fork()`?

### 8. What is `exec`?
**Short:** The syscall family that replaces a process's image with a new program, keeping the same PID.
**Deep:** `execve()` discards the calling process's code/data/heap/stack and loads a new ELF image in their place — it never returns on success (file 08).
**Misconception:** `exec` creates a new process. It transforms an existing one in place — no new PID.
**Follow-up:** Why does `execve()` never return unless it fails?

### 9. Why are `fork` and `exec` separate syscalls?
**Short:** To leave a window for setting up the child before the new program starts.
**Deep:** Between `fork()` and `exec()`, a shell can `dup2()` file descriptors for redirection or wire up a pipe (files 12-13) — a single atomic "create and run" syscall would remove that window entirely.
**Misconception:** They're separate for historical reasons with no real design purpose today. The separation is exactly what makes redirection and pipes possible in their current form.
**Follow-up:** What would break about shell redirection if `fork`+`exec` were one atomic operation?

### 10. What is a zombie process?
**Short:** An exited process whose parent hasn't yet collected its exit status.
**Deep:** The kernel keeps a minimal record (state `Z`) until the parent calls `wait`/`waitpid`; this is how exit status survives long enough to be retrieved (file 08).
**Misconception:** A zombie is still consuming meaningful resources, like a hung process. It's just a PID and an exit code, waiting to be reaped.
**Follow-up:** What happens to a zombie whose parent never calls `wait()` at all?

### 11. What is an orphan process?
**Short:** A process whose original parent exited before it did.
**Deep:** The kernel re-parents it — typically to PID 1 — specifically so something is always available to eventually reap it (files 08, 29).
**Misconception:** An orphan process is automatically terminated. It keeps running; only its parent relationship changes.
**Follow-up:** Why does a container's PID 1 need to handle this responsibility itself?

### 12. What is a process group?
**Short:** A collection of related processes that can be signaled together, as a unit.
**Deep:** Every pipeline shares one process group; this is exactly why `Ctrl+C` reaches every process in `cmd1 | cmd2` at once, not just one of them (files 14-15).
**Misconception:** A process group is just a loose, informal grouping. It's a literal kernel-tracked attribute (PGID) every process has.
**Follow-up:** What creates a new process group — is it automatic, or does something have to request it?

### 13. What is a session?
**Short:** A collection of process groups, typically everything descending from one login or terminal.
**Deep:** Has at most one controlling terminal; exactly one of its process groups is foreground at any moment (file 16).
**Misconception:** Every process has its own session. Many processes typically share one.
**Follow-up:** What is a session leader, and what's special about it?

### 14. What is job control?
**Short:** The shell mechanism for managing foreground and background jobs.
**Deep:** `fg`/`bg`/`jobs`, built on `SIGCONT` and `tcsetpgrp()` reassigning which process group currently holds the terminal's foreground slot (files 15-16).
**Misconception:** `fg`/`bg` use special job-control-specific signals. They use ordinary `SIGCONT`; the only special part is the terminal foreground handoff.
**Follow-up:** What does `&` actually change about how a command is launched?

---

## Syscalls & execution

### 15. What is a system call?
**Short:** A well-defined request a user-mode program makes of the kernel, crossing a hardware-enforced privilege boundary.
**Deep:** Number and arguments in fixed registers, a dedicated `syscall` instruction triggers the mode switch, the kernel dispatches via a syscall table, `sysret` returns to user mode (file 09).
**Misconception:** Every library function is a system call. `printf()` formats and buffers in user space, only occasionally calling the real syscall, `write()`.
**Follow-up:** Why is `printf` sometimes "delayed" compared to `write`?

### 16. What happens when you type `ls`?
**Short:** The shell parses the line, finds `ls`, forks, execs it, waits, and shows a new prompt.
**Deep:** The full 24-step trace from file 07 — keystrokes through the PTY, tokenization, PATH lookup, `fork`/`execve`, the ELF loader and dynamic linker, syscalls for the actual directory read, output back through the same PTY path.
**Misconception:** "The shell runs the command" as a single, atomic-feeling step. It's a specific, traceable sequence of distinct syscalls, each with its own purpose.
**Follow-up:** Which steps happen in the shell's own process, and which only after `fork()`?

### 17. What happens during `execve`?
**Short:** The calling process's entire image is replaced by a new program's.
**Deep:** The kernel reads the ELF header and program headers, maps code/data/BSS, sets up a fresh stack with `argv`/`envp`, and — for dynamically linked binaries — hands control to `ld.so` first (file 08).
**Misconception:** `execve` starts executing the target program's `main()` immediately. For dynamically linked binaries, `ld.so` runs first, resolving shared libraries before the real entry point.
**Follow-up:** What determines whether the kernel runs `ld.so` first at all?

### 18. What is an ELF binary?
**Short:** The standard Linux format for executables, shared libraries, and object files.
**Deep:** An ELF header (architecture, entry point), program headers (segments the kernel actually loads), and section headers (used by linkers/debuggers, not needed to run the program) — file 08.
**Misconception:** Program headers and section headers are the same thing described twice. They serve different purposes — one for loading, one for linking/tooling.
**Follow-up:** Which of the two does the kernel actually need to run the binary?

### 19. What is the dynamic linker?
**Short:** `ld.so` — the program that resolves a binary's shared library dependencies at startup.
**Deep:** Named via the ELF's `PT_INTERP` header; the kernel loads and runs it *before* the target program's own entry point, so it can `mmap` needed `.so` files and patch up addresses first (file 08).
**Misconception:** Dynamic linking happens "automatically" with no separate program involved. `ld.so` is a real, separate program the kernel deliberately runs first.
**Follow-up:** What's the practical tradeoff between static and dynamic linking?

### 20. What is ASLR?
**Short:** Address Space Layout Randomization — randomizing where major memory regions load, to make exploits harder.
**Deep:** Randomizes the *base* address of the stack, shared-library mappings, and (for PIE binaries) the executable's own code, fresh on every `exec` (file 21).
**Misconception:** ASLR randomizes everything about memory layout, down to relative offsets. It randomizes base addresses only — internal layout within one region stays fixed relative to that base.
**Follow-up:** How would you observe ASLR in action from the shell?

### 21. What is copy-on-write?
**Short:** Deferring an actual memory copy until one side tries to write to it.
**Deep:** After `fork()`, parent and child share physical pages, marked read-only in both; a write triggers a page fault that allocates a private copy of just that page (file 08).
**Misconception:** `fork()` duplicates all of memory immediately. COW makes this lazy, often avoiding most of the copy entirely if `exec` follows soon after.
**Follow-up:** Why does this make `fork`+`exec` cheap even for a large process?

---

## Shell mechanics

### 22. Why is `cd` a shell builtin?
**Short:** Because the working directory is a per-process attribute.
**Deep:** If `cd` forked a child to call `chdir()`, only that child's directory would change, and it would vanish the instant the child exited — the invoking shell's own directory would never be touched (file 07).
**Misconception:** `cd` is a builtin just for speed, like `pwd`. `pwd` is a builtin by choice; `cd` is a builtin by necessity — there's no way to make an external version work at all.
**Follow-up:** Which other commands share this same "must be a builtin" constraint, and why?

### 23. What is `PATH`?
**Short:** A colon-separated list of directories the shell searches, in order, for external commands.
**Deep:** Stops at the first match; order determines which same-named program actually runs when multiple exist (file 07).
**Misconception:** `PATH` search order doesn't really matter in practice. It's both a convenience (personal overrides) and a genuine security surface (a hijackable earlier directory).
**Follow-up:** Why can `which` and `type` give different answers for the same command name?

### 24. What is an environment variable?
**Short:** A shell variable explicitly marked (via `export`) for inclusion in a child process's initial environment.
**Deep:** Lives in `environ`, a private per-process array; `execve`'s `envp[]` becomes the new program's own copy — never automatically shared otherwise (file 17).
**Misconception:** Setting a variable in a shell affects its parent shell too. Environment only flows downward, at `fork`/`exec` time, never upward.
**Follow-up:** What's the difference between `fork()`'s handling of environment versus its handling of file descriptors?

---

## I/O model

### 25. What is stdin?
**Short:** File descriptor 0 — a process's standard input, by convention.
**Deep:** Not kernel-enforced as special, but universally relied on; the shell deliberately connects it to the PTY slave before `exec` (files 10-11).
**Misconception:** stdin always comes from the keyboard. It's whatever fd 0 happens to point at — a file, a pipe, `/dev/null`, anything.
**Follow-up:** What does a program see on stdin if it's redirected from `/dev/null`?

### 26. What is stdout?
**Short:** File descriptor 1 — a process's standard output, by convention.
**Deep:** Line-buffered to a terminal, fully block-buffered to a file or pipe — a real behavioral difference, not just naming (files 09, 11).
**Misconception:** stdout always means "the screen." It's whatever fd 1 points at, which is frequently not a screen at all.
**Follow-up:** Why might output appear delayed when redirected to a file but not when going to a terminal?

### 27. What is stderr?
**Short:** File descriptor 2 — a separate channel specifically for diagnostics, kept out of pipelines by default.
**Deep:** Pipelines (`|`) only redirect stdout; stderr stays connected to the original destination so errors don't corrupt piped data (file 11).
**Misconception:** stderr is just a less-important duplicate of stdout. It exists specifically to prevent diagnostic text from contaminating a pipeline's actual output.
**Follow-up:** Why is stderr traditionally unbuffered while stdout often isn't?

### 28. What is a pipe?
**Short:** A kernel-managed, in-memory, one-directional byte queue connecting two processes.
**Deep:** `pipe()` returns a read end and a write end; writes block when full, reads block when empty with the write end still open (file 13).
**Misconception:** A pipe is basically a temporary file. No disk involvement at all — a bounded, in-memory kernel object with real blocking semantics.
**Follow-up:** What happens if a reader never receives EOF — what's the likely cause?

### 29. How does a pipe work internally?
**Short:** `pipe()` + `fork()` + `dup2()` + disciplined `close()`s wire two processes together.
**Deep:** The shell creates the pipe once, forks each command, each child `dup2`s the right end onto its stdin/stdout and closes both original fds, and the shell itself closes its own copies too (file 13).
**Misconception:** Only the two connected processes need to manage pipe fds carefully. The shell's own unclosed copies can just as easily cause a reader to hang forever.
**Follow-up:** What's the precise condition under which a pipe's reader sees EOF?

### 30. What is a file descriptor?
**Short:** A small, per-process integer index into that process's own table of open things.
**Deep:** Points at an open file description (holding the offset and flags), which in turn points at an inode — three distinct layers (file 10).
**Misconception:** A file descriptor *is* the file. It's an index, two layers removed from the actual underlying file.
**Follow-up:** What's shared versus independent when two fds point at the same open file description?

### 31. What is `dup2`?
**Short:** The syscall that makes one file descriptor number point at the same open file description as another.
**Deep:** `dup2(oldfd, newfd)` atomically closes `newfd` if open, then repoints it — the exact mechanism behind shell redirection and pipe wiring (files 10, 12-13).
**Misconception:** `dup2` copies the file's contents or creates a new file. It only repoints a descriptor; nothing about the underlying file changes.
**Follow-up:** Does `dup2` preserve the original fd's close-on-exec flag?

### 32. What does `>` do?
**Short:** Redirects a command's stdout to a file, truncating it first.
**Deep:** The shell opens the file and calls `dup2()` on fd 1, entirely in its own process, *before* `fork`/`exec` — the target program never knows redirection happened at all (file 12).
**Misconception:** The program being redirected needs special code to support it. Redirection is invisible to it — the shell did all the work beforehand.
**Follow-up:** Does `cmd > file1 > file2` leave `file1` completely untouched?

### 33. What does `2>&1` mean?
**Short:** Make fd 2 a duplicate of wherever fd 1 currently points.
**Deep:** Order-dependent — `>out 2>&1` sends both streams to `out`; `2>&1 >out` leaves stderr on the original destination, since the duplication happened before fd 1 moved (file 11).
**Misconception:** `2>&1`'s effect is the same no matter where it appears in the command. Its position relative to other redirections changes the outcome.
**Follow-up:** Why does `&>` sidestep this ordering issue entirely?

---

## Signals

### 34. What happens when you press `Ctrl+C`?
**Short:** The kernel's line discipline generates `SIGINT` and sends it to the terminal's entire foreground process group.
**Deep:** Keystroke → terminal emulator → PTY master → line discipline recognizes `VINTR` → `SIGINT` to every process in the foreground group (file 14).
**Misconception:** It only affects the one process you're watching. It reaches every member of the foreground process group at once.
**Follow-up:** How does this differ from what `Ctrl+Z` does?

### 35. What is `SIGTERM` vs `SIGKILL`?
**Short:** `SIGTERM` is a polite, catchable request to exit; `SIGKILL` is an unconditional, uncatchable one.
**Deep:** A process can intercept `SIGTERM` to clean up before exiting, or ignore it entirely; `SIGKILL` can't be caught, blocked, or ignored by kernel design (file 14).
**Misconception:** `kill PID` force-terminates a process. By default it sends `SIGTERM`, not `SIGKILL` — a request, not a command.
**Follow-up:** Why does the kernel deliberately make `SIGKILL` impossible to catch?

---

## Remote & multiplexed sessions

### 36. How does SSH provide an interactive shell?
**Short:** `sshd` allocates a new PTY pair on the remote machine and execs the user's shell onto its slave.
**Deep:** The forked child calls `setsid()`, allocates the PTY, connects to the slave as its controlling terminal, then `exec`s the shell — architecturally identical to opening a local terminal window (files 02, 16, 26).
**Misconception:** SSH is a screen-sharing protocol. It transports a byte stream; any "screen" you see is the remote program's own escape-sequence output, interpreted by your local terminal emulator.
**Follow-up:** What happens if no PTY is allocated at all?

### 37. What does `ssh -t` do?
**Short:** Forces remote PTY allocation even when running an explicit remote command.
**Deep:** Without it, a specified remote command gets plain pipes — fine for scripts, but breaks anything needing real terminal behavior, like an editor or a password prompt (file 26).
**Misconception:** `-t` is either always necessary or never necessary. It depends specifically on whether the remote command itself needs a real terminal.
**Follow-up:** Why would `sudo` specifically refuse to run without a PTY in some configurations?

### 38. What does tmux do?
**Short:** Decouples running programs from any one terminal's lifetime, via a persistent server.
**Deep:** A thin client connects to a long-lived server over a Unix domain socket; detaching just disconnects the client, leaving the server (which `setsid()`'d away from any terminal at startup) completely unaffected (file 27).
**Misconception:** tmux is just another shell. It's a multiplexer sitting between a terminal and one or more actual shells.
**Follow-up:** Why does a dropped SSH connection not kill anything running inside a tmux session?

### 39. What happens inside `docker exec -it`?
**Short:** A new process is started inside an already-running container's namespaces, with a new PTY allocated across that boundary.
**Deep:** `-i` keeps stdin open; `-t` allocates a PTY the same way `ssh -t` does — architecturally the identical pattern, just with a namespace boundary instead of a network one (file 29).
**Misconception:** It starts a new container. It starts a new process inside the existing one.
**Follow-up:** What's the difference between what namespaces isolate and what cgroups limit?

---

## Filesystem

### 40. What is `/proc`?
**Short:** A pseudo-filesystem exposing live kernel state as if it were ordinary files.
**Deep:** Generated on read by kernel code, not backed by disk — `ps`/`top` are just ordinary programs reading it, with no special process-listing syscall involved (file 33).
**Misconception:** `/proc/self` refers to whichever process you're thinking of, like your shell. It refers to whichever process performs the lookup — usually the command you just ran, not the shell that launched it.
**Follow-up:** How would you inspect your own shell's open file descriptors specifically, rather than some other process's?

### 41. What is VFS?
**Short:** The kernel's Virtual Filesystem layer — one interface, many underlying implementations.
**Deep:** Abstract operations (open/read/write/readdir) as function pointers every filesystem type implements, letting identical syscalls work across ext4, NFS, and kernel-generated pseudo-filesystems alike (file 19).
**Misconception:** Different filesystem types need different tools (`ls`, `cat`) to work with them. The exact same tools work unmodified everywhere, because VFS presents one identical interface regardless of what's underneath.
**Follow-up:** Why is a directory not a fundamentally different kind of object from a file, mechanically?

---

## Summary

Forty-one questions, eight categories, every answer traceable back to a specific file's full explanation. If a follow-up question stops you, that's the precise signal for which file deserves another pass before an actual interview.

---

## What's next

`37_DEBUGGING_SCENARIOS.md` takes five realistic "why isn't this working" situations and walks the actual investigation, using exactly the tools from file 31 and the mechanisms from everywhere else.
