# 17. SoftIRQ Implementation
### *(Covers the book's Chapter 17 — "Linux TCP/IP Stack Implementation of SoftIRQ")*

`01_Kernel_Basics_and_TCPIP_System_Calls.md` introduced process vs. interrupt context; `06_Packet_Movement_Across_Layers.md`'s receive-path diagram showed a hardware interrupt handing off to something called `NET_RX_SOFTIRQ` before real protocol processing happens. This chapter is that handoff mechanism in full — arguably the single piece of infrastructure most responsible for Linux networking's ability to handle high packet rates without falling over.

## 17.1 The problem softIRQs solve

Hardware interrupt handlers must be **fast** — while one is running, on most architectures, interrupts of equal or lower priority are disabled, meaning the machine can't respond to *other* time-critical hardware events until this handler finishes. But real network packet processing (protocol header validation, socket lookups, queue management — everything `07_TCP_Receive.md` walked through) is genuinely substantial work, entirely inappropriate to run with interrupts disabled for however long it takes.

The needed answer is a second execution context: **still cannot sleep** (there's no process to reschedule around, exactly as interrupt context can't — `01`, §1.2), **but interrupts are re-enabled**, so the machine can still respond to new hardware events while this deferred work executes. This is exactly what a **softIRQ** is.

## 17.2 The mechanism: raising and processing

```c
#define NET_TX_SOFTIRQ   0
#define NET_RX_SOFTIRQ   1
/* ...a small, FIXED, compile-time set of other softirq types... */

void raise_softirq(unsigned int nr) {
    /* set a per-CPU pending bit for softirq number `nr` */
}

void do_softirq(void) {
    /* check this CPU's pending bitmask;
       for each pending softirq, run its registered handler function;
       repeat until no more are pending (bounded — see §17.4) */
}
```

The driver's interrupt handler (from `06_Packet_Movement_Across_Layers.md`'s receive path) calls `netif_rx()`, which — critically — does **not** call the actual IP-layer receive function directly. It queues the packet and calls `raise_softirq(NET_RX_SOFTIRQ)`, then returns immediately, letting the interrupt handler finish as fast as possible. `do_softirq()` runs shortly after — specifically, right as the current hardware interrupt handler is finishing and control is about to return to whatever was previously executing — and it's `do_softirq()`'s invocation of the `NET_RX_SOFTIRQ` handler that actually calls into `ip_rcv()` and everything downstream of it.

## 17.3 Per-CPU state — softIRQs run genuinely concurrently across cores

Each CPU maintains its **own** pending-softirq bitmask and its own count of in-progress softirq processing — meaning the *same* softirq type (say, `NET_RX_SOFTIRQ`) can be legitimately running **simultaneously on two different CPUs at once**, each processing a different packet. This is a deliberate, important design choice for scalability on multi-processor machines, and it directly explains *why* networking code that softIRQs call into has to be so careful about locking (the `spin_lock_bh()` family of primitives you'll see throughout the actual source, specifically designed to be safe against another CPU's softirq context, not just against a "normal" concurrent process) — there's no free serialization here at all; concurrent execution across cores is exactly the point, and correctness has to come entirely from explicit locking, not from any implicit "only one of these runs at a time" guarantee.

## 17.4 Fairness — why softIRQs can't just run forever

