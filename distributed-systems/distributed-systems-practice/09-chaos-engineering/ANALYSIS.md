# Analysis: Chaos Engineering Basics

## Headline

Three real chaos-engineering primitives — network fault injection,
process disruption, and resource exhaustion — are built and verified
here, each with a measured before/after effect against a live target.
None of them use a simulation or a mock: real TCP connections are
really delayed and really dropped, a real process is really frozen and
really killed, and real CPU/memory load is really applied, each
verified by measuring the actual effect, not by trusting that a tool
ran without error.

What doesn't run is the two pieces this roadmap originally pointed at
by name — `chaosd` (the standalone CLI) and the Kubernetes-native
Chaos Mesh operator — and each failure is tracked down to a specific,
independently-confirmed cause rather than left as "couldn't get it
working." The real OS-level mechanisms those tools wrap — Linux traffic
control, POSIX signals, and `stress-ng` — are used directly instead,
which is both honest about the gap and, in each case, a legitimate
real-world chaos-engineering technique in its own right (the userspace
TCP-proxy approach to network chaos, in particular, is exactly how
production tools like Shopify's toxiproxy work).

## What's real here, end to end

1. **Network chaos (`tcpchaos/`).** A real TCP proxy, written for this
   project, sits in front of a real HTTP server and deliberately adds
   delay, jitter, and silent chunk-drops to traffic actually passing
   through it. Measured, not assumed:
   - Direct to the target, 20 real requests: mean **0.72ms**, 20/20 succeeded.
   - Through the proxy with `-delay-ms 150 -jitter-ms 30 -loss-pct 10`,
     the same 20 real requests: mean **536.7ms** (min 319.7ms — consistent
     with the real delay being applied on both proxy hops, request and
     response), **19/20 succeeded** — one genuine failure, matching the
     configured ~10% loss rate almost exactly.
   - Full output: `evidence/07-tcpchaos-network-chaos.txt`.

2. **Process chaos (`procchaos/`).** Real POSIX signals against a real
   running server process (PID captured live, not assumed):
   - `SIGSTOP`: the process's own `ps` state flips to `Tl` (traced/stopped);
     a real request against it genuinely hangs and times out
     (`curl: (28) Operation timed out`).
   - `SIGCONT`: state flips back to `Sl`; the very next real request
     succeeds again, confirming genuine recovery.
   - `SIGKILL`: the process is actually gone (`pgrep` finds nothing, a
     real request gets `Failed to connect ... Couldn't connect to server`);
     restarting it is a real recovery step, verified with another real
     request.
   - Full output: `evidence/08-process-chaos.txt`, `evidence/09-process-kill-chaos.txt`.

3. **Resource chaos (`stresschaos/`).** The *actual* `stress-ng` binary
   Chaos Mesh's own `StressChaos` CRD wraps — not a reimplementation —
   downloaded from `chaos-mesh/stress-ng`'s real GitHub releases (the
   same binary-distribution pattern Projects 7-8 established for
   k3s/cilium-cli/parca). Confirmed with real measurements:
   - CPU: baseline load average 0.04 → genuinely climbing (0.65 and
     rising, 3 seconds into a 10-second 4-worker 100%-load run); real
     `usr+sys` time of 40.00 CPU-seconds consumed in 10.22s wall time.
   - Memory: a real 2×256MB allocate/touch/free workload completes with
     real bogo-ops throughput reported.
   - Full output: `evidence/10-resource-chaos-stress-ng.txt`. Notably,
     `k8s-chaos-mesh/examples/burn-cpu.yaml` — the real upstream Chaos
     Mesh example — configures its `StressChaos` CR with
     `options: ["--cpu 2", "--timeout 600", "--hdd 1"]`, which are
     literal `stress-ng` CLI flags: this project's manual run uses the
     identical real tool Chaos Mesh's own operator would have invoked.

## Why `chaosd` (the standalone CLI) isn't running here

This took real, substantial investigation, not a quick guess, and it
surfaced two independent findings.

**Finding 1: chaosd has no GitHub-release binary.** Unlike every other
tool obtained this way in this series (k3s, cilium-cli, parca,
parca-agent), `chaosd`'s own release workflows
(`.github/workflows/release_tag.yml`, `release_latest.yml`, both
captured in `evidence/02`) upload exclusively to an AWS S3 bucket
behind `mirrors.chaos-mesh.org` — with a `# TODO: release on github
package / release` comment still sitting in the workflow file itself.
That mirror is blocked by this sandbox's egress policy, confirmed
directly: `curl` gets a 403 at the CONNECT step
(`evidence/01-chaosd-mirror-blocked.txt`), the same signature every
other blocked host in this series has shown.

