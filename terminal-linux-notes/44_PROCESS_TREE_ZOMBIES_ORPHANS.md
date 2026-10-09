# 44 — Process Tree, Zombies, and Orphans

> **Where this fits in the stack**
> ```
> PID 1  →  sshd  →  shell  →  vim / gcc  →  ...      (every process has exactly one parent)
>           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
>           this file
> ```
> Files 08, 29, and 31 each deferred zombies, orphans, and the process tree to a later file. This is that file — the last piece of process lifecycle: what the whole tree looks like, what happens when a parent leaves early, and why a dead process can linger.

---

## 1. The process tree

Every process except PID 1 has exactly one parent (file 08's PPID), so all processes form a single tree rooted at PID 1 (file 41):

```
systemd (PID 1)
 ├── sshd
 │    └── sshd (your session)
 │         └── bash
 │              ├── vim
 │              └── gcc
 ├── cron
 └── ...
```

Kernel threads (the `[bracketed]` names in `ps`) hang off `kthreadd`, PID 2, separately from the userspace tree.

**Observing it:**

```bash
pstree -p           # the tree, with PIDs
pstree -s $$        # the path from PID 1 down to YOUR shell
ps -ef              # every process; the PPID column encodes the tree
ps -eo pid,ppid,stat,comm --forest    # Linux/procps: ASCII-art tree in the comm column
```

`ps -ef`'s columns: `UID PID PPID C STIME TTY TIME CMD`. Reading the `PPID` column and following it upward is exactly walking the tree yourself — `pstree` just draws it for you.

---

## 2. Orphan processes

An **orphan** is a process whose parent exited before it did. It keeps running — nothing about losing a parent terminates a process — but its parent relationship has to be reassigned, because every process must have one.

**The kernel re-parents it** — traditionally to PID 1, which is the designated orphan reaper (file 41), so someone always exists to `wait()` for it when it eventually exits. More precisely, it's re-parented to the nearest ancestor that has declared itself a **subreaper** (via `prctl(PR_SET_CHILD_SUBREAPER)`, available since Linux 3.4), falling back to PID 1 if none exists. In practice, on a desktop system an orphan's new `PPID` is often a per-user `systemd --user` instance rather than literally `1`.

```bash
sleep 300 &            # a child of your shell
echo $$                # note your shell's PID
exit                   # in a throwaway shell — the sleep is now orphaned
# from another terminal:
ps -o pid,ppid,comm -C sleep      # PPID is now 1 (or a subreaper), not the dead shell
```

> **Common mistake**
> "Orphans get killed when their parent dies." They don't — only a controlling-terminal hangup (`SIGHUP`, files 14, 16) can take them down, and only if they were still part of that session's hierarchy. A daemon is, in effect, an *intentional* orphan (file 28's double-fork leaves the daemon orphaned precisely so PID 1 adopts it).

---

## 3. Zombie processes

A **zombie** is a process that has **exited but has not yet been reaped** — its parent hasn't called `wait()`/`waitpid()` for it. It shows up in `ps` with state `Z` and often a `<defunct>` label.

### Why zombies exist at all

When a process exits, the kernel frees its memory, closes its fds, and tears down nearly everything — but it deliberately **keeps a minimal record**: the PID, the exit status (or the signal that killed it), and some resource-usage accounting. Why? Because the parent may not have asked yet, and **an exit status that vanished the instant a child died would make `wait()` impossible to use correctly**: a parent that called `waitpid()` slightly *after* the child exited would find nothing. The zombie is simply that retained record, waiting to be collected (file 08, Section 10; file 42's exit-status path).

```
child exit()  →  [ ZOMBIE: PID + exit status only ]  →  parent waitpid()  →  record freed, PID reusable
```

### What a zombie costs — and doesn't

A zombie consumes **no CPU and essentially no memory** — it's a small kernel structure and a held PID. The real harm is **PID exhaustion**: a parent that spawns children continuously and never reaps them eventually fills the PID table, and `fork()` starts failing with `EAGAIN`.

### You cannot kill a zombie

```bash
kill -9 <zombie pid>    # does nothing — it's already dead
```

A zombie has no code left to deliver a signal *to*. The only ways to clear it:
1. **Make the parent reap it** — a parent that merely forgot to `wait()` can be nudged with `SIGCHLD`, though a buggy parent often ignores that.
2. **Kill the parent.** The zombie becomes an orphan, is re-parented to PID 1 (or a subreaper), which immediately reaps it. This is the standard practical fix.

### Making zombies never happen

- A parent that sets `SIGCHLD` to `SIG_IGN` (or uses `SA_NOCLDWAIT`) tells the kernel it doesn't care about child status — the kernel then discards children's records automatically, with no zombie stage.
- A `SIGCHLD` handler that loops `waitpid(-1, ..., WNOHANG)` reaps every finished child (file 34, Stage 9 — exactly why your mini-shell needs one).
- The **double-fork** trick (file 28): the intermediate process exits immediately, orphaning the grandchild, which PID 1 adopts and reaps — so the original parent never accumulates a zombie for it.

---

## 4. Seeing both, concretely

**A zombie:**
```c
#include <stdlib.h>
#include <unistd.h>

int main() {
    if (fork() == 0) exit(0);   /* child exits immediately */
    sleep(60);                   /* parent never calls wait() */
    return 0;
}
```
Run it, then in another terminal: `ps -o pid,ppid,stat,comm` — the child shows state `Z`, `<defunct>`, parent PID pointing at the sleeping process.

**An orphan:**
```c
#include <unistd.h>

int main() {
    if (fork() == 0) { sleep(60); return 0; }   /* child lingers */
    return 0;                                    /* parent exits at once */
}
```
Run it, then `ps -o pid,ppid,comm -C a.out` — the child's `PPID` is no longer the (now-dead) original parent, but `1` or a subreaper.

---

## 5. Common misconceptions

| Misconception | Reality |
|---|---|
| "A zombie is a hung or runaway process." | It's already dead — only a retained PID and exit status, using no CPU and almost no memory. |
| "`kill -9` removes a zombie." | It can't — there's nothing left to signal. Kill or fix the parent instead. |
| "Orphans are terminated when their parent exits." | They keep running; only their parent changes. |
| "Zombies are harmless, so ignoring them is fine." | A parent that leaks them indefinitely can exhaust the PID table and make `fork()` fail. |
| "Orphans always end up under PID 1." | Under the nearest *subreaper* if one exists — on desktop systems often `systemd --user`. |
| "`wait()` is optional bookkeeping." | It's how a parent retrieves exit status at all — and what lets the kernel finally free the child's record. |

---

## 6. Try this

> **Try this**
> ```
> pstree -s $$
> ```
> The full ancestry from PID 1 down to your own shell — this file's tree, drawn for your actual machine.

> **Try this**
> Compile and run the zombie program above, then `ps -o pid,ppid,stat,comm` in another terminal. Then `kill` the parent and run `ps` again — watch the zombie disappear, reaped by its new parent, exactly per Section 3.

---

## 7. Summary

- All processes form one tree rooted at PID 1; `ps`'s PPID column *is* that tree, and `pstree` just draws it.
- An orphan keeps running and is re-parented to the nearest subreaper, else PID 1 — the mechanism that guarantees every process always has someone to reap it.
- A zombie is a dead process whose exit status is retained until its parent `wait()`s — it costs a PID, not CPU or memory, and can't be killed directly.
- Zombies are prevented by reaping (`SIGCHLD` handler, `waitpid`), ignoring `SIGCHLD`, or the double-fork trick.

---

## What's next

That completes the process lifecycle. `40_QUICK_REVISION.md` is the final revision pass over the whole handbook — its closing index lists every file, `00` through `44`.
