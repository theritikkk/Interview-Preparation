# 11. TCP Core Processing: The State Machine, Sliding Window, and SACK
### *(Covers the book's Chapter 11 — "TCP: Core Functions")*

This chapter is the conceptual center of the whole book — the state machine every connection's life follows from birth to death, and the two mechanisms (sliding window, Nagle's algorithm) that govern how much data actually flows while a connection is alive, plus the modern refinement (SACK/D-SACK) that makes loss recovery dramatically more precise than the base protocol alone allows.

## 11.1 The TCP state machine

Every `struct sock` representing a TCP connection carries a `state` field taking one of a fixed, well-known set of values, and the *entire* life of a connection is a walk through this state diagram, one event (a received segment, a local `close()` call, a timer firing) at a time:

```
                     CLOSED
                        │
          (application calls listen())             (application calls connect(), sends SYN)
                        │                                          │
                        ▼                                          ▼
                    LISTEN                                      SYN_SENT
                        │                                          │
            (SYN arrives, SYN-ACK sent)                 (SYN-ACK arrives, ACK sent)
                        │                                          │
                        │                                          │
                        ▼                                          │
                  SYN_RECEIVED ──────────────────(ACK arrives)─────┤
                        │                                          │
                        └─────────────┬────────────────────────────┘
                                      ▼
                                ESTABLISHED (data flows both ways)
                                                    │
                                                    │
          ┌─────────────────────────────────────────┼─────────────────────────────────────────────┐
          │                                         │                                             │
(local close(), FIN sent)                (FIN arrives from peer)                 (both happen ~simultaneously)
            │                                       │                                             │
            ▼                                       ▼                                             ▼
        FIN_WAIT_1                             CLOSE_WAIT                           ... (simultaneous close variant)
            │                                       │                        
    (ACK for our FIN arrives)             (local close(), FIN sent)
            │                                       │
            ▼                                       ▼
        FIN_WAIT_2                              LAST_ACK
            │                                       │
    (peer's FIN arrives,ACK sent)        (ACK for our FIN arrives)
            │                                       │
            ▼                                       ▼
        TIME_WAIT                                 CLOSED
            │
            │
            │
    (2×MSL timer expires — 10_TCP_Timers.md §10.6)
            │
            │
            ▼
          CLOSED
```

A few things worth internalizing about this diagram rather than just memorizing the boxes: **`CLOSE_WAIT` is the state a well-behaved server-side application is expected to notice and react to** — it means the peer has already sent its `FIN` (no more data coming from them), but *this* side hasn't called `close()` yet; a connection stuck in `CLOSE_WAIT` for a long time almost always means an application bug (it received the peer's disconnect via a `recv()` returning `0`, exactly as both previous study packs' socket-programming chapters described, but never actually called `close()` in response). And the split between `FIN_WAIT_1`/`FIN_WAIT_2` versus `CLOSE_WAIT`/`LAST_ACK` exists purely because **either side can initiate the close** — the diagram has two genuinely separate paths specifically because "I closed first" and "the peer closed first" are different situations requiring different subsequent handling, even though both eventually converge on the connection being fully torn down.

`tcp_rcv_state_process()` is the function that actually implements this diagram — for any state *other* than `ESTABLISHED`, this is what `tcp_v4_do_rcv()` (from `07_TCP_Receive.md` §7.1) dispatches into, handling exactly the state transitions above based on which flags and sequence numbers a given incoming segment carries.

## 11.2 The sliding window — flow control, made concrete

You've seen the *mechanism* already (the `Window` header field, `09_TCP_Memory_Management.md`'s buffer-driven advertisement) — this section is the *model* that field implements. Picture the sender's outstanding data as a window sliding along the byte stream:

```
      already ACKed          in flight, unACKed         not yet sent
   ─────────────────┤────────────────────────────┤──────────────────►
                    ▲                            ▲
                  snd_una                    snd_una + snd_wnd
                (oldest unacked byte)         (edge of what's currently allowed to be outstanding)
                                                
```

