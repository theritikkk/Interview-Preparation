# 9. TCP Socket Memory Management
### *(Covers the book's Chapter 9 — "TCP Socket Timers and Memory Management," memory portion)*

`08_TCP_Send.md` mentioned a blocking `send()` can sleep waiting for buffer space; `07_TCP_Receive.md` mentioned an out-of-order queue that has to live *somewhere*. This chapter is the accounting system underneath both: **kernel memory is a shared, finite resource, and TCP has to make sure no single connection — through legitimate heavy use or a misbehaving/slow application — can consume an unbounded share of it.**

## 9.1 Two limits, per socket

Every socket tracks, separately, how much kernel memory its **send** side and **receive** side currently consume, each checked against its own configured ceiling:

```c
struct sock {
    /* ... */
    atomic_t   rmem_alloc;   // currently-allocated receive-side memory
    atomic_t   wmem_alloc;   // currently-allocated send-side memory
    int        rcvbuf;       // receive-side ceiling
    int        sndbuf;       // send-side ceiling
    /* ... */
};
```

- **Send side (`wmem_alloc` vs. `sndbuf`)**: every `sk_buff` sitting on `sk->write_queue` — including ones already transmitted but not yet acknowledged, per `08_TCP_Send.md`'s retransmit-queue point — counts against this. Once `wmem_alloc` reaches `sndbuf`, `tcp_sendmsg()` can accept no more data from the application until acknowledgments free some of it back up; this is precisely the "blocking `send()` can sleep on buffer space" behavior flagged in the last chapter.
- **Receive side (`rmem_alloc` vs. `rcvbuf`)**: covers both the ordinary receive queue *and* the out-of-order queue from `07_TCP_Receive.md` §7.4 — data sitting there because the application hasn't read it yet, or because it's out-of-order and can't be delivered yet, both cost real memory and both count here.

**This receive-side accounting is also the concrete mechanism behind the *advertised window*** (`02_Protocols_TCP_IP_ARP_ICMP.md` §2.2's `Window` field) **actually shrinking under real pressure**: the window a socket advertises reflects genuinely available buffer space, computed from `rcvbuf - rmem_alloc` (roughly) rather than some independent, purely-protocol-level notion — an application that reads slowly causes `rmem_alloc` to climb, which directly and mechanically causes the advertised window sent back to the peer to shrink, which is exactly how flow control ends up throttling a fast sender talking to a slow reader without either side needing to do anything special.

## 9.2 System-wide memory pressure — the three-threshold model

Per-socket limits alone don't protect the *system* from a large number of sockets, each individually within its own limit, collectively exhausting available memory. A second, global layer exists on top:

```c
int sysctl_tcp_mem[3];   // { low, pressure, high } — in pages
atomic_t tcp_memory_allocated;   // running total, across ALL TCP sockets
```

Three thresholds, checked against the live running total:

- **Below `low`**: no pressure at all — sockets are allowed to grow their buffers fairly liberally.
- **Between `low` and `pressure`**: a soft warning zone — the kernel becomes somewhat more conservative about granting *further* buffer growth to sockets, without actively taking memory back from anyone.
- **Above `high`**: hard pressure — the kernel actively tries to reduce memory usage, including reclaiming from existing sockets' buffers where it safely can (§9.3), and refusing new allocation requests more aggressively.

This two-tier design (per-socket ceiling *and* global running total against three system-wide thresholds) is deliberately layered: the per-socket ceiling alone protects against one connection hogging memory; the global thresholds protect against death-by-a-thousand-cuts from many individually-reasonable connections all at once.

## 9.3 Receive-queue collapsing — reclaiming space without losing data

Here's a genuinely clever piece of engineering, specifically for the receive side: suppose a connection's receive buffer is nearly exhausted, much of it consumed by a scattered handful of out-of-order segments (real, needed data — just not contiguous yet) plus the ordinary in-order queue. Simply refusing further incoming data at this point would stall the connection even though, in principle, some of that already-buffered memory is being used inefficiently (small `sk_buff` allocations, each with its own overhead, holding data that could in principle be packed more tightly).

```c
void tcp_collapse(struct sock *sk, struct sk_buff_head *queue, ...)
```

This routine walks a socket's queues and **merges adjacent, contiguous small buffers into fewer, larger ones** — freeing up the per-buffer overhead (and any wasted headroom/tailroom, per `05_sk_buff.md`'s allocation model) without discarding a single byte of actual, already-received application data. This is specifically invoked under memory pressure, as a "reclaim space *before* resorting to dropping data" step — genuinely reclaiming real, usable buffer headroom just from more efficient packing, buying a struggling connection some additional room without ever violating TCP's promise that once data has been acknowledged as received, it won't simply vanish.

