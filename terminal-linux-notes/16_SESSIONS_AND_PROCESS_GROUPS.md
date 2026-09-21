# 16 — Sessions and Process Groups

> **Where this fits in the stack**
> ```
> User  →  Terminal Emulator  →  PTY/TTY  →  Shell  →  [ SESSIONS / PROCESS GROUPS ]  →  Signals
>                                                        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
>                                                        this file
> ```
> Files 02, 14, and 15 have been using "session," "controlling terminal," and "foreground process group" as working definitions, good enough to follow along with. This file makes every one of them precise — including naming the exact two syscalls, `tcsetpgrp()` and `tcgetpgrp()`, that `fg` and the kernel's line discipline actually use, which nothing in this handbook has named explicitly until now.

---

## 1. The full hierarchy

```
Session (SID = PID of the session leader)
 |
 +-- Process Group A (PGID = PID of its group leader)   <- currently FOREGROUND
 |    +-- process 1
 |    +-- process 2
 |
 +-- Process Group B (PGID = PID of its group leader)   <- background
 |    +-- process 3
 |
 +-- Process Group C                                     <- another background job
      +-- process 4
```

Three levels: a **session** contains one or more **process groups**; a process group contains one or more **processes**. A session has **at most one** controlling terminal, and — at any given moment — **at most one** of its process groups is the foreground group of that terminal. Every other process group in the session is, by definition, background.

---

## 2. Session leader

The process that **creates** a session (by calling `setsid()`) becomes its **session leader**, and the session's ID (**SID**) is set to that leader's own PID. In the ordinary interactive case, your login shell *is* the session leader of your terminal session.

A session leader has one additional, precise privilege: it is the **only** process in the session ever allowed to acquire a **new** controlling terminal.

---

## 3. `setsid()` — and its critical precondition

```c
pid_t setsid(void);
```

Creates a **new session**, with the caller becoming its leader; simultaneously creates a **new process group** (the caller as its leader too); and **detaches the caller from any previous controlling terminal** — the new session starts with none at all.

