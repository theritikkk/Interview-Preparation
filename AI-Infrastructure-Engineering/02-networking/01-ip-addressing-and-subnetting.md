# IP Addressing, CIDR, and Subnetting

> **The one-sentence model:** an IP address is a 32-bit number; a prefix length says how many leading bits identify the network; the rest identify the host. Everything else — masks, subnets, broadcast, ranges — follows from that split.

If you can do the binary split quickly, subnetting becomes arithmetic, not memorisation.

---

## 1. Binary, decimal, powers of two

An IPv4 address is four octets, each 0–255:

```text
192      .168      .10       .130
11000000 .10101000 .00001010 .10000010
```

| Bit | 128 | 64 | 32 | 16 | 8 | 4 | 2 | 1 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Value | 128 | 64 | 32 | 16 | 8 | 4 | 2 | 1 |

`192 = 128 + 64` → `11000000`
`168 = 128 + 32 + 8` → `10101000`
`130 = 128 + 2` → `10000010`

Powers of two for host counts:

```text
2^1 2   2^2 4   2^3 8   2^4 16   2^5 32   2^6 64
2^7 128 2^8 256 2^9 512 2^10 1024 2^11 2048 2^12 4096
```

---

## 2. Network and host parts

```text
192.168.10.130/26
└──────┬──────┘└┬┘
    network    host
    26 bits    6 bits
```

- Prefix `/26` → 26 network bits, 6 host bits.
- Host bits = 6 → `2^6 = 64` addresses in the subnet.
- Usable hosts = `64 - 2 = 62` (network and broadcast are reserved).
- Mask: 26 ones → `255.255.255.192`.

```text
/26 mask
11111111.11111111.11111111.11000000
255      .255     .255     .192
```

---

## 3. The CIDR table you must know

| Prefix | Mask | Addresses | Usable | Typical use |
|---:|---|---:|---:|---|
| /8 | 255.0.0.0 | 16,777,216 | 16,777,214 | Huge private block |
| /12 | 255.240.0.0 | 1,048,576 | 1,048,574 | Private 172.16/12 |
| /16 | 255.255.0.0 | 65,536 | 65,534 | Large VPC/subnet |
| /20 | 255.255.240.0 | 4,096 | 4,094 | Medium subnet |
| /22 | 255.255.252.0 | 1,024 | 1,022 | Four /24s |
| /23 | 255.255.254.0 | 512 | 510 | Two /24s |
| /24 | 255.255.255.0 | 256 | 254 | Classic LAN |
| /25 | 255.255.255.128 | 128 | 126 | Half a /24 |
| /26 | 255.255.255.192 | 64 | 62 | Quarter |
| /27 | 255.255.255.224 | 32 | 30 | Small subnet |
| /28 | 255.255.255.240 | 16 | 14 | Point-to-point / small |
| /29 | 255.255.255.248 | 8 | 6 | Very small |
| /30 | 255.255.255.252 | 4 | 2 | Router link |
| /31 | 255.255.255.254 | 2 | 2 | Point-to-point (RFC 3021) |
| /32 | 255.255.255.255 | 1 | 1 | Host route |

**Shortcut:** block size in the interesting octet = `256 - mask_octet`.

- `/26` → mask 192 → block 64.
- `/27` → mask 224 → block 32.
- `/28` → mask 240 → block 16.
- `/22` → mask 252 → block 4 in the third octet.
- `/23` → mask 254 → block 2 in the third octet.

Subnet boundaries are multiples of the block size.

---

## 4. The four questions for any address

Given `192.168.10.130/26`:

1. **Network address?** Round down to the nearest block boundary: `.128` → `192.168.10.128`.
2. **Broadcast?** Network + block size - 1: `.191` → `192.168.10.191`.
3. **Usable range?** `.129` to `.190`.
4. **Is an address in this subnet?** Compare network address after applying the mask. `192.168.10.200` is outside (`.192` is the next block).

### Method (no calculator)

```text
1. Write the prefix and the block size.
2. Find the octet where the mask is neither 0 nor 255 ("interesting octet").
3. List multiples of the block size in that octet.
4. The network is the largest multiple ≤ the address.
5. Broadcast is the next multiple - 1.
6. Usable range is network + 1 to broadcast - 1.
```

### Worked example 1 — `172.16.5.77/22`

- `/22` mask = `255.255.252.0`; interesting octet = third; block = 4.
- Multiples: 0, 4, 8, 12, ...
- 5 → network third octet 4.
- Network: `172.16.4.0`.
- Broadcast: `172.16.7.255`.
- Usable: `172.16.4.1`–`172.16.7.254`.

### Worked example 2 — `10.20.30.200/27`

