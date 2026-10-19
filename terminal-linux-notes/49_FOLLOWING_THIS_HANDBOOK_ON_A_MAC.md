# 49 — Following This Handbook on a Mac

> **Where this fits**
> ```
> Files 01-48 (a Linux-centred handbook)  →  [ what works on YOUR machine, and what to do about the rest ]
>                                              ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
>                                              this file
> ```
> File 43 compares Linux and macOS conceptually. This file is its practical companion: you have a MacBook Air, so here is what you can run natively, what needs a Linux environment, and which examples in earlier files need translating. **Everything I tested ran on Linux — I have no macOS machine here.** So each claim below carries a mark:
>
> - **[N]** — stable, documented macOS behaviour I'm confident of.
> - **[V]** — plausible but worth verifying on your Mac; I say how.
> - **[L]** — needs a real Linux kernel.

---

## 1. First, find out exactly what you have

```bash
uname -m          # arm64  → Apple Silicon (M1 or later);  x86_64 → Intel
sw_vers           # your macOS version
echo $SHELL       # /bin/zsh on any recent Mac (file 05)
echo $0           # in Terminal.app, usually -zsh : the leading '-' means a LOGIN shell (file 18)
xcode-select -p   # prints a path if the Command Line Tools are installed
```

If the last command prints nothing, run `xcode-select --install` **[N]**. That gives you `clang`, `make`, `git` and a `python3` — everything the C and Python examples need. Most MacBook Airs since late 2020 are Apple Silicon **[N]**, but `uname -m` is the authority, not my guess.

---

## 2. What carries over unchanged

macOS is a certified UNIX, so the POSIX core of this handbook works as written **[N]**:

| Files | Topic | On your Mac |
|---|---|---|
| 01-03, 05-07 | Terminals, shells, parsing, command lookup | Works. Terminal.app or iTerm2 is your emulator; zsh is your shell. |
| 08, 10-16 | `fork`/`exec`, fds, pipes, redirection, signals, job control, sessions | Works in C with `clang`. |
| 17-18 | Environment, startup files | Works; your files are `.zshenv`, `.zprofile`, `.zshrc`, `.zlogin`. Terminal.app starts **login** shells — the opposite default to most Linux terminals (file 18). |
| 20 | Permissions, `setuid`, `sudo` | Works. |
| 23-25 | `termios`, escape sequences, interactive programs | Works; Terminal.app understands the sequences in file 24. |
| 26-27 | SSH, tmux | Works. `brew install tmux` for tmux (Homebrew is at brew.sh). |
| 30 | `curl` and sockets | Works (but see `/etc/resolv.conf` below). |
| 34 | The mini-shell project | Works with `clang`. |
| 36-39, 41-42, 44 | Interview questions, scenarios, traces, boot, gotchas, zombies | Conceptual — apply to any UNIX. |
| 45-48 | Parser, multiplexers, `minimux` | Python and C; see Section 7 for `minimux`. |

---

## 3. What does *not* work natively — and the replacement

| Handbook uses | Why it fails on macOS | Use instead |
|---|---|---|
| `strace`, `ltrace` (files 09, 13, 19, 30, 32) | Linux tools | `sudo dtruss ./prog` **[V]** — but System Integrity Protection restricts tracing of system binaries and parts of DTrace (file 43). Or `fs_usage`, `sample <pid>`, `lldb`. Or **[L]**. |
| `/proc` (files 10, 17, 21, 33) | **No `/proc` exists** **[N]** | `lsof -p <pid>` (open files), `vmmap <pid>` (memory map), `ps`, `sysctl` |
| `ldd`, `readelf` (files 08, 31) | macOS executables are Mach-O, not ELF | `otool -L` (libraries), `otool -l`, `nm`, `file` |
| `ss`, `ip` (files 30, 31) | Linux tools | `netstat -an`, `lsof -i`, `ifconfig` |
| `systemctl`, `journalctl` (file 28) | No systemd; macOS uses `launchd` | `launchctl`, `log show` |
| Namespaces, cgroups (file 29) | Linux kernel features | Docker Desktop / OrbStack run a Linux VM (file 43) |
| `pstree` (files 31, 44) | Not installed | `brew install pstree`, or `ps -A -o pid,ppid,pgid,tty,stat,command` |
| `nproc` (file 22, Lab 22) | Not a macOS command | `sysctl -n hw.ncpu` |
| `ps --forest`, `ps --ppid` | GNU long options | BSD `ps` flags only; or `pgrep -P <pid>` |
| `script -c 'cmd' file` (file 46) | BSD `script` has a different syntax | `script -q file cmd args…` |
| `cat /etc/resolv.conf` (file 30) | Exists, but isn't what most apps use **[V]** | `scutil --dns` |
| `ulimit -n` (file 21) | Works, but the default soft limit is low — commonly 256 **[V]** | Same command; run it and see |