> **Remember**
> `setsid()` **fails** if the calling process is already a process group leader (its own PID already equals some group's PGID). This is precisely why the standard idiom is **fork first, then call `setsid()` in the child** — a freshly forked child is guaranteed *not* to be a group leader yet, since its brand-new PID couldn't already be anyone's PGID.

```c
pid_t pid = fork();
if (pid > 0) exit(0);      /* parent exits immediately */
/* only the child continues from here */
setsid();                   /* guaranteed to succeed — the child is fresh, not a group leader */
/* ... file 28 covers the rest of the full daemonizing recipe */
```

---

## 4. Controlling terminal, made precise

A session's controlling terminal is established the **first time** its session leader opens a terminal device (without the `O_NOCTTY` flag) that isn't already the controlling terminal of some *other* session. Once set, it persists for that session (barring explicit detachment), and it's this exact link that underlies: `/dev/tty` resolving to it (file 02), `SIGHUP` being sent to the session leader when the terminal "hangs up" (files 01, 14), and the foreground-process-group mechanism below.

---

## 5. `tcsetpgrp()` / `tcgetpgrp()` — the actual API

```c
pid_t tcgetpgrp(int fd);              /* the PGID currently foreground on fd's controlling terminal */
int   tcsetpgrp(int fd, pid_t pgrp);  /* set it */
```

This is precisely what `fg` (file 15) calls to hand the terminal's foreground slot to a different job's process group — and what the shell calls on **itself** once a foreground job finishes, to reclaim that slot. Each terminal device tracks exactly **one** "foreground process group" value, settable only by a process belonging to that terminal's own session — a deliberate restriction, so an unrelated process can't hijack an arbitrary terminal's foreground status.

> **Interview answer**
> "The foreground process group isn't an abstract concept — it's a literal value the kernel tracks per terminal device, readable and settable via `tcgetpgrp`/`tcsetpgrp`. The line discipline consults exactly this value when deciding which process group to deliver `SIGINT` or `SIGTSTP` to on `Ctrl+C`/`Ctrl+Z`."

**`SIGTTIN`/`SIGTTOU`, now precise:** if a *background* process group's member calls `read()` on its session's controlling terminal, the kernel checks whether that process's group currently matches `tcgetpgrp()`'s value for that terminal. If not, the reading process is sent `SIGTTIN` (default: stop) instead of being allowed to read at all — the exact mechanism behind "a background job trying to read input gets suspended," mentioned in file 15.

> **Common mistake**
> "Background processes are blocked from both reading *and* writing their terminal." **Reads are unconditionally blocked** this way. **Writes are usually allowed through** — `SIGTTOU` only fires on a background write if the terminal's `TOSTOP` option is specifically enabled, which is not the default. The asymmetry is real and worth remembering precisely.

---

## 6. Why this matters — five concrete cases

**Terminal emulators:** when your terminal emulator forks a child destined to become your shell (file 01), that child — before `exec`ing into the shell — calls `setsid()` (a fresh fork, so guaranteed not yet a group leader) to become leader of a **brand-new session**, then opens its PTY slave to establish it as that new session's controlling terminal. This is exactly how every terminal window gets its own fully independent session — `Ctrl+C` in one window can never reach a job running in a different one.

**SSH:** `sshd`, on the server side, does the equivalent for each incoming session needing a shell: allocates a new PTY pair (file 02, full depth file 26), forks a child that calls `setsid()` and connects to the new PTY slave as its controlling terminal, *then* execs the user's shell. Every SSH connection gets its own genuinely independent session hierarchy on the remote machine — architecturally identical to opening a local terminal window.

**Daemons:** the classic double-fork-and-`setsid()` recipe (full version: file 28) exists specifically to detach a background service from **any** controlling terminal at all. A properly daemonized process has none — so it can never receive `SIGHUP` from some unrelated terminal closing, and can never be suspended by `SIGTTIN`/`SIGTTOU` from innocently touching a terminal it was never meant to be connected to.

**tmux:** the **tmux server** itself calls `setsid()`, detaching from whatever terminal started it — so it keeps running completely independent of any one terminal. Each **attached client**, and the shells/panes running inside it, form their own nested structure *under* whichever terminal is currently attached. This is precisely the mechanism that lets you detach a tmux session without killing anything inside it: the server (and everything it's running) was never actually part of that terminal's session hierarchy — only the temporarily-attached client was. Full depth: file 27.

**systemd:** as PID 1 (preview: file 41), systemd itself has no controlling terminal from the moment the kernel starts it — there's no terminal involved in its own creation at all. Services it manages inherit this "no controlling terminal" property by default, which is *exactly* the properly-daemonized state the old double-fork dance used to have to construct by hand — a concrete, specific reason modern systemd-managed services don't need to implement their own daemonizing logic anymore. Full contrast: file 28.

---

## 7. Common misconceptions

| Misconception | Reality |
|---|---|
| "Every process has its own session." | Many processes typically share one — everything descending from a single login or terminal. |
| "`setsid()` can detach any process from its terminal at any time." | It fails outright if the caller is already a process group leader — fork-then-`setsid()` is the required idiom. |
| "'Foreground process group' is a loose descriptive phrase." | It's a literal kernel-tracked value per terminal, read/set via `tcgetpgrp`/`tcsetpgrp`, and exactly what the line discipline consults for signal targeting. |
| "Background processes can't read or write their terminal." | Reads are unconditionally blocked (`SIGTTIN`); writes go through by default unless `TOSTOP` is explicitly set. |

---

## 8. Try this

> **Try this**
> ```
> ps -o pid,ppid,pgid,sid,tty,comm
> ```
> Your own shell's PID, parent, process group, session ID, and controlling terminal, all in one line.

> **Try this**
> ```bash
> sleep 100 &
> ps -o pid,pgid,comm
> ```
> Compare the backgrounded job's PGID against your shell's own — they differ, exactly as Section 1's diagram shows.

---

## 9. Summary

- The hierarchy is session → process group(s) → process(es); a session has at most one controlling terminal and at most one currently-foreground process group.
- A session leader is whichever process called `setsid()` to create the session; `setsid()` itself only succeeds on a process that isn't already a group leader, which is why daemons fork first.
- `tcgetpgrp`/`tcsetpgrp` are the literal, precise API behind "foreground process group" — not an abstraction, a kernel-tracked value per terminal.
- Reads from a background process group unconditionally trigger `SIGTTIN`; writes only trigger `SIGTTOU` if `TOSTOP` is set.
- Terminal emulators, SSH, daemons, tmux, and systemd all lean on `setsid()` and controlling-terminal detachment for the same underlying reason: cleanly isolating one session's signals and terminal ownership from another's.

---

## What's next

`17_ENVIRONMENT_AND_PATH.md` moves from process/session structure to the data every process carries with it: `environ`, `argv`/`envp`, and precisely how `export` bridges a shell variable into a child's environment — file 04's claim, now traced at the syscall level.