- `/27` mask = `255.255.255.224`; block = 32.
- Multiples: 0, 32, 64, 96, 128, 160, 192, 224.
- 200 → 192.
- Network: `10.20.30.192`.
- Broadcast: `10.20.30.223`.
- Usable: `.193`–`.222`.

### Worked example 3 — `192.168.1.96/28`

- Block 16; multiples 0, 16, ..., 96, 112.
- Network `.96`; broadcast `.111`; usable `.97`–`.110`.

### Worked example 4 — `198.51.100.0/23`

- `/23` mask `255.255.254.0`; interesting octet = third; block = 2.
- Network `198.51.100.0`; broadcast `198.51.101.255` (the subnet spans two third-octet values).
- Usable `198.51.100.1`–`198.51.101.254`.

### Worked example 5 — split `192.168.1.0/24` into four equal subnets

- Need 4 subnets → 2 extra bits → `/26`.
- Block 64: `.0`, `.64`, `.128`, `.192`.

| Subnet | Network | Broadcast | Usable |
|---|---|---|---|
| 1 | 192.168.1.0 | 192.168.1.63 | .1–.62 |
| 2 | 192.168.1.64 | 192.168.1.127 | .65–.126 |
| 3 | 192.168.1.128 | 192.168.1.191 | .129–.190 |
| 4 | 192.168.1.192 | 192.168.1.255 | .193–.254 |

---

## 5. VLSM: variable-length subnet masks

VLSM allocates different prefix sizes to different needs. Always allocate the **largest** requirement first, aligned to its block size.

Example: `192.168.1.0/24`, requirements 100, 50, 25, 10, 2 hosts.

| Need | Prefix | Usable | Allocated | Range |
|---|---:|---:|---|---|
| 100 | /25 | 126 | 192.168.1.0/25 | .0–.127 |
| 50 | /26 | 62 | 192.168.1.128/26 | .128–.191 |
| 25 | /27 | 30 | 192.168.1.192/27 | .192–.223 |
| 10 | /28 | 14 | 192.168.1.224/28 | .224–.239 |
| 2 | /30 | 2 | 192.168.1.240/30 | .240–.243 |

Remaining: `.244`–`.255` (12 addresses, not enough for another /28; usable for a /28? `.240/28` would cover .240-.255 but overlaps. So leave it).

**Rule:** a subnet's network address must be a multiple of its block size. `192.168.1.240/28` is valid; `192.168.1.244/28` is not (not aligned to 16).

---

## 6. Overlap detection

Two subnets overlap if one network address falls inside the other's range.

```text
10.0.0.0/24    covers 10.0.0.0   – 10.0.0.255
10.0.0.128/25  covers 10.0.0.128 – 10.0.0.255
→ overlap; the /25 is inside the /24
```

```text
10.0.0.0/25    covers 10.0.0.0   – 10.0.0.127
10.0.0.128/25  covers 10.0.0.128 – 10.0.0.255
→ no overlap; adjacent
```

Quick check: compare network and broadcast as integers. If `A.net ≤ B.bcast` and `B.net ≤ A.bcast`, they overlap.

---

## 7. Supernetting and route summarisation

Summarise contiguous blocks by removing the bits that differ.

```text
192.168.4.0/24   11000000.10101000.00000100.00000000
192.168.5.0/24   11000000.10101000.00000101.00000000
                 common prefix: 23 bits
→ 192.168.4.0/23
```

```text
10.1.0.0/24
10.1.1.0/24
10.1.2.0/24
10.1.3.0/24
→ 10.1.0.0/22   (third octet 0–3 = 000000xx)
```

Summarise only addresses that are contiguous and aligned. `10.1.0.0/24` and `10.1.2.0/24` cannot be summarised into a /23 without including `10.1.1.0/24`.

---

## 8. Special ranges

| Range | Purpose |
|---|---|
| `10.0.0.0/8` | Private (RFC 1918) |
| `172.16.0.0/12` | Private (172.16–172.31) |
| `192.168.0.0/16` | Private |
| `127.0.0.0/8` | Loopback |
| `169.254.0.0/16` | Link-local / APIPA |
| `0.0.0.0/8` | "This network" / unspecified |
| `100.64.0.0/10` | Carrier-grade NAT (CGNAT) |
| `224.0.0.0/4` | Multicast |
| `240.0.0.0/4` | Reserved |
| `255.255.255.255/32` | Limited broadcast |

Private addresses are not routable on the public internet. They are translated by NAT at the edge.

---

## 9. IPv6 in ten minutes

