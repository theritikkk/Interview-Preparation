# 28 — Virtualization

## Layer map for this file

```
┌─────────────────────────────────────────────────┐
│ 1. BOOK: emulation vs. virtualization, named    │
│    and distinguished (§2.3) — but never         │
│    explained mechanically                       │
├─────────────────────────────────────────────────┤
│ 2. CORE CONCEPT: trap-and-emulate, VMX root/    │
│    non-root mode, VM exits, the VMCS            │
├─────────────────────────────────────────────────┤
│ 3. MODERN: KVM, Firecracker/microVMs, EPT       │
└─────────────────────────────────────────────────┘
```

Book coverage: **Seed**. File `06` already used the book's own
emulation/virtualization distinction. This file explains the mechanism
behind the word "virtualization" that §2.3 uses but never unpacks.

## Prerequisites

Files `01`–`27`, especially `06`, `08`, `21`, `22`.

---

## [BOOK] The Distinction You Already Have

File `06` gave you the book's own line precisely: emulation simulates
CPU state in software; virtualization runs guest instructions more
directly on the real CPU. True, and the entire extent of what the book
says about it. This file is the "more directly" — as a real mechanism.

---

## [CORE CONCEPT] The Problem: an Unmodified Guest Thinks It Owns Ring 0

A virtualized guest is an ordinary OS (possibly your own toy kernel
from file `13`) that believes it's running at ring 0 with the whole
machine to itself. It executes the exact privileged instructions file
`21` named — `in`/`out`, writes to `CR0`/`CR3`, `lgdt`, `cli`/`sti` —
expecting real, exclusive control of the hardware. It can't be allowed
that, because other guests (and the host) need the same machine.

## [CORE CONCEPT] Trap-and-Emulate, the Classical Answer

Before hardware support existed, the trick was to actually run the
guest at a *reduced* privilege level and let file `21`'s own fault
mechanism do the work: the guest's privileged instructions fault
(exactly as they would for any ring-3 code), but instead of killing the
process, the **hypervisor's** fault handler inspects what the guest was
trying to do, emulates its effect, and resumes the guest — which never
even notices anything unusual happened. Software-only, and correct,
but every single privileged instruction pays a full fault's cost.

## [CORE CONCEPT] VMX: a New Privilege Axis, Not a Lower Ring

Hardware support (Intel VT-x, AMD-V) fixes the cost problem by adding
an entirely new dimension, orthogonal to rings 0–3:

![VMX root and non-root modes: root mode runs the hypervisor at ring 0, issuing VMLAUNCH/VMRESUME to enter the guest; non-root mode runs the guest's own ring 0 kernel and ring 3 apps at near-native speed, until a sensitive instruction triggers a VM exit back to root mode, which reads the VMCS, emulates the operation, and resumes the guest with VM entry](images/28-vmx-modes.svg)

The guest's ring 0 is **real** — its kernel executes ordinary
instructions directly on the CPU, at essentially native speed, no
fault and no emulation involved. Only the specific instructions the
**VMCS** (Virtual Machine Control Structure — the same bit-packed,
descriptor-table idea as the GDT, IDT, and TSS from files `08`, `14`,
`21`) marks as sensitive cause a **VM exit**: control drops to the
hypervisor in root mode, which reads the VMCS to see *why* it exited,
emulates the effect, and issues `VMRESUME` (**VM entry**) to hand
control straight back. Far cheaper than a full fault-based trap for
every sensitive operation, and — critically — everything *else* the
guest does needs no intervention at all.

---

## [RELATED KNOWLEDGE] Hypervisor Types

**Type 1** hypervisors (Xen, VMware ESXi) run directly on hardware,
with no general-purpose host OS underneath at all. **Type 2** (VirtualBox,
older QEMU-without-KVM) run as an application on top of an ordinary
host OS. **KVM** blurs the line deliberately: it's a kernel module that
turns an ordinary Linux kernel itself into a Type-1-style hypervisor,
while that same machine keeps running as a normal Linux host
simultaneously.

---

## [MODERN SYSTEM] KVM, and QEMU's Two Modes Revisited

File `06` flagged that modern QEMU can run in pure software emulation
(**TCG**) or **KVM-accelerated** mode. Now the mechanism is concrete:
`-enable-kvm` tells QEMU to hand the actual instruction execution to
the CPU via `/dev/kvm`, using exactly the VMX root/non-root cycle
above, instead of interpreting every instruction in software. This is
why KVM-accelerated QEMU runs a guest OS at near-native speed while
plain TCG emulation, doing the same job entirely in software, is
dramatically slower.

## [MODERN SYSTEM] microVMs: Virtualization Stripped to Its Essentials

**Firecracker** (built by AWS, powering Lambda and Fargate) uses KVM
directly with an extremely minimal device model — none of a full
VM's usual virtual hardware — to boot a guest in milliseconds instead
of seconds, at a fraction of a full VM's memory overhead. It's the same
VMX mechanism this file describes, stripped down specifically for
running very many short-lived, strongly isolated guests cheaply.

---

## [ADVANCED / BEYOND THE BOOK] Two Layers of Page Tables

File `22` gave a guest OS its own page tables, translating guest
virtual addresses to what the guest *believes* are physical addresses.
Under virtualization, that belief is false — those are **guest-physical**
addresses, needing one more translation to real **host-physical**
memory. **EPT** (Extended Page Tables, Intel; **NPT** on AMD) adds a
second table, walked automatically by hardware alongside the first: a
memory access now walks the guest's own page tables *and* the EPT in
one hardware operation, entirely without trapping — file `22`'s tree,
nested one level deeper.

**Nested virtualization** (running a hypervisor inside a VM) is exactly
what it sounds like, and is precisely why cloud providers care about
whether VMX itself can be exposed one level down.

---

## Practical Experiment

1. `lscpu | grep -i virtualization` and `cat /proc/cpuinfo | grep -o
   'vmx\|svm' | head -1` — confirm your own CPU supports VT-x (`vmx`)
   or AMD-V (`svm`).
2. Boot the same guest image twice: `qemu-system-x86_64 <image>` (TCG)
   vs. `qemu-system-x86_64 -enable-kvm <image>` — time how long each
   takes to reach a shell, and compare responsiveness once booted.
3. `ls /dev/kvm` — confirm the device KVM exposes exists; check its
   permissions.
4. If available: `virt-host-validate` (from `libvirt-clients`) reports
   whether your machine is properly configured for hardware-accelerated
   virtualization at all.

## Debugging

A VM that's inexplicably slow is worth checking against this file
directly: is it actually running with hardware acceleration
(`-enable-kvm`, or your hypervisor's equivalent setting), or has it
silently fallen back to pure software emulation — often because
`/dev/kvm` wasn't accessible (permissions, or virtualization disabled
in firmware) and nothing surfaced a clear error about it.

## Source Code

None new — this file is conceptual. The TCG-vs-KVM timing comparison
above is the closest thing to hands-on evidence.

---

## Common Misconceptions

- **"Virtualization and emulation are the same idea."** File `06`'s own
  distinction, now mechanized: emulation interprets instructions in
  software; hardware virtualization runs most of them directly, only
  trapping specific sensitive ones.
- **"A virtualized guest's kernel doesn't really run at ring 0."** It
  does — VMX adds an orthogonal root/non-root axis; the guest's ring 0
  is real within non-root mode.
- **"Every instruction in a VM causes a trap to the hypervisor."** Only
  instructions the VMCS marks sensitive do; everything else runs at
  native speed with no intervention.

## Interview Questions

**Basic:** What's the difference between emulation and hardware-assisted
virtualization?
**Intermediate:** What causes a VM exit, and what happens during one?
**Advanced:** Why is VMX root/non-root described as an axis orthogonal
to rings 0–3, rather than just "the guest runs at a lower ring"?
**Systems Design:** Why does a Firecracker microVM boot so much faster
than a full traditional VM?
**Debugging:** A VM is unexpectedly slow — what's the first thing to check?
**Explain what happens when...:** a guest OS executes `cli` inside a
KVM-accelerated VM — trace it through a VM exit and back?

## Active Recall

1. What is the book's own emulation/virtualization distinction (file `06`)?
2. What does VMX add that trap-and-emulate alone doesn't have?
3. What is the VMCS, and what earlier structures does it resemble?
4. What extra translation does EPT/NPT add, and why is it needed?

## Answers

1. Emulation simulates CPU state in software; virtualization runs guest
   instructions more directly on the real CPU.
2. A hardware-enforced root/non-root mode distinction, so only
   specifically marked sensitive instructions cause a VM exit — most
   guest code runs at native speed with no trap at all.
3. A structure controlling which operations cause a VM exit and holding
   guest/host state across exits and entries — the same bit-packed,
   descriptor-table idea as the GDT, IDT, and TSS.
4. A second page-table walk from guest-physical to host-physical
   addresses, needed because the guest's own page tables only resolve
   to addresses the guest *believes* are physical, not real memory.

---

## What You Should Now Understand

- What "runs guest instructions more directly on the real CPU" (file
  `06`'s own words) actually means, mechanically.
- Trap-and-emulate, and why VMX's root/non-root axis is a genuinely
  different, cheaper approach.
- Where KVM, QEMU's acceleration flag, and Firecracker each sit in this
  picture.
- EPT as file `22`'s page-table tree, nested one level deeper.

---

**Next:** file `29-containers-and-namespaces.md`.
