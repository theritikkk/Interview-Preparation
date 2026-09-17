# 20. Final Revision Cheat Sheet
### *(All nineteen chapters, compressed)*

Use this as your last pass before considering yourself done. If every section here makes sense at a glance, you've genuinely absorbed the book's material — both the 2.4.20-era design it teaches, and how far the kernel has moved since.

## The whole book, as one picture

```
                          APPLICATION
                               │
   socket()/bind()/listen()/connect()/accept()/send()/recv()   (Ch 1, 3, 4)
                               │
                    ┌──────────┴──────────┐
                    │  struct socket      │  generic, VFS-facing (Ch 3)
                    │  struct sock        │  protocol-specific (Ch 3)
                    └──────────┬──────────┘
                               │
                    every packet = an sk_buff (Ch 5)
                               │
         ┌─────────────────────┴───────────────────────┐
         │                                             │
         │                                             │
     TCP SEND (Ch 8)                             TCP RECEIVE (Ch 7)
     segmentation, Nagle,                        fast/slow path,
     PMTU reaction                               out-of-order queue
         │                                             │
         │                                             │
         │                                             │
         │                                             │
     TCP TIMERS (Ch 10): RTO, delayed-ACK,             │
     keepalive, persist, TIME_WAIT                     │
         │                                             │
         │                                             │
         │                                             │
         │                                             │
     STATE MACHINE, sliding window, SACK (Ch 11)       │
     CONGESTION CONTROL: slow start,                   │
     avoidance, Reno/Vegas (Ch 12)                     │
         │                                             │
         │                                             │
         │                                             │
         │                                             │
         │                                             │
     MEMORY ACCOUNTING (Ch 9): per-socket +            │
     global pressure thresholds                        │
         │                                             │
         │                                             │
         │                                             │
         │                                             │
         └─────────────────────┬───────────────────────┘
                               │
                    IP ROUTING / FIB (Ch 14)
                               │
                    QUEUING DISCIPLINE (Ch 15): PFIFO/CBQ
                               │
                    NETFILTER HOOKS (Ch 16): filter/NAT/conntrack
                               │
         ┌─────────────────────┴────────────────────────┐
         │                                              │
   TRANSMIT: dev_queue_xmit()                   RECEIVE: hardware interrupt
   → hard_start_xmit() → DMA TX ring                  → netif_rx()
   (Ch 18)                                             → NET_RX_SOFTIRQ (Ch 17)
         │                                             → DMA RX ring (Ch 18)
         ▼                                             │
       WIRE  ◄─────────────────────────────────────────┘

   Control plane, running alongside all of the above:
   NETLINK SOCKETS (Ch 13) — how `ip route`/`ip link`/`ip addr` talk to the kernel

   When it all goes wrong:
   CRASH DUMP ANALYSIS (Ch 19) — LKCD in the book; kdump+crash/drgn today
```

## Process context vs. interrupt vs. softIRQ — the distinction underneath everything

| Context | Can sleep? | Triggered by | Where it appears in this book |
|---|---|---|---|
| Process | Yes | An application's own system call | Every send-path chapter (6, 8); most of 1, 3, 4 |
| Interrupt | No | Hardware (a NIC signaling a packet arrived) | The very first step of every receive-path diagram (6, 17, 18) |
| SoftIRQ | No, but interrupts re-enabled | Raised by an interrupt handler, deferred | `NET_RX_SOFTIRQ`/`NET_TX_SOFTIRQ` — Ch 6, 17 |

## Real bugs and architectural shifts this project verified, not just recalled

1. **The route cache (Ch 14) was completely removed in kernel 3.6** — not refined, removed — because `fib_trie` (default since 2.6.39) made direct FIB lookups fast enough that the cache's own DoS-vulnerability and complexity stopped being worth its shrinking performance benefit. Verified directly against kernel-history sources rather than assumed from memory.
2. **`fq_codel`, not `pfifo_fast`, is the practical default queuing discipline on essentially every modern distribution** (Ch 15) — via a `systemd`-provided `sysctl` override layered on top of the kernel's own still-`pfifo_fast` compiled-in default — specifically because it fights bufferbloat, a problem identified only after this book's writing.
3. **`nftables` has replaced `iptables` at the kernel level** on modern distributions (Ch 16) — the `iptables` *command* commonly still works today, but typically as a compatibility shim (`iptables-nft`) rather than talking to the original framework this book describes directly.
4. **`kdump`/`kexec` + `crash`/`drgn` replaced LKCD** (Ch 19) specifically because LKCD's fundamental design flaw — relying on an already-crashed kernel to reliably report its own crash — is exactly the problem `kexec`'s "boot a guaranteed-healthy second kernel first" architecture was built to avoid.

## Self-test

1. Why does `accept()` need a *separate* queue (the accept queue) from the SYN queue, rather than one combined queue for "connections in progress"?
2. What's the actual difference between `snd_wnd` and `snd_cwnd`, and why does TCP need both?
3. Why can't a raw hardware interrupt handler simply call `ip_rcv()` directly, instead of handing off to a softIRQ?
4. What two completely independent problems does Nagle's algorithm and delayed ACK each solve — and why can combining them badly stall a connection?
5. Why was the Linux route cache removed rather than kept alongside `fib_trie`?
6. Name one thing that changed about `sk_buff` specifically to enable zero-copy sends.

*(Answers: 1 — a fully half-open SYN-queue entry is deliberately lightweight specifically to resist SYN-flood attacks; the accept queue holds fully-formed, expensive `struct sock` objects, and conflating the two would mean either wasting memory on every unfinished handshake attempt or losing the `backlog` parameter's actual, specific meaning. 2 — `snd_wnd` is the receiver's stated buffer capacity (flow control); `snd_cwnd` is the sender's own inferred estimate of the network path's capacity (congestion control) — genuinely different constraints, from different sources, and the true limit is always whichever is smaller. 3 — because real protocol processing is substantial work that needs interrupts re-enabled to avoid stalling all other hardware event handling on that CPU, and interrupt handlers must return as fast as possible for exactly that reason. 4 — Nagle (sender: don't send small writes while data is still outstanding) and delayed ACK (receiver: don't send a bare ACK immediately, hope to piggyback) are each individually sensible, but if both fire on the same exchange, each side can end up waiting on the other with nothing available to naturally break the stall until a timer expires. 5 — because `fib_trie` made the "slow path" (a full lookup) fast enough on its own that the cache's real, documented DoS-vulnerability and added complexity stopped being worth its shrinking performance benefit. 6 — paged/fragmented data support (`skb_shared_info`/`frags[]`), letting a packet reference data already sitting in separate memory pages (e.g., a file's page cache) instead of requiring every byte to be copied into one contiguous buffer first.)*

---

You've now been through the architecture, design, and implementation of the Linux TCP/IP stack as this book presents it — the kernel/userspace boundary, the socket and `sk_buff` abstractions, the complete send and receive paths, TCP's timers/state-machine/congestion-control trio, the routing/QoS/netfilter control points, the interrupt-to-softIRQ-to-process handoff, the device-driver layer underneath all of it, and how to debug the whole thing when something breaks — with every major subsystem's evolution since 2002 laid out alongside it, several verified directly against current sources rather than taken on faith. That's a genuinely deep foundation. From here, the most valuable next step is reading real, current kernel source directly (`github.com/torvalds/linux`, `net/ipv4/`) with this book's concepts as your map — the names have shifted in places and the sophistication has grown considerably, but the fundamental architecture this book teaches is still, two decades on, unmistakably the same stack.