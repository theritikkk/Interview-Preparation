# 8. TCP Send
### *(Covers the book's Chapter 8 — "TCP send")*

The mirror image of the last chapter, starting from `06_Packet_Movement_Across_Layers.md`'s other end: your application calls `send()`/`write()`, and this chapter is everything that happens before that data becomes one or more real segments heading toward IP.

## 8.1 `tcp_sendmsg()` — getting user data into the kernel

```c
int tcp_sendmsg(struct sock *sk, struct msghdr *msg, int size)
```

The very first thing this function has to reckon with: **the application's write and the eventual wire segments are not the same size, and don't have to correspond one-to-one.** You might call `send()` once with 50KB of data; that has to become several MSS-sized segments. You might call `send()` several times with small amounts; TCP is free to (and, for efficiency, generally wants to) coalesce that into fewer, larger segments rather than sending one tiny packet per call.

The core loop, conceptually:
1. **Check send-buffer space.** Before copying anything, verify the socket's send buffer (`sk->wmem_alloc` against `sk->sndbuf` — fully detailed in `09_TCP_Memory_Management.md`) has room. If not, and the socket is blocking, the calling process sleeps here — this is one of the two places (the other being receive-buffer exhaustion) where an ordinary `send()` call can legitimately put your process to sleep waiting on TCP's own internal bookkeeping, not just waiting on the network.
2. **Allocate (or reuse) an `sk_buff`**, reserving headroom exactly as `05_sk_buff.md` described, and copy up to one MSS-worth of the application's data into it via `skb_put()`.
2. **Decide whether to actually push this segment out now, or hold it, hoping to combine it with more data the application might send next.** This is exactly where **Nagle's algorithm** (developed fully in `11_TCP_Core_Processing_State_Machine.md`) makes its decision — for now, just note that `tcp_sendmsg()` isn't purely "copy data, send immediately" for every call; whether a given chunk goes out right away or waits is a genuine, deliberate decision point right here.
3. **Repeat** until all of the application's data has been consumed into one or more queued segments.

Every constructed segment gets appended onto `sk->write_queue` — this is both "the queue of data waiting to be sent" *and*, importantly, doubles as the retransmit-tracking structure: a segment isn't removed from this queue merely because it's been transmitted once — it stays there, unacknowledged, until the corresponding ACK actually arrives (or it's given up on and the connection is dropped). This is exactly the `skb_clone()` scenario `05_sk_buff.md` §5.4 flagged: one clone heads down through IP and the driver for actual transmission; the "original" (or another clone, depending on the specific implementation's bookkeeping) stays on the write queue, ready to be handed to IP again unchanged if a retransmit timer (`10_TCP_Timers.md`) fires before an ACK shows up.

## 8.2 Segmentation — how much data goes in one segment

The size of each outgoing segment is bounded by the smaller of two independently-determined limits:

- **MSS (Maximum Segment Size)**, negotiated during the handshake (`02_Protocols_TCP_IP_ARP_ICMP.md` §2.2) — the *peer's* stated limit on what it's willing to receive in one segment.
- **The current path's effective MTU**, minus header overhead — which, unlike MSS, **can change mid-connection**, specifically in reaction to an ICMP fragmentation-needed message (§8.3 below).

`tcp_sendmsg()`'s segmentation loop uses `tp->mss_cache` — the smaller of these two, kept up to date — as its actual per-segment size limit, meaning the "how big is each chunk" decision described in §8.1's loop isn't a fixed constant at all; it's a live value that can shrink partway through a connection's lifetime.

## 8.3 Reacting to Path MTU Discovery, from the sending side

`07_TCP_Receive.md` §7.5 described the *receiving* half of this interaction (an ICMP message arrives, gets routed to the right connection). This chapter is the other half — what actually happens once that notification reaches TCP's send-side state:

1. `tp->mss_cache` is reduced to reflect the new, smaller usable size the ICMP message implied.
2. **Any already-transmitted-but-unacknowledged segments larger than the new limit have to be split and retransmitted** — data that was already correctly sent at the old, larger MSS may no longer fit through the path that just rejected it, so it can't simply be left as-is waiting for an ACK that may never come.
3. All *future* segments for this connection use the new, smaller `mss_cache` value until (if ever) something indicates a larger size is safe again.

This is a genuinely elegant, if reactive, design: TCP doesn't need any special "probe the path's MTU" mechanism of its own — it simply reacts, correctly, to whatever the IP layer and intervening routers tell it, exactly once, exactly when a real oversized-packet problem is actually encountered — which is also exactly why a network that silently drops all ICMP (flagged as a real, common footgun back in `02_Protocols_TCP_IP_ARP_ICMP.md`) breaks this specific mechanism in a way that manifests as connections that work fine for small transfers and then simply stall on larger ones, with no obvious error anywhere.

