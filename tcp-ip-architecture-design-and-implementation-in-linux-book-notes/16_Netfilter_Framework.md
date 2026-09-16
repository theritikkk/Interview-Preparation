# 16. The Netfilter Framework
### *(Covers the book's Chapter 16 — "Netfilter Framework")*

Firewalling, NAT, and packet mangling all need to inspect (and sometimes alter or drop) packets at specific, well-defined points in the IP stack's journey — without permanently wiring that logic into the core routing/forwarding code itself. **Netfilter** is Linux's answer: a small, fixed set of **hook points** in the IP code path, at each of which any number of registered callback functions can inspect a packet and return a verdict — and everything else in this chapter (`iptables`, connection tracking, NAT) is built entirely on top of that one mechanism.

## 16.1 The five hook points

```c
enum nf_ip_hook {
    NF_IP_PRE_ROUTING,   // packet just arrived, BEFORE a routing decision
    NF_IP_LOCAL_IN,      // routing decided this packet is FOR this machine
    NF_IP_FORWARD,       // routing decided this packet is being forwarded elsewhere
    NF_IP_LOCAL_OUT,     // packet originated locally, about to be routed
    NF_IP_POST_ROUTING,  // packet is fully ready, about to actually leave
};
```

Mapped onto the transmit/receive diagrams from `06_Packet_Movement_Across_Layers.md`:

```
 RECEIVE:  wire → [PRE_ROUTING] → routing decision → for us? ────yes────► [LOCAL_IN] → local socket
                                          │
                                          │
                                          no (forwarding)
                                          │
                                          │
                                          ▼
                                    [FORWARD] ──────────────────────────────► [POST_ROUTING] → wire
                                                                                      ▲
 SEND:     local socket → routing decision → [LOCAL_OUT] ─────────────────────────────┘
```

A function registered at any one of these five points receives the packet (as an `sk_buff`, naturally) and returns one of a small set of verdicts: `NF_ACCEPT` (let it continue on its normal path), `NF_DROP` (silently discard it — no response of any kind to whoever sent it), `NF_QUEUE` (hand it to userspace for a decision, rather than deciding in-kernel at all), or a couple of others for more specialized cases. **This is the entire mechanism** — everything `iptables` (or its modern successor, §16.5) does is ultimately "register a chain of match-criteria-plus-verdict rules at one of these five hooks, evaluated in order."

## 16.2 `iptables`: tables and chains, mapped onto hooks

`iptables`'s own vocabulary — **tables** (`filter`, `nat`, `mangle`, and others) and **chains** (`INPUT`, `OUTPUT`, `FORWARD`, `PREROUTING`, `POSTROUTING`) — is a organizational layer built directly on top of §16.1's five raw hook points:

| iptables chain | Netfilter hook |
|---|---|
| `PREROUTING` | `NF_IP_PRE_ROUTING` |
| `INPUT` | `NF_IP_LOCAL_IN` |
| `FORWARD` | `NF_IP_FORWARD` |
| `OUTPUT` | `NF_IP_LOCAL_OUT` |
| `POSTROUTING` | `NF_IP_POST_ROUTING` |