If packets keep arriving fast enough, `NET_RX_SOFTIRQ` could, in principle, keep finding more pending work indefinitely — which would mean the CPU never gets back to running ordinary user processes at all, a real, serious problem (a machine that's "up" but effectively unresponsive to everything except packet processing — a specific instance of the broader **receive livelock** problem). The kernel guards against this with a bounded budget: `do_softirq()` processes pending work only up to some limit (a maximum number of iterations, or the check happens periodically rather than being allowed to loop unconditionally), and if there's still genuinely more work left after that budget is exhausted, the remaining processing is handed off to a dedicated per-CPU kernel thread, **`ksoftirqd`**, which runs at normal (schedulable, preemptible-in-the-ordinary-sense) process-context priority — meaning it competes fairly with everything else the scheduler manages, rather than being able to monopolize the CPU the way unbounded inline softirq processing could.

## 17.5 The historical hierarchy: BH → tasklets → softIRQs

Worth knowing by name, since you may encounter all three terms in the wild even within kernel documentation and older code comments from around this book's era: Linux's deferred-work mechanism went through real evolution before settling on the design above.

- **BH (bottom halves)**: the original, oldest mechanism — a small, fixed set of deferred-work "bottom halves," but **globally serialized**: only one BH of *any* type could run at a time, system-wide, across all CPUs. Simple to write against (you never had to worry about true concurrency), but a genuine scalability ceiling on multi-processor machines.
- **Tasklets**: introduced specifically to let *different* pieces of deferred work run concurrently on different CPUs, while keeping the property that **the same tasklet never runs concurrently with itself** — a useful middle ground: easier to write correctly than a fully concurrent softIRQ (no need to defend against your own code running twice at once), while still allowing real cross-CPU parallelism between *different* tasklets.
- **SoftIRQs** (this chapter's subject): the most general, highest-performance mechanism — genuinely concurrent even for the *same* softirq type across CPUs (§17.3), at the cost of needing to handle that concurrency correctly yourself, with no free serialization guarantee at all. Networking specifically uses raw softIRQs, rather than tasklets, precisely because the sheer packet-processing volume involved makes the extra concurrency worth the added locking discipline required.

## 17.6 What's changed since kernel 2.4.20

- **NAPI (New API)**, already introduced conceptually in `06_Packet_Movement_Across_Layers.md`, is best understood, at this chapter's level of detail, as a specific *change to how `NET_RX_SOFTIRQ` itself behaves*: rather than this chapter's picture (one `netif_rx()` call, and correspondingly one `raise_softirq()`, per received packet), a NAPI-capable driver's interrupt handler **disables further receive interrupts from that device** and schedules the softirq just *once*; the `NET_RX_SOFTIRQ` handler then actively **polls** the device directly for as many packets as are currently available, processing a whole batch before re-enabling interrupts — dramatically reducing the interrupt-handler-invocation overhead per packet under sustained high load, and directly addressing the receive-livelock risk `17.4`'s budget-and-`ksoftirqd` mechanism only partially mitigates on its own. This is arguably the single most impactful change to the exact machinery this chapter describes, precisely because it changes *how often* the interrupt→softIRQ handoff even happens, not just what runs once it does.
- **RPS/RFS** (flagged already in `06`'s "what's changed") build directly on this chapter's per-CPU model (§17.3): rather than a packet always being processed by whichever CPU happened to receive its hardware interrupt, these mechanisms can deliberately steer a given flow's softIRQ processing to a *different*, specifically chosen CPU (ideally, the one already running the application that owns the relevant socket) — a scalability refinement with no counterpart in the comparatively simple "process it wherever the interrupt landed" model 2.4.20 implements.
- **Threaded NAPI / `softirq` vs. dedicated kernel-thread-per-queue models** on modern, high-queue-count network hardware give administrators more explicit control over exactly how receive processing is distributed and scheduled across cores than this book's simpler picture assumes — a refinement specifically aimed at very high-packet-rate, many-core server hardware that didn't exist in anything like today's form in 2002.

---

## Quick recap (for revision)

- SoftIRQs exist to bridge interrupt context (fast, interrupts disabled, cannot sleep) and full protocol processing (substantial work, needs interrupts re-enabled, still cannot sleep) — a genuinely necessary third context, not an arbitrary design choice.
- `netif_rx()`'s entire job (from `06`) is "queue the packet, raise `NET_RX_SOFTIRQ`, return as fast as possible" — the interrupt handler itself never touches IP/TCP processing at all.
- SoftIRQs are **per-CPU** and genuinely concurrent, even for the *same* softirq type across different cores — correctness depends entirely on explicit locking (`spin_lock_bh()` and friends), not on any implicit serialization.
- `ksoftirqd` and a bounded per-call processing budget exist specifically to prevent sustained high packet rates from starving ordinary process scheduling entirely (receive livelock).
- The BH → tasklet → softIRQ lineage reflects a real historical trade-off between ease-of-writing-correctly (more serialization) and scalability (more genuine concurrency) — networking specifically chose the most concurrent, most locking-discipline-demanding option because the volume involved makes it worth the cost.
- **NAPI's poll-based batching is the standout modern change** — it doesn't just add a new feature on top of this chapter's mechanism, it changes how often the fundamental interrupt→softIRQ handoff itself occurs under load.

Next: `18_Link_Layer_and_DMA_Ring_Buffers.md` — one level further down, into the device driver code that actually triggers the very first interrupt this whole chapter's mechanism reacts to.