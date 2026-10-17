# 31 — Debugging Tools

> **Where this fits in the stack**
> ```
> You  →  [ DEBUGGING TOOLS ]  →  every layer this handbook has covered
>          ^^^^^^^^^^^^^^^^^^
>          this file
> ```
> Every tool here observes some part of the stack already covered in an earlier file. This file is the organized reference: for each tool — what it observes, what subsystem it reveals, and when to reach for it — pointing back to the mechanism, not re-explaining it.

---

## Process inspection

**`ps -o pid,ppid,pgid,sid,tty,stat,comm`**
- **Observes:** a snapshot of process state, hierarchy, and session info (files 08, 15, 16, 22).
- **Reveals:** PID/PPID relationships, process group and session membership, controlling terminal, current state (`R`/`S`/`D`/`T`/`Z`).
- **Use when:** you need a one-shot answer to "what's running, and how is it related to what else."

**`top` / `htop`**
- **Observes:** live, continuously refreshing CPU/memory usage per process (file 22's scheduler, file 25's timer-driven redraw pattern).
- **Reveals:** who's actually consuming CPU/memory *right now*, updated on an interval, not a static snapshot.
- **Use when:** something is using unexpected resources and you need to watch it change over time.

**`pstree -p`**
- **Observes:** the process tree, visually (files 08, 44).
- **Reveals:** parent/child relationships at a glance, without manually correlating PID/PPID columns yourself.
- **Use when:** you need the shape of a process hierarchy, not just a flat list.

---

## Tracing

**`strace`**
- **Observes:** every **system call** a process makes, with arguments and return values (file 09).
- **Reveals:** the exact kernel-boundary crossings a program performs — precisely the "what happens when I type `ls`" question this entire handbook has been building toward.
- **Use when:** you need to know exactly what a program is asking the kernel to do. Full dedicated treatment: file 32.

**`ltrace`**
- **Observes:** **library calls**, not syscalls.
- **Reveals:** a different layer entirely — file 09's `printf`-vs-`write` distinction, made toolable: `ltrace` shows you the `printf()` calls themselves; `strace` shows you the `write()` calls that eventually result. Genuinely complementary, not overlapping — use both together for the full picture across both layers.
- **Use when:** the syscall trace alone doesn't explain the behavior, and you need to see what's happening at the library level above it.

**`perf`**
- **Observes:** where a program (or the whole system) is actually spending CPU time, via **sampling**.
- **Reveals:** performance hotspots, at far lower overhead than `strace`'s per-syscall tracing — `perf` is built for *performance* investigation, `strace` for *behavior/correctness* investigation.
- **Use when:** something is slow and you need to know where the time is actually going, not what syscalls are being made.

---

## File descriptor / file inspection

**`lsof`**
- **Observes:** open files — and remember, "everything is a file descriptor" (file 10), so this includes open sockets and pipes, not just regular files.
- **Reveals:** either "which process has *this* file open" or "which files does *this* process have open," in either direction.
- **Use when:** debugging `ulimit -n` exhaustion (file 21), or figuring out what's still holding a file open.

**`fuser`**
- **Observes:** overlapping purpose with `lsof`, specifically strong at "which process(es) are using *this* file or mount point right now" (file 19).
- **Reveals:** exactly what's blocking a `umount`, for instance.
- **Use when:** you have a specific file/mount and need to know who's using it, rather than starting from a process.

---

## Network inspection

**`ss`**
- **Observes:** active sockets (the modern replacement for `netstat`) — file 30.
- **Reveals:** connection state, local/remote addresses, for both `AF_INET` and `AF_UNIX` sockets (files 27, 30).
- **Use when:** you need to know what's actually connected to what, right now.

**`ip`**
- **Observes:** network interfaces and routing tables.
- **Reveals:** the network-namespace-level configuration file 29 introduced — `ip netns` specifically lets you inspect or enter a container's isolated network namespace directly.
- **Use when:** debugging interface/routing configuration, especially across namespace boundaries.

---

## Kernel and system logs

