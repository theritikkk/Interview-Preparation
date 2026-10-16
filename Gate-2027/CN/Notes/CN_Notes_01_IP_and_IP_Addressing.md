# CN Notes 01 — IP & IP Addressing (High-Yield Deep Dive)

> **Why this file exists:** "Internet Protocol & IP Addressing" is on the high-yield list. It's the most *bankable* CN topic — pure arithmetic, once you know the class table, the mask→size table, and three CIDR rules. Built using the PW GATE handbook's framing (classes, NID/HID patterns, CIDR/supernetting rules) plus derivation-first explanations and a verified 10-question practice set.
>
> **Companion question banks:** `CN_06_IPv4_Addressing_CIDR.md`, `CN_07_IPv4_Fragmentation_NAT.md`. PW source: `../Reference/PW_Subject_Handbooks/PW_CN.txt` (§1–§4).

---

## 0. The mental model: an IP address is a *split* number

Every IPv4 address is 32 bits split into two fields:

```
  32-bit address = [ Network part (NID) | Host part (HID) ]
```

Given the split, three addresses fall out of each "network"/"subnet":

| Address | Rule | Meaning |
|---|---|---|
| Network address | host bits all **0** | names the network itself (never assigned to a host) |
| Directed broadcast | host bits all **1** | reaches every host in that network (destination only) |
| Usable hosts | everything in between | `2^(host bits) − 2` addresses |

