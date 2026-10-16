# 07 — Cloud Path I: Why (and How) an Agent Gets Its Own Computer

> **Where this file fits**
> ```
>     the executor's hands (files 02–06) point at *some* machine.
>     For a demo, it can be a container on a laptop.
>     For a product, it is a fleet of disposable computers in a datacenter:
>        this file  →  why, and what those machines are made of
>        08         →  the machinery: disks, snapshots, clones, lifecycle
> ```
> Perception and control say how the agent *uses* a computer. This part of the series says where that computer *comes from* — and why "agent computers" became a genuine infrastructure category with hypervisors, snapshot formats, and cost models of their own.

---

## 1. The product requirement

Watch the same task run in three settings and the requirement writes itself:

- **On your own machine:** the agent fights you for the cursor; it sees your private data; closing your laptop pauses it; you cannot run two of them; you cannot trust it.
- **In a container on a dev box:** better — but it shares a kernel with its host, its lifespan is your shell session, and "throw the whole computer away and start over" means rebuilding a container, not restoring a machine you can keep mid-conversation.
- **In a cloud VM dedicated to it:** it stays on when you sleep; nothing it does touches your machine; it can be snapshotted at the scary moment, cloned for the team, inspected live over VNC like any remote desktop, and deleted without residue.

What a serious agent-computer platform must therefore provide (this list is the design spec for files 07–08):

| Requirement | Why | Covered in |
|---|---|---|
| **Fast create** — sub-second to seconds | fleets scale per task; slow boot kills dense workloads | §3, file 08 |
| **Cheap idle** — near-zero cost while parked | agents sit between user messages for hours; "stopped" must not mean "billing compute" | file 08 |
| **Full-machine snapshot & resume** | park *mid-task* with RAM intact, restore later on different hardware | file 08 |
| **Fork/clone ×N** | branch experiments from a worked-through state; N sub-agents from one setup | file 08 |
| **Isolation from the tenant** and from *each other* | agents run untrusted content and act on the world; blast radius must be one machine | §2–3 |
| **Secrets injection per instance** | the agent needs credentials so *its owner* doesn't hand them over in a chat; snapshots must not leak them | file 08 |
| **Network policy** | prompt injection (file 06) wants exfiltration paths; fleets close them by default | §6 |
| **Observability** | "what did the agent do?" must be answerable: live screen, action log, screenshots | §5, file 08 |

---

## 2. Containers vs VMs: the isolation question

Both shapes appear in real agent stacks, and the differences matter most exactly when things go wrong:

| Dimension | Container (namespaces + cgroups) | Virtual machine (hardware virt.) |
|---|---|---|
| Isolation boundary | shared kernel; syscalls filtered (seccomp) | separate kernel; hardware-enforced (VT-x/AMD-V) |
| Kernel exploit → host | one bug away | needs a hypervisor escape |
| Boot time | ~tens of ms | ~100 ms (microVM) to seconds (full VM) |
| Density per host | very high | high (microVMs) |
| Different kernel / full-machine snapshot | no / no | yes / yes |
| GUI stack | Xvfb inside the container | full desktop inside the guest |
| Sweet spot | trusted code, dev sandboxes, CI | **untrusted code, untrusted content, long-lived agents** |

The honest 2024–2026 industry answer looks like this:

