# 22 — Scheduler

> **Where this fits in the stack**
> ```
> User  →  ...  →  Processes  →  Memory  →  [ SCHEDULER ]  →  CPU
>                                            ^^^^^^^^^^^^^
>                                            this file
> ```
> File 13 claimed `cat file.txt | grep hello`'s two processes "run concurrently" without saying precisely what that means on real hardware. This file gives the precise answer — and it's genuinely more layered than "each process gets a CPU," which is exactly the oversimplification worth avoiding.

---

## 1. What happens after the shell creates a process?

Once `ls` (say) is running, it's just one more `task_struct` (file 08) in the kernel's full collection of processes — competing for CPU time alongside everything else on the system: your browser, background daemons, everything. The **scheduler** is the kernel subsystem that decides, at any given moment, which of the (potentially hundreds of) runnable processes actually gets a physical CPU core right now, and for how long.

---

## 2. Process states, precisely

| State | Meaning |
|---|---|
| **Runnable** | Wants to execute, *could* execute immediately if given a core — but the scheduler hasn't assigned it one yet. There can easily be more runnable processes than cores. |
| **Running** | Actually executing on a core **right now** — the subset of runnable processes the scheduler picked for this instant. |
| **Sleeping (interruptible, `S`)** | Voluntarily waiting for something (a timer, input, a lock) — not competing for CPU at all while waiting, and can be woken early by an arriving signal. |
| **Blocked (uninterruptible, `D`)** | Waiting for something — typically I/O — in a way that *cannot* even be interrupted by a signal; genuinely stuck until the specific event occurs. |

> **Under the hood**
> Tools like `ps`/`top` typically display "Runnable" and "Running" as one combined `R` state, since the distinction — "waiting for a core" vs "actually on one this instant" — usually isn't what you need at a glance. The kernel absolutely does distinguish them internally; the scheduler's entire job is deciding which runnable processes become running ones.

---

## 3. Context switching

**L1 — intuition:** the kernel pauses one process and resumes another on the same physical core.

**L2 — systems detail:** precisely, this means saving the currently-running process's complete register state — including the instruction pointer, exactly where it was mid-execution — into its `task_struct`, then loading a *different* process's previously-saved register state from its own `task_struct` into the CPU, and switching the MMU's page-table pointer to that process's own address space (file 21). A moment later, the same physical core is executing a completely different program with a completely different view of memory — and from that resumed process's own perspective, nothing happened at all; it picks up exactly where it left off, with no way to tell time passed except by checking a clock.

**This switch has a real, non-zero cost** — largely from flushing and reloading CPU caches and the MMU's translation lookaside buffer (TLB). This is exactly why the scheduler doesn't switch between every runnable process every nanosecond — it gives each a **time slice** (typically single-digit milliseconds) before reconsidering, balancing responsiveness against the overhead of switching itself.

**When a context switch actually happens:**
- The running process's time slice expires (a hardware timer interrupt gives the kernel a chance to reconsider).
- The running process **voluntarily blocks** — calls `read()` and has to wait for data, for instance — no reason to keep it "running" while it can't do anything.
- A higher-priority process becomes runnable (an interrupt wakes something that needs to run urgently).
- The running process exits.

---

## 4. Single-core vs multi-core — precisely, not oversimplified

**On a single-core machine:** true parallelism is physically impossible — only one instruction from one process can ever execute at a given physical instant. "Multiple processes running at once" on such a machine is **entirely an illusion**, produced by context-switching many times per second, each process getting a sliver of time — fast enough that it looks simultaneous to a human, whose own perception operates on a far slower timescale.

**On a multi-core machine** (essentially every machine today): genuine parallelism *is* possible — the scheduler can place different processes on different physical cores, and those really do execute at the exact same physical instant, no illusion involved, for that specific pair.

**But even on a multi-core machine**, if there are more runnable processes than cores — the ordinary case; a typical system has dozens of runnable-ish processes and 4/8/16 cores — the *same* rapid-switching illusion still applies to whichever processes didn't get a physical core this instant.

