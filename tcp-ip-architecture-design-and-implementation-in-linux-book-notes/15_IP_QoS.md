# 15. IP Quality of Service: Queuing Disciplines
### *(Covers the book's Chapter 15 — "IP Quality of Service")*

`06_Packet_Movement_Across_Layers.md`'s transmit-path diagram had a step, right before the device driver, labeled "queuing discipline scheduling decisions." This chapter is that step in full: once a packet has been routed (`14_IP_Routing_Table_FIB.md`) and is ready to leave via a specific interface, it doesn't go straight to the driver — it passes through a **queuing discipline (qdisc)**, which decides *when* and *in what order* queued packets actually get dequeued for transmission.

## 3.1 Why a queuing layer exists at all

A network interface can only transmit one packet at a time, at a fixed maximum rate — but packets can be *handed to it* for transmission far faster than that, in bursts. Something has to hold the excess temporarily and decide the order they actually go out in. The simplest possible answer — a plain FIFO queue, first-in-first-out, no distinctions made between packets at all — is exactly **PFIFO**, this chapter's baseline case. Everything more elaborate in this chapter exists to answer: *is plain arrival order actually the right thing to prioritize on, or should some traffic get preferential treatment?*

## 15.2 PFIFO — the trivial baseline

```c
/* conceptually: */
enqueue(qdisc, skb)  { append skb to the tail of a single queue; }
dequeue(qdisc)       { remove and return the queue's head; }
```

No prioritization, no classification, no fairness mechanism at all — genuinely just a queue, with a maximum length (`limit`) beyond which further packets are simply dropped. This is a completely reasonable choice for a simple, lightly-loaded link where nothing is competing for scarce bandwidth — but it has a real, well-known problem the moment a link *is* genuinely contended: a single bulk, bandwidth-hungry flow (a large file transfer) sharing a PFIFO queue with a latency-sensitive flow (an interactive SSH session, a VoIP call) can fill the queue with its own packets, forcing the latency-sensitive flow's packets to wait behind a large backlog of unrelated bulk traffic — visible, real, and (from the interactive flow's perspective) entirely undeserved latency.

## 15.3 CBQ — Class-Based Queuing, this book's flagship example

**CBQ** is this book's primary example of solving PFIFO's problem: rather than one undifferentiated queue, define a **hierarchy of classes**, each with its own configured bandwidth share, and classify packets into the appropriate class (by source/destination, port, or other criteria) before they're queued at all.

The mechanism CBQ specifically uses to enforce each class's bandwidth share is worth understanding, since it's also exactly why CBQ later fell out of favor (§15.5): CBQ **estimates** how busy the underlying link currently is by measuring how much *idle time* has passed since the last packet was sent, and uses that estimate to decide whether a given class is currently exceeding its configured share, throttling it if so (and, in more elaborate configurations, letting a class temporarily "borrow" unused bandwidth from a sibling class that isn't currently using its own full share). This is a genuinely reasonable design on paper — but the *link-idle-time estimation* at its core turns out, in real-world practice, to be **difficult to tune correctly and prone to real inaccuracy**, particularly at higher link speeds where the actual idle/busy timing involved becomes extremely fine-grained relative to what the estimator can reliably measure.

## 15.4 Configuring queuing from userspace — `tc`, and its netlink foundation

Exactly like the routing-table manipulation in `13_Netlink_Sockets.md`/`14_IP_Routing_Table_FIB.md`, queuing discipline configuration isn't a separate, bespoke mechanism — the `tc` ("traffic control") command-line tool builds and sends **netlink** messages (a dedicated set of message types for qdisc/class/filter configuration, conceptually a sibling of `rtnetlink`'s routing-focused messages) to configure exactly the hierarchy described in §15.3. This is worth explicitly noting as a connective thread running through several chapters in a row now: sockets (`03`–`04`), routing (`13`–`14`), and queuing (this chapter) are all, at the control-plane level, ultimately configured through the *same* general netlink mechanism — different message families, same underlying kernel/userspace communication model.