## 9.4 Why this whole chapter matters for behavior you can actually observe

If you've ever seen a TCP connection's throughput degrade specifically when the receiving *application* (not the network) is slow to call `recv()`, or seen a server's connections start behaving oddly under genuinely heavy concurrent load even with apparently-adequate network capacity, this chapter's mechanisms are very likely the direct cause: a slow reader causes `rmem_alloc` to climb, which shrinks the advertised window, which mechanically throttles the sender — entirely correctly, by design, with no bug involved — and a system running many such connections simultaneously can additionally trip the global pressure thresholds in §9.2, at which point the kernel is actively working to reclaim memory across *every* TCP socket on the box, not just the one that's technically responsible.

## 9.5 What's changed since kernel 2.4.20

- **`tcp_rmem`/`tcp_wmem` auto-tuning**: rather than a single fixed `rcvbuf`/`sndbuf` ceiling per socket (largely static, set at socket-creation time, in this book's era), modern kernels expose three-value sysctls (`min`, `default`, `max`) and **actively adjust the effective buffer size for each connection at runtime**, based on estimated bandwidth-delay product (how much data can genuinely be "in flight" on this specific connection's path) — aiming to give a fast, high-latency connection (which benefits from a large window) a bigger buffer automatically, without needing every application to manually call `setsockopt(SO_RCVBUF/SO_SNDBUF)` and guess a good value itself. This is a substantial refinement over the comparatively static picture this chapter describes.
- **cgroups (control groups) memory accounting**: containers and other cgroup-constrained workloads can have **additional**, container-specific memory ceilings layered on top of everything in §9.1–9.2 — meaning a modern system's actual memory-pressure behavior for a given socket depends on both the global system-wide thresholds this chapter describes *and* whatever cgroup the owning process happens to be confined to, a layer of accounting with no equivalent at all in the single-flat-system model 2.4.20 implements.
- **The three-threshold `tcp_mem` global-pressure model (§9.2) is still fundamentally the same design** in current kernels — this is one of the more structurally stable pieces of the stack across the 20+ year gap, even as the auto-tuning layered on top of it has grown considerably more sophisticated.

---

## Quick recap (for revision)

- Every socket tracks send-side (`wmem_alloc`/`sndbuf`) and receive-side (`rmem_alloc`/`rcvbuf`) memory usage separately — hitting either ceiling has real, directly observable behavioral consequences (blocking `send()`, or a shrinking advertised window).
- The advertised TCP window isn't an abstract protocol value — it's mechanically derived from genuinely available receive-buffer space, which is exactly why a slow-reading application throttles its own incoming throughput, correctly, without any bug involved.
- A second, global layer (`tcp_mem`'s three thresholds against a running total across *all* TCP sockets) protects the whole system from many individually-within-limits connections collectively exhausting memory.
- `tcp_collapse()` reclaims wasted per-buffer overhead by merging adjacent small buffers under pressure — a genuine "get more efficient" step, not a "start dropping data" step.
- Modern auto-tuning (bandwidth-delay-product-based `tcp_rmem`/`tcp_wmem` adjustment) and cgroup-layered accounting are both real, substantial additions on top of — not replacements for — the core per-socket/global-threshold model this chapter describes, which remains structurally intact today.

Next: `10_TCP_Timers.md` — the timer-driven half of this same chapter's subject matter in the book, covering retransmission, delayed ACK, keepalive, and zero-window probing.