- 128 bits, eight groups of four hex digits: `2001:0db8:0000:0000:0000:ff00:0042:8329`.
- Compress one run of zeros with `::`: `2001:db8::ff00:42:8329`.
- Prefix length is common: `/64` for a LAN, `/128` for one host, `/48`/`/56` for site allocation.
- No broadcast; use multicast (`ff00::/8`) and NDP (Neighbor Discovery, the ARP equivalent).
- Link-local addresses: `fe80::/10`, automatically configured.
- Global unicast: `2000::/3`.
- Loopback: `::1`.
- SLAAC lets hosts form addresses from router advertisements; DHCPv6 is optional.
- A `/64` has `2^64` addresses; scanning is not practical.

For infrastructure interviews, know: `/64` LAN, link-local, NDP, no NAT by design (NAT66 exists but is not the norm), and dual-stack.

---

## 10. Practice set (20)

Do these on paper. Answers below.

1. `192.168.1.50/24` — network, broadcast, usable range.
2. `10.0.5.130/25` — network, broadcast, usable range.
3. `172.16.20.200/26` — network, broadcast, usable range.
4. `192.168.10.77/27` — network, broadcast, usable range.
5. `10.10.10.10/30` — network, broadcast, usable range.
6. `203.0.113.45/28` — network, broadcast, usable range.
7. `198.51.100.0/22` — network, broadcast, usable range.
8. `192.168.0.0/23` — network, broadcast, usable range.
9. How many /27s fit in a /24?
10. How many usable hosts in a /29?
11. What is the mask for /26?
12. What is the mask for /19?
13. How many /16s fit in a /8?
14. Which subnet contains `172.16.45.99/20`?
15. Summarise `192.168.4.0/24` and `192.168.5.0/24`.
16. Summarise `10.1.0.0/24` through `10.1.3.0/24`.
17. Is `10.0.0.200/24` inside `10.0.0.0/25`?
18. Usable range of `192.168.1.96/28`.
19. Broadcast of `10.1.2.3/23`.
20. Are `192.168.1.10/25` and `192.168.1.130/25` in the same subnet?

### Answers

1. `.0`, `.255`, `.1–.254`.
2. `.128`, `.255`, `.129–.254`.
3. `.192`, `.255`, `.193–.254`.
4. `.64`, `.95`, `.65–.94`.
5. `.8`, `.11`, `.9–.10`.
6. `.32`, `.47`, `.33–.46`.
7. `198.51.100.0`, `198.51.103.255`, `198.51.100.1–198.51.103.254`.
8. `192.168.0.0`, `192.168.1.255`, `192.168.0.1–192.168.1.254`.
9. 4.
10. 6.
11. `255.255.255.192`.
12. `255.255.224.0`.
13. 256.
14. `172.16.32.0/20` (third octet 32–47).
15. `192.168.4.0/23`.
16. `10.1.0.0/22`.
17. No. The /25 covers `.0–.127`; `.200` is outside.
18. `.97–.110`.
19. `10.1.3.255`.
20. No. `/25` splits at `.128`; `.10` is in `.0/25`, `.130` is in `.128/25`. A router is required.

---

## 11. Common mistakes

| Mistake | Why it is wrong | Fix |
|---|---|---|
| Using network/broadcast as hosts | Reserved | Usable = addresses - 2 (except /31, /32) |
| `/23` mask as `255.255.255.0` | Wrong prefix | `/23` = `255.255.254.0` |
| Allocating VLSM out of order | Fragmentation/overlap | Largest first, aligned |
| Forgetting alignment | `x.244/28` is invalid | Network must be a multiple of block size |
| Assuming private = secure | Private is not routable, not encrypted | Firewall + encryption |
| Using `192.168.1.0/24` and `192.168.1.128/25` together | Overlap | Plan the whole space first |
| Using /24 for point-to-point | Wastes 254 addresses | /30 or /31 |
| Ignoring IPv6 | Dual-stack is common | Know /64, link-local, NDP |
| Confusing subnet mask with wildcard | Wildcard is inverse | ACLs/OSPF use wildcards |

---

## 12. Checkpoint

1. What is the network address of `10.20.30.40/22`?
2. How many usable hosts in a /28?
3. What is the broadcast of `192.168.1.130/26`?
4. Split `172.16.0.0/24` into two /25s.
5. Why can `10.0.0.0/24` and `10.0.0.128/25` not coexist as separate subnets?
6. What is the mask for /21?
7. Summarise `172.16.8.0/24`, `172.16.9.0/24`, `172.16.10.0/24`, `172.16.11.0/24`.
8. Is `192.168.1.200/24` in `192.168.1.0/26`?
9. What is the usable range of `10.0.0.4/30`?
10. Why is a /31 usable for point-to-point links?

If you miss more than two, redo the practice set before Lab L2.1.
