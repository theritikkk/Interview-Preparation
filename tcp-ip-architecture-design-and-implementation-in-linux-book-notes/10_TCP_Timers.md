# 10. TCP Timers
### *(Covers the book's Chapter 9/10 material on TCP timers)*

TCP is fundamentally reactive — it responds to segments as they arrive. But plenty of important behavior has to happen when *nothing* arrives: a lost segment needs to be noticed and retransmitted even though, by definition, no packet tells you it was lost. This chapter is every timer the kernel runs per-connection to handle exactly those "waiting for something that might never come" situations.

## 10.1 The retransmission timer (RTO) — and how the timeout value is actually chosen

The single most important timer: if a segment is sent and no ACK arrives before this timer fires, the segment is assumed lost and retransmitted. The hard problem is picking the timeout value (**RTO**, Retransmission TimeOut) — too short, and you retransmit needlessly over ordinary network jitter, wasting bandwidth and potentially making congestion worse; too long, and a genuinely lost segment sits unnoticed far longer than necessary, stalling the connection.

The classic answer — **Jacobson's algorithm** (RFC 6298, building on Van Jacobson's original 1988 paper, and already the standard approach by this book's era) — continuously tracks two statistics from measured round-trip times:

```c
tp->srtt      // smoothed RTT — an exponentially-weighted moving average
tp->rttvar    // smoothed RTT VARIANCE — how much RTT actually fluctuates
```

Updated on every RTT sample:
```
srtt   = (1 - α) * srtt   + α * new_sample            (α typically 1/8)
rttvar = (1 - β) * rttvar + β * |srtt - new_sample|    (β typically 1/4)
RTO    = srtt + 4 * rttvar
```

**Why track variance at all, rather than just the average RTT?** Because a connection with a wildly fluctuating RTT (congested Wi-Fi, a path sharing a bottleneck link with bursty other traffic) genuinely needs a more conservative (larger) timeout than a connection with a rock-steady RTT, even if their *average* RTT happens to be identical — using variance directly in the formula is precisely what makes RTO adapt to *how predictable* a given path is, not just how fast it is on average.

**Karn's algorithm** addresses a specific measurement trap: if a segment had to be retransmitted, and an ACK then arrives, **which** transmission does that ACK actually correspond to — the original, or the retransmission? There's no way to know for certain (this is exactly "retransmission ambiguity"), so Karn's rule is simple and absolute: **never use a retransmitted segment's RTT sample for RTO calculation at all** — only ever measure RTT from segments that were acknowledged without needing any retransmission, sidestepping the ambiguity entirely rather than trying to resolve it.

## 10.2 Exponential backoff

If a retransmission *also* times out without an ACK, the RTO for the *next* retry doesn't just repeat the same value — it **doubles** (up to some maximum), and continues doubling on each successive failure:

```
RTO, 2·RTO, 4·RTO, 8·RTO, ...   (capped at a maximum, and eventually the
                                  connection gives up entirely and resets)
```

This matters most under genuine, sustained congestion: if a whole network path is overloaded, every connection trying to retransmit at its *original* RTO simultaneously would just add to the very congestion causing the problem in the first place. Backing off exponentially spreads retries out over increasing intervals, giving a congested path real breathing room to recover rather than being hit with an ever-more-frantic retry storm.

## 10.3 The delayed ACK timer

Sending an ACK for literally every single received segment, immediately, is wasteful when a reply (carrying its own piggybacked ACK, for free) or a follow-up data segment is likely coming very soon anyway. The **delayed ACK timer** (commonly a short interval, historically around 200ms) lets TCP hold off sending a bare ACK briefly, hoping to either piggyback it on outgoing data the application is about to send, or coalesce it with the ACK for a second incoming segment that arrives shortly after the first.

```c
if (/* application data is about to go out anyway, or */
    /* a second segment arrives before the delayed-ACK timer fires */) {
    /* send ONE ack covering everything so far, right now */
} else if (/* delayed-ACK timer expires with nothing else to piggyback on */) {
    /* send the ACK anyway, standalone — can't wait any longer */
}
```

This directly interacts with **Nagle's algorithm** on the *other* end of the connection (developed fully in `11_TCP_Core_Processing_State_Machine.md`) in a way that's historically caused real, documented performance pathologies: if a sender is deliberately holding small writes (Nagle) waiting for an ACK, and the receiver is deliberately holding that very ACK (delayed ACK) waiting for more data or an outgoing reply that isn't coming — both sides can end up waiting on each other, each for a plausible, individually-reasonable reason, producing a visible stall until one side's timer finally gives up. This specific interaction is well-known enough to have a name (often just called "the Nagle/delayed-ACK interaction") and is a real, practical reason some latency-sensitive applications explicitly disable Nagle's algorithm (`TCP_NODELAY`) on their own sockets.

## 10.4 Keepalive

