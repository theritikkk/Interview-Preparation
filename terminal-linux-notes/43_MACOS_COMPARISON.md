# 43 — macOS Comparison

> **Where this fits in the stack**
> ```
> Same conceptual stack  →  different kernel, different tools underneath
> ```
> Every file in this handbook has carried a short macOS note. This file collects them into one side-by-side comparison. The headline: **the conceptual model transfers almost entirely** — PTYs, `fork`/`exec`, file descriptors, signals, process groups, and sessions all behave the same way, because macOS is a UNIX. What differs is mostly the *tools* you use to observe it, and a handful of genuinely different subsystems.

---

## 1. The vocabulary

- **XNU** — macOS's kernel: a hybrid of the **Mach** microkernel (tasks, threads, virtual memory, IPC via ports), a **BSD** layer (processes, the POSIX syscall interface, the network stack, VFS), and **IOKit** (the driver framework).
- **Darwin** — the open-source core operating system: XNU plus the BSD userland. macOS is Darwin plus Apple's proprietary frameworks and GUI.
- **`launchd`** — macOS's PID 1 and service manager, replacing what `init`, `systemd`, `cron`, and `inetd` do separately on Linux (file 41).
- **Mach-O** — macOS's executable format, instead of ELF (file 08); **`dyld`** is its dynamic linker, instead of `ld.so`.

---

## 2. Side-by-side

| Area | Linux | macOS | Command or behavior difference |
|---|---|---|---|
| **Kernel** | Linux (monolithic) | XNU (Mach + BSD hybrid) | Different internals, same POSIX surface |
| **Processes** | `fork`/`execve`/`wait` | Same — but see the `fork` note below | `ps` flags differ (Section 3) |
| **System calls** | One syscall table | Three classes: BSD syscalls, Mach traps, machine-dependent calls | Numbers and ABI differ from Linux |
| **Shell** | Bash common | **Zsh default** (Catalina+); Bash frozen at 3.2 | Files 03-05; `/bin/sh` is Bash in POSIX mode |
| **Terminal** | Many emulators | Terminal.app, iTerm2 | Terminal.app starts **login** shells by default (file 18) |
| **PTY** | `/dev/ptmx` + `/dev/pts/N` | Same POSIX API; slaves named `/dev/ttysNNN` | Code using `posix_openpt` ports unchanged |
| **Line editing** | GNU Readline | **libedit** (readline-compatible shim) | Subtle edge-case differences (file 23) |
| **Filesystem** | ext4/xfs/btrfs, **`/proc`** | APFS, **no `/proc`** | File 33 |
| **Service manager** | systemd | launchd | `systemctl` → `launchctl` |
| **Network stack** | Linux stack | BSD-derived stack | `ss`/`ip` → `netstat`/`ifconfig` |
| **Containers** | Namespaces + cgroups | **Neither exists** | Docker Desktop runs a Linux VM |
| **Dynamic linker** | `ld.so` | `dyld` | `ldd` → `otool -L` |
| **Boot** | UEFI → GRUB → kernel → systemd | Boot ROM/iBoot → XNU → launchd | File 41 |

---

## 3. Tools, translated

| Purpose | Linux | macOS |
|---|---|---|
| Trace syscalls | `strace` | `dtruss` (DTrace-based; often blocked by System Integrity Protection) |
| Trace library calls | `ltrace` | `dtruss`/`dtrace` — no direct equivalent |
| Shared library deps | `ldd` | `otool -L` |
| Inspect binary format | `readelf` | `otool -l`, `objdump` |
| Debugger | `gdb` | `lldb` (the default) |
| Memory map of a process | `/proc/<pid>/maps` | `vmmap <pid>` |
| Per-process file activity | `strace -e file` | `fs_usage` |
| Sample what a process is doing | `perf` | `sample <pid>` |
| Open files / sockets | `lsof`, `ss` | `lsof` (works), `netstat`; no `ss` |
| Interfaces and routes | `ip` | `ifconfig`, `networksetup`, `scutil` |
| Kernel parameters | `/proc/sys/`, `sysctl` | `sysctl` only — different key names |
| Service control | `systemctl`, `journalctl` | `launchctl`, `log show` |
| Process tree | `pstree` | Not installed by default (`brew install pstree`) |
| Memory stats | `free`, `/proc/meminfo` | `vm_stat`, `top` |

