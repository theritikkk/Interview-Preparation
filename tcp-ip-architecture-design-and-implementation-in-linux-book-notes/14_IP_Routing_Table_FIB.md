# 14. The IP Layer and the Routing Table (FIB)
### *(Covers the book's Chapter 14 — "IP Layer: Routing Packets")*

Every transmit-path diagram in this whole book (`06_Packet_Movement_Across_Layers.md`, most explicitly) includes a "routing lookup" step without yet explaining what that lookup actually consults. This chapter is that structure — the **FIB** (Forwarding Information Base), better known simply as "the routing table" — plus, at this book's specific kernel era, a second structure sitting in front of it that no longer exists in a modern kernel at all, making this one of the more dramatically-changed chapters in this whole series.

## 14.1 The FIB itself — longest-prefix-match lookup

A routing table entry says, in effect, "for destinations matching this network prefix, send via this gateway, out this interface." Multiple entries can match a single destination address at different levels of specificity (a broad default route, plus a narrower route for a specific subnet, plus perhaps an even narrower route for one specific host) — and the rule for picking among matches is always **longest prefix match**: the *most specific* matching entry (the one with the longest prefix length) wins, regardless of what order entries happen to be stored in.

At this book's kernel era, the FIB itself is implemented as **`fib_hash`** — a set of hash tables, one per distinct prefix length actually in use, searched from most-specific (longest prefix) to least-specific (shortest prefix, ultimately falling back to the default route) until a match is found. This works, but scales somewhat awkwardly: a routing table with entries at many different prefix lengths means checking many separate hash tables in sequence for every lookup that doesn't hit an early, highly-specific match.

## 14.2 The route cache — a structure this book relies on that a modern kernel simply doesn't have

Here's the chapter's single most important "know this changed" fact, worth stating plainly before going further: **the design this book describes includes a *route cache* sitting in front of the FIB, and that route cache was later removed from Linux entirely** (kernel 3.6, 2012) — not refined, not replaced with a different cache, genuinely removed, with nothing put back in its place. Understanding *why* it existed, and *why* it was later judged not worth keeping, is worth real attention.

