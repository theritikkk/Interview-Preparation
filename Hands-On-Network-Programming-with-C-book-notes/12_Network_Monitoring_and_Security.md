# 12. Network Monitoring and Security
### *(Covers the book's Chapter 12 — "Network Monitoring and Security")*

This chapter has no accompanying example programs in the book's own repository — it's a survey chapter, covering the *concepts* and *tools* behind network diagnostics rather than building one more client/server pair. I've kept that same shape here, with one real, live exception: raw sockets, which I could actually test directly in this sandbox since I'm running with root privileges.

## 12.1 Why monitor a network at all

Everything in this book so far assumed you're writing *one side* of a connection and can just add `printf()`s to see what your own program is doing. Network monitoring is for the much more common real-world situation: something is going wrong (or you need to verify something is going *right*) in a system where you can't simply instrument the code — because it's someone else's program, a black-box appliance, or traffic between two other machines entirely. The tools in this chapter answer questions like "is this host even reachable?", "what's the path my traffic actually takes?", and "what's really going out on this wire?" — diagnostic questions, not application logic.

## 12.2 Testing reachability — `ping` and ICMP

The most basic question — "is anybody home?" — is answered by the `ping` utility, built on **ICMP** (Internet Control Message Protocol), the Internet layer's own control/diagnostic protocol (distinct from both TCP and UDP — it doesn't have "ports" in the same sense at all). `ping` sends an **ICMP Echo Request** packet and waits for the corresponding **ICMP Echo Reply** — if a reply arrives, the round-trip time it took is your basic latency measurement; if nothing comes back within a timeout, that's your "unreachable" signal, though it's worth knowing this isn't always definitive (some firewalls deliberately block ICMP entirely while leaving normal traffic unaffected, so "ping fails" doesn't always mean "host is down").

Sending a raw ICMP packet yourself needs a **raw socket** rather than the `SOCK_STREAM`/`SOCK_DGRAM` sockets you've used everywhere else in this book — see §12.4.

## 12.3 Checking a route — how `traceroute` actually works

`ping` tells you *whether* you can reach a host; `traceroute` tells you the **path** — every router your traffic passes through on the way. The mechanism behind it is a clever reuse of a completely unrelated IP header field, the **TTL** (Time To Live): every router that forwards a packet decrements its TTL by one, and if a router ever decrements a packet's TTL down to zero, it doesn't forward it — it drops the packet and sends back an ICMP **"Time Exceeded"** message to the original sender, identifying itself in the process.

`traceroute` exploits this deliberately: it sends a series of packets with TTL = 1, then TTL = 2, then TTL = 3, and so on. The TTL=1 packet dies at the very first router, which reports itself; the TTL=2 packet survives that first hop (now with TTL=0... wait, more precisely: the first router decrements it to 1 and forwards it) and dies at the *second* router, which reports itself; and so on, one more hop revealed with each increment, until a packet finally reaches the real destination (or a fixed maximum hop count is reached without success). The full path is reconstructed entirely from a sequence of "you've been rejected, and here's who rejected you" error messages — genuinely one of the more elegant hacks in classic internet tooling.

## 12.4 Raw sockets — a real, live demonstration

Every socket in this entire book, until now, has been `SOCK_STREAM` or `SOCK_DGRAM` — the OS handles constructing the IP header (and, for UDP/TCP, the transport-layer header) for you, and you only ever touch the payload. A **raw socket** (`SOCK_RAW`) hands you a much lower-level view: you can see (and, depending on the protocol, construct) the IP-layer headers yourself. This is exactly what you need to send a raw ICMP Echo Request, since ICMP isn't TCP or UDP and has no "port" for a normal socket to bind to.

```c
int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
```

**A genuine, real note**: raw sockets require elevated privileges on every mainstream OS (root on Linux/macOS, or the `CAP_NET_RAW` capability specifically — a normal, unprivileged user's program simply cannot create one, on purpose, since raw sockets are a meaningfully bigger trust boundary than ordinary sockets). This sandbox happens to run as root, which let me actually build and run a real ICMP exchange rather than only describing one:

```c
struct icmp packet;
memset(&packet, 0, sizeof(packet));
packet.icmp_type = ICMP_ECHO;
packet.icmp_id = getpid();
packet.icmp_seq = 1;
packet.icmp_cksum = checksum(&packet, sizeof(packet));   // ICMP requires a manual checksum

struct sockaddr_in dest;
memset(&dest, 0, sizeof(dest));
dest.sin_family = AF_INET;
inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr);

sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr*)&dest, sizeof(dest));
```

**Real output:**
```
Sent ICMP ECHO request to 127.0.0.1 (id=382, seq=1)
Received 48 bytes back. ICMP type=8 (0=ECHO REPLY), id=382, seq=1
```

An interesting genuine wrinkle worth being honest about: the packet actually received back here has ICMP type `8`, which is `ICMP_ECHO` (a request) — not type `0` (`ICMP_ECHOREPLY`). On Linux, a raw ICMP socket listening on the loopback interface can see **its own outgoing request looped back** to it, since loopback delivery on Linux effectively hands a copy of everything relevant to any raw socket that's listening, before or alongside whatever reply eventually comes back — a real, live example of why raw-socket code has to check the specific fields of what it receives (id, sequence number, and specifically the *type* — is this actually a reply, or just an echo of what I sent?) rather than assuming the first thing that arrives is necessarily the answer you were expecting. This is a good, concrete illustration of exactly the kind of low-level nuance raw sockets expose that ordinary `SOCK_STREAM`/`SOCK_DGRAM` code never has to think about — the OS no longer filters or interprets anything for you.

## 12.5 Checking local connections, and snooping on others'

**Checking your own machine's connections** is what `netstat` (or its modern replacement, `ss`, on Linux) is for — listing every open socket on the local machine, what it's connected to, and what state it's in (`LISTEN`, `ESTABLISHED`, `TIME_WAIT` — that last one straight out of `13_Pitfalls_and_Tips.md`'s `SO_REUSEADDR` discussion). This is entirely local bookkeeping the OS already tracks; no special privilege or packet capture needed.

