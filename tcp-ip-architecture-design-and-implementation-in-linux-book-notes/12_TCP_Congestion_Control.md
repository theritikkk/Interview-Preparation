# 12. TCP Congestion Control
### *(Covers the book's Chapter 12 — "TCP Congestion Control Algorithms")*

`11_TCP_Core_Processing_State_Machine.md`'s sliding window governs how much data the *receiver* can handle. This chapter is about a completely separate constraint: how much data the *network path itself* — routers, links, and shared bandwidth along the way — can handle, which neither side of the connection can directly observe and both have to infer from indirect evidence.

## 12.1 The congestion window — a second, independent limit

Alongside `snd_wnd` (the receiver's advertised window, from the last chapter), every sending socket maintains a second value:

```c
tp->snd_cwnd    // congestion window — this sender's OWN estimate of how
                // much data the network path can currently absorb
```

**The actual amount of data a sender is permitted to have outstanding at any moment is the *smaller* of the two** — `min(snd_wnd, snd_cwnd)`. This is a genuinely important distinction: `snd_wnd` reflects the *receiver's* stated buffer capacity; `snd_cwnd` reflects the *sender's own estimate* of the network's capacity, and no one ever tells a sender directly what that capacity actually is — it has to be inferred, adjusted continuously, from how the network behaves.

## 12.2 Slow start — probing a new (or reset) connection carefully

A brand new connection has no information at all about the path's actual capacity. Rather than guessing large and risking immediately overwhelming a congested or low-bandwidth path, **slow start** begins deliberately conservative and ramps up quickly if things go well:

```c
tp->snd_cwnd = /* initial value — historically as low as 1 MSS,
                  later RFCs standardized somewhat larger initial
                  values as typical bandwidths grew */;

/* on EVERY new ACK received while in slow start: */
tp->snd_cwnd += 1;   /* (in MSS units) — roughly, this DOUBLES
                        the window every single round-trip time,
                        since every segment in a full window gets
                        its own ACK, each incrementing cwnd by one */
```

The name is genuinely a little misleading — "slow start" grows **exponentially** (doubling per RTT), which is actually one of the *faster* possible ramp-up strategies; the "slow" refers only to the deliberately tiny *starting* point, not the growth rate. This continues until either `cwnd` reaches a separate threshold (`ssthresh`, "slow start threshold" — an estimate, refined over the connection's lifetime, of roughly where congestion previously started becoming a problem) or a loss is actually detected — at which point control shifts to the next phase.

## 12.3 Congestion avoidance — cautious linear growth

Once `cwnd` reaches `ssthresh` (or after recovering from a loss event, as below), aggressive exponential growth would risk immediately re-triggering the exact congestion that was just avoided. **Congestion avoidance** instead grows far more cautiously — roughly linearly, about one MSS *per round-trip-time* (rather than per-ACK, as in slow start):

```c
if (tp->snd_cwnd < tp->ssthresh) {
    /* still in slow start — exponential growth, §12.2 */
} else {
    /* congestion avoidance — much slower, roughly +1 MSS per RTT,
       often implemented as a small fractional increment per ACK
       that sums to about one MSS across a full RTT's worth of ACKs */
}
```

The combined shape — fast exponential ramp-up, then a much more careful, incremental probe for the actual ceiling — is a genuinely sensible general strategy for "find the maximum safe rate of an unknown, shared resource without repeatedly slamming into its limit": grow fast while you're confidently far below any plausible ceiling, then grow cautiously once you're in territory where you might actually be approaching it.

## 12.4 Reacting to loss — and the split between two loss signals

Not all evidence of loss is equally severe, and TCP Reno (the dominant algorithm at this book's writing) distinguishes two cases:

- **Three duplicate ACKs** (the receiver keeps re-acknowledging the same cumulative byte number, meaning something after it clearly isn't arriving, but the receiver evidently *is* still getting *later* segments — otherwise it wouldn't have anything to keep re-ACKing) triggers **Fast Retransmit**: retransmit the apparently-missing segment immediately, without waiting for the full RTO timer (`10_TCP_Timers.md`) to expire — and cut `ssthresh` (and, in the base algorithm, `cwnd` itself) roughly in half, a comparatively *moderate* response, since duplicate ACKs are actually reasonably strong evidence the network is still basically functioning (data is still flowing, just with one gap).
- **A full RTO expiring with no ACK progress at all** is treated as far more severe evidence — reset `cwnd` all the way back down to slow start's minimum, and reset `ssthresh` to about half of whatever `cwnd` had grown to — because a full timeout with *zero* ACK activity is much weaker evidence that the path is even basically healthy; the conservative response assumes the worst and re-probes cautiously from scratch.

**Fast Recovery** is the refinement sitting between "Fast Retransmit just happened" and "back to normal congestion avoidance": rather than dropping straight to slow start after a moderate, duplicate-ACK-detected loss, the connection holds `cwnd` at the halved `ssthresh` value and resumes congestion avoidance's careful linear growth directly from there — avoiding an unnecessarily harsh full reset for what was, again, comparatively mild evidence of a problem.

## 12.5 TCP Vegas — an alternative already known at this book's writing

Reno (and its Fast-Retransmit/Fast-Recovery refinements) is fundamentally **loss-based**: it only learns the path is congested *after* something is actually lost. **TCP Vegas**, covered by this book as an alternative already in circulation, instead watches **RTT itself** as an early-warning signal: if RTT starts creeping up (packets are visibly starting to queue somewhere along the path) *before* any actual loss occurs, Vegas backs off proactively — aiming to operate right at the edge of available capacity without ever actually tipping over into loss-triggering congestion at all. Vegas never achieved anything close to Reno's real-world dominance (a range of practical deployment and fairness-when-competing-with-Reno-flows issues), but the core idea — **use delay, not just loss, as a congestion signal** — is directly the intellectual ancestor of this chapter's most significant "what's changed" entry below.

## 12.6 What's changed since kernel 2.4.20

- **BBR (Bottleneck Bandwidth and Round-trip propagation time, Google, 2016)** is the single biggest conceptual shift in this whole area since this book was written. Rather than Reno's "grow until something is lost, then back off" reactive loop, BBR builds an active **model** of the path — continuously estimating both the bottleneck link's actual bandwidth and the minimum (uncongested) RTT — and paces sending to match that model directly, rather than treating packet loss as the primary signal at all. This directly picks up the "watch delay, not just loss" instinct behind Vegas (§12.5) and builds a considerably more sophisticated, actively-model-based algorithm on top of it, and is in wide real-world production use today (notably by Google itself, and available as a selectable congestion-control module in current Linux kernels alongside Reno/Cubic).
- **CUBIC**, not covered by this book at all, has been Linux's actual **default** congestion-control algorithm since relatively early in the 2.6 series — a Reno-family, still fundamentally loss-based algorithm, but using a cubic (rather than purely linear) growth function specifically tuned to perform well on the kind of high-bandwidth, high-latency ("long fat pipe") links that have become far more common since 2002 than they were when this book's Reno-centric picture was written.
- **Pluggable congestion control**: modern Linux doesn't hardcode one single algorithm system-wide — `tcp_congestion_control` (a per-connection-settable, module-based framework) lets Reno, Cubic, BBR, Vegas, and several others coexist, with the specific algorithm selectable per-socket or system-wide-default — a flexibility this book's single-algorithm-at-a-time picture doesn't anticipate at all.
- **ECN** (flagged already in `02_Protocols_TCP_IP_ARP_ICMP.md`'s "what's changed") gives modern congestion control a genuine third signal beyond "loss" and "RTT increase": an explicit, in-band marking from a congested router, received *before* that router would otherwise have had to drop anything — letting a connection react to real, current congestion without needing to infer it indirectly from either lost packets or subtle delay changes at all.

---

## Quick recap (for revision)

- Two independent limits on outstanding data: `snd_wnd` (receiver's stated buffer capacity — last chapter) and `snd_cwnd` (sender's own inferred estimate of the *network's* capacity — this chapter). The actual limit is always the smaller of the two.
- Slow start grows exponentially (doubling per RTT) despite the name — "slow" describes the tiny starting point, not the growth rate; congestion avoidance afterward grows much more cautiously, roughly linearly per RTT.
- Three duplicate ACKs (Fast Retransmit) is treated as moderate evidence of loss (halve `cwnd`/`ssthresh`, keep going via Fast Recovery); a full RTO timeout with zero ACK progress is treated as severe evidence (reset all the way back to slow start).
- Reno/Vegas is this book's whole world: Reno reacts to loss; Vegas reacts proactively to rising RTT — a genuinely different philosophy that didn't win in practice but directly foreshadows BBR.
- **BBR is the standout "what's changed" here**: an actively bandwidth/RTT-modeling algorithm, not merely a refined loss-reactive one — arguably the most significant conceptual advance in mainstream TCP congestion control since this book's entire Reno/Vegas-centered chapter was written.

Next: `13_Netlink_Sockets.md` — stepping away from the data path entirely, into the kernel/userspace control-plane interface behind tools like `ip route` and `netstat`.