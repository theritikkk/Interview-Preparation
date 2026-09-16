# 6. Movement of a Packet Across the Layers
### *(Covers the book's Chapter 6 — "Movement of Packet Across the Layers")*

Chapters 3–5 introduced the pieces (sockets, protocol tables, `sk_buff`). This chapter assembles them into the complete journey — a full function-call chain, both directions — that every later deep-dive chapter (7, 8, 11, 17, 18) is really just zooming into one specific leg of.

## 6.1 The transmit path: from `write()` to the wire

```
 Application: write(sockfd, buf, len)   /   send(sockfd, buf, len, 0)
        │
        ▼
 sock_sendmsg()               generic socket layer — dispatches via sock->ops
        │
        ▼
 tcp_sendmsg()                 TCP-specific: copies user data into sk_buff(s),
        │                      queued on sk->write_queue  (08_TCP_Send.md)
        │
        ▼
 tcp_transmit_skb()            builds the actual TCP header (seq/ack numbers,
        │                      window, flags) into the sk_buff's reserved
        │                      headroom (skb_push — see 05_sk_buff.md)
        │
        ▼
 ip_queue_xmit()               IP layer: route lookup (or reuse the socket's
        │                      cached route — 14_IP_Routing_Table_FIB.md),
        │                      builds the IP header (another skb_push)
        │
        ▼
 ip_output() / ip_finish_output()   possible netfilter NF_IP_POST_ROUTING
        │                            hook fires here (16_Netfilter_Framework.md)
        ▼
 Neighbor/ARP resolution       need the next-hop's link-layer address before
        │                      a frame can actually be built (02, §2.3)
        ▼
 dev_queue_xmit()              queuing discipline (qdisc) — PFIFO/CBQ scheduling
        │                      decisions happen here (15_IP_QoS.md)
        ▼
 hard_start_xmit()             the device driver's own transmit function —
                                 hands the packet to hardware DMA
                                 (18_Link_Layer_and_DMA_Ring_Buffers.md)
```

Every arrow in this chain runs in **process context** (§1.2) — this is code running because *your application* called `write()`/`send()`, all the way down until the very last step hands off to hardware. This matters: it means this entire path, unlike the receive path below, can legitimately sleep if it needs to (waiting for socket buffer space to free up, for instance — `09_TCP_Memory_Management.md`).

## 6.2 The receive path: from the wire to `read()`

The reverse journey has a fundamentally different character, because it *starts* in a context your application has no control over at all: a hardware interrupt.

```
 Network card: frame arrives, DMA writes it into a ring buffer
        │                                  (18_Link_Layer_and_DMA_Ring_Buffers.md)
        ▼
 Hardware interrupt fires  ────────►  INTERRUPT CONTEXT (§1.2) — cannot sleep,
        │                              must be fast, minimal work only
        ▼
 Driver's interrupt handler: allocates an sk_buff, copies/references the
        │                     received frame into it, calls netif_rx()
        ▼
 netif_rx()                    hands the sk_buff to a per-CPU input queue,
        │                      then SCHEDULES the softIRQ and returns —
        │                      deliberately doing NO further processing here
        ▼
 ──────────── interrupt handler returns; hardware interrupt is done ────────────
        ▼
 NET_RX_SOFTIRQ fires   ────────►  SOFTIRQ CONTEXT (17_SoftIRQ_Implementation.md)
        │                          — still can't sleep, but far less time-
        │                          pressured than the raw interrupt handler was
        ▼
 net_rx_action() → the registered per-protocol receive function:
        │
        ▼
 ip_rcv()                       IP header validation, possible netfilter
        │                       NF_IP_PRE_ROUTING / NF_IP_LOCAL_IN hooks
        │                       (16_Netfilter_Framework.md), routing decision:
        │                       is this for ME, or should it be forwarded?
        ▼
 tcp_v4_rcv()                   TCP-specific: locate the owning socket via
        │                       the 4-tuple hash lookup (03_BSD_Socket_...)
        ▼
 tcp_v4_do_rcv() → tcp_rcv_established()   (07_TCP_Receive.md, 11_TCP_Core...)
        │                       sequence-number checks, ACK processing,
        │                       out-of-order handling, and finally:
        ▼
 sock_queue_rcv_skb()           appends the now-validated data onto
        │                       sk->receive_queue, and WAKES UP any process
        │                       currently sleeping in recv()/read() on this
        │                       socket (its wait_queue, from struct socket
        ▼                       in 03_BSD_Socket_Layer_Implementation.md)
 ──────────── back to process context — the sleeping recv() call resumes ───────
        ▼
 Application: recv()/read() returns with the data
```