**Special address patterns** (the PW handbook's NID/HID table, modernized):

| Pattern | Meaning |
|---|---|
| Host part = all 0s | network address |
| Host part = all 1s | directed broadcast (any network) |
| All 1s (255.255.255.255) | **limited broadcast** (same LAN, destination only, never a source) |
| All 0s (`0.0.0.0`) | "this host" (source before configuration / default route) |
| Net part 1s, host part 0s | the **subnet mask** itself |
| 127.x.x.x | **loopback** — never assignable to a host interface |

Communication types: **unicast** (1↔1), **broadcast** (1→all in the network), **multicast** (1→many, class D addresses 224.0.0.0–239.255.255.255).

---

## 1. Classful addressing (still asked; still the fastest way to read an address)

Leading bits decide the class:

| Class | Leading bits | First octet | Default mask | Networks | Hosts per network |
|---|---|---|---|---|---|
| A | `0` | 1–126 | 255.0.0.0 (/8) | 2⁷ − 2 = 126 | 2²⁴ − 2 ≈ 16.7M |
| B | `10` | 128–191 | 255.255.0.0 (/16) | 2¹⁴ = 16384 | 2¹⁶ − 2 = 65534 |
| C | `110` | 192–223 | 255.255.255.0 (/24) | 2²¹ ≈ 2.1M | 2⁸ − 2 = 254 |
| D | `1110` | 224–239 | — | multicast | — |
| E | `1111` | 240–255 | — | research/future | — |

**Private address ranges** (never routed on the public Internet):

- `10.0.0.0 – 10.255.255.255` (1 class-A block)
- `172.16.0.0 – 172.31.255.255` (16 class-B blocks)
- `192.168.0.0 – 192.168.255.255` (256 class-C blocks)

> Why 126 class-A networks (not 128)? Network 0 ("this network") and 127 (loopback) are reserved.

---

## 2. Masks, CIDR, and the only table you need

A mask marks the network bits with 1s. **CIDR** `/n` = n network bits = a mask of n 1s.

```
  /24 = 24 network bits  8 host bits   → 256 addresses, 254 usable
  /25 = 25 / 7  → 128 addresses, 126 usable
  /26 = 26 / 6  → 64 addresses, 62 usable
  /27 → 32 addresses, 30 usable
  /28 → 16 addresses, 14 usable
  /29 → 8 addresses, 6 usable
  /30 → 4 addresses, 2 usable   (point-to-point links)
```

**Formulas (memorize shapes, not numbers):**

```
  addresses per subnet = 2^(32−n)
  usable hosts          = 2^(32−n) − 2
  #subnets when splitting /p into /n = 2^(n−p)
```

**The "magic octet" method (fastest in-exam subnetting):**

1. Find the mask octet that is *not* 0 or 255 (say 224 for /27).
2. Block size = 256 − 224 = **32**.
3. Subnets start at multiples of the block size in that octet: 0, 32, 64, 96, 128, 160, 192, 224.
4. The subnet containing address X = largest start ≤ X; broadcast = next start − 1.

**Worked example.** `10.20.30.100/27`:

- /27 → mask 255.255.255.224 → block 32 → starts 96? 96 ≤ 100 < 128 → network `10.20.30.96`, broadcast `10.20.30.127`, usable `97–126` (30 hosts).

---

## 3. CIDR rules — the part most students skip (and GATE loves)

A block `a.b.c.d/n` is a **valid CIDR block** iff (all three, from the PW handbook §1.8):

1. All addresses in the block are contiguous (automatic, since it's a prefix).
2. Block size is a power of 2 (automatic: `2^(32−n)`).
3. **First address of the block must be divisible by the block size** — i.e., all host bits are 0.

Quick check: convert the "interesting" octet to a number and test divisibility by the block size in that octet.

**Examples:**

- `10.1.4.0/22` → /22 → third octet block = 4 → 4 mod 4 = 0 ✔ valid.
- `10.1.1.0/22` → 1 mod 4 = 1 ✘ **invalid** (the actual /22 network is 10.1.0.0).
- `192.168.3.128/25` → last octet block 128 → 128 mod 128 = 0 ✔ valid.
- `172.16.32.0/19` → /19 → third octet block 32 → 32 mod 32 = 0 ✔ valid.

### Supernetting (route aggregation)

Combine 2^k **contiguous** networks of **equal size** into one block (reduces routing-table entries — PW lists this as the big win).

Rules: (1) network IDs contiguous; (2) same size, count a power of 2; (3) first network ID divisible by the supernet size.

**Worked example:** `198.51.100.0/24 … 198.51.103.0/24` (4 consecutive /24s):

- Low two bits of the third octet must be free → /22.
- First block 100 → 100 mod 4 = 0 ✔ → aggregate = **198.51.100.0/22**.

---

## 4. VLSM — variable-length subnet masking

Split a block by *requirement*, largest subnet first, so small subnets don't waste big holes.

**Worked example:** `200.1.1.0/24`; needs: 100 hosts, 50 hosts, 20 hosts.

| Need | Smallest block with enough usable hosts | Assignment |
|---|---|---|
| 100 | /25 (126 usable) | 200.1.1.0 – 200.1.1.127 |
| 50 | /26 (62 usable) | 200.1.1.128 – 200.1.1.191 |
| 20 | /27 (30 usable) | 200.1.1.192 – 200.1.1.223 |

Leftover: 200.1.1.224–255 (32 addresses) free for future/point-to-point links.

**Rule of thumb:** usable host requirement h → need `2^m − 2 ≥ h` host bits m; round up to the smallest mask.

---

## 5. IPv4 header — the GATE-relevant tour

```
  4-bit VER | 4-bit HLEN | 8-bit ToS | 16-bit Total Length
  16-bit ID | 3-bit flags (X, DF, MF) | 13-bit Fragment Offset
  8-bit TTL | 8-bit Protocol | 16-bit Header Checksum
  32-bit Source IP | 32-bit Destination IP | Options (0–40 B, padded)
```

Facts GATE has asked (PW §4 verbatim anchors):

- **HLEN**: min header 20 B, max 60 B (with 40 B options).
- **Total Length** = header + data (16 bits → max 65535 B).
- **Identification**: same value for all fragments of one datagram.
- **MF/DF/offset**: MF=1 means more fragments; offset counts **data bytes ahead/8**.
- **TTL**: decremented per router; at 0 → discard + ICMP "time exceeded". Prevents loops.
- **Protocol**: carries TCP (6), UDP (17), ICMP (1), IGMP (2). Router discard order under pressure: **ICMP > IGMP > UDP > TCP** (lower protocol number dropped first here).
- **Header checksum**: header only, recomputed at **every router** (TTL changes). Data is TCP's job.
- **Options**: strict/loose source routing, record route (9 addresses max: 38 B / 4), timestamp, padding.

---

## 6. Traps & exam patterns

1. **±2 everything**: usable = total − 2 (network + broadcast), unless the question is about a block of *addresses* or point-to-point (/31 in modern practice, /30 in GATE).
2. **"Number of subnets" vs "number of addresses"**: `2^(n−p)` is subnets; `2^(32−n)` is addresses per subnet.
3. **Divisibility rule** decides validity of a proposed block — check the interesting octet, not the whole 32-bit value.
4. **Broadcast semantics**: directed broadcast (subnet-specific) vs limited broadcast 255.255.255.255 (never a source, never forwarded).
5. **Class detection from first octet only**: 191 = class B, 192 = class C — boundary octets are favorite options.
6. **Private vs public**: 172.16–172.31 (not all of 172.x!), 192.168.0–255 only.

---

## 7. Practice set (verified by simulation)

### Q1 · MCQ · Fundamental
The address `172.16.5.10` belongs to class \_\_\_ with default mask \_\_\_.

- (A) B, 255.0.0.0
- (B) B, 255.255.0.0
- (C) C, 255.255.255.0
- (D) A, 255.0.0.0

---

### Q2 · NAT · Fundamental
For `10.20.30.100/27`, the last octet of the **broadcast address** is ______.

---

### Q3 · NAT · Fundamental
A subnet is defined by `192.168.0.0/20`. The number of **usable host addresses** is ______.

---

### Q4 · NAT · Standard
A `172.16.0.0/24` block is split into subnets of prefix `/26`. The number of subnets created is ______.

---

### Q5 · MCQ · Standard
Block `200.1.1.0/24` is being subdivided with VLSM (largest first) for requirements of 100, 50 and 20 hosts. Which one names the subnet allocated to the **50-host** requirement?

- (A) 200.1.1.0/26
- (B) 200.1.1.64/26
- (C) 200.1.1.128/26
- (D) 200.1.1.192/26

---

### Q6 · NAT · Standard
Continuing the Q5 allocation, the **first usable host address** of the 50-host subnet is 200.1.1.\_\_\_.

---

### Q7 · MCQ · Standard
Four contiguous class-C blocks `198.51.100.0/24 … 198.51.103.0/24`. The correct aggregate (supernet) is

- (A) 198.51.100.0/23
- (B) 198.51.96.0/21
- (C) 198.51.100.0/22
- (D) 198.51.100.0/21

---

### Q8 · MCQ · Moderate
Which one of the following is **NOT** a valid CIDR block start?

- (A) 10.1.4.0/22
- (B) 10.1.1.0/22
- (C) 192.168.3.128/25
- (D) 172.16.1.32/27

---

### Q9 · MSQ · Moderate
Which of the following statements is/are TRUE?

- (A) A packet whose destination has all host bits 1 is a directed broadcast; it cannot be used as a source address.
- (B) 255.255.255.255 is a limited broadcast address, valid only as a destination.
- (C) The subnet mask has network bits set to 1 and host bits set to 0.
- (D) 127.0.0.1 is a loopback address and may be assigned to any host's network interface as its unicast IP.

---

### Q10 · NAT · Killer
The block `172.16.32.0/19` is fully subdivided into `/26` subnets. The number of /26 subnets is ______.

---

## Answer Key

| Q | Answer | Why (condensed) |
|---|---|---|
| 1 | B | First octet 172 ∈ 128–191 → class B, /16 mask |
| 2 | 127 | /27 block 32 → 96 ≤ 100 < 128; broadcast = 127 |
| 3 | 4094 | 2¹² − 2 |
| 4 | 4 | 2^(26−24) |
| 5 | C | /25→0–127, then /26 at .128 |
| 6 | 129 | .128 is network; first usable .129 |
| 7 | C | 4×/24 → /22; 100 mod 4 = 0 valid; third octet 100 |
| 8 | B | 1 mod 4 ≠ 0 → first address not divisible by block size |
| 9 | A, B, C | (D) loopback is never assigned to interfaces |
| 10 | 128 | 2^(26−19) = 2⁷ |

### Full solutions

**Q1.** 172 → class B (`10` prefix), default mask /16 = 255.255.0.0. **B**. (Trap option A is class A's mask; C is class C's.)

**Q2.** /27 → mask 255.255.255.224 → block size 256−224 = 32. Subnet starts in the last octet: 96 (since 96 ≤ 100 < 128). Network = 10.20.30.96, broadcast = 10.20.30.127 → **127**.

**Q3.** /20 → host bits = 32 − 20 = 12 → 2¹² = 4096 total, minus 2 = **4094**.

**Q4.** New bits = 26 − 24 = 2 → 2² = **4** subnets (each with 62 usable hosts).

**Q5.** Largest first: 100 hosts needs 2^m − 2 ≥ 100 → m = 7 → /25 at 200.1.1.0–127. Next, 50 hosts → m = 6 → /26 at the next free address: **200.1.1.128/26** → C. (Then 20 hosts → /27 at .192.)

**Q6.** The 50-host subnet is 200.1.1.128/26: network = .128, broadcast = .191, usable = .129–.190 → first usable = **129**.

**Q7.** Four /24s = 1024 addresses → /22. Check rule 3: third octet 100 must be divisible by 4 → 100 = 4×25 ✔. Third octet values in the block: 100–103 all share the top 22 bits. **C**. (B would overlap other space; D is too big — needs eight /24s.)

**Q8.** Rule 3: first address divisible by block size. (B): /22 → third octet must be a multiple of 4; 10.1.**1**.0 has 1 mod 4 = 1 → invalid. (A) 4 mod 4 = 0 ✔; (C) 128 mod 128 = 0 ✔; (D) 32 mod 32 = 0 ✔. Answer **B**.

**Q9.** A ✔ — directed broadcast = host bits all 1; as a source it would be ambiguous/wrong. B ✔ — limited broadcast semantics. C ✔ — definition of the mask. D ✘ — 127/8 is reserved for loopback; hosts use 127.0.0.1 only to talk to themselves, it is never configured as an interface's unicast address. **A, B, C**.

**Q10.** Subnetting /19 → /26 adds 7 bits → 2⁷ = **128** subnets. (Sanity check via the divisibility rule: /19 → third octet block 32; 172.16.32.0 starts one such block ✔; /26 subnets inside: 32 × 4 = 128 per /19 with masks 0,32,64,96 per third octet.)

---

## 8. Next steps

1. Redo Q2, Q5–Q7 from scratch with the magic-octet method — target < 60 seconds each.
2. Practice reps: `CN_06_IPv4_Addressing_CIDR.md` (10 Q), then `CN_07_IPv4_Fragmentation_NAT.md` for fragmentation arithmetic.
3. Continue to `CN_Notes_02_TCP_and_Flow_Control.md`.