Ordinary TCP has no built-in "are you still there?" mechanism — an idle connection with nothing to say generates no traffic at all, by design, and TCP alone genuinely cannot distinguish "the peer is fine but has nothing to send" from "the peer's machine crashed/lost power/vanished entirely with no chance to send a `RST` or `FIN`." The **keepalive timer**, an opt-in feature (`SO_KEEPALIVE`, not on by default for exactly the reason above — plenty of legitimately-idle-for-a-long-time connections are completely normal and shouldn't be probed) periodically sends a essentially-empty probe segment after a configurable period of inactivity, specifically to provoke *some* response (even just a plain ACK) proving the peer is genuinely still reachable, or to eventually notice its absence (no response after several probes) and tear the connection down.

## 10.5 The persist timer — surviving a zero window

Recall from `09_TCP_Memory_Management.md`: a receiver whose buffer is genuinely full advertises a **zero window**, correctly telling the sender "stop, I have no room." But this creates a real problem: once the sender stops (correctly, obediently), how does it ever find out the window has opened back up again? The receiver's own window-update segment announcing new space is itself just an ordinary segment, and if it happens to be lost, there's no *data* segment coming from the sender's side to naturally provoke a fresh reply — both sides could, in principle, sit there forever, each waiting on the other, with no future event that breaks the standoff.

The **persist timer** is the sender's insurance against exactly this: periodically, regardless of what the last known window was, send a small **window probe** anyway — deliberately provoking a fresh ACK from the receiver (which necessarily carries the receiver's *current* window value), breaking any possible deadlock caused by a lost window-update segment. This is the single-timer explanation for something that might otherwise look mysterious: "how does a stalled, zero-window connection ever recover if the one segment that would have unstalled it got lost?" — it doesn't rely on that one segment at all; it periodically re-asks instead.

## 10.6 The `FIN_WAIT_2` and `TIME_WAIT` timers

Two timers specifically related to orderly connection shutdown (the full state machine is `11_TCP_Core_Processing_State_Machine.md`'s subject, but both timers are worth introducing here alongside the others):

- **`FIN_WAIT_2` timer**: after this side has sent its own `FIN` and had it acknowledged, but hasn't yet received the peer's corresponding `FIN` — normally a fairly transient state — a timer guards against the peer never sending its half of the closure at all (an application-level bug, or a peer that's simply vanished), eventually forcing the connection closed rather than leaving it in limbo indefinitely.
- **`TIME_WAIT` timer**: after *both* sides have exchanged `FIN`s, the side that closed *last* (sent the final ACK of the closure) lingers in `TIME_WAIT` for a fixed duration — conventionally **2×MSL** (Maximum Segment Lifetime, an estimate of the longest a stray packet could plausibly still be wandering the network) — specifically so any of this connection's own delayed/duplicated packets, still possibly in flight somewhere, get safely discarded (by simply being outside any live connection's recognized state) rather than being confused for a genuinely new, unrelated connection that happens to reuse the exact same 4-tuple soon after. **This is the exact mechanism behind the previous study pack's `SO_REUSEADDR` discussion** — that socket option specifically lets a new `bind()` succeed *despite* a lingering `TIME_WAIT` entry for the same local address, rather than changing anything about `TIME_WAIT` itself.

## 10.7 What's changed since kernel 2.4.20

- **RFC 6298's refined RTO calculation** (published 2011, superseding the original RFC 2988 and, before that, the informal Jacobson-paper-based approach this book's kernel implements) tightens several edge cases in the formula from §10.1 — the core srtt/rttvar-based approach is unchanged in spirit, but minimum-RTO clamping and a few other details have been refined.
- **TCP Timestamps (RFC 1323)**, flagged already in `02_Protocols_TCP_IP_ARP_ICMP.md`, let a modern connection get *far* more frequent, precise RTT samples (essentially one per segment, rather than being constrained by Karn's algorithm to only cleanly-acknowledged, non-retransmitted segments) — meaningfully improving RTO accuracy in practice versus this book's more sample-starved picture.
- **RACK (RFC 8985)**, mentioned already in `07_TCP_Receive.md`'s "what's changed," largely supplements (in some configurations, effectively supersedes for loss-detection purposes) the pure timeout-based retransmission trigger this chapter describes, by using more detailed, per-segment ACK timing to infer loss considerably faster than waiting for the full RTO timer to expire in many common cases.
- **`TCP_USER_TIMEOUT` (a per-socket `setsockopt`)** lets an application set its own, application-specific ceiling on how long unacknowledged data can sit before the *connection itself* is given up on entirely — a finer-grained, application-controllable escalation beyond this chapter's fixed retransmission-timer/backoff model, with no counterpart in the book's era.

---

## Quick recap (for revision)

- RTO = `srtt + 4 × rttvar` — tracking variance, not just average RTT, is what lets the timeout adapt to how *predictable* a path is, independent of how fast it is.
- Karn's rule: never use a retransmitted segment's ACK to sample RTT — the ambiguity (which transmission does this ACK really correspond to?) is unresolvable, so the sample is simply discarded rather than risked.
- Failed retransmissions back off exponentially specifically to avoid making real network congestion worse with synchronized, ever-more-frantic retry storms.
- Delayed ACK (receiver holds a bare ACK briefly, hoping to piggyback) can interact badly with Nagle's algorithm (sender holds small writes, hoping for an ACK) — a real, named, historically significant performance pathology, not a hypothetical one.
- The persist timer's whole job is surviving the loss of a single specific packet (a window-update after a zero-window condition) that would otherwise leave both sides deadlocked with nothing to naturally break the silence.
- `TIME_WAIT`'s ~2×MSL linger is exactly what `SO_REUSEADDR` (from both previous study packs in this series) works *around* — not a duration that option itself changes.

Next: `11_TCP_Core_Processing_State_Machine.md` — the state machine these timers all operate within, plus sliding window, Nagle's algorithm in full, and SACK/D-SACK.