# 28 — Daemons and systemd

> **Where this fits in the stack**
> ```
> User  →  ...  →  Sessions  →  [ DAEMONS / SYSTEMD ]  →  Processes
>                                ^^^^^^^^^^^^^^^^^^^^^^
>                                this file
> ```
> File 16 previewed the double-fork daemonizing recipe and named systemd as the modern alternative, without completing either. This file finishes both, and contrasts directly against file 27's tmux — another process that outlives a terminal, but for structurally different reasons.

---

## 1. Interactive, background, daemon, service — precisely distinguished

| Term | Defining property |
|---|---|
| **Interactive process** | Connected to a controlling terminal, in the foreground process group at least some of the time, expects to interact with a human through it (file 15). |
| **Background process** | Launched via `&` — still part of the same session, potentially still with a controlling terminal, just not currently in the foreground. Can still be affected by that terminal closing (`SIGHUP`, unless `nohup`'d — files 14, 16). |
| **Daemon** | No controlling terminal **at all** — fully detached from any terminal's lifecycle, typically started at boot or on demand, intended to run indefinitely, providing an ongoing capability rather than doing one task and exiting (file 16, Section 6). |
| **Service** | Essentially a *managed* daemon — one whose lifecycle (start, stop, restart, enable-at-boot, restart-on-crash, log collection) is handled by a dedicated supervisor, rather than the process bootstrapping and detaching itself. |

---

## 2. The classic double-fork daemonizing recipe, completed

File 16 previewed the first half. Here's the whole thing:

```c
pid_t pid = fork();
if (pid > 0) exit(0);          /* first parent exits */
setsid();                       /* child becomes session leader, no controlling terminal */
pid = fork();                   /* SECOND fork */
if (pid > 0) exit(0);           /* first child ALSO exits */
/* only the grandchild continues from here */
chdir("/");
umask(0);
close(STDIN_FILENO);
close(STDOUT_FILENO);
close(STDERR_FILENO);
/* re-open 0/1/2 to /dev/null or a log file, as appropriate */
/* ... now the actual daemon's real work begins ... */
```

> **Remember — why the second fork specifically**
> After the first `fork` + `setsid()`, the calling process **is now a session leader** (file 16's precise definition). And per file 16's controlling-terminal rule, a session leader is the *one* process type ever allowed to **acquire a new controlling terminal** — if it happens to open a terminal device without `O_NOCTTY`, even accidentally, it could reacquire one. The second `fork` produces a process that is explicitly **not** a session leader — an ordinary member of that session — removing even the *possibility* of accidentally reacquiring a controlling terminal later. This is the precise, often-skipped reason for the second fork, not just "extra caution."

**`chdir("/")`, `umask(0)`, closing and reopening 0/1/2:** each removes an implicit dependency inherited from whatever environment happened to start the daemon (file 17's inheritance model, generalized). Not holding an arbitrary working directory open (which could prevent it from being unmounted later); not inheriting an unexpectedly restrictive or permissive `umask` (file 20); not leaving stdin/stdout/stderr connected to whatever they happened to be at launch — which might not even still exist by the time the daemon is doing real work, much later.

---

## 3. `systemd`

**L1:** the modern init system and service manager on most major Linux distributions — **PID 1** (preview: file 41), the very first userspace process the kernel starts directly, with **no controlling terminal from the moment of its own creation.**

**L2:** systemd manages services via **unit files** — declarative configuration, not shell scripts, describing what to run, when to start it relative to other services, how to restart it on failure, and resource limits (file 21's `ulimit`/`rlimit` discussion — unit files can set these directly, per service). systemd itself starts each service via an ordinary `fork`+`exec` (file 08 — nothing magical about the mechanism itself), **but without the service's own code needing any double-fork/`setsid()` dance at all** — because systemd already ensures the child starts with no controlling terminal in the first place, since systemd itself, PID 1, never had one to pass down.

> **Interview answer**
> "Modern systemd-managed services don't need to implement the double-fork daemonizing dance themselves, because systemd — as PID 1, with no controlling terminal of its own — already provides that property for free to every process it launches. The old recipe existed specifically to construct, by hand, exactly the state systemd's own process-launching mechanics now provide automatically."

Services **can** still self-daemonize the old way if their unit file specifies `Type=forking`, for compatibility with older software not written with systemd in mind — but the modern, preferred pattern (`Type=simple`/`Type=exec`) is for the service's main process to simply run, directly, in the foreground from its own code's perspective, with zero manual daemonizing logic required.

---

## 4. `systemctl`

The command-line tool for interacting with systemd:

```bash
systemctl start sshd
systemctl stop sshd
systemctl restart sshd
systemctl enable sshd
systemctl status sshd
journalctl -u sshd      # systemd's integrated logging (file 31/38 preview)
```

> **Common mistake**
> "`systemctl enable` also starts the service immediately." **Two separate actions.** `enable` means "start this at boot time going forward" — it creates a symlink connecting the unit file to the appropriate boot target — and does **not** itself start anything right now. `start` is the separate action for that. They're frequently used together, but they're not one operation.

---

## 5. Why modern services are managed differently

**The old SysV-init-script pattern:** init scripts were typically shell scripts (file 04) that themselves invoked the double-fork dance, or relied on the daemon's own code to self-daemonize — and the init system had relatively limited visibility into whether a service actually started successfully, was still running, or had crashed and needed restarting. This commonly relied on fragile **PID-file tracking**: a daemon writes its own PID to a file, and other tools read that file later to check on it — a mechanism that can easily go stale or wrong (a crashed process leaving a stale file, a reused PID pointing at something unrelated).

**systemd's approach:** since it is the one directly forking the service's process — rather than the service daemonizing itself away from any supervision — systemd always knows the exact PID, can directly detect when it exits (file 08's `SIGCHLD`/`waitpid` mechanics, received naturally as the actual parent, not inferred from a file), and can automatically restart it per policy, without any of the old fragility.

---

## 6. Common misconceptions

| Misconception | Reality |
|---|---|
| "Daemon and service mean exactly the same thing." | Close, but "service" specifically implies a *supervised, managed* lifecycle; "daemon" is the more general property (no controlling terminal, long-running) that most services also happen to have. |
| "systemd services still need to double-fork themselves." | Exactly what `Type=simple`/`Type=exec` services avoid — systemd's own process creation already provides the "no controlling terminal" property. |
| "`systemctl enable` starts the service right now." | Two separate actions — `enable` affects boot-time behavior only, `start` runs it now. |

---

## 7. Try this

> **Try this**
> ```
> systemctl status sshd
> ```
> (Or any running service on your system.) The main PID, current state, and recent log lines, all in one place.

> **Try this**
> ```
> ps -o pid,ppid,comm 1
> ```
> Confirms `systemd` as PID 1 — or whatever your system's actual init is, if not systemd.

---

## 8. Summary

- Interactive, background, daemon, and service form a precise progression — from "actively driven by a human at a terminal" to "fully supervised, no terminal involved at all."
- The double-fork recipe's second fork exists specifically to strip session-leader status, removing even the possibility of later reacquiring a controlling terminal.
- systemd, as PID 1 with no controlling terminal from birth, gives every service it launches that same property automatically — no manual daemonizing needed.
- `enable` and `start` are genuinely separate actions, commonly confused for one.
- systemd's direct-parent relationship to each service (real PID, real `SIGCHLD`) replaces the old, fragile PID-file-based tracking entirely.

---

## What's next

`29_CONTAINERS.md` connects PID 1's special properties — just covered here for systemd — to what happens when a *container's* PID 1 is the process being supervised instead, and what namespaces and cgroups actually do to make that isolation real.