**Watching *other* machines' traffic** on a shared network segment is a different, more powerful capability, and needs a network interface running in **promiscuous mode** — normally, a network card silently discards any packet not addressed to it; promiscuous mode disables that filtering, handing every packet on the physical medium up to the OS regardless of destination. This is the foundation of tools like `tcpdump` and Wireshark. On modern switched networks (as opposed to old-style shared-hub networks), promiscuous mode alone usually only shows you traffic actually destined for or from your own machine, since a switch deliberately only forwards each packet to the port it's actually addressed to — seeing genuinely unrelated traffic between two *other* machines on a switched network typically requires additional techniques (port mirroring configured by whoever administers the switch, or various forms of active traffic redirection) that are a meaningfully bigger deal, both technically and ethically, than simply flipping on promiscuous mode.

## 12.6 Deep packet inspection, and capturing everything

**Deep packet inspection (DPI)** means looking past the basic addressing/port headers and actually parsing the *payload* of traffic — for example, recognizing that a given TCP stream on port 443 is carrying a TLS handshake for `example.com` specifically (visible in the unencrypted SNI field from `09_TLS_and_HTTPS_Client.md`, even though the rest of the traffic is encrypted), or that a stream on some non-standard port is actually HTTP traffic in disguise. This is what corporate firewalls and intrusion detection systems use to enforce policy or spot anomalies based on *what* is being sent, not just *where* it's going.

**Capturing all network traffic** for later, offline analysis is exactly what packet-capture files (`.pcap`) are — a raw, byte-for-byte record of every frame seen on an interface (typically via `libpcap`, the same underlying library `tcpdump` and Wireshark both use), which can then be replayed, filtered, and inspected long after the fact.

## 12.7 Network security and application safety

A recurring, important theme worth stating plainly: **being able to monitor a network doesn't mean you're automatically authorized to.** Everything from raw sockets to promiscuous-mode packet capture is powerful enough to be genuinely useful for legitimate diagnostics — and just as capable of being a serious privacy/security violation if pointed at a network or traffic you don't have explicit permission to inspect. This isn't a hypothetical concern layered on top of otherwise-neutral technology; it's the same reason raw sockets specifically require elevated privileges in the first place (§12.4) — the OS is deliberately gatekeeping a capability it considers meaningfully more dangerous than ordinary networking.

On the *application* side, the recurring lesson from this entire book applies directly here too: never trust data from the network at face value (Chapter 7's directory-traversal guard, Chapter 8's spoofable `MAIL FROM`, Chapter 13's `SIGPIPE`/buffer-exhaustion pitfalls) — and monitoring tools exist precisely because, in practice, *someone* eventually sends malformed, oversized, or malicious input, whether by accident or on purpose, and you'd rather find out from your own diagnostics than from an actual incident.

## 12.8 Network-testing etiquette

A short, practical set of norms worth internalizing, distinct from strict legality: don't port-scan, packet-capture, or load-test infrastructure you don't own or explicitly have permission to test, even "just out of curiosity" — aggressive scanning can look identical to an actual attack from the receiving end's monitoring (which may trigger real incident response on their side over what you intended as harmless exploration), and can generate real load or false alarms on shared infrastructure you don't have visibility into. When you *do* have legitimate reason to test something (your own servers, a client's infrastructure under a written agreement, a CTF/lab environment built for the purpose), scope your testing narrowly to what's actually authorized, and prefer testing during low-traffic windows against production systems specifically to minimize any chance of real user impact.

---

## Quick recap (for revision)

- ICMP is the Internet layer's own diagnostic protocol — distinct from TCP/UDP, with no ports, used by both `ping` (Echo Request/Reply) and `traceroute`.
- `traceroute` works by deliberately sending packets with an incrementing TTL and reading the "Time Exceeded" error each expired packet provokes from whichever router happened to kill it — reconstructing the path one rejection at a time.
- **Raw sockets (`SOCK_RAW`) require elevated privileges** and hand you IP-layer-level control real `SOCK_STREAM`/`SOCK_DGRAM` sockets never expose — including the responsibility to compute your own checksums and correctly distinguish your own looped-back traffic from a genuine reply, as the real live test above demonstrated firsthand.
- Promiscuous mode + `libpcap`-based tools (`tcpdump`, Wireshark) is how "watch what's actually on the wire" tooling works — on modern switched networks, this alone mostly only reveals traffic already involving your own machine.
- Deep packet inspection means parsing payload content, not just addressing — and it's exactly why unencrypted metadata (like TLS's SNI field) can leak information even from otherwise-encrypted connections.
- Monitoring capability and monitoring authorization are two separate things — treat them that way, always.

Next: `13_Pitfalls_and_Tips.md` was already covered — proceeding to `14_IoT_Networking.md`.