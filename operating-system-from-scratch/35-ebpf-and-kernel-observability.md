# 35 — eBPF and Kernel Observability

## Layer map for this file

```
┌──────────────────────────────────────────────────┐
│ 1. BOOK: none — foreshadowed three times         │
│    (files 21, 30, 32) and never explained        │
├──────────────────────────────────────────────────┤
│ 2. CORE CONCEPT: the verifier, JIT compilation,  │
│    maps — what actually makes this safe          │
├──────────────────────────────────────────────────┤
│ 3. MODERN: bpftrace, Cilium, seccomp-bpf,        │
│    sched_ext                                     │
└──────────────────────────────────────────────────┘
```

Book coverage: **None.** Three earlier files named a tool from this
world without explaining the mechanism behind it: file `21`'s `strace`
has an eBPF-based cousin; file `30` named XDP; file `32` named
`bpftrace`. This file is the mechanism.

## Prerequisites

Files `01`–`34`, especially `11`, `20`, `21`, `25`, `27`, `29`, `30`.

---

## [CORE CONCEPT] The Problem eBPF Solves

File `25` named kernel modules as a way to add code to a running
kernel without rebuilding it — but a module is fully trusted, ring-0
code with no safety net at all; a bug in one can crash or corrupt the
entire machine. eBPF answers a narrower, much safer question: **can a
small, specific piece of logic run inside the kernel, at a chosen
point, without that risk?**

## [CORE CONCEPT] The Life Cycle

![eBPF program life cycle: restricted C compiled to eBPF bytecode, loaded via the bpf() syscall, checked by the verifier for safety and termination, JIT-compiled to native code only after verification passes, attached to a hook point such as a kprobe or XDP or cgroup hook, running safely and writing results to a shared map that a user-space tool then reads](images/35-ebpf-lifecycle.svg)

The step that makes everything else trustworthy is the **verifier**: before
an eBPF program is allowed to run at all, the kernel statically analyzes
it and rejects anything that can't be proven to terminate, might access
memory out of bounds, or could otherwise destabilize the kernel. Only
code the verifier accepts ever gets **JIT-compiled** (file `11`'s
compilation idea, happening at load time instead of ahead of time) into
native machine code and actually attached to a hook. This is the exact
contrast with file `25`'s kernel modules: a module is trusted because
it's root-installed; an eBPF program is trusted because it's been
*proven* safe before it ever executes a single instruction.

**Maps** are how a running eBPF program gets information back out: a
key-value store the kernel-side program writes into and a user-space
program (`bpftrace`, a custom tool) reads from — no new syscall needed
per use case, just a shared structure both sides already know how to
read.

---

## [RELATED KNOWLEDGE] Where the Name Comes From

File `30`'s `tcpdump` already uses the *original*, much simpler **BPF**
(Berkeley Packet Filter) — a tiny bytecode VM from the 1990s built for
exactly one job: deciding whether to keep or drop a captured packet,
fast. `tcpdump -i eth0 port 80` compiles to classic BPF bytecode under
the hood. **eBPF** ("extended" BPF) generalized that same safe,
verified-bytecode idea from "filter packets" into "run arbitrary safe
logic almost anywhere in the kernel" — same lineage, enormously wider
scope.

---

## [MODERN SYSTEM] bpftrace and the Tooling Around It

**bpftrace** is a high-level scripting language that compiles down to
eBPF programs, letting you write something close to `awk` and attach it
to kernel events without touching C or the verifier directly:

```
bpftrace -e 'tracepoint:syscalls:sys_enter_openat { printf("%s %s\n", comm, str(args->filename)); }'
```

— every file a running system opens, printed live, with no kernel
module, no recompilation, no reboot. **BCC** (BPF Compiler Collection)
provides a lower-level Python/C toolkit for more elaborate tools built
the same way.

## [MODERN SYSTEM] Real Production Uses

**Cilium** replaces traditional `iptables`-based Kubernetes networking
and security policy with eBPF programs attached via XDP (file `30`) and
cgroup hooks (file `29`) — enforcing network policy at the earliest
possible point, in the kernel, with none of `iptables`' rule-chain
overhead. **Falco** uses eBPF for runtime security monitoring —
watching syscalls (file `21`) system-wide for suspicious patterns,
live.

---

## [ADVANCED / BEYOND THE BOOK] Restricting What a Process May Even Ask For

