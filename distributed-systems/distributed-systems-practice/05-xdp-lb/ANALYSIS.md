# ANALYSIS — what's real, what's simplified, and the two facts that had to be discovered first

## The honest boundary

Everything here is real, running kernel-level packet manipulation, not a
simulation of a load balancer or a userspace proxy wearing XDP's name:

- **Real XDP**: `xdp_lb.bpf.c` is a genuine `SEC("xdp")` program, compiled
  by `clang -target bpf`, loaded via `bpf_object__load()`/the real
  `BPF_PROG_LOAD` syscall, attached via libbpf's real netlink-based
  `bpf_xdp_attach()` — the same mechanism `ip link set dev lo xdp obj ...`
  would use, just driven from C instead of a CLI tool that isn't
  installed here.
- **Real TC-BPF**: `tc_lb_return.bpf.c` is a genuine `SEC("tc")` program
  attached to a real `clsact` qdisc at egress, via libbpf's native
  `bpf_tc_hook_create()`/`bpf_tc_attach()` API — again the real kernel
  mechanism, without needing the `tc` binary (not installed here either;
  see below).
- **Real packet rewriting**: both programs read and write actual packet
  bytes in place (`ctx->data`/`ctx->data_end` for XDP, `skb->data`/
  `skb->data_end` for TC) — no packet is copied into userspace, inspected,
  and reconstructed; the kernel's own in-flight packet buffer is modified
  directly, the way production XDP/TC data-plane code does it.
- **Real checksums**: the TCP checksum is updated with the same
  incremental, RFC 1624-style arithmetic the kernel's own NAT code uses
  for a single-field change — not skipped, not recomputed wastefully, and
  not "left for the kernel to fix," because nothing downstream of these
  hooks recomputes it.
- **Real traffic, real backends, real distribution**: the demo isn't
  synthetic counters — three independent `backend` processes, each a real
  listening TCP socket, actually answer real HTTP requests from real
  `curl` and real `papy` processes, and the response bodies (which name
  the backend that produced them) are how the load-balancing claim is
  checked, not asserted.

Nothing here required a stand-in. The honest limitations are about
*scope* (see "What this doesn't do" below), not about anything being
faked.

## Why `lo`, not `eth0` — checked, not assumed

The first instinct for "a load balancer in front of Papy" is to put the
virtual service on the container's real address (`eth0`, `192.0.2.2`) and
have Papy connect to that. That doesn't work here, and the reason is
worth stating precisely because it's a general Linux networking fact, not
a sandbox quirk: **a packet from a host to its own address is delivered
through the local routing table, internally, without ever reaching the
real interface's receive path** — so an XDP program attached to `eth0`
never sees same-host-to-self traffic at all, no matter how real `eth0`'s
own address is.

This was confirmed, not assumed, with a purpose-built isolation test:

```
# xdp_probe_port, filtered to only count TCP packets with dst port 8080
$ xdp_probe_port eth0 6 &
$ for i in 1 2 3 4 5; do curl -s http://192.0.2.2:8080/; done   # all 5 succeed, HTTP 200
...
packets seen: 0   packets seen: 0   packets seen: 0   packets seen: 0   packets seen: 0   packets seen: 0
```

