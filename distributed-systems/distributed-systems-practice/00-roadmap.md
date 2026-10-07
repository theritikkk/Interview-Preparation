# Distributed Systems Practice — Roadmap

This is the practice curriculum that follows on from the BPF Performance Tools study notes and the Papy deep-dive. Each item below is a self-contained project: real code, a guidelines doc (how it works, how to build/run it), and an analysis doc (design decisions, trade-offs, what's simplified vs. a production system).

**Pacing:** same pattern as the book notes — one project built and verified at a time, delivered as soon as it's working, rather than everything half-built at once. Say "continue" for the next one.

## A note on what this environment can actually do

This all runs in a single, non-privileged cloud container — worth being upfront about what that means for each project, so nothing here is oversold:

| Capability | Available here? | Impact |
|---|---|---|
| C/C++ compilers (g++, clang), cmake, make | **Yes** | Anything pure-userspace-and-networking compiles and runs for real |
| TCP networking between local processes | **Yes** | Multi-process distributed algorithms (Raft, gRPC) run and are genuinely testable |
| Docker CLI present | Partially — daemon/network access not guaranteed | Single-container demos may work; multi-container compose stacks are written as real, deployable files but may not be runnable end-to-end here |
| Loading real eBPF programs into the live kernel (bpf() syscall, XDP attach, sched_ext) | **Unverified / unlikely** — needs CAP_BPF/CAP_SYS_ADMIN and is typically blocked in sandboxed containers even as root | Code is written to compile cleanly against libbpf/clang -target bpf, with a documented command to actually load it **on a real Linux host you control** |
| A real multi-node Kubernetes cluster (for Cilium/Hubble/Tetragon) | **No** | Delivered as real, applyable YAML/Helm + a runbook, not as something demoed live here |

Where a project can't be fully executed here, that's stated plainly in its own README rather than silently claimed as "done and tested."

## Projects

| # | Project | Status | Fully testable in this environment? |
|---|---|---|---|
| 1 | **Raft consensus** (C++, multi-process over TCP) — leader election, log replication, crash-survives-committed-data demo | **Built & verified** | Yes — compiled, run, and the safety property demonstrated live |
| 2 | gRPC + Protocol Buffers service communication | **Built & verified** | Yes — confirmed apt/pip/npm all block the real gRPC library in this sandbox, so it's real `.proto` schemas compiled by the real `protoc`, real protobuf wire serialization, and a hand-rolled transport for the 4 RPC shapes (unary/server-stream/client-stream/bidi) standing in for gRPC's own HTTP/2 layer — see its ANALYSIS.md for the exact boundary |
| 3 | OpenTelemetry distributed tracing on Papy | Not started | Partially — can instrument Papy's C++ source and run a local OTel collector printing spans to console; a full Jaeger/Tempo UI needs infra this sandbox doesn't have |
| 4 | CO-RE / libbpf rewrite of a BCC tool (`biolatency`) | Not started | Code only — compiles against libbpf/BTF headers; loading into the live kernel needs a real host |
| 5 | XDP toy load balancer in front of Papy | Not started | Code only, same caveat as above |
| 6 | sched_ext toy scheduler | Not started | Code only, same caveat as above |
| 7 | Cilium / Hubble / Tetragon on Kubernetes | Not started | Docs + real YAML/Helm manifests + runbook — no live cluster here |
| 8 | Continuous profiling (Parca or Pyroscope) setup | Not started | Docker-compose file provided; runnable only if the Docker daemon in this sandbox can actually pull images |
| 9 | Chaos engineering basics (Chaos Mesh) | Not started | Docs + manifests only |

## Why this order

Raft first because it's pure CS with zero infrastructure dependency — the single highest-leverage exercise in the field, per the original recommendation, and the best fit for your C++ background from Papy. Everything after it trades some real-execution confidence for closer ties to the modern eBPF/cloud-native stack discussed earlier; each project says so honestly rather than pretending otherwise.