**The idea, as this book presents it**: a full FIB lookup — walking potentially several hash tables, checking prefixes from most to least specific — costs real CPU time on every single packet. But real traffic is very often *repetitive*: many packets, over a short window, heading to the *same* destination (an ongoing TCP connection's stream of segments, for instance). So: cache the *result* of a completed FIB lookup, keyed roughly by destination (and source/other flow attributes), in a fast hash table (`rt_hash_table`, holding `struct rtable`/`dst_entry` objects) — and consult *that* first; only fall through to the full FIB lookup on a cache miss, then populate the cache with the result for next time.

```
        packet needs a routing decision
                    │
                    ▼
         route cache lookup (rt_hash_table)
                    │
        ┌───────────┴───────────┐
        │ HIT                   │ MISS
        ▼                       ▼
  use cached decision      full FIB lookup (fib_hash, §14.1)
  immediately                    │
                                  ▼
                        populate the route cache
                        with this result, for next time
```

**Why it was removed, rather than just kept:** two separate, both genuinely serious, problems. First, **it was a real, practical denial-of-service vector**: an attacker (or even just unusual, legitimate traffic patterns — plenty of internet-facing servers genuinely talk to enormous numbers of *distinct* remote addresses) could flood a machine with packets addressed to many different, mostly-unique destinations, each one being a guaranteed cache miss that still has to populate a *new* cache entry — ballooning the cache's memory footprint and the CPU cost of managing it, precisely the "attacker exploiting an optimization meant to help the common case" pattern that recurs throughout kernel networking security history. Second, and just as decisive in practice: once the FIB's own lookup structure was replaced with something genuinely fast on its own merits (`fib_trie`, §14.3), the cache's performance benefit shrank dramatically, while its complexity and attack surface didn't — at which point removing it entirely, and simply doing a full (now genuinely fast) FIB lookup on every packet, was judged the better trade-off. This is a clean, well-documented example of a real architectural decision reversing over time as the *rest* of the system it was built to compensate for got faster — an optimization can become net-negative once its own justification quietly stops holding.

## 14.3 `fib_trie` — the structure that made the cache's removal viable

Rather than `fib_hash`'s "separate hash table per prefix length, checked in sequence" design, **`fib_trie`** (an LC-trie — "Level-Compressed trie," a specific, well-studied data structure for efficient longest-prefix matching) organizes the *entire* routing table as a single trie structure, walked once per lookup, regardless of how many distinct prefix lengths the table actually contains. This genuinely delivers both **better and more consistent** performance than `fib_hash`'s approach, especially as routing table size grows — and it's specifically this improvement that made §14.2's cache-removal decision a net performance win rather than a regression: once the "slow path" (a full FIB lookup) got fast enough on its own, having a cache in front of it stopped paying for its own overhead and risk.

## 14.4 Policy routing and multiple routing tables

Beyond one single, global routing table, Linux supports **policy routing**: multiple independent routing tables, plus a set of **rules** (`fib_rules`) that decide *which* table a given packet's lookup should actually consult, based on criteria beyond just destination address — source address, incoming interface, firewall mark (tying directly into `16_Netfilter_Framework.md`), and more. This is genuinely powerful, real-world functionality: routing traffic from one specific source subnet out a completely different gateway/interface than traffic from another, without needing separate physical machines, is a standard policy-routing use case, configured via `ip rule` (itself, per `13_Netlink_Sockets.md`, ultimately a netlink-based control-plane operation) alongside `ip route`.

## 14.5 What's changed since kernel 2.4.20

- **The route cache is gone, entirely, as of kernel 3.6** — §14.2's whole "cache in front of the FIB" picture describes a structure that existed for roughly a decade of Linux's history and then was deliberately, permanently removed, precisely because the reasons for having it stopped applying once `fib_trie` made the "slow path" fast enough to just always take.
- **`fib_trie` became the default FIB implementation in kernel 2.6.39** (available as a compile-time option somewhat earlier, from 2.6.13) — fully superseding `fib_hash`'s per-prefix-length hash-table design described in §14.1.
- **Multipath routing and ECMP (Equal-Cost Multi-Path)** — spreading traffic to a single destination across several equally-good routes/gateways simultaneously (for both load-balancing and redundancy) — has been considerably more built-out and more commonly deployed in production networking since this book's era, though the basic *concept* isn't entirely absent from the 2.4 series.
- **VRF (Virtual Routing and Forwarding)**, added considerably later, lets a single Linux machine maintain genuinely separate, isolated routing tables per virtual "instance" — a capability with real practical importance in modern multi-tenant and network-function-virtualization deployments, with no real equivalent in this book's single (or policy-rule-selected, but still comparatively simple) routing-table model.

---

## Quick recap (for revision)

- The FIB implements longest-prefix-match: among all entries whose prefix covers a destination, the most specific one always wins, regardless of table ordering.
- **This book's route cache no longer exists in any modern kernel** — removed in kernel 3.6 (2012), specifically because it was a real, exploitable DoS vector *and* had stopped paying for its own complexity once `fib_trie` made direct lookups fast.
- `fib_trie` (default since 2.6.39) replaced `fib_hash`'s per-prefix-length-hash-table approach with a single trie walked once per lookup — the direct enabling factor behind the cache's later removal.
- Policy routing (`fib_rules`, multiple tables, `ip rule`) lets routing decisions depend on more than just destination address — source, incoming interface, and firewall marks can all influence which table a lookup actually consults.
- This chapter is a genuinely good case study in "an optimization can become a net liability once the thing it was compensating for gets fixed on its own merits" — worth remembering well beyond networking specifically.

Next: `15_IP_QoS.md` — what happens to a packet *after* routing has decided where it's going, but before it actually leaves the machine.