Five real, successful HTTP requests to the box's own `eth0` address, and
the XDP hook on `eth0` counted exactly zero of them — while an earlier,
*unfiltered* counter had shown nonzero, climbing numbers on `eth0` the
whole time, which turned out to be unrelated background container
traffic (the sandbox's own control/monitoring connections) coincidentally
overlapping the test window. That first, unfiltered result would have
been easy to misread as "it's working" — filtering to the exact test port
was what caught the mistake. This is the direct reason `xdp_probe_port`
exists as a separate tool from `xdp_probe`, and the reason to distrust
"the counter went up" without also checking *what* made it go up.

So the load balancer sits on `lo`, and Papy (along with every backend)
connects via `127.0.0.1`. That's the honest scope: this demonstrates a
transparent L4 load balancer's actual packet-rewriting mechanism, on the
real wire that same-host traffic actually travels — it does not, and in
this single-container sandbox cannot, demonstrate balancing traffic
arriving from a genuinely separate client host. A multi-host version
would attach the exact same `xdp_lb.bpf.c` to a real external-facing NIC
in native (driver) mode and would not need the TC return-path program at
all in the common "backends reply directly to the client" (DSR-style)
deployment — seeing this gap is itself a useful, honest side effect of
building the single-host version first.

## Why `lo` carries an Ethernet header — also checked, not assumed

`lo` is `ARPHRD_LOOPBACK` (`/sys/class/net/lo/type` = 772), not
`ARPHRD_ETHER` (`eth0` is 1) — so it would be reasonable to guess that
loopback packets have no L2 header at all, and that `xdp_lb.bpf.c` should
parse an IP header straight from `ctx->data`. That guess is wrong on this
kernel, confirmed with a small program (`xdp_framing_probe.bpf.c`) that
tries *both* interpretations on every packet and counts which one looks
like a sane IPv4 header:

```
$ xdp_framing_probe lo 5 &
$ curl http://127.0.0.1:8080/   # x3, real HTTP requests
...
raw-IP-at-offset-0: 0        eth-then-IP: 37
```

37/37 real packets parsed correctly as "14-byte Ethernet header, then
IP" and 0/37 as "raw IP at offset 0." Generic (SKB-mode) XDP on `lo`
genuinely presents a synthetic Ethernet header — this is a real, if
slightly obscure, piece of Linux network-stack behavior (loopback gets a
null/zero Ethernet header prepended for compatibility with code that
assumes L2 framing), and it's why `xdp_lb.bpf.c` and `tc_lb_return.bpf.c`
both parse a normal `struct ethhdr` even though they only ever run on
`lo`. Skipping this check and parsing raw IP (the "obvious" choice given
`ARPHRD_LOOPBACK`) would have made every single packet fail its bounds
check silently and fall through to `XDP_PASS` un-rewritten — the load
balancer would have compiled, loaded, attached, and done precisely
nothing, with no error anywhere to say why.

## Why native XDP/TC fails here, and why that's fine

Both attach attempts try native (driver) mode first and fall back to
generic (SKB) mode — real tooling (`xdp-loader`, `ip link set xdp`) does
exactly this. Native mode genuinely fails here, for two different,
correctly-diagnosed, driver-level reasons:

```
lo:   "Underlying driver does not support XDP in native mode"
      — loopback has no real driver to support it; expected on any kernel.
eth0: "virtio_net: Can't set XDP while host is implementing GRO_HW/CSUM,
       disable GRO_HW/CSUM first"
      — a genuine virtio_net constraint: native XDP needs the guest to
      fully own the receive queues, which conflicts with the host-side
      hardware offloads this virtio_net device has enabled. Not attempted
      further since the LB runs on `lo` anyway (see above), where native
      mode was never going to be available regardless.
```

Generic (SKB) mode works on both, and is what the load balancer actually
uses. Generic mode runs later in the receive path (after the driver, in
software, per-packet) than native mode would, so it carries more
per-packet overhead — irrelevant for a toy/demo workload, the first thing
to revisit for anything latency-sensitive on real hardware with native
XDP support.

## Backend selection and the checksum math

**Hashing**: `flow_hash(src_ip, src_port)` is a plain multiplicative hash
(Knuth's constant, `2654435761u`) over the client's (IP, port) — picked
for being simple and deterministic-per-flow, not for statistical quality.
Every packet of the same TCP connection hashes identically (neither input
changes mid-connection), so a connection always lands on, and stays on,
one backend; there is no connection-tracking map, because the hash itself
already provides flow-stickiness for free, the same property a
production hash-based L4 LB (e.g. Maglev-style) relies on.

**Forward-path checksum fix (XDP)**: only the TCP destination port
changes, a single 16-bit field inside the region the TCP checksum covers.
Rather than recomputing the checksum from scratch — expensive, and
genuinely awkward under the BPF verifier for a payload of unknown,
MTU-bounded length — `update_csum()` applies the standard RFC 1624
incremental update: `HC' = ~(~HC + ~old + new)`, folding carries twice.
This is the same technique the kernel's own connection-tracking/NAT code
uses for exactly this situation. The IP header checksum is deliberately
left untouched: it covers only the IP header's own fields (addresses,
TTL, length, etc.), none of which change — the port lives in the TCP
header, outside the IP checksum's coverage, so touching it would be not
just unnecessary but wrong.

**Return-path checksum fix (TC)**: symmetric — the source port changes
back from the backend's real port to the VIP port, same incremental
update, same reasoning.

**No `bpf_l4_csum_replace` helper**: that helper exists specifically for
this kind of incremental update, but it takes a `struct __sk_buff *` and
is only usable from TC/cls_bpf-type programs — XDP programs operate on
raw packet memory (`xdp_md`) before an `__sk_buff` even exists, so
`xdp_lb.bpf.c`'s forward-path fix has to be the manual version regardless;
`tc_lb_return.bpf.c` *could* have used the helper instead of hand-rolling
the same math, and uses the hand-rolled version anyway so both programs
share one obviously-identical implementation rather than one calling a
helper and the other reimplementing it — fewer places for the two to
quietly drift apart.

**Why the return path needs no per-connection state**: `tc_lb_return`
rewrites the source port of *any* packet leaving on a known backend port,
regardless of which client it's addressed to. That's safe specifically
because, in this toy setup, nothing other than the load balancer's own
chosen backends ever sends traffic from those ports — a real production
LB can't assume that and generally does need a connection-tracking table
(exactly what `conntrack`/netfilter maintains for iptables DNAT) to know
which reply belongs to which original flow, particularly once backends
might also act as clients of other services on overlapping ports. Calling
this out explicitly, because it's the single biggest simplification in
the return-path design.

## No `ip`/`tc`/`ethtool` CLI tools — and why that didn't block anything

This sandbox has none of iproute2 (`ip`, `tc`) or `ethtool` installed, and
`apt` is blocked the same way it has been in every project in this
series. That turned out not to matter: every piece of control-plane work
that would normally go through those CLIs — reading interface state,
attaching/detaching XDP, creating a `clsact` qdisc and attaching a TC
program at egress — has a real, direct libbpf API
(`if_nametoindex()`, `bpf_xdp_attach()`/`bpf_xdp_detach()`,
`bpf_tc_hook_create()`/`bpf_tc_attach()`/`bpf_tc_detach()`), which is what
`lb_ctl.c` and the probes use throughout. Interface enumeration (which
interfaces exist, their MAC/MTU/driver) came from `/sys/class/net/*`
directly, which needs no tooling at all. Nothing here is working around a
missing CLI tool with something weaker — the CLI tools are themselves
thin wrappers over this same netlink API.

## What was actually verified

- `raw_load_test`-style zero-assumption probing before design: confirmed
  real BPF_PROG_LOAD capability was already established in Project 4;
  this project's own first probes (`xdp_probe`, `tc_probe`) extended that
  to confirm XDP and TC-BPF specifically, with real, live-incrementing
  packet counters against real generated traffic (not just "attach
  succeeded, trust it").
- Full end-to-end runs, multiple independent rounds, this exact
  `make`-built binary:
  - 12 sequential `curl` requests — 3/3 backends hit, each response body
    naming the correct serving backend and the *client's own* real
    ephemeral source port (proof the rewrite is genuinely transparent
    DNAT, not a proxy terminating the connection under its own identity).
  - Two separate multi-threaded Papy runs (`--threads 4`, 40 requests
    each) plus a verbose run — Papy's own live counters: `Failed: 0`
    throughout every run.
  - Cumulative across all rounds: backend hit counters
    (`[:9001]=772 [:9002]=853 [:9003]=818, total=2443` after the heaviest
    run) consistently within ~10% of each other — the hash spreads load
    evenly in practice, not just in theory.
  - Backend processes' own exit logs (`served 56/50/54 requests,
    exiting`) confirm clean shutdown with no crashes, no stuck
    connections, across every test round.
- Re-verified from a completely clean `make clean && make` rebuild
  (confirming the makefile itself is correct and complete, not relying
  on stale manually-built objects) — same result.

## What this doesn't do

- **Single host only** — see "Why `lo`, not `eth0`" above. A real
  multi-host deployment attaches the forward-path program in native mode
  on a real external NIC; the return-path program's necessity depends on
  the deployment style (DSR deployments, where backends reply directly to
  clients bypassing the LB entirely, wouldn't need it at all).
- **No connection-tracking table** — see "Why the return path needs no
  per-connection state" above for exactly what this simplifies away and
  why it's safe only in this toy setup.
- **No health checking** — a backend that stops listening still receives
  its share of hashed traffic; those connections simply fail. A real LB
  tracks backend health and reroutes around failures.
- **No weighted/least-connections balancing** — purely hash-based, so
  every backend gets an equal share by design; no notion of backend
  capacity or current load.
- **TCP only, no IP options** — `ip->ihl != 5` (i.e., any packet with IP
  options) is passed through unmodified rather than handled. Real traffic
  in this demo never sends options, so this never triggers, but it's a
  real, deliberate limitation, not an oversight: handling options
  correctly means parsing a variable-length IP header before reaching
  TCP, which the verifier makes considerably more fiddly for a program
  this small to bother with.
