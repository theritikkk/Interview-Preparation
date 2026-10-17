# Project 10: Service Mesh Basics (Istio)

Four real service-mesh data-plane behaviors — **mTLS**, **traffic
splitting**, **retries**, and **circuit breaking** — built with the
actual `envoy` binary (the same proxy Istio injects as a sidecar) and
verified against Envoy's own stats, not just app-level pass/fail.
Istio's own control plane (`istiod`) doesn't run in this sandbox; the
reason is precisely diagnosed, and it's the same cgroup-delegation wall
Project 7 found, now confirmed with the cleanest reproduction yet (even
`k3s`'s own bundled CoreDNS can't start a pod here). See
[`ANALYSIS.md`](ANALYSIS.md) for the full story — this file is the
quick "what's here / how to run it" guide.

## What's here

```
10-service-mesh-deliverable/
├── README.md                    this file
├── ANALYSIS.md                  full design/trade-off writeup — read this for the "why"
├── run-mesh-demo.sh              runs all four data-plane demos end to end
├── backends/app.py               tiny backend service (identifies itself, can be made flaky/slow)
├── envoy-config/
│   ├── front-proxy.yaml          traffic-split + retry + circuit-breaker routes
│   └── mtls/
│       ├── gen-certs.sh          generates a real local CA + client/server identity certs
│       ├── server-sidecar.yaml   mTLS-terminating sidecar (require_client_certificate: true)
│       └── client-sidecar.yaml   mTLS-originating sidecar
├── k8s-istio/
│   └── install-istio-attempt.sh  the real istioctl install runbook, annotated with why it can't complete here
└── evidence/                     6 files of captured command output backing every claim in ANALYSIS.md
```

## Run it

Requires Python 3, `openssl`, and `curl`, on Linux x86_64. No root,
no Kubernetes needed for the demo path.

```bash
./run-mesh-demo.sh
```

This downloads the real Envoy v1.39.4 binary (~100MB, not shipped in
this zip) if not already present, generates a local CA and certs,
starts 5 backend services and 3 Envoy instances, and runs all four
demos in sequence, printing live results. It reproduces exactly what's
captured in `evidence/01` through `evidence/04`. It cleans up all its
processes on exit (including Ctrl-C).

To explore one feature at a time after running the script once (certs
and the `envoy` binary are now present), start the pieces by hand:

```bash
./envoy -c envoy-config/front-proxy.yaml &
curl http://127.0.0.1:10000/split     # weighted 80/20 routing
curl http://127.0.0.1:10000/retry     # retry policy against a flaky backend
curl http://127.0.0.1:10000/circuit   # circuit breaker (send several concurrently to see it trip)
```

Envoy's live admin/stats page for the front proxy is at
`http://127.0.0.1:9901/stats` while it's running — the retry and
circuit-breaker counters referenced in `ANALYSIS.md` live there.

## The control plane that doesn't run here

`k8s-istio/install-istio-attempt.sh` is the real install runbook, using
the real `istioctl` CLI (Istio does ship via GitHub Releases — unlike
`chaosd`/`helm` in Project 9). It's not run end-to-end in this sandbox:
a real, fresh k3s cluster with its **default CNI left on** (ruling out
the CNI-specific confound in Project 7's Cilium test) still can't start
*any* pod sandbox — not Istio's, not even k3s's own bundled CoreDNS,
with zero container registries involved. Full evidence in
`evidence/05` and `evidence/06`; full reasoning in `ANALYSIS.md`.

## What the four demos prove, briefly

- **Traffic split** — a real 80/20 `weighted_clusters` route, measured
  across 100+ requests landing within a few points of the configured
  split every run.
- **Retries** — a backend that really fails ~40% of the time; Envoy's
  `retry_policy` turns that into 98%+ success, and Envoy's own
  `/stats` shows the actual retry attempts and their outcomes, not just
  that failures went away.
- **Circuit breaking** — `max_connections: 2` / `max_pending_requests: 2`
  genuinely rejects 6 of 10 concurrent requests with a real `503`
  before they ever reach the backend, confirmed via Envoy's overflow
  counters.
- **mTLS** — a real CA and real identity certs; the mTLS-only listener
  genuinely refuses plaintext, a missing client cert, and an
  untrusted (non-CA-signed) client cert, and genuinely accepts the
  real one, each verified with a raw `openssl s_client` handshake, not
  just the app-level request.
