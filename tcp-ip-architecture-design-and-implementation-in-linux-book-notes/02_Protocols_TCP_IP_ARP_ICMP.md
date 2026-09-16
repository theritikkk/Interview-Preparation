# 2. Protocols: TCP, IP, ARP, and ICMP
### *(Covers the book's Chapter 2 — "Protocols")*

Every later chapter walks kernel code that builds, parses, or reacts to the exact headers this chapter lays out. If you can't read a TCP or IP header field-by-field from memory by the end of this chapter, the implementation chapters will feel like arbitrary bit-shifting instead of "oh, that's just setting the flags field."

## 2.1 The IP header (RFC 791)

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|Version|  IHL  |Type of Service|          Total Length        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|         Identification       |Flags|      Fragment Offset    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Time to Live |    Protocol   |         Header Checksum      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       Source Address                         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Destination Address                       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

Field by field, with the "why does this exist" that the implementation chapters lean on:

- **Version** (4 bits): `4` for IPv4. This is the very first thing any IP-layer code checks — Chapter 6 shows the receive path branching on this before anything else.
- **IHL** (4 bits, "Internet Header Length"): the header's length in 32-bit words, *not* bytes — a plain header with no options is `IHL = 5` (20 bytes); options make it longer, up to `IHL = 15` (60 bytes). Every routine that needs to find where the header ends and the payload begins computes `IHL * 4`.
- **Type of Service (ToS)**: originally meant to let senders request low-delay, high-throughput, or high-reliability handling; today this byte is almost universally repurposed as **DSCP** (Differentiated Services Code Point) for QoS marking, which is exactly what Chapter 15's queuing disciplines act on.
- **Total Length**: the *entire* packet's length (header + data) in bytes, up to 65,535 — this is how a receiver knows where this packet ends, distinct from the underlying link layer's own framing.
- **Identification, Flags, Fragment Offset**: together, these implement **IP fragmentation** — if a packet is too big for some link along the path, a router can split it into fragments, each carrying the same Identification value (so the receiver knows which fragments belong together), a flag bit (`MF`, "more fragments") on all but the last piece, and a Fragment Offset giving each piece's position (in 8-byte units) within the original packet. The receiver reassembles using these three fields together.
- **Time to Live (TTL)**: decremented by one at every router hop; a router that would decrement it to zero drops the packet instead and sends back an ICMP "Time Exceeded" — this is the *exact* mechanism `traceroute` exploits, as covered from the diagnostic-tooling side in the previous study pack's Chapter 12.
- **Protocol**: which upper-layer protocol the payload belongs to — `6` for TCP, `17` for UDP, `1` for ICMP. This is the field every later chapter's "which handler do I dispatch this packet to" logic switches on.
- **Header Checksum**: a simple ones'-complement checksum over the IP header *only* (not the payload) — recomputed at every hop, since TTL (and therefore the header) changes at every hop.
- **Source Address / Destination Address**: the 32-bit IPv4 addresses you've used throughout the previous two study packs' `sockaddr_in`/`inet_pton()` calls — this is where they actually live on the wire.

## 2.2 The TCP header (RFC 793)

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|          Source Port         |       Destination Port       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        Sequence Number                       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Acknowledgment Number                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Data |     |U|A|P|R|S|F|                                     |
| Offset| Rsvd|R|C|S|S|Y|I|            Window                   |
|       |     |G|K|H|T|N|N|                                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|           Checksum           |         Urgent Pointer        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

