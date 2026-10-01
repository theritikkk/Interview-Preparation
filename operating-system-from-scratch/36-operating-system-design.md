# 36 — Operating System Design

## Layer map for this file

```
┌─────────────────────────────────────────────────┐
│ 1. BOOK: an implicit design choice, never       │
│    named — pure, undifferentiated ring-0 code,  │
│    and an explicit aspiration (Ch.1): Minix     │
├─────────────────────────────────────────────────┤
│ 2. CORE CONCEPT: the monolithic/microkernel     │
│    spectrum, and the real cost each trade       │
│    makes                                        │
├─────────────────────────────────────────────────┤
│ 3. MODERN: seL4, and why monolithic won the     │
│    mainstream                                   │
└─────────────────────────────────────────────────┘
```

Book coverage: **Seed**. The book never discusses "OS design" as a
topic, but it makes a design choice constantly, by never doing
anything else — and it names its own aspiration directly.

## Prerequisites

Files `01`–`35`. This is a synthesis chapter, best read after
everything else.

---

## [BOOK] The Choice the Book Never Names

Every file since `13` has built one thing: a single flow of execution,
at ring 0, in one address space, with drivers (file `16`) linked
directly alongside kernel logic with no boundary between them at all.
That's a design choice — the most extreme possible version of
**monolithic** — even more so than Linux, which at least gains process
isolation (file `19`) and paging (file `22`) once those exist. The
book's toy kernel never isolates anything from anything else, internally.

## [BOOK] The Aspiration the Book States Directly

File `01` already quoted it: Blundell frames this book as a stepping
stone toward real projects **"like Minix."** Worth taking seriously —
Minix is not just "another OS," it's the most famous **microkernel**
teaching operating system in history, built by Andrew Tanenbaum for
exactly the purpose this book serves: to teach OS concepts hands-on.
The book you've been studying points, by its own stated aspiration,
toward the opposite end of the design spectrum from what it actually
builds.

---

## [CORE CONCEPT] The Spectrum

![The monolithic-microkernel spectrum from unikernel through monolithic, hybrid, microkernel, to exokernel, trading isolation against speed, with the book's own toy OS marked as even more monolithic than Linux, and Minix marked at the microkernel end as the book's own stated aspiration](images/36-kernel-spectrum.svg)

- **Monolithic** (Linux, and your toy kernel) — drivers, filesystems,
  the network stack, all run in the kernel's own address space, at
  ring 0. A driver bug can corrupt anything. Calling a driver is just
  an ordinary function call — essentially free.
- **Microkernel** (Minix, seL4) — the kernel itself is kept to an
  absolute minimum: basic IPC, basic scheduling, basic memory
  management. Drivers, filesystems, and most of what Linux calls
  "kernel code" run as **isolated user-space processes** instead,
  talking to each other and to the kernel via message passing. A
  driver crashing takes down only that driver's process — genuinely
  better fault isolation, file `19`'s process boundary now protecting
  subsystems from each other, not just applications from each other.
- **The real cost**: every "driver call" in a microkernel is a full
  message send plus a context switch (files `20`, `21`) between
  processes, instead of a plain function call within one address
  space. More isolation, measurably less speed — not a free lunch
  either direction.