`snd_wnd` (the sender's current understanding of the receiver's advertised window) defines how far past the oldest still-unacknowledged byte the sender is currently permitted to have data outstanding. As ACKs arrive, `snd_una` advances (the window's trailing edge slides forward) — and, independently, as the receiver's own buffer situation changes, `snd_wnd` itself can grow or shrink, sliding the window's *leading* edge. **This is exactly why it's called a "sliding" window rather than a fixed-size buffer**: both edges move, independently, driven by two different signals (data being acknowledged vs. the receiver's own advertised capacity).

### Silly Window Syndrome — and the fixes on both ends

A genuine historical pathology: if a receiver's application reads data one byte at a time, slowly, the receiver's advertised window can end up creeping open by tiny increments (one byte at a time) — and a naive sender, seeing *any* nonzero window, might respond by immediately sending a single-byte segment to fill it. The result: a stream of absurdly tiny, wildly inefficient segments (each carrying more header overhead than payload) — "silly window syndrome." Two independent fixes, one per side, both still standard:

- **Receiver-side (Clark's algorithm)**: don't advertise a window increase at all until it's grown to a genuinely worthwhile size (a full MSS, or half the receive buffer, whichever is smaller) — better to advertise nothing new for a while than to advertise a tiny, tempting-but-wasteful sliver.
- **Sender-side**: don't rush to fill a small advertised window either — wait for either a genuinely full-MSS's worth of window, or a reasonable timeout, before sending, rather than opportunistically stuffing in a tiny segment the instant any window at all appears.

## 11.3 Nagle's algorithm — the sender-side small-write problem

Distinct from silly-window syndrome (which is about a *receiver's* tiny window), **Nagle's algorithm** addresses a *sender-side* problem: an application that calls `send()` repeatedly with small amounts of data (a classic example: an old-style remote terminal program sending one keystroke at a time) would, without Nagle, generate one tiny, header-heavy segment per keystroke — again, wasteful.

The rule: **if there's already unacknowledged data outstanding on this connection, hold any further small writes until either that outstanding data gets ACKed, or enough data has accumulated to fill a full segment.** Concretely:

```c
if (unacked_data_already_outstanding && this_write_is_small) {
    /* buffer it, don't send yet — wait for the ACK, or for more data
       to accumulate to a full segment's worth */
} else {
    /* send immediately — either the connection is currently "quiet"
       (nothing outstanding to wait on), or this write is big enough
       to justify sending on its own */
}
```

This is precisely the mechanism `10_TCP_Timers.md` §10.3 flagged interacting badly with the receiver's delayed-ACK timer — Nagle waits for an ACK before sending more; delayed ACK waits before sending that very ACK; each side's individually-sensible optimization can combine into a visible stall. **`TCP_NODELAY`** is the socket option (available in every socket API you've used across this whole study-pack series) that disables Nagle's algorithm entirely for a given socket — the standard fix applications reach for when they need every small write to hit the wire immediately (interactive protocols, latency-sensitive request/response patterns where the "wait for more data" assumption Nagle makes simply doesn't hold).

## 11.4 SACK and D-SACK — precise loss recovery

Plain cumulative acknowledgment (`02_Protocols_TCP_IP_ARP_ICMP.md` §2.2 — "everything up to byte X") has a real weakness: if a sender has multiple segments outstanding and *one* in the middle is lost, cumulative ACKs alone can only ever say "I got everything up to right before the gap" — giving the sender no direct information about which of the *later*, already-arrived segments don't need to be resent.

**SACK (Selective Acknowledgment, RFC 2018)** fixes this directly: a receiver can report specific, individual ranges of data it has successfully received, even if there's a gap before them:

```
 Sent segments:     [1-1000] [1001-2000] [2001-3000] [3001-4000]
 Received:          [1-1000]     LOST     [2001-3000] [3001-4000]

 Cumulative ACK alone says:  "I have everything up to byte 1000."
                              (says nothing about 2001-4000, even
                               though it genuinely all arrived!)

 SACK adds:  "...and ALSO, separately, I already have 2001-4000."
```

Armed with this, the sender knows to retransmit *only* the genuinely missing `1001-2000` range — not the already-received-but-unacknowledged-by-the-cumulative-number segments that SACK is now explicitly vouching for. This is a substantial efficiency win whenever more than one segment is lost from a single window, which cumulative-ACK-only TCP handles considerably more clumsily (often needing several round trips to recover the same information SACK conveys in one).

**D-SACK (Duplicate SACK, RFC 2883)** extends the same mechanism to report data that was received **more than once** — useful for a receiver to tell the sender "that retransmission you just sent was unnecessary, I already had this" — which, in turn, gives the sender (and its congestion-control logic, `12_TCP_Congestion_Control.md`) useful evidence that a recent retransmission was spurious (perhaps triggered by reordering rather than genuine loss), information that can feed back into avoiding an unnecessarily conservative congestion-window reduction for what turned out not to be real loss at all.

## 11.5 What's changed since kernel 2.4.20

- **SACK, described here as an enhancement, is essentially universally negotiated on the modern internet** — practically every real-world TCP connection today uses it; this book's era is closer to the transition point where SACK was still a meaningful, non-default-everywhere improvement rather than baseline behavior.
- **The state machine diagram in §11.1 is essentially unchanged** — this is one of the most stable parts of the entire protocol across the 20+-year gap; RFC 793's states are still exactly the states a modern `ss -tan` or `netstat` output will show you.
- **TCP Fast Open (RFC 7413)**, flagged already in `02`'s "what's changed," effectively adds a new, book-absent shortcut across part of this diagram — letting data ride along with the initial `SYN` for connections to a previously-contacted server, under specific security-cookie-based conditions, rather than strictly waiting for the full three-way handshake to complete first.
- **`TCP_NODELAY`'s existence and purpose (§11.3) is completely unchanged** — this remains the standard, correct answer today for exactly the same "Nagle is fighting my latency-sensitive application" problem it was in 2002.

---

## Quick recap (for revision)

- The state machine has two genuinely separate closure paths (`FIN_WAIT_1/2` vs. `CLOSE_WAIT`/`LAST_ACK`) specifically because either side can initiate closure — know which path a given connection is on by knowing who sent the first `FIN`.
- A connection stuck in `CLOSE_WAIT` almost always means an application bug — the peer already left, and this side simply hasn't called `close()` yet.
- Sliding window: `snd_una` (trailing edge) advances on ACKs; `snd_wnd` (window size, hence the leading edge) is driven independently by the receiver's advertised capacity.
- Silly Window Syndrome has two independent fixes — Clark's algorithm (receiver: don't advertise tiny window increases) and a matching sender-side "don't rush to fill a tiny window" rule.
- Nagle's algorithm (sender: hold small writes while data is outstanding) can interact badly with delayed ACK (receiver: hold bare ACKs hoping to piggyback) — `TCP_NODELAY` is the standard escape hatch for latency-sensitive applications.
- SACK reports specific received ranges beyond the cumulative ACK number, letting a sender retransmit precisely what's actually missing rather than guessing; D-SACK additionally reports *duplicate* receipt, useful evidence against unnecessary congestion-window reduction.

Next: `12_TCP_Congestion_Control.md` — slow start, congestion avoidance, and how a sender decides how much data the *network* (as opposed to the receiver) can currently handle.