- **Start with `container + Xvfb`.** The reference implementations of computer use (Anthropic's own demo among them) run exactly this: a Linux container with a virtual X server, the agent's apps inside, screenshots and XTEST inside. It is cheap, fast, and perfect for development and single-user tools.
- **Ship fleets on microVMs.** The moment agents (a) run untrusted code or content by design, (b) act on behalf of many tenants, or (c) need whole-machine snapshots, the kernel boundary matters. Agents browse the web, open attachments, and *execute what they read*; that is the definition of a workload you do not run on your host kernel.

> **Under the hood**
> The row "full-machine snapshot" is quietly the biggest difference. Containers can checkpoint processes (CRIU), but "freeze this entire computer — memory, open windows, half-finished form — and resume it later" is a hypervisor trick. An agent platform's undo button, its park-before-cost, and its fork-from-here all ride on VM snapshots (file 08).

---

## 3. Firecracker: the microVM that powers agent fleets

The hypervisor most agent-computer platforms build on is **Firecracker**, the open-source VMM AWS built for Lambda and Fargate — and its design notes read like a checklist for this exact use case. From the project itself:

- **A VMM built on KVM** that creates "microVMs": the security and isolation of hardware virtualization with the speed and flexibility of containers.
- **Minimalist by construction.** It "excludes unnecessary devices and guest-facing functionality to reduce the memory footprint and attack surface area of each microVM" — no BIOS, no legacy device zoo; a small fixed device set (virtio block/net, vsock, serial, a few others).
- **Fast and dense.** Startup times in the ~125 ms class and per-VM memory overhead of a few MiB (about 5 MiB in the published design), with **demand-fault paging and CPU oversubscription enabled by default** — i.e., the host schedules more vCPUs and more guest RAM than physically present, relying on guests using a fraction at any instant.
- **Hardened per instance.** A **jailer** process wraps each Firecracker: it applies cgroup/namespace isolation and then drops privileges; the VMM itself runs under **thread-specific seccomp filters** (a per-thread syscall allowlist, not a coarse global one).
- **API-driven.** A running microVM is configured over a local API endpoint: vCPUs (default 1), memory (default 128 MiB), disks as file-backed block devices, network interfaces, vsock, entropy device, per-device rate limiters — all things an orchestrator sets programmatically.
- **Proven in multi-tenant production.** It runs serverless fleets at AWS scale and is embedded in container runtimes like Kata Containers and Flintlock — the "VM per workload" pattern with container ergonomics.

### 3.1 The host stack for one agent computer

```
   ═══════════════════════════════ host (one big Linux box) ═══════════════════════════════
   physical CPU/RAM/disk  ──  host kernel (KVM)  ──  many microVMs
                                                     │
   ┌─────────────────────────────────────────────────┼──────────────────────────────────┐
   │  firecracker process #271 (one per agent)       │        … VMs #272, #273, …        │
   │  ┌──────────────────────────────────────────────┴─────────────────────────────────┐│
   │  │  jailer: cgroups (cpu.max, memory.max) + namespaces + seccomp + dropped privs  ││
   │  │  ┌──────────────────────────────────────────────────────────────────────────┐ ││
   │  │  │  guest kernel (its own!)                                                  │ ││
   │  │  │  Ubuntu + Xorg/Xvfb + window manager + Chromium + the agent runtime       │ ││
   │  │  │  virtual disk = copy-on-write overlay (file 08)                           │ ││
   │  │  └──────────────────────────────────────────────────────────────────────────┘ ││
   │  └────────────────────────────────────────────────────────────────────────────────┘│
   │   virtio-net (TAP → NAT → egress policy)     vsock (control plane)     serial       │
   └─────────────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 The anatomy of "one agent computer"

Inside the guest, a production agent computer is just a Linux desktop — with a daemon:

- **Desktop stack:** Xorg or Xvfb (file 02; X11 keeps XTEST available), a light window manager, a browser, a file manager, and whatever productivity apps the product promises. (The dashboard apps — spreadsheets, editors — are usually the *point* of the product.)
- **Agent daemon / control channel:** a small service that receives commands from the orchestrator over **vsock** (host↔guest socket that needs no guest networking), executes them (start/stop capture, run a shell command, inject input, fetch files), and streams status back. vsock matters because it works *even when the guest's network is off* — which is exactly the default posture for safety.
- **Display streaming:** `x11vnc`-style servers feeding a noVNC viewer so a human can watch the live screen (demos, audits, intervention) — the "peek at the agent's computer" feature.
- **Storage:** a per-instance overlay disk on top of the golden template (file 08).
- **Secrets:** per-instance, injected at boot by the control plane, never part of the base image.
- **Network:** off, or tightly allowlisted (next section).

---

## 4. Networking: two channels with different jobs

| Channel | Mechanism | Purpose | Availability |
|---|---|---|---|
| **Control plane** | vsock (host-mediated socket) | orchestrator ↔ agent daemon: lifecycle, input, files, streaming | always, independent of guest net |
| **Data plane** | TAP device → host NAT/bridge → policy-enforced egress | the agent's own browsing, API calls, downloads | explicitly granted per task/product |

Keeping these separate is deliberate and pays off twice:

1. **Reliability:** the human can still watch and stop a VM whose network is wedged (or maliciously reconfigured by its own contents).
2. **Safety:** egress can be a *policy decision*: no network for a task that doesn't need it; an allowlist for the domains a task does need; full logging of what was fetched. File 06's prompt-injection threat model is precisely "the agent reads an instruction and then reaches the network" — cut the second half and the injection mostly dies as an *exfiltration* event, even if the agent still gets confused.

---

## 5. The three common shapes (and when each is right)

| Shape | Example use | Isolation | Lifecycle unit | Snapshot story |
|---|---|---|---|---|
| **Shared desktop VM** | benchmarks (OSWorld-style), classroom demos | one VM, many tasks sequentially | task = reset to snapshot | pre-task snapshot, reset per task |
| **Container per session** | dev tools, quickstarts, single-user assistants | shared kernel with host | container create/destroy | image-level only; process pause via CRIU at best |
| **MicroVM per agent** | commercial agent computers, fleets | per-VM kernel, jailed VMM | the *agent* owns a machine for days | full-machine snapshot/resume/clone (file 08) |

The middle row is where every prototype starts; the bottom row is where products converge once agents run long tasks (hours) for many users, with credentials, on content nobody vetted.

---

## 6. Security posture: two independent layers

Because agents fail in two different ways, platforms need two different defenses, and they are *not substitutes*:

| Layer | Threat | Defense |
|---|---|---|
| **Machine isolation** | *escape*: malicious code/content compromises the host or other agents' computers | microVM boundary, jailer, seccomp, per-VM cgroups, minimal device models, patched KVM |
| **Content safety** | *misuse*: the agent is convinced (or instructed) to do harmful things through the UI/network | refusal behavior, confirmations, egress policy, scoped accounts, audit logging (file 06 §6.1) |

The OS-Harm results made the second column mandatory to say out loud: a perfectly isolated VM running a perfectly manipulated agent still sends the phishing email, still deletes the customer data, still leaks the secret — with impeccable isolation the whole way. Isolation bounds the *blast radius to that machine*; it does not bound *what that machine does to the world*.

---

## 7. Key ideas to carry forward

- Agents need their own computers for the boring reasons (uptime, privacy, parallelism) and the scary ones (they run untrusted content *and* act on the world).
- The platform spec: fast create, cheap idle, snapshot/resume, fork, per-instance secrets, network policy, observability — files 08 delivers the mechanisms.
- Containers with Xvfb are the dev sweet spot; production fleets converge on **microVMs** — Firecracker being the reference design: KVM-based, minimalist device model, ~125 ms-class boots, a few MiB overhead, jailed and seccomp-filtered per instance, configured over an API.
- One agent computer = guest desktop + control daemon (vsock) + display streaming for humans + overlay disk + injected secrets + policy-controlled network.
- Isolation and content-safety are separate security layers; OS-Harm showed the second cannot be assumed.

Next: [08 — snapshots, clones, and the disk machinery](08-cloud-path-ii-machines-snapshots-forks.md).