> **Remember**
> The precise answer is always a **blend**: some apparent concurrency is genuine hardware parallelism (different cores, same instant); some is still the rapid-switching illusion (more runnable work than available cores). From inside a shell pipeline, you cannot know — and should never assume — which situation applies at any given moment; it depends entirely on system load and scheduler decisions outside your control.

---

## 5. Why `cat file.txt | grep foo` really runs "simultaneously"

Recap file 13: `cat` and `grep` are separate, independent processes — separate `task_struct`s — and the **scheduler treats them as two completely ordinary, unrelated runnable entities.** Being connected by a pipe has **no special bearing on how the scheduler treats them** — the pipe only affects their *data flow*, via blocking reads and writes (file 13); it grants no special CPU-scheduling relationship, priority, or core affinity.

**If multiple cores are free:** the scheduler might genuinely run both on different cores at the exact same instant — real parallelism, no illusion.

**If only one core is available** (or the system is busy): the scheduler time-slices between them, and everything else runnable, rapidly alternating who's actually executing at any microsecond-scale instant. But because `cat`'s writes fill the pipe buffer and then it naturally blocks (state `S`/`D`, file 13) while `grep`'s reads drain it — and then `grep` naturally blocks waiting for more — the overall streaming effect from file 13 still emerges, **even under pure time-slicing with zero real parallelism.**

> **Interview answer**
> "A pipeline's concurrency comes from two separate, stacked mechanisms, not one. The pipe's blocking semantics (file 13) determine the *logical* interleaving — correct data ordering, each side naturally yielding when it can't make progress. The scheduler separately determines the *physical* timing of that interleaving — genuine parallelism if cores are free, rapid time-slicing if they aren't, usually some mix of both. Saying 'each process gets a CPU' conflates these two layers and is only sometimes even true."

This is exactly why file 13's streaming behavior works identically whether you're on a single-core embedded device or a 64-core server — the pipe's blocking semantics produce correct behavior regardless of *how* the scheduler happens to be interleaving execution underneath it.

---

## 6. Common misconceptions

| Misconception | Reality |
|---|---|
| "Each process gets its own CPU." | Only true if you happen to have at least as many free cores as runnable processes — not a safe general assumption. |
| "Context switching is free or instantaneous." | Real, measurable cost from cache/TLB effects — exactly why the scheduler uses time slices rather than switching constantly. |
| "On a single-core machine, simultaneous execution is a literal physical fact." | Pure illusion there — genuine simultaneity is physically impossible with one core; only rapid switching creates the appearance. |
| "A pipe creates a special scheduling relationship between the two processes." | No formal relationship at all — the scheduler sees two ordinary, independent runnable entities; the pipe only affects data flow and blocking. |

---

## 7. Try this

> **Try this**
> ```
> nproc
> ```
> How many CPU cores your machine actually has — the number that determines how much of Section 4's "genuine parallelism" is even possible at once.

> **Try this**
> ```bash
> for i in $(seq 1 $(nproc)); do yes > /dev/null & done
> top
> ```
> Launch exactly as many CPU-bound background jobs as you have cores, and watch `top` — each should show close to 100% CPU, genuinely in parallel. Now launch a few *more* than `nproc` and watch them start sharing time instead — a direct, felt distinction between real parallelism and time-slicing. (Remember `kill` them afterward — `jobs`, then `kill %1 %2 ...`, per file 15.)

---

## 8. Summary

- The scheduler decides which runnable processes become running ones, on which cores, at any given moment.
- Runnable, running, sleeping, and blocked are genuinely distinct states, even when tools display the first two as one.
- Context switches carry a real, non-zero cost, which is why the scheduler uses time slices rather than switching continuously.
- True simultaneity requires multiple cores; with more runnable processes than cores, the rapid-switching illusion still fills the gap.
- A pipeline's "simultaneous" execution is two stacked mechanisms — the pipe's blocking semantics (logical interleaving) and the scheduler's core placement or time-slicing (physical timing) — never one single fact, and never guaranteed to be genuine parallelism.

---

## What's next

`23_TERMINAL_LINE_DISCIPLINE.md` returns to the terminal layer for the deepest file in that part of the handbook: `termios`, `tcgetattr`/`tcsetattr`, and — the piece file 02 flagged as a genuine gap — how Bash's Readline actually gives you arrow-key history editing on top of all of this.