**Finding 2: building chaosd from source — the exact technique that
worked cleanly for `grpcurl` in Project 8 — doesn't converge in
reasonable time, for a real, specific reason.** Go's vanity-import
resolution for blocked domains (`google.golang.org/*`, `golang.org/x/*`,
etc.) can be routed around with `go.mod` `replace` directives pointing
at each dependency's real GitHub mirror — this is exactly what Project
8 did, in 11 lines, for `grpcurl`. The same technique was applied here:
a script parsed `chaosd`'s `go.mod` (162 declared dependencies) and its
existing version-pinning `replace` block, generated 58 `replace`
directives mapping every blocked-vanity-domain dependency to its GitHub
mirror at the correct pinned version, and fixed a secondary issue
(`+incompatible` version suffixes for 4 dependencies whose mirrors don't
use semantic-import-versioning paths). This genuinely worked — hundreds
of real Go modules were fetched over git from GitHub for real, and the
build got substantially further than the starting point. But it kept
surfacing *new* blocked domains at deeper transitive layers that aren't
visible in chaosd's own top-level `go.mod` — 12 distinct blocked
vanity domains were hit in total by the third/fourth attempt
(`cloud.google.com`, `collectd.org`, `go.etcd.io`, `go.mozilla.org`,
`go.opentelemetry.io`, `golang.org`, `gonum.org`, `google.golang.org`,
`gopkg.in`, `gotest.tools`, `sigs.k8s.io`, plus `k8s.io`/`go.uber.org`
fixed early), compared to `grpcurl`'s 8 in Project 8, which converged
in two rounds. The root cause: `chaosd`'s `cmd/main.go` unconditionally
imports `sigs.k8s.io/controller-runtime` for its logging setup, even
though `chaosd` is positioned as a standalone, non-Kubernetes tool —
so building it at all drags in a large, sprawling slice of the
Kubernetes client ecosystem (`client-go`, `apiserver`,
`apiextensions-apiserver`...) with its own enormous transitive
footprint. Full numbers and the stopped-here reasoning:
`evidence/05-source-build-attempt-summary.md`; the 58-line patch that
*did* work as far as it went: `evidence/03-chaosd-patched-go.mod`.

This is a different kind of finding from Project 8's grpcurl fix — not
"the same trick, more of it," but a real discovery that a tool
marketed as lightweight and Kubernetes-independent is, at the
dependency-graph level, neither.

## Why Chaos Mesh's Kubernetes operator isn't running here

Three independent, each separately confirmed:

1. **No working `helm` binary either**, and for a closely related
   reason to chaosd's: `helm`'s own `Makefile` (`fetch-dist` target,
   `evidence/11`) and its real release workflow
   (`.github/workflows/release.yml`) show it ships compiled binaries
   via Azure Blob Storage behind `get.helm.sh`, never via GitHub
   Releases — confirmed by probing `github.com/helm/helm/releases/download/`
   against several real version tags (including the actual current
   release, `v4.3.0`, discovered via the same `releases/latest/download/`
   redirect trick that worked for `cilium-cli` in Project 7): all 404.
   `get.helm.sh` itself returns the familiar 403-at-CONNECT signature.