- **Source/Destination Port**: identifies which application on each end — this, combined with the two IP addresses, forms the **socket 4-tuple** (source IP, source port, dest IP, dest port) that Chapter 3's socket lookup hash tables key on.
- **Sequence Number**: identifies the position of the *first byte of data in this segment* within the overall byte stream — remember, from the socket-programming study packs, that TCP delivers a byte stream with no message boundaries; the sequence number is literally how the receiver reassembles that stream in the correct order even if segments arrive out of order.
- **Acknowledgment Number**: valid whenever the `ACK` flag is set (which is essentially always, after the initial `SYN`) — it means "I have successfully received everything up to (but not including) this byte number." This is a **cumulative** acknowledgment — receiving ack number 5001 means "I have all of bytes 1–5000," not just "I got the specific segment you're now acking."
- **Data Offset** (4 bits): like IP's IHL, the TCP header's length in 32-bit words — needed because TCP options (below) make the header variable-length.
- **Flags**: `URG` (urgent pointer is valid), `ACK` (acknowledgment number is valid), `PSH` (push — deliver this data to the application immediately, don't buffer waiting for more), `RST` (reset — abort the connection, immediately, no graceful handshake), `SYN` (synchronize — used only during connection establishment to exchange initial sequence numbers), `FIN` (finish — sender has no more data, begin graceful closure).
- **Window**: the receiver's current **advertised receive window** — "I have this many bytes of buffer space free right now, don't send me more than that without waiting for it to open up." This single field is the entire mechanism behind TCP's flow control, walked through in detail in `11_TCP_Core_Processing_State_Machine.md`.
- **Checksum**: unlike IP's checksum, TCP's checksum covers the TCP header **and** the data **and** a "pseudo-header" borrowed from the IP layer (source/dest IP, protocol number, TCP length) — meaning TCP's checksum is only valid in the context of a specific IP source/destination pair, a deliberate design choice to catch certain classes of misdelivery that a payload-only checksum wouldn't.
- **Urgent Pointer**: valid when `URG` is set — an offset marking where "urgent data" ends within the segment. In practice, genuinely rare in real-world use; Telnet's interrupt-processing is close to the canonical example, and the previous study pack's Beej's Guide notes didn't need to touch it at all.

### TCP options worth knowing by name

Beyond the fixed 20-byte header, TCP options (carried in the space `Data Offset` accounts for) are how the protocol has been extended over decades without breaking the base format:

- **MSS (Maximum Segment Size)**: exchanged during the `SYN`/`SYN-ACK` handshake — each side tells the other the largest segment it's willing to receive, directly shaping how big a chunk of data the sending side's segmentation logic (`08_TCP_Send.md`) will ever put in one packet.
- **Window Scale (RFC 1323)**: the base `Window` field is only 16 bits (max 65,535 bytes) — hopelessly small for high-bandwidth, high-latency ("long fat pipe") modern links. This option, negotiated once at connection setup, adds a scale factor so the *effective* window can be far larger.
- **Timestamps (RFC 1323)**: lets both sides measure round-trip time far more precisely (feeding the RTO calculations in `10_TCP_Timers.md`) and, as a secondary benefit, guards against a subtle old-sequence-number-wraparound ambiguity ("PAWS," Protection Against Wrapped Sequence numbers) on very fast connections.
- **SACK-Permitted / SACK (RFC 2018)**: lets a receiver acknowledge *specific* non-contiguous ranges of received data, rather than only the single cumulative "everything up to X" number — hugely valuable when multiple segments are lost from one window, since the sender then knows exactly which gaps to retransmit instead of guessing. Covered in depth in `11_TCP_Core_Processing_State_Machine.md`.

## 2.3 ARP (RFC 826) — mapping an IP address to a hardware address

IP addresses are a software-layer construct; actually putting a frame on an Ethernet wire needs a hardware (MAC) address. **ARP** is the protocol that answers "what's the MAC address for this IP address, on this local network segment?" — a genuinely simple broadcast-request/unicast-reply exchange:

1. Host A broadcasts an ARP request to the entire local segment: *"Who has 192.0.2.5? Tell 192.0.2.10."*
2. Every host on the segment receives it; only the host that actually owns `192.0.2.5` replies, directly (unicast) to A, with its MAC address.
3. A caches the mapping (in the kernel's **ARP/neighbor table**) so it doesn't have to repeat this exchange for every single packet.

ARP is deliberately **not** routed beyond the local network segment — it's a Link-layer-adjacent mechanism, meaningful only among hosts that can directly reach each other without going through a router. This is exactly why every host needs its *own* ARP cache per local network it's attached to, and why ARP entries expire and get refreshed periodically (hardware can change — a network card gets replaced, a machine gets a new NIC — without the IP address changing).

## 2.4 ICMP (RFC 792) — the network's own control/error-signaling protocol

ICMP is carried directly inside IP (protocol number `1`), but it isn't really "just another payload" the way TCP or UDP are — it's the mechanism IP itself uses to report problems and diagnostics *about* IP delivery. Key message types, several of which you'll see specific kernel handling for in later chapters:

| Type | Meaning | Where it matters later in this book |
|---|---|---|
| 0 | Echo Reply | `ping`'s response |
| 3 | Destination Unreachable (with sub-codes: network/host/port/protocol unreachable, fragmentation needed but DF set, etc.) | The "fragmentation needed" sub-code is exactly how **Path MTU Discovery** works — a router that can't forward an oversized packet without fragmenting it (and the packet has the "Don't Fragment" bit set) sends this back instead, and TCP's send path (`08_TCP_Send.md`) has to react by shrinking its effective MSS and retransmitting |
| 5 | Redirect | A router telling a host "you should have sent that via a different, better gateway" — updates the sender's routing decisions |
| 8 | Echo Request | `ping`'s request |
| 11 | Time Exceeded | TTL hit zero (or a fragment-reassembly timer expired) — `traceroute`'s core mechanism |

The **Destination Unreachable / fragmentation-needed** interaction with TCP deserves emphasis, since Chapter 8 revisits it directly: TCP has no idea, on its own, what the actual MTU of every link along a path is — it finds out reactively, from ICMP telling it "that didn't fit," and has to adjust after the fact. This reactive-discovery design is also exactly why a network that silently drops ICMP entirely (a surprisingly common, well-intentioned-but-harmful firewall misconfiguration) causes a specific, notorious class of "connection hangs, but only for large transfers" bugs — the sender never receives the signal telling it to shrink its segments.

## 2.5 What's changed since kernel 2.4.20

- **IPv6** existed in only early, experimental form in the 2.4 series. It has since matured into a fully first-class, production-grade part of the stack — with its own header format (simpler and fixed-length compared to IPv4's variable-length-with-options design), its own control protocol (**ICMPv6**, which subsumes some of plain ICMP's job *and* takes over ARP's job via **Neighbor Discovery Protocol**, meaning a pure-IPv6 network has no ARP at all), and is the default-preferred protocol on most modern systems and networks when both are available.
- **SACK** (§2.2) was still relatively fresh and not universally deployed at the time this book was written; it's been standard, close-to-universally-negotiated behavior on the modern internet for a long time now.
- **ECN (Explicit Congestion Notification, RFC 3168)**: repurposes two previously-reserved bits in the IP header's ToS/DSCP byte and two TCP flag bits, letting a congested router mark a packet instead of dropping it outright, and letting TCP react to that mark as a congestion signal without needing an actual packet loss — not covered by this book at all, and worth knowing exists when you get to `12_TCP_Congestion_Control.md`.
- **TCP Fast Open (RFC 7413)** and **Multipath TCP (RFC 8684)** are two considerably newer extensions — the former lets data ride along with the very first `SYN` in specific trusted-repeat-connection scenarios, the latter lets a single TCP connection span multiple physical network paths simultaneously (a phone using Wi-Fi and cellular at once, for the same connection) — both entirely absent from this book's 2002-era protocol picture.

---

## Quick recap (for revision)

- IP: variable-length header (IHL field), fragmentation via Identification+Flags+Fragment-Offset, TTL decremented per hop (the `traceroute` mechanism), Protocol field picks the upper-layer handler.
- TCP: Sequence Number = position of this segment's first byte in the stream; Acknowledgment Number is cumulative ("everything up to here"); Window is the single field behind flow control; checksum covers a pseudo-header too, tying it to a specific IP pair.
- MSS/Window-Scale/Timestamps/SACK are options riding in the header's variable-length tail — each solving a specific, real limitation of the base 1981-era design.
- ARP resolves IP→MAC on a local segment only, via broadcast request/unicast reply, cached in the neighbor table — never routed beyond the local segment.
- ICMP is IP's own error/diagnostic channel — "Destination Unreachable / fragmentation needed" specifically is how Path MTU Discovery works, and a network that blackholes ICMP breaks it in a way that manifests as mysterious large-transfer hangs.
- IPv6 didn't exist in mature form at this book's writing, and has since replaced ARP with ICMPv6-based Neighbor Discovery entirely on IPv6-only networks — one of the largest protocol-level shifts since this book was published.

Next: `03_BSD_Socket_Layer_Implementation.md` — where these wire formats meet actual kernel code for the first time.