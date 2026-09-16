# 7. TCP Receive
### *(Covers the book's Chapter 7 — "TCP recv")*

`06_Packet_Movement_Across_Layers.md` left off at `tcp_v4_rcv()` — a segment has arrived, passed IP-layer validation, and now needs full TCP processing. This chapter is entirely about what happens between that point and data actually landing in `sk->receive_queue`, ready for an application's `recv()` call.

## 7.1 Finding the owning socket

Before anything else, the kernel needs to know *which* socket this segment belongs to — determined entirely by the 4-tuple (source IP, source port, dest IP, dest port) carried in the IP and TCP headers.

```c
struct sock *tcp_v4_lookup(u32 saddr, u16 sport, u32 daddr, u16 dport, int dif);
```

This book's kernel era maintains **two separate hash tables** for this, reflecting the two states a socket can meaningfully be looked up in:

- **`tcp_listening_hash`**: listening sockets, keyed more loosely (often just by local port, since a listener by definition hasn't fixed a specific remote peer yet).
- **`tcp_ehash`** ("established hash"): fully connected sockets, keyed by the complete 4-tuple — this is where the vast majority of lookups actually land, since a busy server has far more established connections than listening sockets, and every single data segment on every one of those connections needs this lookup.

A segment carrying a `SYN` with no matching established-hash entry gets checked against the listening hash instead (this is the entry point back into `04_Connection_Setup_Kernel_Implementation.md`'s SYN-queue machinery); everything else looks itself up directly in `tcp_ehash`.

## 7.2 The fast path vs. the slow path — `tcp_rcv_established()`

Once the owning socket is found and confirmed to be in the `TCP_ESTABLISHED` state (by far the most common case, by volume of actual data segments processed), control reaches:

```c
void tcp_rcv_established(struct sock *sk, struct sk_buff *skb, struct tcphdr *th, unsigned len)
```

This function is explicitly optimized around a real, well-supported observation: **most TCP segments, on a healthy connection, are exactly the "next expected" segment** — the sequence number is exactly what was expected, there's no reordering, no loss, nothing unusual. Rather than running the full battery of possible checks (out-of-order handling, retransmission-timer interactions, window updates, congestion-control state changes) on *every* segment, `tcp_rcv_established()` performs a quick **header prediction** check first:

```c
if (tcp_fast_path_check(sk, th, /* ...expected seq/ack/flags... */)) {
    /* FAST PATH: exactly what we expected —
       just copy the data in, minimal bookkeeping */
} else {
    /* SLOW PATH: something's unusual — go through
       full generic segment processing */
}
```

If the prediction hits (expected sequence number, no unexpected flags, window looks normal), the function takes the **fast path**: copy the payload directly into the receive queue (or, in some implementations, straight into the waiting application's buffer if one is already blocked in `recv()` — "copy-on-interrupt," avoiding an extra queue/dequeue round trip entirely when possible), update the minimal bookkeeping actually needed, and decide whether to ACK immediately or defer (`10_TCP_Timers.md`'s delayed-ACK timer). Any genuinely unusual condition — out-of-order data, a retransmitted segment, a window update needing more careful handling, urgent data, congestion-relevant signals — falls through to the considerably more thorough **slow path**, which is where most of `11_TCP_Core_Processing_State_Machine.md`'s sliding-window and SACK material actually executes.

**Why bother with this split at all, rather than one uniform code path?** Because the fast path really is taken the overwhelming majority of the time on any healthy connection, and every cycle saved there is a cycle saved on literally every data segment a busy server processes — a clear, deliberate performance-vs-generality trade, and a good example of the kind of "handle the common case cheaply, fall back to a thorough path for everything else" design that recurs throughout this whole book.

## 7.3 Sequence number validation

Regardless of fast or slow path, every incoming segment's sequence number has to be checked against the receive window before its data can be trusted at all:

```c
if (!before(TCP_SKB_CB(skb)->seq, tp->rcv_nxt + tcp_receive_window(tp)) &&
     !before(TCP_SKB_CB(skb)->end_seq, tp->rcv_nxt)) {
    /* at least partially within the acceptable window */
}
```

Three broad outcomes: the segment's data is **entirely new and in-order** (starts exactly at `rcv_nxt`, the next byte the receiver expects — the fast-path-friendly case), **out-of-order** (starts somewhere *past* `rcv_nxt` — some earlier segment hasn't arrived yet; queued separately, see §7.4), or **entirely old/duplicate** (already fully covered by data already received — safely discarded, though still worth ACKing again in case the sender's own ACK for it was lost).

## 7.4 Out-of-order segments — the out-of-order queue

When a segment arrives that isn't the next expected byte (an earlier segment is still missing — lost, reordered by the network, or simply taking a slower path), it can't be handed to the application yet (remember: TCP promises an *in-order* byte stream), but it also shouldn't be thrown away — the missing earlier segment might still show up, at which point this one becomes usable immediately.

```c
sk->tp_pinfo.af_tcp.out_of_order_queue
```

Out-of-order segments accumulate here, sorted by sequence number. Each time *any* new data arrives (whether in-order or not), the receive logic checks whether the out-of-order queue's front now connects seamlessly to `rcv_nxt` — if the missing piece has finally arrived, potentially a whole run of previously-out-of-order segments gets spliced onto the front of the real receive queue in one go, advancing `rcv_nxt` past all of them at once. This queue, and its interaction with **SACK** (letting the sender know exactly what's already sitting here, so it doesn't need to blindly retransmit data that's already arrived, just out of order), is developed further in `11_TCP_Core_Processing_State_Machine.md`.

## 7.5 ICMP-triggered adjustments

`02_Protocols_TCP_IP_ARP_ICMP.md` §2.4 flagged the Destination-Unreachable/fragmentation-needed ICMP message as something TCP has to react to. The receive-side machinery is where that reaction actually happens: an ICMP error for an existing connection gets routed to that connection's own error handler, which (for the fragmentation-needed case specifically) reduces the socket's effective MSS and marks pending unacknowledged data for retransmission at the new, smaller size — this is Path MTU Discovery's entire mechanism, viewed from the code that actually implements the "adjust after being told" half of it.

## 7.6 Urgent data, briefly

TCP's urgent-pointer mechanism (`02`, §2.2) gets handled as a genuinely special case: when a segment's `URG` flag is set, the code locates the urgent byte (via the header's urgent pointer offset) and can deliver it to the application out of the normal queued-data path (traditionally surfaced to applications via `SIGURG`/`select()`'s "exceptional condition" readiness, or read with `MSG_OOB`). In practice, as the previous study packs' socket-programming material already noted, genuinely urgent-data-using applications are rare — this book (correctly) spends comparatively little code and attention here relative to the ordinary data path.

## 7.7 What's changed since kernel 2.4.20

- **The fast-path/slow-path split (§7.2) is still a real, current design pattern** in modern TCP implementations, though the exact prediction logic and what counts as "unusual enough to fall to the slow path" has been refined repeatedly over 20+ years, particularly as SACK (§7.4) went from optional/uncommon to essentially universal.
- **RACK (Recent ACKnowledgment, RFC 8985)** and other modern loss-detection heuristics use received ACK/SACK timing information far more aggressively than this era's implementation did, to detect loss faster and more accurately than pure sequence-number/duplicate-ACK counting alone — a receive-side (well, ACK-side) sophistication layered on top of the fundamental sequence-checking logic this chapter describes, not a replacement for it.
- **eBPF-based socket-level hooks** (`sockops`/`sk_msg` program types) can now observe or even alter behavior at points in this receive path that had no user-programmable hook at all in 2.4.20 — used for things like transparent proxying and custom congestion-signaling without kernel source modification.
- **The two-hash-table lookup design (§7.1) is conceptually unchanged**, though the actual hash table implementations have been tuned repeatedly for better distribution and, more importantly, for safe concurrent lookup without contending a single global lock on every packet — a direct beneficiary of the RCU adoption flagged repeatedly in earlier chapters' "what's changed" sections.

---

## Quick recap (for revision)

- Every incoming segment is first matched to an owning socket via a 4-tuple hash lookup — `tcp_ehash` for established connections (the overwhelming majority of real traffic), a separate listening-socket table for new connection attempts.
- `tcp_rcv_established()`'s fast path vs. slow path split exists purely for performance: the common, unremarkable case (exactly the expected next segment) is handled with minimal work; anything unusual falls through to full, thorough processing.
- Sequence numbers sort every arriving segment into exactly one of: new in-order data, out-of-order data (queued separately, spliced in once the gap closes), or old/duplicate data (discarded but still ACKed).
- ICMP fragmentation-needed messages feed directly back into a connection's MSS and trigger retransmission at a smaller size — the concrete code-level half of Path MTU Discovery.
- SACK, largely uncommon at this book's writing, is close to universal today — a good reminder that "optional in the RFC" and "optional in practice" can diverge substantially over 20 years.

Next: `08_TCP_Send.md` — the send-side mirror image, including TCP's own segmentation decisions.