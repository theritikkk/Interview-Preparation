# 29 — Containers

> **Where this fits in the stack**
> ```
> User  →  ...  →  Processes  →  Sessions  →  [ NAMESPACES / CGROUPS ]  →  Kernel
>                                              ^^^^^^^^^^^^^^^^^^^^^^^^^
>                                              this file
> ```
> "Container ≠ VM" is one of your spec's explicit distinctions never to blur. Namespaces alone don't fully justify that claim — cgroups are the other half, and this file treats both as required, not optional context.

---

## 1. Container vs VM, precisely

A **VM** virtualizes *hardware*: it runs a complete, separate kernel and OS on virtualized (or emulated) CPU, memory, and devices — genuine isolation at the hardware level.

A **container** virtualizes *nothing* at the hardware level and runs **no separate kernel at all**. It's ordinary processes, running under the exact same host kernel as everything else on the machine, with the kernel's own isolation features applied so those processes *believe and behave* as if they're on their own isolated machine. This is precisely why containers are dramatically lighter-weight than VMs — no second kernel to boot, no hardware emulation overhead — and it's the entire justification for never treating the two as interchangeable.

---

## 2. Namespaces, conceptually

A kernel mechanism letting a group of processes have their own, isolated **view** of some global system resource, while processes outside that group see the normal, host-wide version. Several kinds, each isolating a different resource:

| Namespace | Isolates |
|---|---|
| **PID** | Its own set of process ID numbers — Section 3 |
| **Mount** | Its own filesystem mount table — a container's `/` can be an entirely different tree from the host's |
| **Network** | Its own network interfaces, routing table — a container's "eth0" can be a virtual interface, isolated from the host's real one |
| **UTS** | Its own hostname, independent of the host machine's |
| **User** | Its own UID/GID mapping — a process can be "root" *inside* its namespace while being an unprivileged, ordinary user *outside* it, on the host |
| **IPC** | Its own System V IPC objects and POSIX message queues |

---

## 3. PID namespaces, and why a container's PID 1 is special

Normally, host PID 1 carries a special kernel-level responsibility: it's the designated **reaper** for orphaned processes — ones whose original parent exited before they did. The kernel re-parents these to PID 1 specifically, so *someone* is always around to eventually `wait()` for them and prevent permanent zombies (full depth: file 40).

A PID namespace creates an entirely separate PID numbering scheme for the processes inside it. The **first** process started inside a new PID namespace is assigned PID 1 *within that namespace* — even though it also has a completely different, ordinary-looking PID from the *host's* perspective. A single process genuinely has multiple, different PID values simultaneously, one per PID namespace it's visible within.

> **Remember**
> The container's internal PID 1 inherits the **same** special reaping responsibility, scoped to its own namespace. If that process exits, **the kernel tears down the entire PID namespace** — every other process inside it, regardless of what it was doing. This is exactly why containers are conventionally built to run one main process as PID 1 (often the application itself — `nginx`, directly, rather than a full init system): if that one process dies, everything in the container is *meant* to die with it, matching the container's intended lifecycle.

**A real, recurring gotcha:** if a container's PID 1 doesn't properly reap its own children — a shell script run directly as PID 1, which itself spawns background jobs — orphaned processes inside that container can accumulate as zombies with nowhere to be reaped to, since the host's own PID-1 reaping is a completely separate namespace's mechanism. This is precisely why tools like `tini` or `dumb-init` exist: their entire job is to run **as** a container's PID 1, correctly reaping orphans within that namespace, before launching the actual intended application as their own child.

---

## 4. cgroups — the other half of "container ≠ VM"

Namespaces isolate **what a process can see.** They say nothing about **how much** of the host's actual, shared, finite resources — CPU time, memory, block I/O bandwidth — that process is allowed to *use*. Without an additional mechanism, a process inside a namespace could in principle consume all of the host's CPU or memory, starving everything else on the machine, including other containers. This is exactly the gap **cgroups** (control groups) fill.

