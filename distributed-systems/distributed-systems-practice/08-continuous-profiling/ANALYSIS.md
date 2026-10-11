# Analysis: Continuous Profiling (Parca)

## Headline

A real Parca server runs here, scraping two real pprof targets (itself,
and a small real Go workload written for this project) over plain HTTP,
storing real profile data, and answering real gRPC queries that return
the workload's actual function names (`crypto/sha256.blockSHANI`,
`main.hashBurn`, `main.allocBurn`) pulled back out of storage — a
complete, working, end-to-end continuous-profiling pipeline, verified by
querying it, not just by watching it start without erroring.

The one piece that does **not** work is `parca-agent`, Parca's flagship
eBPF-based whole-system profiler. It fails at eBPF-tracer load time with
a precise, reproducible, single-cause error: this sandbox's container is
missing exactly one Linux capability, `CAP_SYS_RESOURCE`, out of the 41
standard ones — and a specific, unconditional line of code in the
profiler library parca-agent embeds needs that exact capability to raise
`RLIMIT_MEMLOCK`'s hard limit. There is no flag, config, or env var that
routes around it; it was checked. This is a different kind of finding
than Project 6's "the kernel hard-refuses to load this" or Project 7's
"two independent infrastructure walls" — it's a *missing grant*, not a
missing kernel feature or a network policy, and it's the cleanest,
most-precisely-isolated root cause of any blocker in this series so far.

## What's real here, end to end

1. **Real binaries, from real GitHub releases.** `parca` v0.29.1 and
   `parca-agent` v0.50.0, both downloaded from
   `github.com/parca-dev/{parca,parca-agent}/releases/download/...` —
   the same "container registries blocked, GitHub release assets
   allowed" pattern Project 7 discovered for k3s and cilium-cli. Neither
   binary needed Docker or a registry at all; this is a much simpler
   distribution model than Cilium's.

2. **A real Go workload, written for this project**
   (`workload/main.go`). `profworkload` is a genuine compiled Go binary
   with two deliberately distinct, named hot paths:
   - `hashBurn`: SHA-256 hashing in a tight loop — pure CPU work.
   - `allocBurn`: repeated map/string construction — pure allocation
     work.

   It serves these over the Go standard library's `net/http/pprof`,
   wired in with a single `import _ "net/http/pprof"` — nothing custom,
   nothing simulated. The point of writing distinct, recognizably-named
   functions was so a resulting profile's contents could be checked
   against what the code is actually supposed to be doing, not just
   checked for "did a response come back."

3. **A real Parca server, configured with real `scrape_configs`.**
   `server/parca.yaml` follows the exact schema in Parca's own upstream
   root `parca.yaml` (cloned from `github.com/parca-dev/parca` to
   confirm — see `evidence/` for how the schema was verified before
   being used, same "read the source, don't guess the config format"
   discipline as every other project in this series). Two scrape jobs:
   - `parca` — Parca scraping its own `/debug/pprof/*` endpoints. This
     matches the project's own documented default behavior: "By
     default, Parca is scraping its own pprof endpoints" (from the
     `README.md` bundled inside the real release tarball).
   - `profworkload` — Parca scraping `profworkload`'s endpoints on
     `:9999`.

4. **Verified scraping, not assumed.** Parca's own `/metrics` endpoint
   and gRPC `ScrapeService.Targets` call show, in real numbers: all 10
   scrape targets (5 profile types × 2 jobs) at `HEALTH_GOOD`, with
   live `lastScrape` timestamps advancing every ~10s, and
   `parca_target_samples_total` climbing into the tens of thousands
   over the course of the run (see `evidence/09-*` and `evidence/10-*`).

5. **Verified ingestion and query, pulled back out of Parca's own
   storage.** This is the strongest evidence in the project. A real
   gRPC client (`grpcurl`, see below for why building it was itself a
   small real finding) queried Parca's `QueryService.Query` RPC for a
   merged CPU profile and a merged memory profile over `job="profworkload"`,
   for a time range covering the live run. The responses — straight
   from Parca's storage and symbolization layer, not from re-reading
   the raw pprof file directly — came back with:
   - CPU top: `crypto/internal/fips140/sha256.blockSHANI` at the top by
     a wide margin (the real low-level SHA-256 block function Go's
     `crypto/sha256` compiles down to on this CPU), with a `filename`
     field pointing at the real Go standard library source path.
   - Memory top: `main.hashBurn` and `main.allocBurn` — the project's
     own named functions — at the top, by far outweighing the Go
     runtime's own allocations.

   The full request/response pairs are in `evidence/11-query-cpu-top.txt`
   and `evidence/12-query-memory-top.txt`. This proves scrape → ingest →
   store → query → symbolize all work for real, independent of the
   eBPF agent entirely.