**Two handbook explanations are Linux-specific, not just the tools:**

- **File 21's "`malloc` calls `brk()` or `mmap()`"** describes glibc on Linux. macOS's allocator does not use `brk` **[N]**; `sbrk` exists only as a deprecated stub. The *idea* — a library pool that occasionally asks the kernel for more address space — is the same.
- **Signal numbers beyond the classic few differ.** `SIGINT` (2), `SIGKILL` (9), `SIGTERM` (15), `SIGHUP` (1), `SIGPIPE` (13) match, so the `128+N` exit statuses in file 42 for those are the same. But `SIGSTOP`, `SIGTSTP`, `SIGCONT`, `SIGCHLD`, `SIGUSR1` and `SIGUSR2` have **different numbers** on macOS **[N]** — file 14's table shows Linux's. Always use signal *names* (`kill -STOP`, not `kill -19`).

---

## 4. Your shell is zsh — and some examples assume Bash

You came to this handbook with zsh as your default, and file 05 already covers the main trap: **zsh does not word-split unquoted expansions** the way Bash does. Practical consequences for earlier files:

- Examples in files 06 and 42 that rely on splitting (`for x in $*`, `rm $f` with spaces) behave differently in zsh. Run them under **`bash`**, or after `emulate sh` in zsh **[N]**.
- `$BASHPID` (files 04, 42) does not exist in zsh, and the Bash that Apple ships at `/bin/bash` is **version 3.2** **[N]** — too old for `$BASHPID` (added in Bash 4) or associative arrays. For a modern Bash: `brew install bash`, then run `/opt/homebrew/bin/bash` on Apple Silicon (`/usr/local/bin/bash` on Intel) **[V]**.
- `/bin/sh` on macOS is Bash 3.2 in POSIX mode, not `dash` — so "what is `/bin/sh`?" (file 03) has a different answer on your Mac than on Ubuntu.

---

## 5. Apple Silicon: what changes if you run Linux

Linux VMs on an Apple Silicon Mac run **arm64 Linux**, not x86-64 **[N]**. File 09 describes the x86-64 system-call convention (`rax`, `rdi`, `rsi`, `rdx`, `r10`, `r8`, `r9`, the `syscall` instruction). On arm64:

| | x86-64 Linux (file 09) | arm64 Linux |
|---|---|---|
| Trap instruction | `syscall` | `svc #0` |
| Syscall number | `rax` | `x8` |
| Arguments | `rdi, rsi, rdx, r10, r8, r9` | `x0`-`x5` |
| Return value | `rax` | `x0` |

(Those arm64 Linux details are from the kernel's documented ABI **[N]**. Native macOS on arm64 uses yet another convention — the syscall number in `x16` and `svc #0x80`, per Apple's XNU sources **[V]** — which only matters if you disassemble macOS binaries.)

**It also changes what `strace` shows.** The arm64 syscall table is "generic" and omits the legacy calls, so traces differ from the idealised ones in files 09, 13 and 32 **[N]**:

| File's trace shows | On arm64 Linux you will see |
|---|---|
| `open(...)` | `openat(...)` (x86-64 uses `openat` for modern glibc too) |
| `dup2(4, 1)` | `dup3(4, 1, 0)` |
| `pipe([3, 4])` | `pipe2([3, 4], 0)` |
| `fork()` | `clone(...)` — and this is true on **x86-64 too**, since glibc implements `fork` with `clone` |
| `stat(...)` | `newfstatat(...)` |

