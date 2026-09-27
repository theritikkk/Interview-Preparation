# 20 — Scheduling and Context Switching

## Layer map for this file

```
┌────────────────────────────────────────────────┐
│ 1. BOOK: none. §7.2 "Multi-processing" is a    │
│    section heading with zero body text         │
├────────────────────────────────────────────────┤
│ 2. BEYOND THE BOOK: the context switch itself  │
│    — the single most concrete "beyond the      │
│    book" mechanism in this whole course        │
├────────────────────────────────────────────────┤
│ 3. MODERN: Linux's CFS/EEVDF, multi-core       │
│    scheduling                                  │
└────────────────────────────────────────────────┘
```

**Book coverage: Stub.** One disambiguation worth making explicit
before anything else: the book's own heading, "multi-processing,"
almost certainly means **multitasking** — several processes
time-sharing one CPU — not **multiprocessing** in the modern sense of
multiple physical cores (file `24`, a completely separate later
topic). The book's whole world assumes one CPU throughout.

## Prerequisites

Files `01`–`19`.

---

## [BOOK] What's Actually Here

Nothing — confirmed directly against the source text. File `19`
covered the conceptual side (what a process even is); this file builds
the mechanism the book's own stub heading promised and never wrote a
single line toward.

---

## [BEYOND THE BOOK] The Whole Trick: Swap Which Stack You Pop From

This is worth building up to slowly, because the payoff is genuinely
elegant. File `05` established that `pusha` saves every general
register onto the *current* stack. File `19`'s Process Control Block
needs to save a process's entire state somewhere durable. Put those
two ideas together: **if you push every register a process needs onto
its own stack, then the PCB doesn't need to store the registers at
all — it only needs to remember the stack pointer.**

![Context switch as swapping stack pointers: Process A's stack and Process B's stack each hold their own saved registers (eflags, cs, eip, eax...); switching means saving the current esp into PCB_A, loading esp from PCB_B, and popping — the CPU resumes B exactly where it last stopped](images/20-context-switch.svg)

A context switch, then, reduces to almost nothing:

```
switch(from_pcb, to_pcb):
    push everything (eflags, cs, eip*, general registers) onto the current stack
    from_pcb.esp = current esp
    esp = to_pcb.esp
    pop everything back off  — but now it's the OTHER process's saved state
```

*(`eip` specifically is captured via a `call` into the switch routine
itself, and restored via its matching `ret` — the exact `call`/`ret`
mechanism file `05` built, now doing double duty as a process-resume
mechanism.)* The CPU has no idea a switch happened at all — it just
executes whatever instruction comes next, which now belongs to a
different process, resumed exactly where it last left off.

## [BEYOND THE BOOK] From Manual Switching to Preemption

The simplest possible version is **cooperative**: a process calls
`yield()` itself, which performs the switch above — exactly how early
Windows and classic Mac OS worked, and exactly why one badly-behaved
program could freeze an entire cooperative system: nothing forces it
to ever call `yield()`.

The real fix is **preemptive multitasking**: a hardware timer — the
**PIT (Programmable Interval Timer)**, programmed via I/O ports (file
`15`) to fire IRQ0 at a fixed frequency — triggers the switch from
*inside its own interrupt handler* (file `14`), with or without the
running process's cooperation. The process never calls anything; it's
simply interrupted, its state saved by the ISR exactly as above, and a
different process resumed.

## [BEYOND THE BOOK] Round-Robin: the Simplest Real Scheduler

Once switching works, *choosing who runs next* is a separate,
independent decision. **Round-robin** — a circular list of Ready
processes (file `19`'s state diagram), each given a fixed time slice
before the timer forces a switch to the next one in the list — is the
natural first policy: simple, fair by construction, and needs nothing
more than the mechanism already built above.

---

## [RELATED KNOWLEDGE] Cooperative vs. Preemptive, as History

This isn't just a design choice — it's a real historical dividing
line. Classic Mac OS and Windows 3.1 were cooperative; Windows 95
(partially) and Windows NT, along with Unix from very early on, were
preemptive. The difference is exactly this file's mechanism: whether a
misbehaving process *can* be forcibly interrupted at all.

---

## [MODERN SYSTEM] Linux's Real Scheduler

Round-robin treats every process identically — no priorities, no
fairness weighting. Linux's scheduler (**CFS**, the Completely Fair
Scheduler, for years, superseded by **EEVDF** in recent kernels) tracks
each task's accumulated virtual runtime and always picks the task
that's received the least CPU time so far, weighted by priority — far
more sophisticated, but built on exactly the same context-switch
primitive this file just constructed. **Multi-core scheduling** gives
each CPU core its own run queue, with periodic load balancing moving
tasks between cores (file `24`) — a dimension this book's single-CPU
world never has to consider at all.

