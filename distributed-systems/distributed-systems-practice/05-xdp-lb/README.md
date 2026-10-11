# XDP toy load balancer in front of Papy

A real, running L4 (TCP, port-based) load balancer, built entirely from
hand-written eBPF: an **XDP** program rewrites each client packet's
destination port to one of several backend ports, a **TC (traffic
control)** program rewrites it back on the reply, and in between, three
real backend HTTP servers answer — with [Papy](https://github.com/noahpop77/Papy),
this series' recurring load-testing client, as the real traffic source.

```
         client (Papy)                    backends
        127.0.0.1:*  ──┐                ┌─ 127.0.0.1:9001
                       ▼                ▼
    ┌───────────────────────────────────────────────────────┐
    │ lo                                                    │
    │  XDP ingress: xdp_lb        dst:8080 → dst:9001/2/3   │  (hash of client)
    │  TC  egress:  tc_lb_return  src:9001/2/3 → src:8080   │  (reply rewrite)
    └───────────────────────────────────────────────────────┘
                        ▲                ▲
                        │                └─ 127.0.0.1:9002, :9003
          127.0.0.1:8080 is the virtual service —
          nothing ever actually listens there
```

Everything here actually runs: real packets, real in-place header
rewriting, real incremental checksum fixups, real kernel attach via
libbpf's XDP and TC APIs — no `ip`/`tc` CLI tools involved (none are even
installed in this sandbox; see `ANALYSIS.md`). See `ANALYSIS.md` for why
this sits on `lo` rather than the container's real `eth0` interface, and
for the two things that had to be empirically discovered — not assumed —
before this design would even have been possible to get right.

## Build

Reuses Project 4's already-built libbpf + libelf shim (`04-core-libbpf/toolchain/`)
rather than a second copy — keep this zip extracted next to that one, or
see the makefile comments for pointing at your own libbpf checkout.

```
cd xdp-lb
make        # -> xdp_lb.bpf.o, tc_lb_return.bpf.o, lb_ctl, backend
```

## Run

```
# as root, 3 terminals (or 3 backgrounded processes)
./backend 9001 &
./backend 9002 &
./backend 9003 &

# 4th: the load balancer itself — attaches both BPF programs, configures
# them, and prints live per-backend hit counts until Ctrl-C
./lb_ctl lo 8080 9001 9002 9003
```

Then, from anywhere else on the same host:

```
curl http://127.0.0.1:8080/
# Hello from backend on port 9002 (request #1, client port 51231)

# or drive it with Papy, the way this project is actually meant to be used:
papy --threads 4 --target "http://127.0.0.1:8080" --endpoint "/printJson" --count 40 --rate 50
```

Each response names the real backend that served it — load-balancing is
happening, not just packet-passing — and `lb_ctl`'s live counters
(`[:9001]=772 [:9002]=853 [:9003]=818 total=2443`, from an actual run of
this exact build) show it landing roughly evenly across all three.

## What's here

```
05-xdp-lb/
├── xdp-lb/                    the real load balancer
│   ├── xdp_lb.bpf.c           XDP ingress: client -> backend port rewrite
│   ├── tc_lb_return.bpf.c     TC egress: backend -> client port rewrite (the "return path")
│   ├── lb_ctl.c                control plane: loads/attaches both, configures backends, live stats
│   ├── backend.c               minimal real HTTP server (one of several, each a separate process)
│   └── makefile
└── toolchain-probes/          small diagnostic tools that found this design, before it was built
    ├── xdp-probe/              xdp_probe: native-vs-generic XDP mode fallback, with real packet counts
    │                           xdp_probe_port: same, filtered to one TCP port (isolates test traffic
    │                             from background container noise — this is what caught eth0 self-
    │                             traffic never actually reaching eth0's XDP hook; see ANALYSIS.md)
    │                           xdp_framing_probe: does `lo` carry an Ethernet header or not? (it does)
    └── tc-probe/                tc_probe: confirms TC-BPF egress attach works, via libbpf's native
                                  TC API (bpf_tc_hook_create/bpf_tc_attach) — no `tc` CLI needed
```

## The two things that had to be checked, not assumed

1. **Same-host traffic to this container's own `eth0` address never
   reaches `eth0`'s XDP hook at all** — confirmed empirically (a
   port-filtered XDP counter stayed at exactly 0 through 5 real,
   successful HTTP requests to that address), because Linux's local
   routing table delivers locally-owned-address traffic straight through
   loopback internally, without ever touching the real interface's RX
   path. This is why the load balancer sits on `lo`, not `eth0`.
2. **`lo` packets, in this kernel's generic-XDP view, do carry a
   (synthetic) 14-byte Ethernet header before the IP header** — also
   confirmed empirically (a probe that tries both interpretations in
   parallel came back 37/37 for "Ethernet-then-IP", 0/37 for "raw IP"),
   which is why `xdp_lb.bpf.c` parses a normal `ethhdr` even on loopback.

Neither of these is written up from memory or a tutorial; both came from
small, disposable probe programs built and run against this specific
kernel first. See `ANALYSIS.md` for the full detail, including the
checksum math and the backend-selection hashing.