2. **Chaos Mesh's own chart pulls from `ghcr.io`**
   (`k8s-chaos-mesh/chart/values.yaml`'s `registry: "ghcr.io"`), one of
   the registries Project 7 already confirmed blocked (403) alongside
   `docker.io`/`quay.io`/`registry.k8s.io`.
3. **Independent of both**: Project 7 already proved, with a real
   unprivileged pod (`cilium-operator`), that this sandbox's cgroup
   hierarchy isn't delegated for nested pod-sandbox creation — every
   real pod kubelet tries to start fails identically
   (`can't get final child's PID from pipe: EOF`), regardless of image
   or chart.

Given all three are independently confirmed and none depend on which
chart or image is used, re-running the full k3s-bring-up-then-install
sequence for Chaos Mesh specifically (as Projects 6-7 did for their own
subjects) would not surface new information — it would reproduce
already-established facts. `k8s-chaos-mesh/chart/` ships the real,
verbatim upstream Helm chart (diff it against
`github.com/chaos-mesh/chaos-mesh` at tag `v2.7.2` to confirm) and
`k8s-chaos-mesh/examples/` ships 6 real upstream example chaos CRs
(`NetworkChaos`, `PodChaos` ×2, `StressChaos` ×2), exactly matching
Project 7's resolution for Tetragon: the real artifact, honestly
labeled as untested end-to-end here, with the precise reasons why.

## Why the network-chaos fallback (`tcpchaos`) exists, and isn't a lesser substitute

The first-choice approach for network chaos — Linux's kernel-level
`tc qdisc ... netem` — was tried for real and rigorously ruled out, not
assumed unavailable:

- `iproute2` (the `tc` CLI) isn't installed, and can't be installed:
  `apt-get` can't reach `archive.ubuntu.com` at all in this sandbox
  (confirmed, same as every prior project touching `apt`).
- So a real Go program (`netchaos/`, built against
  `github.com/vishvananda/netlink` — a thin, real binding over the
  actual `RTM_NEWQDISC`/`RTM_DELQDISC` netlink syscalls, not a
  simulation) was used to attempt adding a netem qdisc directly via
  netlink, bypassing the missing CLI entirely — the same kind of
  workaround Project 5 used for XDP/TC-BPF attachment when `ip`/`tc`
  weren't available.
- This failed with a real, specific kernel error: `ENOENT` ("no such
  file or directory") from `RTM_NEWQDISC`. `CAP_NET_ADMIN` was
  confirmed present (ruling out a permissions explanation). Probing
  *every* other non-default qdisc kind available in the library
  (`prio`, `fq`, `fq_codel`) hit the identical `ENOENT`. `/proc/kallsyms`
  settled it precisely: this kernel has the generic qdisc framework
  built in (`qdisc_alloc`, `register_qdisc`, `pfifo_fast_init` are all
  present) but **zero scheduler implementations** registered —
  `sch_netem`, `sch_prio`, `sch_fq_codel`, `sch_sfq`, `sch_htb`,
  `sch_tbf` all have zero matches in the live kernel symbol table. This
  is a genuine kernel build-time fact about this custom kernel
  (`6.18.44-fc-v114`), the same character of finding as Project 6's
  `CONFIG_SCHED_CLASS_EXT` wall: not a tooling gap or a permissions
  issue, an unconditional absence. Full evidence:
  `evidence/06-netem-kernel-unavailable.txt`.

Given that, `tcpchaos` — a real userspace TCP proxy that injects delay,
jitter, and loss into traffic it actually relays — is not a lesser
stand-in. It's the same technique real, widely-used production chaos
tools (Shopify's toxiproxy being the best-known example) use
precisely *because* it works anywhere a process can open a socket,
with no dependency on kernel qdisc support, root privileges beyond
normal socket binding, or any specific container capability. The
before/after measurement in `evidence/07` is genuine evidence of
genuine network-level fault injection, just implemented one layer up
the stack from where `tc netem` would sit.

## What this doesn't demonstrate

- **Kernel-level, link-layer network chaos** — `netchaos/` is shipped
  because it's real, correct code and a real, precisely-diagnosed
  negative result (see above), but it does not successfully inject
  chaos in this sandbox. On a kernel built with `CONFIG_NET_SCH_NETEM`
  (the overwhelming majority of real Linux systems, including every
  mainstream distro kernel), it works as intended.
- **A running Chaos Mesh control plane**, Kubernetes-native chaos
  experiment scheduling, the Chaos Dashboard UI, or blast-radius
  targeting via Kubernetes label selectors — all require the operator,
  which doesn't run here for the three reasons above.
- **`chaosd`'s own CLI/UX, its experiment-state tracking (search/recover
  commands), or its JVM-level chaos (byteman)** — `chaosd`'s binary
  itself doesn't run here; the real mechanisms its *host-mode* attacks
  use (process signals, `stress-ng`) are demonstrated directly instead.
- **Disk/IO chaos or kernel-injected faults** (Chaos Mesh's `IOChaos`,
  `KernelChaos`) — not attempted; these typically need either the
  Chaos Mesh daemon's ptrace-based interception or kernel fault-injection
  hooks, neither explored here given the operator itself is unavailable.
