# Linux Kernel Development — A Complete Learning Course

> Built from *Linux Kernel Development, 3rd Edition* (Robert Love, 2010) as the primary source, expanded into a full teaching course with modern-Linux annotations, diagrams, code, experiments, and active recall. This is not a summary of the book — it is a course that uses the book's structure and terminology as its backbone.

---

## What This Course Is

Robert Love's book is the best conceptual introduction to Linux kernel internals ever written, but it describes **Linux 2.6.x** (kernel versions from roughly 2008-2010). The kernel has changed substantially since then — in some places superficially (renamed functions), in some places structurally (the scheduler has been rewritten twice since this book was published; the versioning scheme itself changed in 2011).

This course does three things the book alone cannot:

1. **Teaches every concept from first principles**, at the depth of intuition -> problem -> design -> data structures -> algorithm -> implementation -> code -> runtime behavior -> debugging -> real-world application — not "Linux uses X," but *why X exists, what breaks without it, and how to observe it yourself.*
2. **Explicitly separates book-era Linux 2.6 from modern Linux**, using this convention everywhere it matters:
   > **[BOOK — Linux 2.6]** what Robert Love describes.
   > **[MODERN LINUX]** how it works today, where this can be established reliably.
   > **[CONCEPTUAL CORE]** what remains true regardless of kernel version.

   If a modern detail cannot be established reliably, the course says so explicitly rather than inventing it.
3. **Forces active engagement** — every chapter includes hands-on experiments on a real Linux system, source-code navigation exercises, debugging scenarios, interview questions, and active-recall questions with answers held back until you've attempted them.

## Who This Is For

You already know how to program in C and want to genuinely understand Linux kernel internals — not just pass a quiz on terminology, but reach the point where you can open an unfamiliar `.c` file in the kernel source tree and reason about what subsystem it belongs to, what execution context it runs in, what locking it needs, and how to trace and debug it.

## Required Environment

> [!WARNING]
> Several experiments in this course involve loading kernel modules, editing kernel parameters, or intentionally triggering faults. **Do these in a virtual machine, never on your primary machine.** A crashed VM costs you a `virsh destroy` and a restore from snapshot. A crashed primary machine costs you your afternoon and possibly your data.

**Recommended setup:**
- **Host:** any machine capable of running a VM (4+ GB RAM allocated to the guest, 2+ vCPUs so you can observe SMP/scheduling behavior).
- **Guest:** **Ubuntu 22.04 LTS or 24.04 LTS**, installed under VirtualBox, KVM/QEMU (preferred if your host is Linux — `virt-manager` is a friendly front end), or UTM (Apple Silicon Macs).
- **Snapshot the VM immediately after first boot and after installing build tools.** Before any experiment marked WARNING KERNEL-MODIFYING or DANGEROUS, take a fresh snapshot.
- **Packages to install up front:**
  ```bash
  sudo apt update
  sudo apt install -y build-essential git bc bison flex libssl-dev libelf-dev \
      linux-headers-$(uname -r) kmod \
      strace ltrace perf-tools-unstable linux-tools-common linux-tools-generic \
      trace-cmd bpftrace bpfcc-tools \
      gdb crash \
      binutils elfutils \
      universal-ctags cscope silversearcher-ag ripgrep \
      pahole
  ```
  Not every package is needed for every chapter — each chapter lists exactly which tools it uses and why.
- **Kernel source, for source-navigation exercises:**
  ```bash
  git clone --depth 1 https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git ~/linux
  ```
  A shallow clone (`--depth 1`) is enough for reading current code with `grep`/`ripgrep`/`cscope`; if you later want `git log`/`git blame` history on a file, `git fetch --unshallow` (multi-GB download, be patient).

## How the Files Are Organized

