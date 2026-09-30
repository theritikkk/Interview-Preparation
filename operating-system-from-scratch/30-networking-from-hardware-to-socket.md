# 30 — Networking, From Hardware to Socket

## Layer map for this file

```
┌──────────────────────────────────────────────────┐
│ 1. BOOK: none directly, but this file is the     │
│    literal payoff of a promise file 01 made:     │
│    the "hardware event flows up" diagram         │
├──────────────────────────────────────────────────┤
│ 2. CORE CONCEPT: the packet's path through       │
│    DMA, interrupts, the stack, and a socket      │
├──────────────────────────────────────────────────┤
│ 3. MODERN: NAPI, RSS, XDP/eBPF, kernel bypass    │
└──────────────────────────────────────────────────┘
```

Book coverage: **None.** This is a genuine payoff, though: file `01`'s
very first diagram showed "hardware event → interrupt/DMA → driver →
kernel → process → application" as one of two directions this whole
course would need. This file is that exact diagram, filled in
completely, with a real subsystem.

## Prerequisites

Files `01`–`29`, especially `14`, `15`, `19`, `20`, `21`.

---

## [CORE CONCEPT] The Path, Both Directions

![A packet's path: electrical signal at the NIC, DMA into a ring buffer, an interrupt (or NAPI polling), the driver handing off to the network stack through Ethernet, IP, and TCP/UDP layers, arriving in a socket's receive buffer, waking the waiting process, and copy_to_user delivering the bytes to the application](images/30-packet-path.svg)

Every step is a callback, not a new idea:

- **DMA** (file `15`) — the NIC writes incoming frames directly into a
  ring buffer in RAM; the CPU never touches the raw bytes arriving off
  the wire.
- **Interrupt** (file `14`) — the NIC signals "data is here," the same
  mechanism as a keyboard press, just a different device.
- **The stack** — the driver hands a raw frame upward through
  Ethernet, then IP, then TCP or UDP, each layer stripping its own
  header and passing the rest up.
- **The socket** — a **file descriptor** (file `18`/`26`'s
  `file_operations` indirection, again) backed by network-stack code
  instead of a filesystem; the same `read()`/`write()` syscalls (file
  `21`) work on it exactly as they do on an ordinary file.
- **The process** — blocked in `recv()`, sitting in the **Waiting**
  state (file `19`) until data arrives, then moved back to **Ready**
  (file `20`) once it does.

Sending reverses the same path: `write()`/`send()` copies from user
space, the stack builds headers going the other direction, the driver
queues the frame in a transmit ring, and DMA pushes it out to the wire
— no new mechanism anywhere in either direction.

## [CORE CONCEPT] Why Naive Interrupts Don't Scale

File `14` treated "one interrupt per event" as the whole story. At
network line rates, a naive driver would raise an interrupt for
**every single packet** — an interrupt storm (file `14` named the
term) that could consume more CPU handling interrupts than doing
anything useful. **NAPI** (New API) is the real fix: take one interrupt
for the first packet, then switch to **polling** the ring buffer
directly for a burst of packets with interrupts held off, only
re-enabling interrupts once the queue drains. A genuine, necessary
refinement of file `14`'s model the instant "occasional event" becomes
"tens of thousands of events per second."

---

## [RELATED KNOWLEDGE] Endianness, Already Covered

File `02` already gave you the one fact worth carrying into this file:
network protocols use **network byte order** — big-endian — precisely
because hosts disagree on their own native order, and a protocol has
to fix one. Nothing new to learn here; just the payoff of a fact
planted early.

---

## [MODERN SYSTEM] Spreading Packets Across Cores

**RSS** (Receive Side Scaling) has the NIC hash each incoming packet's
header and direct it to one of several receive queues, each serviced
by a different CPU core — file `24`'s SMP scheduling, applied to
network interrupts specifically, so a single fast NIC doesn't bottleneck
on one core's interrupt handling capacity alone.

## [MODERN SYSTEM] Processing Packets Before the Stack Even Sees Them

**XDP** (eXpress Data Path) runs an eBPF program (file `35`) directly
in the driver, before the packet ever reaches the normal network
stack — used for extremely fast filtering, load balancing, or DDoS
mitigation, discarding or redirecting packets at the earliest possible
point.

## [MODERN SYSTEM] Skipping the Kernel Entirely

