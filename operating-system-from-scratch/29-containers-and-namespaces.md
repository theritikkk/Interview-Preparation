# 29 — Containers and Namespaces

## Layer map for this file

```
┌─────────────────────────────────────────────────┐
│ 1. BOOK: none — but file 28 just built the      │
│    exact contrast this file needs               │
├─────────────────────────────────────────────────┤
│ 2. CORE CONCEPT: namespaces, cgroups, and why   │
│    "a container" isn't a kernel object at all   │
├─────────────────────────────────────────────────┤
│ 3. MODERN: overlayfs, runc/containerd/Docker,   │
│    gVisor/Kata                                  │
└─────────────────────────────────────────────────┘
```

Book coverage: **None.** File `28` set up the contrast this file needs:
virtualization gives each guest its own kernel; containers give many
processes a shared kernel with different *views* of it.

## Prerequisites

Files `01`–`28`, especially `18`, `19`, `28`.

---

## The Contrast With File 28, Stated Plainly

A VM's isolation comes from **hardware** (VMX, file `28`) — each guest
runs its own kernel, unaware anything else exists. A container's
isolation comes entirely from **one shared kernel** presenting
different processes with different, restricted views of itself. No new
privilege mode, no VM exit, no VMCS — just kernel data structures that
already exist, used to hide and limit things per-process.

---

## [CORE CONCEPT] Namespaces: Different Views of One Kernel

A **namespace** wraps a global kernel resource so a process (or group
of processes) sees its own private instance of it:

- **PID namespace** — a process can be PID 1 inside its namespace while
  being an ordinary PID outside it; it cannot see or signal processes
  in other PID namespaces at all.