```
linux-kernel-development-notes/
|
+-- README.md                          <- you are here
+-- 00-learning-roadmap.md             <- phases, dependency graph, how to use this course
+-- 01-introduction-to-linux-kernel.md <- Chapter 1 (complete)
+-- 02-getting-started-with-kernel.md  <- Chapter 2 (upcoming)
+-- 03-process-management.md           <- Chapter 3 (upcoming)
+-- 04-process-scheduling.md           <- Chapter 4 (upcoming)
+-- 05-system-calls.md                 <- Chapter 5 (upcoming)
+-- 06-kernel-data-structures.md       <- Chapter 6 (upcoming)
+-- 07-interrupts.md                   <- Chapter 7 (upcoming)
+-- 08-bottom-halves.md                <- Chapter 8 (upcoming)
+-- 09-kernel-synchronization-introduction.md   <- Chapter 9 (upcoming)
+-- 10-kernel-synchronization-methods.md        <- Chapter 10 (upcoming)
+-- 11-timers-and-time-management.md   <- Chapter 11 (upcoming)
+-- 12-memory-management.md            <- Chapter 12 (upcoming)
+-- 13-virtual-filesystem.md           <- Chapter 13 (upcoming)
+-- 14-block-io.md                     <- Chapter 14 (upcoming)
+-- 15-process-address-space.md        <- Chapter 15 (upcoming)
+-- 16-page-cache-and-writeback.md     <- Chapter 16 (upcoming)
+-- 17-devices-and-modules.md          <- Chapter 17 (upcoming)
+-- 18-kernel-debugging.md             <- Chapter 18 (upcoming)
+-- 19-portability.md                  <- Chapter 19 (upcoming)
+-- 20-patches-hacking-community.md    <- Chapter 20 (upcoming)
|
+-- glossary.md                        <- every important term, defined once, cross-referenced
+-- source-code-map.md                 <- kernel source tree tour, updated every chapter
+-- debugging-playbook.md              <- symptom -> hypothesis -> tool -> root cause, per subsystem
+-- interview-questions.md             <- aggregated interview question bank, all chapters
+-- final-knowledge-map.md             <- the whole book as one dependency/concept graph
```

> [!NOTE]
> This is a living course built one chapter at a time, in the same depth throughout. Chapter 1 is complete below. Each subsequent chapter will be delivered as its own file, in the same structure, and the five cross-cutting files (`glossary.md`, `source-code-map.md`, `debugging-playbook.md`, `interview-questions.md`, `final-knowledge-map.md`) will grow with each one — treat them as living documents you revisit, not one-time reads.

## Recommended Learning Order

Follow the numbered files in order. The dependency graph in `00-learning-roadmap.md` explains exactly why this order is not arbitrary — e.g., you cannot meaningfully understand **Chapter 9/10 (synchronization)** without first understanding **Chapter 7 (interrupts)**, because half of *why* synchronization primitives are designed the way they are is "this code might run in interrupt context, where you cannot sleep."

## How to Use the Experiments

Every chapter's experiments are labeled by risk:

| Label | Meaning |
|---|---|
| **SAFE USER-SPACE** | Ordinary commands/programs, safe on any machine, including your daily driver |
| **KERNEL-MODIFYING** | Loads/unloads kernel modules, changes `/proc` or `/sys` tunables — reversible, but do it in a VM |
| **DANGEROUS** | Can panic, hang, or corrupt the running kernel — VM + snapshot only, recovery steps always provided |

Read the **Objective** before running anything — the goal is never "run this command," it's "predict what will happen, then verify, then explain *why* the kernel behaves that way."

## How to Read Kernel Source Alongside This Course

Each chapter's **Source Code Navigation** section gives you exact starting points. The general workflow:
```bash
cd ~/linux
rg --type c "task_struct" kernel/ include/linux/sched.h   # ripgrep: fast, respects .gitignore
git grep -n "asmlinkage long sys_"                          # git-aware grep across the whole tree
ctags -R .                                                   # build a tag index once
# then, in your editor: jump-to-definition, jump-to-references
```
`source-code-map.md` explains `cscope`, `ctags`, `git grep`, and `gdb`/`pahole` in depth the first time each is needed — you are not expected to already know them.

## Tracking Your Progress

At the end of every chapter is a **"What I Should Be Able to Explain Now"** checklist. Do not move to the next chapter until you can answer every item **without looking back at the notes.** If you cannot, that is not a failure — it is the signal telling you exactly what to re-read. This course is built around that feedback loop; use it.

## What You Should Be Able to Do After Completing This Course

- Open an unfamiliar file in the Linux kernel source tree and correctly identify: which subsystem it belongs to, what execution context its functions run in, what data structures and locks it touches, and how user space reaches it (or doesn't).
- Trace a complete operation — a `read()` system call, a scheduling decision, an interrupt, a page fault — from user space, through the kernel, to hardware, and back.
- Reproduce, diagnose, and fix a race condition using the appropriate synchronization primitive, and explain *why* that primitive (not another) was correct.
- Write, build, load, and debug a simple kernel module.
- Use `strace`, `ftrace`, `perf`, `bpftrace`, `/proc`, `/sys`, and `gdb`/`crash` to investigate real kernel behavior, not just read about it.
- Reason confidently about a capstone-scale systems project that touches processes, scheduling, synchronization, memory, and device I/O — and articulate its failure modes before they occur.