**DPDK** (Data Plane Development Kit) takes the opposite approach from
everything else in this file: it maps a NIC's DMA rings directly into
a *user-space* process, bypassing the kernel network stack (and the
syscall boundary from file `21`) altogether, for applications where
even the kernel's own overhead is too much. A real, production
technique — and a genuine tradeoff against everything this file's
normal path provides for free (the socket abstraction, protocol
handling, isolation between processes).

---

## Practical Experiment

1. `ip link show` / `ethtool -S eth0` (or your interface's name) — find
   interrupt/packet counters directly.
2. `cat /proc/interrupts | grep -i eth` (or your NIC's driver name) —
   file `14`'s counter table, this time for network hardware
   specifically; watch it change under load (`ping -f`, briefly, to a
   local host).
3. `tcpdump -i lo -c 5` while running `curl localhost` in another
   terminal (or any local request) — watch packets captured at the
   interface, below the socket layer your application actually calls
   into.
4. Write a minimal client: `socket()`, `connect()`, `write()`,
   `read()`, `close()` in C — `strace` it and find every syscall from
   this file's diagram, in order.
5. If `ethtool -k eth0` shows `generic-receive-offload` or similar,
   read about what it does — another real optimization layered on top
   of this file's basic path.

## Debugging

Network slowness decomposes along exactly this file's layers, and each
has its own tool:

```
Is the packet arriving at the NIC at all?      → ethtool, /proc/interrupts
Is the driver keeping up (drops)?               → ip -s link (RX/TX drops)
Is the stack/socket buffer the bottleneck?      → ss -i (socket stats)
Is the application slow to read/write?          → strace -T on the process
```

A socket that "hangs" is very often just a process sitting correctly in
**Waiting** (file `19`) for data that legitimately hasn't arrived yet
— `ss -i` and `/proc/<pid>/status` both show this state directly,
turning "it's stuck" into "it's waiting on X," a completely different
diagnosis.

## Source Code

Your minimal socket client/server pair, plus its `strace` output
matched against this file's diagram.

---

## Common Misconceptions

- **"An interrupt fires for every network packet."** Only under NAPI's
  initial trigger; sustained traffic is serviced by polling instead,
  specifically to avoid an interrupt storm.
- **"A socket is fundamentally different from a file."** Same
  `file_operations`-style indirection (files `18`, `26`) — a socket is
  a file descriptor backed by network code instead of a filesystem.
- **"DPDK is how all high-performance networking works."** It's one
  specific, real tradeoff — bypassing the kernel stack for raw speed,
  at the cost of everything the normal path provides for free.

## Interview Questions

**Basic:** What hardware mechanism gets an incoming packet's bytes into
RAM in the first place?
**Intermediate:** Why doesn't a high-throughput NIC just raise one
interrupt per packet?
**Advanced:** Why can `read()`/`write()` work on a socket exactly the
same way they work on a file?
**Systems Design:** What does DPDK trade away in exchange for
bypassing the kernel network stack?
**Debugging:** An application appears "stuck" reading from a socket —
how do you find out whether it's actually stuck or just correctly
waiting?
**Explain what happens when...:** you run `curl example.com` — trace
the outgoing request from the syscall to the wire, and the response
back to the terminal?

## Active Recall

1. Which two mechanisms from earlier files does an arriving packet use
   before the network stack ever sees it?
2. Why was NAPI needed, given file `14`'s interrupt model already existed?
3. What makes a socket usable with ordinary `read()`/`write()` calls?
4. What does DPDK bypass, and at what cost?

## Answers

1. DMA (file `15`), to get the frame into memory, and an interrupt
   (file `14`), to notify the CPU it's arrived.
2. One interrupt per packet at high packet rates would itself consume
   more CPU than useful processing — an interrupt storm; NAPI switches
   to polling under sustained load instead.
3. The same `file_operations`-style dispatch (files `18`, `26`) that
   lets any file descriptor route generic syscalls to
   subsystem-specific code — here, the network stack instead of a
   filesystem.
4. The kernel's own network stack and the syscall boundary entirely,
   trading away the socket abstraction, protocol handling, and
   inter-process isolation for raw throughput.

---

## What You Should Now Understand

- File `01`'s "hardware event flows up" diagram, finally completed with
  a real subsystem, both directions.
- Why NAPI exists as a direct, necessary refinement of file `14`'s
  interrupt model under real load.
- The socket as one more instance of the file-descriptor indirection
  from files `18` and `26`.
- Where RSS, XDP, and DPDK each sit relative to the basic path.

---

**Next:** file `31-security-and-protection.md`.