**The critical structural fact this whole chapter is really teaching**: the receive path *starts* in interrupt context (forced on you by hardware, unavoidably time-constrained, cannot sleep) but *finishes* the substantial protocol processing in softIRQ context, and only actually **delivers** data to your application by handing off to whatever process context is already sleeping in a blocking `recv()` call. This three-stage handoff — interrupt → softIRQ → sleeping process, woken up — exists specifically because none of the real TCP/IP logic (sequence number checks, socket lookups, queue management) is something you'd want running with interrupts disabled, but *getting into* a context where interrupts are enabled again requires this careful staged handoff rather than just "doing everything in the interrupt handler."

## 6.3 Where each later chapter zooms in

This chapter's value is almost entirely in having the *whole* picture in view once, so that when later chapters go deep on one box in these diagrams, you know exactly where that box sits relative to everything else:

| Box above | Chapter that goes deep on it |
|---|---|
| `tcp_sendmsg` / segmentation / PMTU interaction | `08_TCP_Send.md` |
| `tcp_rcv_established` / sequence checks / out-of-order | `07_TCP_Receive.md`, `11_TCP_Core_Processing_State_Machine.md` |
| Socket buffer space, queue collapsing | `09_TCP_Memory_Management.md` |
| Retransmission, delayed ACK, keepalive scheduling | `10_TCP_Timers.md` |
| Route lookup, destination cache | `14_IP_Routing_Table_FIB.md` |
| `dev_queue_xmit` scheduling decisions | `15_IP_QoS.md` |
| The `NF_IP_*` hook points | `16_Netfilter_Framework.md` |
| `netif_rx()` → `NET_RX_SOFTIRQ` handoff itself | `17_SoftIRQ_Implementation.md` |
| The driver's interrupt handler and DMA ring buffer | `18_Link_Layer_and_DMA_Ring_Buffers.md` |

## 6.4 What's changed since kernel 2.4.20

- **NAPI (New API)**, largely adopted through the 2.4→2.6 transition and universal since, changes the very first step of the receive path significantly: rather than one hardware interrupt per received packet (which becomes a genuinely crippling overhead under high packet rates — "receive livelock," where a CPU spends all its time in interrupt handlers and never gets to actually process anything), NAPI-capable drivers **disable further receive interrupts once one arrives** and switch to *polling* the device from softIRQ context instead, re-enabling interrupts only once the receive queue is actually drained. The `netif_rx()`-per-packet model this chapter describes is the pre-NAPI picture; nearly every modern network driver uses NAPI's poll-based model instead.
- **XDP** (as noted in `05_sk_buff.md`) can intercept and fully dispose of a packet (drop, redirect, or bounce to userspace via `AF_XDP`) even *before* this chapter's `netif_rx()` step — for the specific workloads that need it, bypassing several of this diagram's own stages entirely.
- **RPS/RFS (Receive Packet Steering / Receive Flow Steering)**: on a multi-core machine, these mechanisms deliberately direct a given connection's packets to be processed by a *specific* CPU core (ideally, the same core the owning application is running on) rather than whichever core happened to handle the hardware interrupt — a scalability refinement layered on top of the interrupt→softIRQ handoff this chapter describes, aimed at cache locality, that has no counterpart in the simpler 2.4-era model.

---

## Quick recap (for revision)

- Transmit path runs entirely in process context, top to bottom — it can sleep if it needs to.
- Receive path is a three-stage handoff: hardware interrupt (fastest, can't sleep) → softIRQ (does the real protocol processing, still can't sleep) → wakes a sleeping process in `recv()` to actually deliver data.
- `netif_rx()`'s entire job is "get this off the interrupt handler's hands as fast as possible" — real processing happens after the interrupt handler has already returned.
- NAPI's poll-based receive model (interrupts disabled while actively draining the queue) is the single biggest structural change to this whole picture since 2.4.20 — solving a genuine scalability failure mode (receive livelock) the pure per-packet-interrupt model this book describes is vulnerable to under heavy load.

Next: `07_TCP_Receive.md` — zooming into the TCP-specific portion of the receive path in full detail.