**seccomp-bpf** uses this exact mechanism to restrict which *syscalls*
(file `21`) a process is allowed to make at all — a container runtime
(file `29`) commonly installs a seccomp-bpf filter denying dozens of
rarely-needed, higher-risk syscalls before a container's own code ever
runs, narrowing file `21`'s user/kernel boundary per-process, not just
globally.

## [ADVANCED / BEYOND THE BOOK] Scheduling, Loaded Like a Plugin

**sched_ext**, in current kernels, lets a custom **scheduler** (file
`20`'s entire subject) be written as an eBPF program and loaded without
rebuilding or rebooting the kernel at all — swap out the actual
scheduling policy this course built conceptually in file `20`, live, on
a running system, verified safe before a single decision it makes ever
takes effect.

---

## Practical Experiment

1. Install `bpftrace` if available; run the `sys_enter_openat` one-liner
   above and watch files open live as you use the system normally.
2. `bpftrace -l 'tracepoint:syscalls:*' | wc -l` — see how many stable
   syscall tracepoints exist to attach to.
3. Try a trivially broken script (an intentionally unbounded loop) and
   observe the verifier reject it at load time, with an explanatory
   error — the safety mechanism, caught in the act.
4. If available: `bpftool prog list` — see every eBPF program currently
   loaded on your system, likely more than you'd expect, from networking
   and security tooling already running.

## Debugging

An eBPF program that fails to load is a verifier rejection, and the
error it gives (`bpftool` or `bpftrace`'s own error output) names the
*exact* instruction and reason — unbounded loop, out-of-bounds map
access, an unprovable code path. This is a fundamentally different
debugging experience from a crashed kernel module: the failure happens
at load time, with a specific reason, never at ring 0 with the machine
already compromised.

## Source Code

Your `sys_enter_openat` trace output, plus the deliberately-broken
verifier-rejection script.

---

## Common Misconceptions

- **"eBPF programs are just kernel modules with extra steps."** A
  module is unconstrained, trusted code; an eBPF program is
  provably-safe code, checked before it ever runs — a fundamentally
  different trust model.
- **"eBPF is only for networking."** XDP (file `30`) is one use among
  many — tracing, security policy (seccomp-bpf), and even scheduling
  (sched_ext) all use the same mechanism.
- **"bpftrace and eBPF are the same thing."** `bpftrace` is a
  user-friendly scripting language that *compiles to* eBPF — one
  convenient front end among several (BCC is another).

## Interview Questions

**Basic:** What does the eBPF verifier actually check, before a program
is allowed to run?
**Intermediate:** How does classic BPF (used by `tcpdump`) relate to
modern eBPF?
**Advanced:** Why is an eBPF program considered safer than a kernel
module, given both ultimately run inside the kernel?
**Systems Design:** Why would a Kubernetes networking layer (Cilium)
prefer eBPF/XDP over traditional `iptables` rules?
**Debugging:** An eBPF program fails to load — what kind of error
message should you expect, and when does the failure happen relative to
execution?
**Explain what happens when...:** you run a `bpftrace` one-liner
tracing `openat` — trace it from your script to the kernel's verifier
to live output in your terminal?

## Active Recall

1. What three earlier files named something from this world without
   explaining it?
2. What does the verifier guarantee before a program is allowed to run?
3. What's the relationship between classic BPF and eBPF?
4. Name two production uses of eBPF beyond simple tracing.

## Answers

1. File `21` (an eBPF-based `strace` alternative), file `30` (XDP),
   file `32` (`bpftrace`).
2. That the program will terminate, won't access memory out of bounds,
   and can't destabilize the kernel — checked statically, before any
   execution or JIT compilation happens.
3. eBPF ("extended" BPF) generalizes the original, narrow
   packet-filtering bytecode VM (still used by `tcpdump`) into a
   general-purpose, verified, in-kernel execution engine.
4. Any two of: Cilium (networking/security policy), Falco (runtime
   security monitoring), seccomp-bpf (syscall restriction),
   sched_ext (custom schedulers).

---

## What You Should Now Understand

- The mechanism behind three tools named in earlier files without
  explanation.
- Why the verifier is what makes eBPF fundamentally safer than a kernel
  module, despite both running inside the kernel.
- Classic BPF's lineage, from `tcpdump` to today's general-purpose engine.
- Where seccomp-bpf (file `21`) and sched_ext (file `20`) extend ideas
  this course already built.

---

**Next:** file `36-operating-system-design.md`.
