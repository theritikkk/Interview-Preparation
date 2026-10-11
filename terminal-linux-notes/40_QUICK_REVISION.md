# 40 — Quick Revision

The last file, written last. Five sections, exactly as originally scoped: a 5-minute pass, a 15-minute pass, a pointer for a 30-minute pass, a command reference, a syscall reference, and a final self-check list with no answers given — those live in files 01-39 and the interview-question file, 36.

---

# 5-Minute Revision

The twelve mental models from file 00, compressed to one line each:

```
Terminal emulator ≠ shell — two processes, one the parent of the other
PTY = master/slave + a real line discipline (canonical mode, echo, signals) — NOT a plain pipe
fork returns twice (0 in child, PID in parent); exec never returns on success
fd → open file description (offset+flags, maybe shared) → inode — three layers, not one
Ctrl+C/Z/D all pass through the line discipline; only D generates no signal at all
SIGKILL/SIGSTOP: kernel-enforced uncatchable, by design — everything else can be caught
Redirection + pipes = dup2() called BEFORE exec, invisible to the program being redirected
EOF = read() returning 0 — same mechanism for a pipe, a keyboard, or a file
A session has ONE controlling terminal and ONE foreground process group at a time
setsid() fails on an existing group leader — fork first, then setsid, always
Namespaces isolate what's visible; cgroups bound what's usable — BOTH make a container
tmux's client talks to a persistent server over a Unix domain socket, never a network one
```

---

# 15-Minute Revision

The mechanisms underneath the one-liners above:

- **Canonical vs raw mode** — canonical buffers, edits, and echoes a full line in the kernel before any process sees it; raw delivers bytes immediately, pushing all responsibility onto the reading program. Readline and ZLE both disable canonical mode entirely and reimplement editing themselves, in userspace (files 02, 23).
- **Copy-on-write** — `fork()` shares physical pages read-only; a write by either side triggers the kernel to allocate a private copy of just that page. This is why `fork`+immediate-`exec` is cheap even for a large process (file 08).
- **The fd three-layer model** — same open file description shared after `fork`/`dup`/`dup2` means a shared offset; two independent `open()` calls on the same path never share one (file 10).
- **`dup2`-before-`exec`** is the entire mechanism behind redirection and pipes — the target program never knows it happened (files 12-13).
- **Pipe EOF** requires every copy of the write end, across every process, to be closed — one forgotten copy anywhere hangs the reader forever (file 13).
- **Signal targeting** — `Ctrl+C`/`Ctrl+Z` reach the *entire* foreground process group, a literal kernel-tracked value per terminal, read/set via `tcgetpgrp`/`tcsetpgrp` (files 14, 16).
- **`setsid()`'s precondition** — fails if the caller is already a process group leader, which is exactly why daemonizing forks first (file 16, 28).
- **ELF loading order** — for a dynamically linked binary, the kernel runs `ld.so` *before* the program's own entry point (file 08).
- **Namespaces vs cgroups** — isolation of view vs. bounding of actual resource usage; a container needs both to be more than just an unusually isolated process (file 29).
- **`AF_UNIX` vs `AF_INET`** — local-file-path-addressed, never network-capable, vs IP-and-port-addressed (files 27, 30).
- **VFS** — one syscall interface, dispatched by the kernel to whichever filesystem actually owns a given path — ext4, NFS, or a kernel-generated pseudo-filesystem, indistinguishably from userspace (file 19).

---

# 30-Minute Revision

Re-read `39_MASTER_EXECUTION_TRACE.md` in full, slowly, pausing at every `Remember` callout. That one file is deliberately built to *be* the 30-minute detailed-flow revision — tracing `grep "hello" file.txt | sort > result.txt` through literally every mechanism above, in the order they actually fire, rather than as an isolated list. If any single step doesn't make sense on its own, that's the exact file to go back to.

---

# Command Reference

Commands this handbook used for understanding or debugging the system — not a general command tutorial.