A **table** (`filter` for plain accept/drop firewalling, `nat` for address translation, `mangle` for altering packet fields like the ToS/DSCP byte from `02_Protocols_TCP_IP_ARP_ICMP.md`) groups related chains by *purpose*; which chains actually exist in which table reflects which hook points make sense for that purpose (NAT's address-rewriting logic, for instance, naturally belongs at `PRE_ROUTING`/`POST_ROUTING`/`OUTPUT`, not `FORWARD` alone). Each chain is an ordered list of rules — a match specification (source/destination address, port, protocol, and much more) paired with a target (`ACCEPT`, `DROP`, `REJECT` — like `DROP`, but sends an explicit rejection response instead of silence — or a NAT-specific target, below) — evaluated top to bottom until one matches.

## 16.3 Connection tracking (`conntrack`) — the mechanism behind *stateful* firewalling

A purely stateless firewall rule set has a real, practical problem: to let a client's outbound web request's *response* back in, you'd either need an explicit, separately-written rule anticipating every possible legitimate response pattern, or you'd need to open your inbound rules dangerously wide. **Connection tracking** solves this properly: the kernel actively tracks the state of every connection (or connection-like flow, even for UDP/ICMP, which have no formal "connection" of their own) passing through, categorizing each packet as:

- **`NEW`**: the first packet seen for a flow that doesn't match any existing tracked connection.
- **`ESTABLISHED`**: part of a flow that's already been seen going in both directions (a genuine, two-way conversation).
- **`RELATED`**: not itself part of an existing tracked flow, but *logically connected* to one — the canonical example being an FTP data connection, which is a genuinely separate TCP connection from FTP's own control connection, but one that conntrack's FTP-aware helper module specifically recognizes as belonging together, letting a single simple rule ("allow `RELATED` traffic") correctly handle a protocol that would otherwise need considerably more elaborate, protocol-specific firewall rules.

A typical, genuinely common real-world rule set — "allow outbound connections freely; allow their `ESTABLISHED`/`RELATED` return traffic; drop everything else inbound by default" — is entirely built on this state classification, and is *why* a home router or corporate firewall can safely let you browse the web outbound without opening any corresponding inbound hole at all: the return traffic is recognized as `ESTABLISHED`, not treated as an unsolicited new inbound connection.

## 16.4 NAT — rewriting addresses, correctly, using the same tracking

**NAT (Network Address Translation)** — already mentioned in `01_Networking_Fundamentals.md`'s discussion of private address ranges — is implemented as a specific *target* type operating at the `PRE_ROUTING`/`POST_ROUTING`/`OUTPUT` hooks, and it depends entirely on conntrack (§16.3) to work correctly: rewriting a packet's source or destination address on the way out is only half the job — the *reply*, arriving with the rewritten address as its own destination, has to be correctly un-rewritten back to the original internal address before being delivered, and conntrack's per-flow state is exactly what remembers "this rewritten address corresponds to that original one" long enough for the reply to be matched up correctly.

- **`SNAT`** (Source NAT): rewrite the *source* address — the standard mechanism letting many internal, private-address machines share one public IP for outbound connections (briefly introduced in `01`).
- **`DNAT`** (Destination NAT): rewrite the *destination* address — used for "port forwarding," directing externally-addressed traffic to an internal machine that doesn't itself have a public address.
- **`MASQUERADE`**: a convenience variant of SNAT specifically for connections whose *own* outbound IP address isn't fixed in advance (a home router whose ISP-assigned address can change) — automatically uses whatever the outbound interface's current address happens to be, rather than requiring it configured explicitly.

## 16.5 What's changed since kernel 2.4.20

- **`nftables`, not `iptables`, is the actual kernel-level packet-filtering framework on essentially every modern Linux distribution today** — this is a genuinely major shift, not a minor refinement. `nftables` (in-kernel since 3.13, 2014, and the default backend on major distributions since around 2019–2020) replaces `iptables`'s comparatively rigid, compiled-in-C match/target module system with a considerably more flexible, unified, bytecode-based rule representation — one single framework handling IPv4, IPv6, ARP, and bridge filtering, instead of `iptables`'s separate `iptables`/`ip6tables`/`arptables`/`ebtables` tools. **The `iptables` *command* still exists and still works** on a modern system — but it typically now operates as `iptables-nft`, a compatibility shim that translates classic `iptables` syntax into `nftables` rules under the hood, rather than talking to the older, now largely-retired-at-the-kernel-level `x_tables` framework this book describes directly.
- **Connection tracking (§16.3) and NAT (§16.4) are both still conceptually identical** under `nftables` — the *concepts* (stateful flow tracking, `NEW`/`ESTABLISHED`/`RELATED` classification, SNAT/DNAT/MASQUERADE) this book teaches remain completely correct and directly applicable; what's changed is the underlying rule-representation and configuration-tool layer sitting on top of the same fundamental hook-based architecture from §16.1, which itself is essentially unchanged.
- **eBPF-based filtering** (XDP, TC-eBPF — both flagged in earlier chapters' "what's changed" sections) offers an increasingly common *alternative* to the netfilter-hook model entirely for the highest-performance use cases — letting a custom, verified program make accept/drop/redirect decisions even earlier in the packet's journey than `NF_IP_PRE_ROUTING`, for workloads where netfilter's own per-hook overhead is itself the bottleneck. Projects like Cilium build entire container-networking security models primarily on eBPF rather than classic netfilter for exactly this reason.

---

## Quick recap (for revision)

- Netfilter's whole mechanism is five fixed hook points (`PRE_ROUTING`, `LOCAL_IN`, `FORWARD`, `LOCAL_OUT`, `POST_ROUTING`) in the IP packet path, each accepting registered callbacks that return a verdict (`ACCEPT`/`DROP`/`REJECT`/others).
- `iptables`'s tables (by purpose: `filter`/`nat`/`mangle`) and chains (mapped directly onto the five hooks) are an organizational layer built on top of that one mechanism — not a separate mechanism of their own.
- Connection tracking's `NEW`/`ESTABLISHED`/`RELATED` classification is what makes "allow outbound, allow the replies, drop everything else inbound" both safe and simple — and is a hard prerequisite for NAT working correctly at all (the reply has to be un-rewritten using exactly this tracked state).
- **`nftables` has replaced `iptables` as the actual kernel-level backend on modern systems** — the biggest single change in this chapter's whole subject area since 2002 — while the `iptables` command itself often still works today, now as a compatibility layer rather than talking to the original framework directly.
- eBPF-based approaches (XDP, TC-eBPF, and projects like Cilium built on them) are an increasingly common alternative to the entire netfilter-hook model for the highest-performance filtering/redirection use cases.

Next: `17_SoftIRQ_Implementation.md` — the interrupt/softIRQ split that `06_Packet_Movement_Across_Layers.md` introduced, covered in full.