# Continuous Profiling (Parca)

A real Parca server, scraping real pprof endpoints from a real Go
workload written for this project, with real profile data queried back
out of Parca's own storage via its actual gRPC API — plus a real,
precisely diagnosed reason why Parca's eBPF agent specifically can't run
in this sandbox.

The roadmap's original plan for this project was pessimistic: "every
container registry tested is egress-blocked, so a compose stack pulling
third-party images is expected to hit the same wall." That assumption
turned out not to matter, because — like k3s and cilium-cli in Project 7
— both `parca` and `parca-agent` are distributed as plain binaries on
GitHub releases, no registry needed at all. What actually stopped the
eBPF agent was narrower and more specific: a single missing Linux
capability, `CAP_SYS_RESOURCE`, traced all the way to the exact line of
library code that needs it. Everything else — server, scraping,
storage, querying, a real workload to profile — works and is verified
end to end.

## What's here

```
08-continuous-profiling/
├── server/
│   ├── setup-parca-server.sh   downloads + runs the real parca binary
│   └── parca.yaml              real config: scrape_configs for self +
│                                workload (schema confirmed against the
│                                real upstream parca.yaml)
├── agent/
│   └── setup-parca-agent.sh    downloads + attempts to run the real
│                                parca-agent — fails here, exactly as
│                                documented in ANALYSIS.md
├── workload/
│   ├── main.go                 a real Go service with two named,
│                                recognizable hot paths (hashBurn:
│                                CPU/SHA-256, allocBurn: allocation),
│                                serving net/http/pprof on :9999
│   ├── go.mod
│   └── setup-workload.sh       builds + runs it
├── verify/
│   ├── build-grpcurl.sh        builds a real gRPC client from source
│   │                            (needed a small, documented workaround
│   │                            — see ANALYSIS.md)
│   └── query-examples.sh       the exact real queries used to pull
│                                real profile data back out of Parca
├── evidence/                   13 captured real outputs: capability
│                                dump + decode, rlimit checks, the real
│                                parca-agent failure log, the eBPF
│                                library source, real scrape-target
│                                health, and the real CPU/memory query
│                                results (with real function names)
└── ANALYSIS.md                 the full story
```

## Run it

```bash
cd workload && ./setup-workload.sh &          # real CPU+alloc workload, :9999
cd ../server && ./setup-parca-server.sh &      # real Parca server, :7070
                                                 # (starts scraping both
                                                 # itself and the workload
                                                 # every 10s)
cd ../verify && ./build-grpcurl.sh             # build a real gRPC client
./query-examples.sh                            # pull real profile data
                                                 # back out of Parca —
                                                 # output matches
                                                 # evidence/11 and 12
```

Or open `http://127.0.0.1:7070/` in a browser for the real Parca web UI
once the server and workload are both running.

```bash
cd ../agent && ./setup-parca-agent.sh          # the real eBPF agent —
                                                 # fails here; see below
```

On infrastructure where the container has `CAP_SYS_RESOURCE` (a real
cloud VM, most CI runners, a laptop) `agent/setup-parca-agent.sh` is the
complete, correct command and parca-agent will start profiling every
process on the machine via eBPF with zero code changes.

## The short version of why the eBPF agent stops here

This sandbox's container has 40 of the 41 standard Linux capabilities —
missing exactly `CAP_SYS_RESOURCE`, confirmed by decoding the actual
`/proc/self/status` capability bitmask bit by bit
(`evidence/02-capabilities-decoded.txt`). parca-agent's eBPF tracer
(via `go.opentelemetry.io/ebpf-profiler`) unconditionally calls
`prlimit()` to raise `RLIMIT_MEMLOCK`'s *hard* limit to infinity before
loading any BPF program — raising a hard limit is specifically gated by
`CAP_SYS_RESOURCE` in Linux, and this sandbox's hard limit is fixed at
8192 KB and cannot be raised even as root:

```
$ ulimit -l unlimited
/bin/bash: line 35: ulimit: max locked memory: cannot modify limit: Operation not permitted
```

The exact library call (`rlimit.MaximizeMemlock()`,
`evidence/05-rlimit-go-source.go`) has no flag or fallback — it's an
unconditional precondition used across the library's core eBPF-loading
paths. Full diagnosis, each step independently verified, in
`ANALYSIS.md`.

## The short version of what does work, verified

`server/parca.yaml` configures real `scrape_configs` (schema confirmed
against Parca's own upstream repo) against two real targets: Parca
itself, and `workload/profworkload` — a small real Go binary doing real
SHA-256 hashing and real map allocation, serving the standard library's
own `net/http/pprof`. After ~5 minutes of running, a real gRPC query
(`verify/query-examples.sh`, via a real `grpcurl` built from source) against
Parca's `QueryService` returns, straight from Parca's storage:

- **CPU profile, top function:** `crypto/internal/fips140/sha256.blockSHANI`
- **Memory profile, top functions:** `main.hashBurn`, `main.allocBurn`

— the workload's own real functions, symbolized and queryable, proving
scrape → ingest → store → query works completely, independent of the
eBPF agent. Full detail in `ANALYSIS.md` and `evidence/`.