| Command | Purpose |
|---|---|
| `tty` | Print your controlling terminal's device path |
| `who` | Logged-in users and their terminal devices |
| `ps -o pid,ppid,pgid,sid,tty,stat,comm` | Process/session/group snapshot |
| `pstree -p` | Visual process tree |
| `top` / `htop` | Live, continuously refreshing resource usage |
| `jobs` / `fg` / `bg` | Job control |
| `kill` / `kill -9` / `kill -STOP` | Send a signal (not necessarily fatal) |
| `trap` | Register a shell-level signal handler |
| `type` / `command -v` / `hash` | Shell-aware command resolution (vs `which`, which isn't) |
| `strace` / `ltrace` | Syscall / library-call tracing |
| `perf` | CPU-time sampling/profiling |
| `lsof` / `fuser` | What has this file open / who's using this file or mount |
| `ss` / `ip` | Active sockets / interfaces and routing |
| `dmesg` | Kernel ring buffer |
| `journalctl` | systemd's integrated log viewer |
| `file` / `ldd` / `readelf` / `objdump` / `nm` | Binary/ELF inspection |
| `gdb` | Interactive debugger |
| `time` / `/usr/bin/time` | Timing, with `/usr/bin/time` reporting far more detail |
| `stty -a` | Dump current `termios` settings |
| `ulimit -a` | Current resource limits |
| `systemctl` | Service management |
| `tmux new` / `attach` / detach (`Ctrl+B D`) | Session multiplexing |
| `ssh -t` | Force remote PTY allocation |
| `docker exec -it` | Run a process inside an existing container |

---

# System Call Reference

| Syscall | One-line purpose |
|---|---|
| `fork()` | Create a near-exact copy of the calling process |
| `execve()` | Replace the calling process's image with a new program, same PID |
| `wait()` | Block until any child changes state, retrieve its status |
| `waitpid()` | Like `wait()`, for a specific child, with non-blocking/stopped-child options |
| `openat()` | Open a file relative to a directory fd, returning a new fd |
| `read()` | Read bytes from an fd; returning 0 signals EOF |
| `write()` | Write bytes to an fd |
| `close()` | Release an fd; destroys the open file description if it was the last reference |
| `dup2()` | Repoint one fd number at another's open file description |
| `pipe()` | Create a kernel-managed, in-memory, one-directional byte queue |
| `socket()` | Create a new, unconnected network or Unix-domain endpoint |
| `connect()` | Initiate a connection on a socket |
| `mmap()` | Map a file, or anonymous memory, into the process's address space |
| `brk()` | Adjust the size of the process's heap |
| `chdir()` | Change the calling process's own current working directory |
| `kill()` | Send a signal to a process or process group |
| `setsid()` | Create a new session, with the caller as its leader — fails if already a group leader |

---

# "Can I Explain This?"

No answers here — this is the self-check. If any of these stall you, that's the signal for which file to reread, and file 36 has the full short/deep/misconception/follow-up treatment for every one of them.

- Can I explain exactly what happens when I type `ls` and press Enter?
- Can I explain why `cd` must be a shell builtin?
- Can I explain `2>&1`, including why its position in a command changes the result?
- Can I explain how a pipe connects two processes, and precisely when its reader sees EOF?
- Can I explain what happens, mechanically, when I press `Ctrl+C`?
- Can I explain `fork` + `exec`, and why they're kept as two separate syscalls?
- Can I explain what a PTY is, and how it differs from a plain pipe?
- Can I explain how SSH provides an interactive remote shell, end to end?
- Can I explain why stdout sometimes doesn't appear immediately?
- Can I explain what `docker exec -it` actually does, underneath?
- Can I explain how tmux survives a dropped SSH connection?
- Can I explain why `Ctrl+D` isn't a signal, despite behaving like one?
- Can I explain the three-layer fd / open-file-description / inode model, with an example where it actually matters?
- Can I explain why a container is not a lightweight VM, precisely?
- Can I explain what `ld.so` does, and when the kernel runs it relative to the program's own code?

---

## Complete file index

| # | File | Covers |
|---|---|---|
| 00 | `MASTER_MENTAL_MODEL` | The twelve mental-model diagrams |
| 01 | `TERMINALS_AND_TERMINAL_EMULATORS` | Terminal, console, teletype, emulator; terminal ≠ shell |
| 02 | `TTY_AND_PTY` | Device files, PTY master/slave, line discipline, echo |
| 03 | `SHELLS_OVERVIEW` | Shell landscape, POSIX boundary |
| 04 | `BASH` | Bash as language and process launcher |
| 05 | `ZSH` | Zsh as a diff against Bash |
| 06 | `SHELL_PARSING` | Tokenization, quoting, expansion order |
| 07 | `COMMAND_EXECUTION` | The 24-step trace; PATH; builtins vs external |
| 08 | `PROCESSES_FORK_EXEC` | fork, exec, copy-on-write, ELF, dynamic linking |
| 09 | `SYSTEM_CALLS` | User/kernel mode, the syscall transition |
| 10 | `FILE_DESCRIPTORS` | fd table, open file description, inode, dup family |
| 11 | `STDIN_STDOUT_STDERR` | Why stderr is separate; `2>&1` in detail |
| 12 | `REDIRECTION` | Operators, here-documents, process substitution |
| 13 | `PIPES` | pipe(), wiring, buffering, EOF, close discipline |
| 14 | `SIGNALS` | Signal catalog, Ctrl+C/Z, SIGWINCH |
| 15 | `JOB_CONTROL` | Foreground/background, fg/bg |
| 16 | `SESSIONS_AND_PROCESS_GROUPS` | setsid, tcsetpgrp, SIGTTIN/TTOU |
| 17 | `ENVIRONMENT_AND_PATH` | environ, envp, export, IFS |
| 18 | `SHELL_STARTUP_FILES` | Login vs interactive; Bash and Zsh file sets |
| 19 | `FILESYSTEM_AND_VFS` | VFS, inodes, dentries, directories |
| 20 | `PERMISSIONS_AND_SUDO` | rwx, setuid, sudo and PAM |
| 21 | `PROCESS_MEMORY` | Address-space layout, malloc, ASLR, ulimit |
| 22 | `SCHEDULER` | States, context switching, true concurrency |
| 23 | `TERMINAL_LINE_DISCIPLINE` | termios, Ctrl+D, Readline/ZLE |
| 24 | `TERMINAL_CONTROL_SEQUENCES` | ANSI escapes, alternate screen |
| 25 | `INTERACTIVE_TERMINAL_PROGRAMS` | vim/less/top/ssh patterns |
| 26 | `SSH` | Keys, encrypted channel, remote PTY, `-t` |
| 27 | `TMUX` | Client/server, Unix domain sockets |
| 28 | `DAEMONS_SYSTEMD` | Double-fork recipe, systemd |
| 29 | `CONTAINERS` | Namespaces, cgroups, `docker exec -it` |
| 30 | `NETWORKING_FROM_TERMINAL` | `curl` traced; AF_UNIX vs AF_INET |
| 31 | `DEBUGGING_TOOLS` | Tool-by-tool reference |
| 32 | `STRACE` | `ls`, `cat`, a full pipeline traced |
| 33 | `PROC_FILESYSTEM` | `/proc`, `/proc/self` vs `/proc/$$` |
| 34 | `MINI_SHELL_PROJECT` | 12-stage C build |
| 35 | `LABS` | 24 hands-on labs |
| 36 | `INTERVIEW_QUESTIONS` | 41 questions, four parts each |
| 37 | `DEBUGGING_SCENARIOS` | Five investigations |
| 38 | `COMMON_MISCONCEPTIONS` | 34 corrected misconceptions |
| 39 | `MASTER_EXECUTION_TRACE` | One command through every layer |
| 40 | `QUICK_REVISION` | This file |
| 41 | `BOOT_TO_TERMINAL` | Power button to shell; what PID 1 is |
| 42 | `SHELL_GOTCHAS_AND_EXIT_STATUS` | Traps, `$?`, `set -e`, operators |
| 43 | `MACOS_COMPARISON` | Linux vs macOS, side by side |
| 44 | `PROCESS_TREE_ZOMBIES_ORPHANS` | The process tree, zombies, orphans |

---

## This is the end of the handbook

Forty-five files, `00` through `44`. Every mechanism is traced to a syscall or a kernel concept, and every syscall is tied back to something you can run and observe yourself. The pressure test in file 00 and the checklist above are the two places to return to, periodically, long after the first read-through.
