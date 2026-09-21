# 15 — Job Control

> **Where this fits in the stack**
> ```
> User  →  Terminal Emulator  →  PTY/TTY  →  Shell  →  [ JOB CONTROL ]  →  Processes  →  Signals
>                                                        ^^^^^^^^^^^^^^^
>                                                        this file
> ```
> File 14 traced `Ctrl+Z` as far as "`SIGTSTP` reaches the foreground process group." This file picks up exactly there: what makes a process group "foreground" in the first place, what `&` actually changes about the fork/exec sequence from file 08, and the full `jobs`/`fg`/`bg` toolkit built on top of it. The deepest mechanics — `setsid()`, `tcsetpgrp()`, the session hierarchy — get their full treatment in file 16; this file uses working definitions sufficient for job control itself.

---

## 1. Foreground and background jobs

A **job**, in shell terminology, is a process — or an entire pipeline of processes (file 13) — that the shell launched and is tracking, given a small reference number (`%1`, `%2`, ...) for convenience.

- **Foreground job:** connected to the terminal's input, and its process group is currently the terminal's *foreground* process group (file 02/14). You can type into it; `Ctrl+C`/`Ctrl+Z` reach it; the shell blocks in `waitpid()`, showing no new prompt, until it finishes.
- **Background job:** not connected to terminal input (if it tries to read from the terminal anyway, it's suspended via `SIGTTIN` — file 16 covers this precisely); does **not** receive `Ctrl+C`/`Ctrl+Z`, since those only ever reach the foreground process group; the shell does **not** block on it, and shows you a new prompt immediately.

---

## 2. `./program` vs `./program &` — precisely what changes

**`./program`:** the shell `fork()`s (file 08, unchanged mechanics), the child becomes the terminal's foreground process group, and the shell blocks in `waitpid()` until it exits.

**`./program &`:** the shell still `fork()`s — **identically** — but:
1. The new process is placed into its **own, new process group** rather than joining whatever the shell currently considers foreground.
2. That new process group is **explicitly not** made the terminal's foreground process group — the shell's own (or whatever job is currently foreground) stays foreground.
3. The shell **doesn't** block on it — it immediately shows you a new prompt and returns to reading input — while recording this new job in its internal job table for `jobs`/`fg`/`bg` to reference.

> **Remember**
> `&` does not change `fork`/`exec` at all. It changes what the shell does **after** forking: whether it waits synchronously, and how it configures the new process's process-group and terminal-foreground status. Everything from file 08 about `fork`/`execve` applies identically either way.

---

## 3. `jobs`, `fg`, `bg`

- **`jobs`** — lists the shell's currently tracked background/stopped jobs, with job numbers and status (`Running`/`Stopped`).
- **`fg [%N]`** — brings a job to the foreground: makes its process group the terminal's foreground process group (`tcsetpgrp()` — full mechanics, file 16), sends it `SIGCONT` if it was stopped, and the shell now blocks on it exactly like an ordinary foreground command.
- **`bg [%N]`** — resumes a *stopped* job (`SIGCONT`) but **keeps it in the background** — no foreground handoff, no blocking; the shell keeps showing prompts.

---

## 4. The `Ctrl+Z` → `bg` → `fg` cycle, traced

```
$ sleep 100
^Z
[1]+  Stopped                 sleep 100
$ bg
[1]+ sleep 100 &
$ jobs
[1]+  Running                 sleep 100 &
$ fg
sleep 100
```

- `Ctrl+Z` sends `SIGTSTP` (file 14) to the foreground process group → `sleep` stops (process state `T`, file 08).
- The shell notices the state change (via `waitpid(..., WUNTRACED)` or a `SIGCHLD` handler — file 14's `SIGCHLD` entry, now doing real work) and reports `[1]+ Stopped`.
- `bg` sends `SIGCONT` to that job's process group **without** taking the foreground handoff — `sleep` resumes running, but stays in the background, so the shell continues showing prompts undisturbed.
- `fg` reclaims the foreground for it (`tcsetpgrp()`) and the shell blocks on it exactly as if you'd never backgrounded it.

> **Interview answer**
> "`fg` and `bg` don't use any job-control-specific signal — they send the same ordinary `SIGCONT` from file 14's catalog. The only difference between them is whether the shell also reassigns the terminal's foreground process group to that job before returning control."

---

## 5. Process groups, in working terms

A **process group** is a collection of related processes that can be signaled **together, as a unit**. Every job a shell launches gets its own new process group — and every process in *one pipeline* (`cat file | grep x | sort`) is placed in the **same** process group. This is exactly why file 14's `Ctrl+C` example hit every process in `sleep 100 | cat` simultaneously: the signal targets the whole group, not a single PID, and both processes were members of one group.

Having each job in its own process group is also precisely what lets the shell distinguish "job 1" from "job 2" for targeting, and lets it track, independently, which *one* of potentially several concurrent jobs currently holds the terminal's foreground slot.

---

## 6. Common misconceptions

| Misconception | Reality |
|---|---|
| "`&` means 'run this faster' or 'in parallel with itself.'" | It means "don't wait synchronously, and don't make this the terminal's foreground process group" — nothing about execution speed changes. |
| "A backgrounded job is automatically immune to my terminal closing." | Not by default — file 14's `SIGHUP` scenario still applies; `nohup`/`disown` are the deliberate opt-outs, covered fully in file 16. |
| "`fg`/`bg` use some special job-control-only signal." | Plain `SIGCONT`, the same signal from file 14's table — `fg` just additionally reassigns terminal foreground status. |

---

## 7. Try this

> **Try this**
> Work through Section 4's exact cycle yourself: `sleep 100`, `Ctrl+Z`, `bg`, `jobs`, `fg`.

> **Try this**
> ```bash
> sleep 100 &
> jobs
> kill %1
> ```
> Signaling by **job number** rather than PID — `kill` accepts either.

---

## 8. Summary

- A job is foreground if its process group currently holds the terminal's foreground slot; background otherwise.
- `&` doesn't touch `fork`/`exec` — it changes whether the shell waits synchronously and whether the new process group becomes foreground.
- `fg`/`bg` are built entirely on ordinary mechanisms already covered: `SIGCONT` from file 14, and reassigning the terminal's foreground process group.
- Every pipeline shares one process group, which is why a signal like `SIGINT` reaches every process in it at once.

---

## What's next

`16_SESSIONS_AND_PROCESS_GROUPS.md` goes fully precise on everything this file used loosely: `setsid()`, `tcsetpgrp()`/`tcgetpgrp()`, the session/process-group hierarchy, and why this all matters for SSH, daemons, and tmux specifically.