**cgroups** are a genuinely separate kernel mechanism — often confused with namespaces or lumped together imprecisely — that let you group processes and apply resource limits and accounting to the group as a whole: maximum memory (with group-scoped OOM-killing behavior if exceeded), CPU shares or quotas, block I/O throttling, and more.

> **Interview answer**
> "A container is namespaces plus cgroups, running under the host kernel — nothing more exotic than that. Namespaces give it an isolated *view* of the system; cgroups give it a *bounded share* of the system's real resources. Neither one alone is sufficient to justify calling something a container instead of just an unusually well-isolated process — you need both, which is also exactly why containers are fundamentally lighter than VMs: there's no second kernel anywhere in this picture."

---

## 5. `docker exec -it container bash`, fully tied together

**`docker exec`** runs a *new* process inside an *already-running* container's namespaces (via mechanisms like `setns()`, briefly named, not deep) — it does not start a new container, only a new process within the existing one's isolated view.

**`-i`** (interactive): keeps stdin open even when not attached to a terminal. Without it, stdin would typically give immediate EOF (file 23's `Ctrl+D` mechanics, structurally) — making an interactive shell essentially useless, since it couldn't read any input at all.

**`-t`** (allocate a PTY): Docker allocates a new PTY pair — the exact same `posix_openpt`/`grantpt`/`unlockpt` mechanics from file 02 — connects it to the new process's stdin/stdout/stderr inside the container, and Docker's own client process, running on the host, becomes something like a terminal-emulator equivalent: it puts *your* actual local terminal into raw mode and forwards bytes, while the Docker daemon on the other end actually manages the PTY connected to the containerized process.

> **Remember**
> `-it` is architecturally the exact same shape as `ssh -t` (file 26) — both mean "give me a real, interactive terminal on the other side of this boundary." The only difference is what the boundary *is*: a network connection for SSH, a namespace boundary for Docker. Without `-t`, you get plain pipes instead — file 26's identical no-PTY consequences apply: no line discipline, no canonical-mode editing, no kernel echo, and the shell inside will likely detect this via `isatty()` and behave differently (no colored prompt, for instance).

---

## 6. stdin/stdout/stderr/PTY inside containers

Exactly the same fd 0/1/2 conventions from files 10-11 apply inside a container as anywhere else. Containers don't change the fundamental fd/PTY/pipe mechanics at all — only *which namespace and cgroup context* the process doing the reading and writing happens to be running within.

---

## 7. Common misconceptions

| Misconception | Reality |
|---|---|
| "Containers run their own kernel." | They share the host kernel entirely — precisely the distinction that makes them not VMs. |
| "Namespaces alone are what makes something a container." | Both namespaces (isolated view) and cgroups (bounded resource usage) are required for the full picture. |
| "A container's PID 1 is just an ordinary process." | It has the same special orphan-reaping responsibility as host PID 1, scoped to its own namespace — and its death tears down the entire namespace. |
| "`docker exec -it` starts a new container." | It starts a new process inside the existing, already-running container. |

---

## 8. Try this

> **Try this**
> ```bash
> docker run -d --name test nginx
> docker exec -it test bash
> # inside:
> echo $$          # a low number — this container's own PID namespace
> ps aux            # ONLY container-internal processes, not the host's full list
> ```

> **Try this**
> ```
> docker stats
> ```
> Live, cgroup-enforced resource usage and limits for running containers — cgroups, directly observed.

---

## 9. Summary

- A container is namespaces plus cgroups on the host kernel — no separate kernel, no hardware virtualization, which is the complete answer to "container ≠ VM."
- A PID namespace gives its first process PID 1 *within that namespace*, inheriting host PID 1's orphan-reaping role, scoped locally — and its exit tears down the whole namespace.
- cgroups bound actual resource consumption; namespaces alone only bound what's *visible*.
- `docker exec -it` is architecturally identical to `ssh -t` — both allocate a PTY across a boundary, network for one, namespace for the other.

---

## What's next

`30_NETWORKING_FROM_TERMINAL.md` covers the networking sockets this file's namespace discussion assumed — `curl` traced through DNS, `connect()`, and the kernel's TCP stack, contrasted directly against file 27's Unix domain sockets.