**`dmesg`**
- **Observes:** the kernel's own ring buffer — boot messages, hardware events, OOM-killer activity, kernel-level errors.
- **Reveals:** what the kernel itself has logged, independent of any particular process (preview: file 41's boot sequence).
- **Use when:** something looks like a hardware or kernel-level problem, not an application one.

**`journalctl`**
- **Observes:** systemd's integrated log store (file 28).
- **Reveals:** both service-specific logs (`journalctl -u servicename`) and system-wide logs, in one unified tool.
- **Use when:** investigating a systemd-managed service's behavior or crash history.

---

## Binary / ELF inspection

**`file`**
- **Observes:** a file's actual content, not its name or extension.
- **Reveals:** its real type — "ELF 64-bit LSB executable," "ASCII text," and so on.
- **Use when:** you're not sure what kind of file you're actually looking at.

**`ldd`**
- **Observes:** a binary's shared library dependencies (file 08's dynamic linking).
- **Reveals:** exactly which `.so` files it needs, and whether they're found.
- **Use when:** debugging a "shared library not found" failure, or auditing what a binary actually links against.

**`readelf`**
- **Observes:** ELF headers, program headers, section headers, in detail (file 08).
- **Reveals:** the exact structure file 08 described in prose — entry point, segments, sections — made directly inspectable.
- **Use when:** you need to verify something about a binary's actual ELF structure, not just its behavior.

**`objdump`**
- **Observes:** a binary's actual machine instructions (disassembly), or intermediate assembly (file 08's compilation pipeline).
- **Reveals:** what the compiler/linker actually produced, at the instruction level.
- **Use when:** behavior can only be explained by looking at the actual generated code.

**`nm`**
- **Observes:** symbols (function/variable names) defined or referenced in an object file or binary.
- **Reveals:** the linker/symbol-resolution picture from file 08 — what a binary provides, and what it still needs resolved.
- **Use when:** debugging a linking failure, or checking whether a symbol exists in a library at all.

---

## Interactive debugging

**`gdb`**
- **Observes:** a running (or launched) program at the deepest level available outside kernel debugging — breakpoints, memory, registers, single-stepping.
- **Reveals:** exactly what a program is doing at any chosen instant, down to individual instructions if needed.
- **Use when:** `strace`/`ltrace` show *that* something happened but not *why* — you need to actually stop the program and inspect its internal state.

---

## Timing

**`time`** (shell builtin, file 04) **vs `/usr/bin/time`** (a separate, external program — file 07's PATH/builtin distinction, directly applicable here)
- **Observes:** both measure how long a command took.
- **Reveals:** `/usr/bin/time` typically reports substantially more — max memory used, page faults, context switches (files 21, 22) — since it isn't limited to whatever the shell's own builtin chooses to expose.
- **Use when:** the shell builtin's basic real/user/sys timing is enough, use `time`; when you need memory/fault/context-switch detail, explicitly invoke `/usr/bin/time` (note the full path — otherwise the shell builtin usually wins per file 07's resolution order).

---

## Common misconceptions

| Misconception | Reality |
|---|---|
| "`strace` and `ltrace` show the same information." | Different layers entirely — syscalls vs library calls — complementary, not redundant. |
| "`top`'s CPU percentage is a precise, instantaneous value." | A sampled average over some interval, not a literal snapshot at one instant. |
| "`time` and `/usr/bin/time` give identical output." | Meaningfully different detail levels — the external binary reports far more. |

---

## Try this

> **Try this**
> ```
> strace -c ls
> lsof -p $$
> ldd /bin/ls; file /bin/ls; readelf -h /bin/ls
> ```
> Three different tools converging on the same binary from three different angles — dependencies, type, and ELF structure, all in one small combo.

---

## Summary

- Every debugging tool here observes a specific layer this handbook already explained — the tool is only as useful as your understanding of what it's showing.
- `strace`/`ltrace`/`perf` form a depth progression: syscalls, library calls, and CPU-time sampling respectively.
- `lsof`/`fuser` answer the same broad question ("what's using this?") from opposite directions.
- Binary-inspection tools (`file`/`ldd`/`readelf`/`objdump`/`nm`) each expose one specific piece of file 08's ELF/linking picture.

---

## What's next

`32_STRACE.md` gives the tool this file only introduced its full dedicated treatment — `ls`, `cat`, and a full pipeline, traced syscall by syscall and mapped back to the conceptual model built across the entire handbook.