## 8.4 Building the actual segment: `tcp_transmit_skb()`

```c
void tcp_transmit_skb(struct sock *sk, struct sk_buff *skb)
```

Once a segment's payload is settled (§8.1–8.2), this function is where the actual TCP header gets written into the buffer's reserved headroom (`skb_push()`, per `05_sk_buff.md`): sequence number, the *current* acknowledgment number and window (both read fresh, right at transmission time, from the connection's live receive-side state — not decided back when the segment was originally queued, since plenty of time, and plenty of newly-arrived data, may have passed since then), flags, and finally the checksum (computed over the pseudo-header + TCP header + data, per `02_Protocols_TCP_IP_ARP_ICMP.md` §2.2). Once the header is written, the `sk_buff` is handed down to `ip_queue_xmit()` — leaving TCP's own responsibility for this segment and continuing down the transmit path `06_Packet_Movement_Across_Layers.md` already laid out in full.

## 8.5 Avoiding tiny, wasteful segments — a preview

Two related but distinct concerns, both fully developed in `11_TCP_Core_Processing_State_Machine.md`, are worth flagging here since they're both *send-side* decisions that live conceptually right alongside everything in this chapter: **Nagle's algorithm** (should *this sender* delay sending a small amount of data, hoping to combine it with more?) and the **silly window syndrome** avoidance logic (should this sender bother sending *anything* right now, if the receiver has only advertised a tiny sliver of window space?). Both exist to prevent the network from being flooded with a stream of needlessly small segments — one deciding on behalf of a chatty sender, the other reacting to a receiver that's revealing window space only a little at a time.

## 8.6 What's changed since kernel 2.4.20

- **TSO/GSO (TCP/Generic Segmentation Offload)**: rather than `tcp_sendmsg()`'s loop building one MSS-sized `sk_buff` per segment as described in §8.2, a modern kernel very commonly constructs **one much larger "superpacket"** `sk_buff` (tagged with GSO metadata) and defers the actual splitting into wire-sized segments to either the network card's own hardware (TSO) or a late software step just before the driver (GSO) — a substantial restructuring of exactly the loop this chapter walks through, done purely for CPU-efficiency reasons (far fewer trips through the full header-construction and checksum logic, for the same amount of data).
- **Zero-copy sends** (`MSG_ZEROCOPY`, and the `sendfile()`/`splice()` mechanisms noted in `05_sk_buff.md`'s "what's changed") can avoid even the one copy step §8.1 describes (copying application data into an `sk_buff`) for data that's already sitting in kernel-accessible memory (a file's page cache, most commonly) — genuinely eliminating a copy this chapter's model treats as simply unavoidable.
- **BBR and other modern congestion-control algorithms** (fully covered in `12_TCP_Congestion_Control.md`) influence *when* and *how much* data `tcp_sendmsg()`'s loop is actually permitted to push out at any given moment far more dynamically, and based on rather different signals (measured bottleneck bandwidth and RTT, rather than primarily loss-based signals), than the Reno-era congestion window logic this book's send-path chapter is written against.

---

## Quick recap (for revision)

- Application writes and wire segments aren't 1:1 — `tcp_sendmsg()`'s job includes both splitting large writes into MSS-sized pieces and potentially coalescing several small writes, subject to Nagle's algorithm's send-now-or-wait decision.
- Segments stay on `sk->write_queue` even after transmission, specifically so they're available for retransmission if no ACK arrives in time — this queue is a retransmit buffer, not just an outbound staging area.
- The effective per-segment size limit (`mss_cache`) is the smaller of the peer-negotiated MSS and the current path's discovered MTU — and the latter can shrink mid-connection in direct reaction to an ICMP fragmentation-needed message, forcing already-sent-but-unacked data to be resegmented and retransmitted.
- `tcp_transmit_skb()` fills in sequence/ack/window/checksum fresh, at the actual moment of transmission — not back when a segment was first queued — since connection state can have moved on in the meantime.
- TSO/GSO's "build one big superpacket, let hardware or a late software step split it" model is the single biggest structural change to this whole chapter's picture since 2.4.20 — a real restructuring of the segmentation loop, driven purely by the CPU cost of doing full per-segment header/checksum work at high packet rates.

Next: `09_TCP_Memory_Management.md` — the send-buffer and receive-buffer accounting this chapter's §8.1 leaned on without fully explaining.