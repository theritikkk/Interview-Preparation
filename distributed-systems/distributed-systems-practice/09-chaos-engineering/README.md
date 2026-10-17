# Project 9: Chaos Engineering Basics

Three real chaos-engineering primitives — **network fault injection**,
**process disruption**, and **resource exhaustion** — built from scratch
and verified end to end against a live target, with measured before/after
evidence for each. `chaosd` and the Kubernetes-native Chaos Mesh operator
(the two tools this roadmap entry originally named) don't run in this
sandbox; each reason is independently diagnosed and documented rather than
glossed over. See [`ANALYSIS.md`](ANALYSIS.md) for the full story — this
file is the quick "what's here / how to run it" guide.

## What's here

```
09-chaos-mesh-deliverable/
├── README.md                    this file
├── ANALYSIS.md                  full design/trade-off writeup — read this for the "why"
├── run-demo.sh                  runs all three chaos experiments end to end
├── tcpchaos/                    real TCP proxy: delay + jitter + packet loss
│   ├── main.go
│   └── go.mod
├── netchaos/                    real netlink qdisc tool (attempted; kernel lacks schedulers — see below)
│   ├── main.go
│   └── go.mod
├── procchaos/
│   └── procchaos.sh             real SIGSTOP/SIGCONT/SIGKILL process chaos
├── stresschaos/
│   └── run-stress.sh            real stress-ng CPU/memory chaos
├── target-server/
│   └── index.html               trivial target for `python3 -m http.server`
├── k8s-chaos-mesh/              real upstream Chaos Mesh v2.7.2 Helm chart + example CRs (reference material; not run here — see ANALYSIS.md)
│   ├── chart/
│   ├── examples/
│   └── install-chaos-mesh.sh    the real install runbook, annotated with why it can't complete here
└── evidence/                    11 files of captured command output backing every claim in ANALYSIS.md
```

## Run it

Requires Go 1.24+ (for `tcpchaos`/`netchaos`) and Python 3 (for the
target server). No root, no Kubernetes, no special kernel features needed
for the demo path.

```bash
./run-demo.sh
```

This builds `tcpchaos`, downloads the real `stress-ng` binary (same one
Chaos Mesh's `StressChaos` CRD uses) if not already present, starts a
target HTTP server, and runs all three experiments in sequence, printing
before/after numbers live. It reproduces exactly what's captured in
`evidence/07` through `evidence/10`.

To run one experiment at a time:

**Network chaos** — proxy a target through delay/jitter/loss:
```bash
cd tcpchaos && go build -o tcpchaos .
./tcpchaos -listen 127.0.0.1:18765 -target 127.0.0.1:8765 \
  -delay-ms 150 -jitter-ms 30 -loss-pct 10
# then curl http://127.0.0.1:18765/ and compare against curling :8765 directly
```

**Process chaos** — pause/resume or kill a running PID:
```bash
./procchaos/procchaos.sh pause <pid> 5    # SIGSTOP, wait 5s, SIGCONT
./procchaos/procchaos.sh kill <pid>       # SIGKILL
```

**Resource chaos** — real CPU/memory load via `stress-ng`:
```bash
./stresschaos/run-stress.sh
```

## The two tools named in the roadmap that don't run here

- **`chaosd`** (standalone CLI): ships binaries via an S3 bucket, not
  GitHub Releases, which is blocked here. A from-source build got
  substantially far (58 working Go module replace directives) before
  being stopped because its dependency graph unconditionally pulls in
  `sigs.k8s.io/controller-runtime` — the full Kubernetes client stack —
  even for host-only use, and kept surfacing newly-blocked vanity-import
  domains with no sign of converging (12 distinct domains, vs. 8 for a
  comparable `grpcurl` build in Project 8).
- **Chaos Mesh operator** (Kubernetes-native): three independent,
  confirmed reasons — no `helm` binary available (also ships via cloud
  storage, not GitHub Releases), its chart's images come from `ghcr.io`
  (blocked, per Project 7), and Project 7 already proved this sandbox's
  cgroup hierarchy can't run *any* pod sandbox regardless of image.

Full detail, evidence, and reasoning for both: `ANALYSIS.md`.

## A kernel-level finding along the way

`netchaos/` is a real Go tool built on `github.com/vishvananda/netlink`
that attempts genuine `tc`-style netem qdisc injection (delay/jitter/loss
at the kernel level, the same mechanism Chaos Mesh's own `NetworkChaos`
CRD uses on a normal host). It fails here with `ENOENT`, and that's a real
kernel-build fact, not a permissions issue: `CAP_NET_ADMIN` is confirmed
present, but `/proc/kallsyms` shows this kernel's qdisc framework was
built with zero pluggable scheduler implementations (`sch_netem`,
`sch_prio`, `sch_fq_codel`, etc. — all absent). `tcpchaos/`'s userspace
proxy is the real, working substitute — the same technique production
tools like toxiproxy use. Details: `ANALYSIS.md`, `evidence/06`.