> **Common mistake**
> "`ps -ef --forest` works the same everywhere." `ps` on macOS is BSD-derived — `ps aux` and `ps -ef` both work, but GNU-specific options like `--forest` and the `-o` field names don't all carry over. Likewise, `sed -i` on macOS requires an explicit (possibly empty) backup-suffix argument — `sed -i '' ...` — and `ls --color` becomes `ls -G`. Scripts written against GNU coreutils often need adjusting; installing GNU tools via Homebrew gives them `g`-prefixed names (`gsed`, `gls`).

---

## 4. The differences that actually change how you reason

**No namespaces, no cgroups.** File 29's entire container mechanism is Linux-specific. On macOS, Docker Desktop runs a **Linux virtual machine** and your containers run *inside it* — so on a Mac, "container ≠ VM" has an ironic caveat: the containers genuinely are living inside one. `docker exec -it` still behaves like file 29 describes, but the namespace/cgroup machinery is operating inside that VM's Linux kernel, not on macOS itself.

**No `/proc`.** Every `/proc/<pid>/...` example in this handbook (file 33) fails on macOS. Equivalents exist, but they're entirely different tools: `vmmap`, `lsof`, `sysctl`, `libproc` APIs.

**Case-insensitive filesystem by default.** APFS on macOS is typically *case-insensitive but case-preserving* — `File.txt` and `file.txt` are the same file. Linux filesystems are normally case-sensitive. This breaks scripts and build systems moving between the two in ways that are easy to miss.

**Different directory layout.** User homes live in `/Users`, not `/home`; mounted volumes appear under `/Volumes`; `/etc` is a symlink into `/private/etc`; the system volume is read-only and cryptographically sealed on current macOS versions.

**`fork()` exists but is discouraged in some contexts.** It works as file 08 describes, but a forked child of a process that has already loaded Apple's frameworks or started threads can hit restrictions or crashes in the child before `exec` — which is why `posix_spawn` is the preferred process-creation call on macOS for programs that can't guarantee they're single-threaded.

**System Integrity Protection (SIP).** A kernel-enforced restriction that blocks even root from modifying protected system locations and from tracing certain system processes — the reason `dtruss` frequently fails on system binaries where `strace` would simply work on Linux.

**Terminal.app starts login shells.** On most Linux terminal emulators a new window is a non-login interactive shell (file 18); on macOS's Terminal.app it's a login shell by default, so `~/.zprofile`/`~/.bash_profile` run on every new window. This is a common reason a `PATH` edit placed in the "wrong" startup file appears to work on one OS and not the other.

---

## 5. What does *not* differ

Worth stating plainly, since it's the majority of this handbook:

- PTYs, line discipline, canonical vs raw mode, `termios`, escape sequences
- `fork`, `execve`, `wait`, copy-on-write, process groups, sessions, `setsid()`, `tcsetpgrp()`
- File descriptors, `dup2`, `pipe`, redirection, signals and `SIGCHLD`/`SIGHUP`/`SIGPIPE` semantics
- POSIX shell language, quoting and expansion order, environment variables, `PATH`
- Permissions, `setuid`, the VFS-style inode/dentry model, virtual memory and ASLR
- OpenSSH client and server behavior (file 26)

---

## 6. Common misconceptions

| Misconception | Reality |
|---|---|
| "macOS is just Linux with a different GUI." | A different kernel (XNU), different executable format (Mach-O), different service manager, different tooling — sharing POSIX, not an implementation. |
| "My Linux `/proc`, `strace`, and `ss` commands will work on a Mac." | None of them exist; each has a different equivalent. |
| "Containers on a Mac use macOS's own isolation features." | They run inside a Linux VM — macOS has no namespaces or cgroups. |
| "Bash on macOS is current Bash." | The bundled one is frozen at 3.2 (no associative arrays); Zsh is the default. |

---

## 7. Summary

- The conceptual model of this handbook transfers to macOS almost entirely — PTYs, process creation, fds, signals, sessions, and pipes are POSIX, not Linux-specific.
- What changes is the kernel internals (Mach + BSD), the binary format and linker (Mach-O, `dyld`), the service manager (`launchd`), and nearly all the inspection tools.
- Namespaces, cgroups, and `/proc` don't exist on macOS; containers there run inside a Linux VM.
- Several small behavioral differences — case-insensitive APFS, login-shell Terminal windows, BSD flags — cause most real-world cross-platform breakage.

---

## What's next

`44_PROCESS_TREE_ZOMBIES_ORPHANS.md` — the last file — covers the process tree itself, and finally gives zombies and orphans the full treatment files 08, 29, and 31 kept deferring.