---

## [ADVANCED / BEYOND THE BOOK] Real-Time Scheduling

Some workloads need *guaranteed* response times, not just fairness —
real-time scheduling policies (fixed-priority preemptive scheduling,
deadline scheduling) trade fairness for predictability. **Priority
inversion** — a low-priority task holding a resource a high-priority
task needs, effectively blocking it — is a famous, real bug class this
opens up (notoriously, on the Mars Pathfinder mission).

---

## Practical Experiment

1. Build the simplest possible version first: two functions that each
   loop forever, calling a hand-written `yield()` that manually swaps
   between two hardcoded stacks — no timer yet, purely cooperative.
2. Add the PIT: program it (file `15`'s port I/O) to fire IRQ0 at a
   visible rate (a few times a second), and perform the switch from
   inside the timer ISR instead of via `yield()` — now it's preemptive.
3. Extend from two processes to three or four in a round-robin list;
   print each process's ID as it runs to visually confirm fair rotation.
4. On Linux: `chrt -p <pid>` to see a real process's scheduling policy
   and priority; compare a normal process against one run with
   `chrt -f 50` (real-time, if you have permission).

## Debugging

A context switch that corrupts state (a process resumes with garbage
registers, or crashes immediately after being switched back to) is
almost always a **stack discipline** bug: pushing and popping in
mismatched order, or switching stacks before every register that
matters has actually been saved. Verify by dumping both stacks'
contents right at the switch point and confirming, by hand, that
what's about to be popped is exactly what was pushed for that process
last time.

## Source Code

Your cooperative two-task switch, then its preemptive PIT-driven
upgrade, then the round-robin extension — a genuine, staged, from-scratch
build with no book original to check against at any step.

---

## Common Misconceptions

- **"The book's 'multi-processing' means multiple CPU cores."** Almost
  certainly means multitasking on one CPU — multi-core (file `24`) is
  a separate, later topic the book's world never touches at all.
- **"A context switch needs to save registers into a special
  structure."** The elegant version needs only a stack pointer — the
  registers are already saved, on the process's own stack, via
  ordinary pushes.
- **"Cooperative multitasking is just an inferior version of
  preemptive."** It's simpler and lower-overhead when every program is
  trusted to behave — the real cost only shows up when one doesn't.

## Interview Questions

**Basic:** What's the difference between cooperative and preemptive multitasking?
**Intermediate:** Why does a context switch only need to save a stack
pointer, if the stack itself already holds every register?
**Advanced:** What role does the PIT play in making multitasking
preemptive rather than cooperative?
**Systems Design:** Why does Linux use a virtual-runtime-based
scheduler instead of simple round-robin?
**Debugging:** A process resumes after a context switch with corrupted
register values — what's the most likely cause?
**Explain what happens when...:** a timer interrupt fires while process
A is running, and process B resumes instead — trace every step?

## Active Recall

1. What does the book's "multi-processing" heading most likely mean,
   and what does it not mean?
2. What's the one field a PCB needs to store, given the stack-based
   context switch design?
3. What makes multitasking preemptive rather than cooperative?
4. What's round-robin scheduling's core policy?

## Answers

1. Multitasking — multiple processes time-sharing one CPU — not
   multiple physical cores (a separate, later topic).
2. The stack pointer — every register is already saved on the
   process's own stack via ordinary pushes.
3. A hardware timer (the PIT) forcibly triggers the switch from inside
   its own interrupt handler, regardless of whether the running
   process cooperates.
4. Give each Ready process a fixed time slice, cycling through them in
   a fixed circular order.

---

## What You Should Now Understand

- Exactly how thin the book's coverage is here (nothing) — and the
  correct reading of what its own stub heading would have meant.
- The context switch as a stack-pointer swap — the single most elegant
  mechanism in this entire course, built directly on file `05`'s
  `pusha`/`call`/`ret`.
- Cooperative vs. preemptive multitasking, and the PIT's role in
  making the latter possible.
- Round-robin as the simplest real scheduling policy, and where
  Linux's actual scheduler goes further.

---

**Next:** file `21-system-calls-and-user-kernel-boundary.md` — nothing
in the book even gestures at this one; entirely new ground.
