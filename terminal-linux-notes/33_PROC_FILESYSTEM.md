# 33 — The /proc Filesystem

> **Where this fits in the stack**
> ```
> You  →  cat / ls / grep  →  VFS  →  [ procfs ]  →  live kernel state, generated on demand
>                                      ^^^^^^^^^^
>                                      this file
> ```
> This handbook has forward-referenced `/proc` from files 10, 17, 19, 21, 22, and 25. This file fulfills all of them — and, in doing so, corrects two earlier examples that misused `/proc/self`. The correction is in Section 3, and it's worth reading carefully, because the mistake is an easy and very common one.

---

## 1. What `/proc` actually is

`/proc` is a **pseudo-filesystem** (`procfs`), implemented by the kernel through the VFS abstraction from file 19. It is **not backed by disk blocks at all** — its "files" are generated on demand, by kernel code, at the instant you read them, reflecting the kernel's current internal state. This is file 19's "generated on the fly" claim, now fully explained.

Because it goes through the VFS layer, ordinary tools — `cat`, `ls`, `grep` — work on it with **zero special support**. That's file 19's VFS payoff, made concrete: none of those tools contain any `/proc`-specific code.

> **Under the hood**
> Most `/proc` entries report a size of `0` in `ls -l`, even when reading them yields plenty of content. That's not a bug — the content isn't stored anywhere to have a size; it's generated at read time.

---

## 2. Per-process directories: `/proc/<pid>/`

Every running process has a directory `/proc/<its PID>/`. The most useful entries:

| Entry | What it shows |
|---|---|
| `fd/` | A directory of symlinks, one per open fd, each pointing at what that fd refers to (file 10). Pipes appear as `pipe:[inode]` (file 13), sockets as `socket:[inode]` (files 27, 30). |
| `status` | Human-readable summary: name, state (files 08, 22), PID/PPID, UIDs and GIDs (file 20 — the `Uid:` line shows real, effective, saved, and filesystem UIDs), memory summary (`VmSize`, `VmRSS`), signal masks (file 14). |
| `maps` | The process's **virtual memory map** — every mapped region with address range, permissions, and backing file (file 21's entire layout, as literal lines, with `[heap]` and `[stack]` labeled explicitly). |
| `environ` | The process's **initial** environment, NUL-separated (file 17) — see the caveat below. |
| `cwd` | A **symlink** to the process's current working directory (file 08's `fs_struct`, file 07's `cd` argument — the per-process cwd, directly observable). |
| `exe` | A symlink to the actual executable file the process is running (file 08's process image). Shows `(deleted)` if the binary was removed while the process kept running — file 19's "a deleted file survives while something still has it open." |
| `cmdline` | The process's `argv`, NUL-separated (file 17). |

> **Common mistake**
> "`/proc/<pid>/environ` shows the process's *current* environment." It shows the **initial** environment passed at `execve` time. If the process later modifies its environment — `putenv()`, `setenv()`, or an `export` inside a running shell — this file does **not** reflect those changes.

---

## 3. `/proc/self` — precisely, and a correction

`/proc/self` is a special symlink that always resolves to `/proc/<PID of whichever process is performing the lookup>`.

> **Remember — the correction**
> When you type `ls -l /proc/self/fd` in a shell, **the shell doesn't do the lookup — the `ls` process does** (file 07/08: the shell forked a child and exec'd `ls`). So `/proc/self` resolves to **`ls`'s own directory**, and you see `ls`'s file descriptors — not your shell's. Likewise, `cat /proc/self/status` shows `cat`'s status, not the shell's.
>
> To inspect your **shell** itself, use **`/proc/$$/`**. `$$` is expanded by the shell *before* the child ever runs (file 06's expansion pipeline, file 04's `$$`), so the child receives a literal number — your shell's real PID.

Files 10 and 17 originally said `/proc/self` shows "your own shell's" fds and environment. That was wrong for exactly this reason, and both have been corrected to use `/proc/$$/`.