6. **A real, if initially unexpected, second finding: grpcurl itself
   needed a workaround to build.** `grpcurl` isn't distributed as a
   GitHub-release binary for this use case the way k3s/cilium-cli/Parca
   are (fullstorydev/grpcurl does publish release tarballs, but the
   simplest path was building it, see below for why). Building it with
   `go install` failed — not because the target repo was unreachable,
   but because Go's *vanity import path* resolution for its
   dependencies (`google.golang.org/grpc`, `google.golang.org/protobuf`,
   `golang.org/x/*`, `cloud.google.com/go/*`) requires an HTTP GET to
   the vanity domain itself (e.g. `https://google.golang.org/grpc?go-get=1`)
   to discover the real backing repository, and those specific domains
   are blocked by this sandbox's egress policy (403), exactly like
   `quay.io`/`docker.io`/etc. were in Project 7. The actual GitHub
   mirrors those vanity imports resolve to (`github.com/grpc/grpc-go`,
   `github.com/protocolbuffers/protobuf-go`, etc.) are perfectly
   reachable. Adding `replace` directives in `go.mod` that point each
   blocked vanity import straight at its GitHub mirror (11 lines, see
   `verify/build-grpcurl.sh` and `evidence/13-grpcurl-build-go.mod`)
   fixed it completely — `go build` then fetches everything over git
   directly from GitHub, never touching the blocked domains. This
   is the same shape of finding as Project 7's registry block, just one
   layer further down the stack (module resolution instead of image
   pulls) — documented here rather than silently worked around, because
   it's a genuine, reusable technique for this sandbox's constraints.

## The eBPF agent: the real, precise blocker

### The symptom

```
time=2026-10-11T08:56:39.908Z level=INFO msg="VMA lookup disabled: failed to adjust rlimit for VMA helper probe: failed to set temporary rlimit: operation not permitted; patched 6 instructions"
time="2026-10-11T08:56:39Z" level=error msg="Failed to load eBPF tracer: failed to load eBPF code: failed to set RODATA variables: failed to prepare programs and maps for system analysis: failed to adjust rlimit: failed to set temporary rlimit: operation not permitted"
```

(Full real log: `evidence/04-parca-agent-rlimit-failure.log`.)

### Diagnosis, step by step, each step independently checked

**Step 1 — is this actually an `RLIMIT_MEMLOCK` problem?** Checked
`ulimit -a`, `-Hl`, `-Sl` directly:

```
max locked memory           (kbytes, -l) 8192
```

Both the soft *and hard* limit are exactly 8192 KB — not "unset" or
"unlimited," a real fixed cap. (`evidence/03-rlimit-evidence.txt`.)

**Step 2 — can the hard limit be raised, even as root?** Tried directly:

```
$ ulimit -l unlimited
/bin/bash: line 35: ulimit: max locked memory: cannot modify limit: Operation not permitted
```

Raising a resource limit's *hard* ceiling (not just the soft value up to
the existing hard ceiling) is specifically gated by `CAP_SYS_RESOURCE`
in Linux — this is textbook behavior, not a guess, so this pointed
straight at one specific capability.

**Step 3 — is `CAP_SYS_RESOURCE` actually missing, or is something else
going on?** Rather than assume, decoded this container's actual
effective-capability bitmask from `/proc/self/status`
(`CapEff: 000001fffeffffff`) against all 41 standard Linux capabilities
(`evidence/01-capabilities-raw.txt`, `evidence/02-capabilities-decoded.txt`).
Result: **40 of 41 capabilities are present** — including
`CAP_SYS_ADMIN`, `CAP_BPF`, `CAP_PERFMON`, and `CAP_IPC_LOCK` (which is
the capability that historically governed `mlock()`/memlock behavior
before `RLIMIT_MEMLOCK`-based accounting became standard — its presence
here rules out "no memlock-related capability at all" and narrows it to
specifically the resource-limit-raising capability). The **one** missing
capability is `CAP_SYS_RESOURCE`. This is about as clean a root cause as
a diagnosis gets in this series: not "probably," not "most likely" — a
single named capability, confirmed absent by decoding the actual bitmask
bit by bit.

**Step 4 — is this actually required by the code, unconditionally, with
no bypass flag?** Checked `parca-agent --help` first: there's a
`--memlock-rlimit` flag, but it's marked `[deprecated]` and, per its own
help text, only sets *which value* to request — it doesn't skip the
underlying raise-to-hard-limit call (`evidence/07-parca-agent-help.txt`).
So the actual code was read. parca-agent v0.50.0's `go.mod` shows its
eBPF tracer is implemented via `go.opentelemetry.io/ebpf-profiler`
(replaced with a `parca-dev` fork — OTel's own eBPF profiling library,
not a parca-specific one). Cloning that library and grepping for the
exact error string `"failed to set temporary rlimit"` landed on
`rlimit/rlimit.go`:

```go
func MaximizeMemlock() (func(), error) {
	var oldLimit unix.Rlimit
	tmpLimit := unix.Rlimit{
		Cur: unix.RLIM_INFINITY,
		Max: unix.RLIM_INFINITY,
	}
	if err := unix.Prlimit(0, unix.RLIMIT_MEMLOCK, &tmpLimit, &oldLimit); err != nil {
		return nil, fmt.Errorf("failed to set temporary rlimit: %w", err)
	}
	...
}
```

(Full file: `evidence/05-rlimit-go-source.go`.) It unconditionally
requests `RLIM_INFINITY` for *both* the soft and the **hard** limit in
one `prlimit()` call — which is exactly the operation that needs
`CAP_SYS_RESOURCE` to succeed when the current hard limit (8192 KB here)
is below infinity. `grep -rn MaximizeMemlock` across the library
(`evidence/06-maximizememlock-callsites.txt`) shows it's called from
core paths — `tracer.go`, `tracer/tracepoints.go`,
`processmanager/ebpf/ebpf.go`, `tracer/systemconfig.go` — with no
conditional, no feature flag, no fallback to "just use the existing
limit." On a modern kernel (5.11+) that does cgroup-based BPF memory
accounting, raising `RLIMIT_MEMLOCK` to infinity isn't strictly
necessary for BPF map creation to succeed — but this library doesn't
know that or attempt it; it tries to maximize the rlimit first, as a
blanket precondition, and refuses to proceed if that specific call
fails, regardless of whether the subsequent map allocations would
actually have fit under the existing 8192 KB cap.

### The conclusion

This is a genuine, precisely-isolated hard wall for this specific
sandbox, not a workaround-able gap: **one capability, `CAP_SYS_RESOURCE`,
absent from an otherwise near-complete 40/41 capability set, required by
one unconditional library call with no bypass, each fact independently
verified** (the ulimit behavior, the capability bitmask, the source
code) rather than inferred from the error message alone. Unlike Project
6's finding (a kernel *build-time* configuration choice, unchangeable
from userspace on any kernel with that config) or Project 7's findings
(network policy + cgroup delegation, both properties of this sandbox's
host configuration), this one has an extremely narrow, specific fix:
grant the container `CAP_SYS_RESOURCE`. That's a one-line change to
whatever created this container (Docker `--cap-add=SYS_RESOURCE`, a
Kubernetes pod's `securityContext.capabilities.add`, etc.) — outside
what this sandbox can do to itself, but worth stating precisely rather
than vaguely, since it's the most actionable of any blocker so far.

## Why the alternative (scrape-based) path is a legitimate demonstration, not a consolation prize

Parca is explicitly designed around two ingestion paths, not one:
eBPF-based (via `parca-agent`) and pprof-scrape-based (built into the
server itself, no agent required — see the `scrape_configs` block
documented directly in Parca's own upstream `parca.yaml`). The
eBPF path's value proposition is *zero instrumentation* — profiling
processes that weren't written with profiling in mind. The scrape path's
value proposition is *using the instrumentation your language runtime
already has* — every Go binary gets `net/http/pprof` for free. Both are
real, supported, documented parts of Parca; this project demonstrates
the second one completely, with real ingested data queried back out of
real storage, and documents exactly where and why the first one stops,
with the same rigor.

## What this doesn't demonstrate

- **eBPF-based whole-system profiling** (profiling `profworkload` or any
  other process *without* that process cooperating via `net/http/pprof`)
  — this is exactly what `parca-agent` provides and exactly what's
  blocked here. On infrastructure with `CAP_SYS_RESOURCE` available
  (essentially everywhere outside this specific sandbox configuration),
  `agent/setup-parca-agent.sh` is the complete, correct command.
- **Persistent storage across restarts** — Parca's default active-memory
  storage (no `--enable-persistence`/`--storage-enable-wal`) was used,
  matching the quickstart default; data was queried live rather than
  demonstrating durability across a restart, which wasn't the question
  this project was testing.
- **Multi-node / Kubernetes-native target discovery** — the scrape
  config here is two static targets on localhost; Parca also supports
  Kubernetes pod discovery for scrape targets, not exercised here (and,
  per Project 7, a live multi-pod cluster with arbitrary workloads has
  its own separate, already-documented blockers in this sandbox).
- **The Parca web UI's flamegraph rendering** — verified via the
  `QueryService` gRPC API directly (top-list reports with real function
  names), not via a browser screenshot of the actual flame graph widget;
  the underlying data is identical, the UI is React composing the same
  API responses.