- **Mount namespace** — a private view of the filesystem mount table
  (file `18`'s VFS, per-namespace).
- **Network namespace** — its own network interfaces, routing table,
  and port space, entirely separate from the host's.
- **UTS namespace** — its own hostname.
- **IPC namespace** — its own shared memory segments and message queues.
- **User namespace** — maps UIDs: a process can be root (UID 0) *inside*
  the namespace while mapping to an unprivileged UID *outside* it — a
  subtle, important security property, since a compromise inside stays
  bounded by the outside UID's real permissions.

## [CORE CONCEPT] cgroups: a Genuinely Different Job

**Control groups** don't hide anything — they **limit and account for**
resource usage (CPU shares, memory ceilings, I/O bandwidth,
process/thread counts) for a group of processes. A process can be fully
visible to another (no namespace separating them) and still have its
CPU time capped by a cgroup, and vice versa. Namespaces answer "what can
you see;" cgroups answer "how much can you use" — genuinely orthogonal,
routinely confused.

## [CORE CONCEPT] "A Container" Is Not a Kernel Object

This is the single most clarifying fact in this file:

![A container built from three existing kernel features: namespaces for isolation, cgroups for resource limits, and overlayfs plus pivot_root for the filesystem view — combined by userspace tools like runc, containerd, and Docker into what's called a container; there is no struct container in the kernel itself](images/29-container-ingredients.svg)

Grep the kernel source (file `26`'s skill) for `struct container` and
you won't find one. **A "container" is a userspace name for a specific
combination of namespaces, a cgroup, and a filesystem setup**, assembled
by tools like `runc` (following the OCI runtime spec), orchestrated by
`containerd`, with `Docker` as the familiar top layer most people
actually type commands into. All of it rests on kernel features that
existed, individually, for other purposes long before "containers"
were a product category.

---

## [RELATED KNOWLEDGE] chroot: the Incomplete Ancestor

`chroot` changes what a process considers to be the filesystem root —
the oldest, simplest piece of this puzzle, dating back decades before
containers as a concept. It does nothing for PIDs, networking, or
resource limits, and is famously escapable by a privileged process
(there are well-known "chroot breakout" techniques). Real container
runtimes use **`pivot_root`** instead, a more robust primitive with far
fewer escape paths, combined with the namespaces above.

---

## [MODERN SYSTEM] Layered Images via overlayfs

A container image is typically a stack of **read-only layers** (file
`18`'s filesystem ideas, union-mounted): a base OS layer, then each
`Dockerfile` instruction adding another layer on top, plus one
**writable layer** unique to each running container. Multiple containers
from the same image share every read-only layer's actual disk blocks —
only each container's own writes consume new space. This is why
spinning up ten containers from one image is fast and cheap: nine of
them are sharing, not copying.

## [MODERN SYSTEM] The Real Stack Under "Docker"

`Docker` is the familiar CLI and daemon most people mean when they say
"containers"; underneath, it delegates to **`containerd`** (image
management, lifecycle) which delegates to **`runc`** (the actual
process that sets up namespaces and cgroups per the **OCI (Open
Container Initiative)** spec, then executes the container's process).
Kubernetes talks to `containerd` (or another OCI-compatible runtime)
directly, without needing Docker's own daemon at all.

---

## [ADVANCED / BEYOND THE BOOK] When Namespace Isolation Isn't Enough

Because every container on a host shares one kernel, a kernel
vulnerability can cross container boundaries in ways it can't cross
VM boundaries. **gVisor** intercepts a container's syscalls in
userspace, implementing much of the kernel interface itself rather than
passing calls straight through. **Kata Containers** takes the other
approach: run each container inside its own lightweight VM (file `28`'s
mechanism), trading some of containers' speed and density for
VM-grade isolation. Both exist specifically to answer "containers share
a kernel — what if that's not isolated enough?"

---

## Practical Experiment

1. `unshare --pid --fork --mount-proc bash` — a new PID namespace; run
   `ps aux` inside it and see your shell as PID 1.
2. In a second terminal, confirm that same process has an ordinary PID
   from the host's own view (`ps aux | grep bash`).
3. Create a cgroup by hand (`/sys/fs/cgroup/...` on a cgroup v2 system),
   set a memory limit, add a process's PID to it, and watch it get
   killed (OOM) once it exceeds the limit — without touching any
   namespace at all, demonstrating the isolation/limits split directly.
4. `docker run` something simple, then `ps aux | grep <process>` on the
   host — the container's process is visible as an ordinary process to
   the host, just inside its own namespaces.
5. `docker history <image>` — see an image's actual layer stack.

## Debugging

"It works outside the container but not inside" is almost always a
namespace or cgroup boundary, not application logic: a network
namespace with no route to the host, a mount namespace missing an
expected file, or a cgroup memory limit killing the process silently
(`dmesg` will show an OOM kill even when the container just reports
"exited"). Check `docker inspect` or `runc state` for the actual
resource limits and namespace configuration in force before debugging
the application itself.

## Source Code

None new — `unshare` and a hand-built cgroup are this file's hands-on
work.

---

## Common Misconceptions

- **"A container is a lightweight VM."** It shares the host kernel
  entirely; a VM (file `28`) runs its own kernel. Different isolation
  mechanism, different guarantees.
- **"Namespaces and cgroups do the same job."** Namespaces control
  visibility; cgroups control resource consumption — a process can be
  limited by one without being isolated by the other.
- **"Docker is what implements containers."** Docker is a convenient
  top layer; the actual isolation is `runc` assembling kernel
  namespaces and cgroups that predate Docker entirely.

## Interview Questions

**Basic:** What's the fundamental isolation difference between a
container and a VM?
**Intermediate:** What's the difference between what a namespace does
and what a cgroup does?
**Advanced:** Why is there no `struct container` in the Linux kernel?
**Systems Design:** Why do multiple containers built from the same base
image start up so quickly and use so little extra disk space?
**Debugging:** A containerized process is silently killed under load —
what's the first thing to check, and where?
**Explain what happens when...:** you run `docker run <image>` — trace
it through containerd, runc, and the kernel features actually created?

## Active Recall

1. What's the core isolation difference between file `28`'s VMs and
   this file's containers?
2. Name three kinds of namespace and what each hides.
3. What does a cgroup do that a namespace doesn't?
4. What tool actually creates the namespaces and cgroup for a container,
   underneath Docker?

## Answers

1. A VM's guest runs its own separate kernel (hardware-enforced, file
   `28`); a container is a process with a restricted view of one shared
   host kernel.
2. PID (other processes), mount (the filesystem view), network
   (interfaces and routing) — among others (UTS, IPC, user).
3. It limits and accounts for resource usage (CPU, memory, I/O) rather
   than controlling what's visible.
4. `runc`, following the OCI runtime spec, orchestrated by `containerd`.

---

## What You Should Now Understand

- The precise contrast between container and VM isolation, building
  directly on file `28`.
- Namespaces (visibility) vs. cgroups (limits) as genuinely different
  mechanisms.
- That "a container" names a combination of pre-existing kernel
  features, not a kernel object of its own.
- Where overlayfs, runc/containerd/Docker, and gVisor/Kata each fit.

---

**Next:** file `30-networking-from-hardware-to-socket.md`.