The *structure* — `execve`, loader `mmap`s, `openat`, `getdents64`, `read`/`write` loops, the pipe-wiring dance — is identical. Read for the pattern, not the exact names. (I couldn't run `strace` in my sandbox, so this comes from the syscall tables, not a captured trace; file 32 has been annotated to match.)

---

## 6. Getting a real Linux environment on a MacBook Air

For everything marked **[L]**:

| Option | What it is | Notes |
|---|---|---|
| **Multipass** | Canonical's Ubuntu VMs | `brew install --cask multipass`, then `multipass launch --name lab` and `multipass shell lab` **[V]**. A real VM with its own kernel: `strace`, `/proc`, `systemd`, namespaces all work. |
| **Lima** | Linux VMs via the command line | `brew install lima`, then `limactl start` **[V]** |
| **OrbStack / Docker Desktop** | Containers on a shared Linux VM | `docker run -it --rm ubuntu bash`. Convenient, but it's a *container* (file 29): no systemd by default, and `strace` may need `--cap-add=SYS_PTRACE` **[V]**. |
| **UTM** | A general VM app with a GUI | Most flexible, most setup |

For the handbook, **a Multipass or Lima VM is the best fit**: it gives you the whole kernel the labs assume. Then inside the VM:

```bash
sudo apt update && sudo apt install -y strace ltrace gdb build-essential binutils procps psmisc iproute2 tmux
```

On Apple Silicon you get arm64 Ubuntu — which is fine, with Section 5's differences in mind.

---

## 7. `minimux` on macOS

I wrote `minimux` on Linux and tested it only there; its first line used to say "Linux only." On macOS it **probably works** — PTYs, Unix sockets, `select`, `termios` and `fork` are all POSIX — but I can't promise it, so I built a way for your Mac to tell us.

**Step 1 — run the probe** (`code/macos_probe.py`). It tests each OS assumption `minimux` relies on and prints PASS, FAIL or INFO:

```bash
cd code
python3 macos_probe.py
```

**What the probe found on a real Mac.** A run on **macOS 26.7.1, Apple Silicon (arm64), Python 3.13.0** gave 10 PASS, 0 FAIL, 3 INFO. That settles several things I had marked unverified:

| Question | Result on that Mac |
|---|---|
| Does `pty.fork()` plus `select()` on the master work? | **Yes** |
| What does reading a PTY master do after the child exits? | Returns **end-of-file** (Linux raises an error instead). `minimux` handles both. |
| Does resizing a PTY deliver `SIGWINCH` to the program? | **Yes** — the claim in files 14 and 48 holds on macOS too |
| Is `TIOCSWINSZ` on the master visible on the slave? | **Yes** |
| Longest Unix-socket path that bound | **103 characters** — the 104-byte `sun_path` limit including the terminator, as expected |
| Does a double-forked daemon have no controlling terminal? | **Yes** — `ppid=1`, `tty=??` (macOS's spelling of "none") |
| What is `/bin/sh`? | **Bash 3.2** — its prompt was `sh-3.2$`, not `dash`'s `$` |
| Do `tty.setraw`, `TIOCSCTTY` and `pgrep -P` exist? | **Yes** |

One result is worth noticing: `sh -i` printed **`ESC [ ? 1034 h`** before its prompt. That is a private mode (xterm's "interpret meta key" setting) emitted by Bash 3.2's readline. It's a real-world instance of file 45's private-marker sequences, and `minimux`'s model ignores it harmlessly, since only modes `1`, `2004` and `1049` are tracked.

*A flaw in my own probe, found by that run:* its `stty size` check originally passed on `0 0`, because a fresh PTY has no size until one is set and I only looked for two numbers. The probe now sets 27×91 and requires exactly that back, so it is a meaningful check. (Run it again if you'd like the stricter version.)

What this does **not** yet tell us is whether the *test suite* and an interactive `attach` behave on macOS — the probe tests the operating-system pieces, not `minimux` itself. That is the next step:

**Step 2 — try it by hand:**

```bash
python3 minimux.py start work
python3 minimux.py attach work       # Ctrl+B then d to detach
python3 minimux.py attach work       # or attach from a second Terminal window
python3 minimux.py kill work
```

**Step 3 — run the tests:** `python3 test_minimux.py`. I made them portable for you: they now use `pgrep -P` instead of GNU `ps` options, accept macOS's `??` spelling for "no controlling terminal", skip the one Linux-only session check, wait on a marker instead of assuming a `# ` prompt, and prefer `dash` when it exists. On macOS, where `/bin/sh` is Bash, you can force a particular shell:

```bash
MINIMUX_TEST_SHELL='bash --noprofile --norc -i' python3 test_minimux.py
```

**What I could and couldn't verify.** On Linux the suite passes 53/53 against `dash`, and **also against `bash -i`** — a readline shell, the nearest stand-in for macOS's `sh` I had. (That second run caught a real mistake in one of my own tests: bash's readline turns bracketed paste *on* at every prompt, which `dash` never does, so my "it must be off" assertion was only valid for `dash`. The code was right; the test was wrong.) I could **not** test macOS's actual Bash 3.2, `zsh`, Terminal.app, or Apple's `python3`. Things to watch for:

- If you set `MINIMUX_DIR` yourself, **keep the path short** — macOS's `$TMPDIR` paths are long and Unix socket paths have the 104-character limit. The default (`/tmp/minimux-<uid>`) is short.
- The Apple-provided `python3` is often an older version **[V]**; I tested 3.12, and nothing in the code needs newer than about 3.8, but I haven't run it on anything else.
- **Real `vim`, `less` and `top` inside `minimux` are untried.** The model now understands scroll regions, insert/delete, saved cursor, cursor visibility, mouse and keypad modes, and the `ESC ( B` that ncurses programs emit — but I verified that with hand-derived expected screens and a randomized self-consistency test, not with the real programs. On your Mac, running `vim` in a session, detaching and reattaching is the experiment that tells us whether it holds up; please tell me exactly what you see.
- If a test fails, copy the `FAIL` line and the probe's output back to me — that is exactly the information I'm missing.

---

## 8. Lab by lab

| Lab | Topic | On your Mac |
|---|---|---|
| 1 | Trace `echo hello` by hand | **Native** — paper |
| 2, 3 | `strace ls`, `strace cat` | **[L]**, or `sudo dtruss` **[V]** (SIP) |
| 4-8 | Pipe, `fork`/`exec`, `dup2`, 3-stage pipeline, `SIGINT` in C | **Native** — compile with `clang -Wall -o prog prog.c` |
| 9 | `pstree` | `brew install pstree`, or `ps` with BSD flags |
| 10 | Explore `/proc` | **[L]** — no `/proc` on macOS |
| 11 | `/proc/<pid>/fd` | **Native equivalent:** `lsof -p <pid>` |
| 12-16 | Mini-shell stages | **Native** |
| 17 | PTYs | **Native** — `posix_openpt` works; `script` uses BSD syntax |
| 18 | `ssh` with and without `-t` | **Native.** To try it on one machine: enable *Remote Login* (System Settings → General → Sharing), then compare `ssh localhost tty` with `ssh -t localhost tty` — expect `/dev/ttys00N` devices **[N]** |
| 19 | tmux survives the terminal closing | **Native** — `brew install tmux` |
| 20 | `docker exec -it`, PID 1 | Docker Desktop / OrbStack (a Linux VM underneath) |
| 21, 22 | Raw mode, `SIGWINCH` in C | **Native** |
| 22 (CPU part) | `$(nproc)` | Use `$(sysctl -n hw.ncpu)`; clean up with `killall yes` |
| 23 | tmux's socket | **Native** — `tmux display -p '#{socket_path}'` |
| 24 | Hit `ulimit -n` | **Native** |
| 25 | Build a VT parser | **Native** |
| 26 | Toy multiplexer | **Native** (Python). The real-tmux comparison needs BSD `script -q /tmp/client.out tmux attach -t lab`. |
| 27 | Replicated state machines | **Native** (Python) |
| 28 | `minimux` | **Probably native** — run the probe first (Section 7) |

---

## 9. A reading order for a Mac

1. **Read 01-08, 10-18, 20, 23-25, 34-39, 41-42, 44** straight through — all conceptual or natively runnable.
2. **Read 09 and 32** knowing they're Linux/x86-64 flavoured (Section 5), and run their labs in a Linux VM.
3. **Read 19, 21, 22, 28, 29, 31, 33** for the concepts; do their hands-on parts in a Linux VM.
4. **Read 43** alongside whichever of those you find yourself translating.
5. **Read 45-48** — they run natively, with the caveats in Section 7.

---

## 10. Summary

- macOS is a UNIX: the process, descriptor, signal, pipe, PTY and shell material transfers directly; what doesn't is Linux's *tools* (`strace`, `/proc`, `ss`, `systemctl`) and Linux-only *mechanisms* (namespaces, cgroups, ELF, `brk`).
- Your default shell is zsh (no word-splitting; no `$BASHPID`), and Apple's Bash is 3.2 — use `bash` from Homebrew for the Bash-specific examples.
- On Apple Silicon, a Linux VM is arm64: different registers and different syscall names (`openat`, `dup3`, `pipe2`, `clone`) than the x86-64 traces in earlier files.
- For the labs that need Linux, a Multipass or Lima VM is the closest match to what the handbook assumes.
- `minimux` should work on macOS but is **unverified there**: run `macos_probe.py` first, and send me any failure lines.

---

## What's next

If the probe and tests pass on your Mac, you have a working multiplexer running natively. If anything fails, the probe output will show exactly which OS assumption broke, and that's a short fix.