---

## 4. System-wide entries

Not tied to any one process:

- **`/proc/cpuinfo`, `/proc/meminfo`** — hardware and memory information.
- **`/proc/loadavg`, `/proc/uptime`** — what `uptime` and `top`'s header line actually read.
- **`/proc/sys/`** — tunable kernel parameters (the `sysctl` interface), such as `/proc/sys/kernel/randomize_va_space` (file 21's ASLR mode). Unlike most of `/proc`, these are often **writable**: with sufficient privilege, writing to one of these files **changes kernel behavior at runtime** — a genuinely unusual property for something that looks like an ordinary file.

**How `ps` and `top` actually work** (files 22, 25, 31): there is no special "list all processes" syscall they call. They **read files under `/proc`** — `/proc/<pid>/stat`, `/proc/<pid>/status`, and so on — exactly as any ordinary program would read files, then format the results. `ps` is, in essence, a very well-formatted loop of `cat`s.

---

## 5. macOS: there is no `/proc`

File 10 promised this file would cover the difference. macOS has **no `/proc` filesystem at all**. Process inspection uses different mechanisms: the `sysctl` interface (`kern.proc.*`), the `libproc` APIs, and tools like `ps`, `lsof`, `vmmap` (roughly the analog of `/proc/<pid>/maps`), and `dtruss` (roughly the analog of `strace`, though often restricted by System Integrity Protection). If you're following this handbook's `/proc` examples on a Mac, they simply won't work — full comparison in file 43.

---

## 6. Common misconceptions

| Misconception | Reality |
|---|---|
| "`/proc` is a directory of real files on disk." | A pseudo-filesystem — content is generated by the kernel at read time. |
| "`/proc/self` refers to my shell." | It refers to whichever process performs the lookup — usually the command you just ran, not the shell that launched it. |
| "`/proc/<pid>/environ` shows the current environment." | Only the initial environment from `execve` — later changes aren't reflected. |
| "`ps` calls a special syscall to enumerate processes." | It reads `/proc` files like any ordinary program. |
| "`/proc` exists on macOS." | It doesn't. |

---

## 7. Try this

> **Try this**
> ```bash
> ls -l /proc/$$/fd       # YOUR SHELL's open fds (note $$, not "self")
> ls -l /proc/self/fd      # ls's OWN fds — compare, and notice the extra fd for /proc itself
> ```

> **Try this — watching per-process `cwd` change**
> ```bash
> ls -l /proc/$$/cwd
> cd /tmp
> ls -l /proc/$$/cwd       # the symlink target changed
> ```
> Direct, empirical confirmation of file 07's argument: the working directory is per-process state, changed by the shell's own `chdir()`. Now try `(cd /var; ls -l /proc/$BASHPID/cwd)` and compare — a subshell (file 04) has its own separate cwd.

> **Try this — catching `ps` reading `/proc`**
> ```
> strace -e openat ps 2>&1 | head -30
> ```
> You'll see `ps` opening `/proc/<pid>/stat` and friends, one process at a time — file 32's technique applied to prove Section 4's claim.

---

## 8. Summary

- `/proc` is a VFS pseudo-filesystem — kernel-generated on read, no disk involved, usable by ordinary tools unchanged.
- Per-process entries (`fd`, `status`, `maps`, `environ`, `cwd`, `exe`, `cmdline`) make files 08, 10, 17, 19, and 21's abstract state directly observable.
- `/proc/self` means "whichever process is doing the lookup" — use `/proc/$$` to inspect your own shell.
- `/proc/<pid>/environ` is the *initial* environment only.
- `ps` and `top` are ordinary programs reading `/proc` — no special process-listing syscall exists behind them.
- macOS has no `/proc`; equivalents exist but are entirely different tools.

---

## What's next

`34_MINI_SHELL_PROJECT.md` — the practical capstone: a minimal shell in C, built in twelve stages, each stage introducing exactly one OS primitive from files 08-16.