- **Hybrid** (Windows NT, macOS's XNU) — mostly monolithic for
  performance, with some microkernel-inspired internal structuring; a
  pragmatic middle point rather than a pure design.
- **Unikernel** (file `01` named this already) — the opposite move
  from microkernel in one sense (no internal isolation at all, one
  single linked binary) but the same instinct in another (keep "the
  kernel" as thin as possible, specialized to exactly one application).
- **Exokernel** — the most extreme minimalism of all: expose raw
  hardware resources nearly directly to applications, letting each one
  implement its own OS-level policy rather than trusting a shared
  kernel abstraction. Largely a research design, worth knowing exists
  at the far end of the spectrum.

---

## [RELATED KNOWLEDGE] seL4: Proving a Microkernel Correct

**seL4** is a microkernel with a genuinely rare property: its
implementation has been **formally, mathematically verified** against
a specification, proving entire classes of bugs (certain crashes,
certain security violations) cannot occur, rather than merely testing
for their absence. Real, production use in high-assurance contexts
(aviation, defense) exists specifically because this guarantee is
worth the engineering cost — a direct, modern answer to exactly the
reliability case for microkernels this file describes.

---

## [MODERN SYSTEM] Why Monolithic Won the Mainstream

This isn't a live technical dispute — it's settled, well-documented
history, usefully summarized as: Linux (monolithic) overwhelmingly won
general-purpose adoption over Minix and other microkernels
(Tanenbaum and Torvalds publicly debated the tradeoff directly in the
early 1990s), largely because the performance cost of pervasive
message-passing was severe on the hardware of that era, and monolithic
systems proved "good enough" in practice on reliability while being
clearly faster. **Loadable kernel modules** (file `25`) turned out to
capture a meaningful share of microkernels' flexibility benefit —
adding code to a running kernel without a rebuild — without paying
the full IPC cost for every single call, which blunted one of
microkernels' strongest practical arguments.

---

## Practical Experiment

1. Pick any one driver from file `16` (the screen driver) and sketch,
   on paper, what it would look like structurally as an isolated
   microkernel process instead: what messages would it need to
   receive, what would it send back, and to whom.
2. Research Minix's current architecture (it's a real, maintained OS) and
   compare its driver model directly against what you built in files
   `16`/`17` — note what Minix does differently specifically to achieve
   fault isolation.
3. Read a short account of the actual 1992 Tanenbaum–Torvalds Usenet
   exchange — it's a well-documented piece of OS history, directly
   relevant given this book's own stated aspiration toward Minix.

## Debugging

This file reframes a debugging instinct from file `25`: when a crash's
blast radius matters as much as its cause, the design in question
matters. "A driver crashed and corrupted something unrelated" is a
design-level fact about a monolithic system (yours, and Linux's), not
a bug to fix in that one driver — true isolation would require a
different kernel architecture entirely, not a better driver.

## Source Code

Your microkernel-structure sketch for the screen driver — the
closest this file comes to new code, intentionally conceptual.

---

## Common Misconceptions

- **"Microkernels are strictly better because they're more reliable."**
  They trade real performance for that reliability — not a strictly
  better design, a different point on a real tradeoff curve.
- **"Linux has no microkernel-like properties at all."** Loadable
  modules (file `25`) capture some of the same flexibility benefit,
  without full process isolation between them.
- **"The book's toy OS is 'a simple version of Linux.'" ** It's more
  monolithic than Linux — no process isolation, no paging-based
  protection — and its own stated aspiration points toward Minix, the
  opposite design philosophy, not toward Linux at all.

## Interview Questions

**Basic:** What's the core structural difference between a monolithic
kernel and a microkernel?
**Intermediate:** Why is a microkernel's isolation advantage not "free" —
what does it actually cost?
**Advanced:** How do loadable kernel modules blunt one of the classic
arguments for microkernels?
**Systems Design:** Where would you place a unikernel and an exokernel
on the isolation-vs-speed spectrum, and why do they sit at different
ends despite both being "minimal"?
**Debugging:** A driver bug corrupts unrelated kernel memory — what
does that failure mode tell you about the system's architecture, not
just the driver?
**Explain what happens when...:** a device driver crashes on a
microkernel like Minix, versus on a monolithic kernel like Linux?

## Active Recall

1. What design choice does the book make implicitly, and what does it
   explicitly aspire toward instead?
2. What's the real cost of a microkernel's stronger isolation?
3. What does seL4 prove that most kernels only test for?
4. Why did loadable kernel modules weaken the case for microkernels?

## Answers

1. The most extreme form of monolithic design (one address space, ring
   0, no internal isolation at all); it explicitly names Minix, a
   microkernel, as its own stated aspiration.
2. Every cross-subsystem call becomes a full message-passing operation
   and context switch, instead of a plain function call — real,
   measurable overhead.
3. That certain classes of bugs (specific crashes, specific security
   violations) are mathematically impossible in its implementation,
   rather than merely untested-for.
4. They let code be added to a running kernel without a full rebuild —
   one of microkernels' practical advantages — without paying the full
   IPC overhead of true process-level isolation.

---

## What You Should Now Understand

- The book's own, never-stated design choice, made explicit.
- The real spectrum from unikernel to exokernel, and the genuine
  performance/isolation tradeoff at its center, not just labels.
- Why Minix — the book's own aspiration — represents a fundamentally
  different design philosophy than what the book actually builds.
- Where seL4 and loadable kernel modules each sit in this picture today.

---

**Next:** file `37-building-a-simple-os.md` — the checkpoint marking
exactly where Blundell's draft stops.