## 15.5 What's changed since kernel 2.4.20

- **CBQ has substantially fallen out of favor.** Its link-idle-time-estimation approach (§15.3) proved difficult to configure correctly in practice, and its real-world behavior didn't always match what its configuration implied it should do. **HTB (Hierarchical Token Bucket)**, developed specifically as a more predictable, easier-to-reason-about alternative for the *same* class-based-hierarchical-bandwidth-sharing goal, uses token-bucket accounting (each class accumulates "tokens" at its configured rate, and can only send when it has tokens available — a fundamentally more direct, deterministic mechanism than estimating link idle time) and became the de facto standard for this kind of hierarchical shaping, largely displacing CBQ in real-world configurations.
- **Bufferbloat** — a problem *identified and named* only some years after this book's writing — turned out to be a serious, widespread, real-world issue: many devices (home routers especially) shipped with queues configured far larger than actually useful, on the reasoning that "more buffer means fewer dropped packets." In practice, oversized buffers mean packets sit queued for a genuinely long time under load *before* being dropped or sent — manifesting as exactly the kind of "my downloads are fine but my video call/SSH session is suddenly laggy" complaint this chapter's whole subject exists to prevent, just via a different, less obvious mechanism (excessive delay, not raw unfairness) than PFIFO's simple lack of prioritization.
- **`fq_codel`** (Fair Queuing with Controlled Delay) is the direct, purpose-built answer to bufferbloat, combining **per-flow fair queuing** (automatically, without manual class configuration, giving each distinct traffic flow roughly its own fair share — a form of the fairness CBQ/HTB require manual class setup to achieve) with the **CoDel** algorithm (actively managing queue *delay*, not just queue *length*, dropping packets specifically when they've been sitting queued too long, rather than only when the queue is physically full). This is genuinely significant: `fq_codel` is now **the actual default queuing discipline on most major Linux distributions** (set via a `systemd`-provided `sysctl` override, layered on top of the kernel's own still-`pfifo_fast`-by-default baseline) specifically *because* it requires zero manual class configuration to get most of both PFIFO's simplicity and CBQ/HTB's fairness, while also directly solving the bufferbloat problem neither of this chapter's two qdiscs was designed with in mind at all.
- **CAKE** (a newer, more sophisticated, more CPU-intensive qdisc than `fq_codel`) exists for cases wanting an even more refined combination of fairness, bufferbloat mitigation, and additional traffic-shaping features — worth knowing by name as the current "if `fq_codel` isn't quite enough" answer, without needing deep familiarity with it for this book's purposes.

---

## Quick recap (for revision)

- PFIFO: no prioritization at all — simple, but lets one bulk flow starve a latency-sensitive flow sharing the same queue.
- CBQ: hierarchical, bandwidth-shared classes, enforced via link-idle-time estimation — this book's flagship example, but one whose core enforcement mechanism proved genuinely hard to tune accurately in real deployments.
- Queuing discipline configuration (`tc`) is, like routing table configuration, ultimately a netlink-based control-plane operation — the same underlying kernel/userspace mechanism from `13_Netlink_Sockets.md`, applied to a different subsystem.
- **HTB's token-bucket accounting replaced CBQ** as the standard hierarchical-shaping mechanism specifically because it's more predictable and easier to configure correctly.
- **Bufferbloat** (identified after this book's writing) and **`fq_codel`** (its purpose-built, now-near-universal-default answer) are the single biggest conceptual addition to this whole area since 2002 — a real-world problem this book's PFIFO/CBQ picture doesn't anticipate, and a solution built specifically to address it without requiring manual class configuration at all.

Next: `16_Netfilter_Framework.md` — the hook-based framework behind firewalling, NAT, and connection tracking, and its own major historical transition (iptables → nftables).