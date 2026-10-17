# Analysis: Service Mesh Basics (Istio)

## Headline

A real Istio control plane doesn't run in this sandbox — not because of
anything specific to Istio, but because of the same cgroup-delegation
wall Project 7 found, now confirmed with a cleaner, more minimal
reproduction than before: even `k3s`'s own bundled CoreDNS pod, with a
normal CNI and zero external registries involved, can't start a pod
sandbox here. What's real and fully verified instead is the actual
data-plane mechanics a service mesh provides — mTLS, traffic splitting,
retries, and circuit breaking — built with the real `envoy` binary
(the exact proxy Istio injects as a sidecar), configured statically
instead of by istiod's dynamic xDS, and each one verified against
Envoy's own stats/admin API, not just by observing whether the app-level
request happened to succeed.

## What's real here, end to end

All four demos use the genuine Envoy v1.39.4 binary (downloaded from
`envoyproxy/envoy`'s real GitHub release), real backend processes, and
real network connections on 127.0.0.1 — nothing mocked, nothing
simulated at the protocol level.

1. **Traffic splitting (`/split`).** A real `weighted_clusters` route
   (80% `service-v1` / 20% `service-v2`). 100 real requests measured:
   79/21, 84/16, 78/22 across three separate runs — consistently close
   to the configured split. `evidence/01-traffic-split.txt`.

2. **Retry policy (`/retry`).** A backend configured to fail ~40% of
   requests with a real 503. Direct requests: ~65-70% success (matching
   the configured failure rate). Through Envoy's `retry_policy`
   (`retry_on: 5xx`, `num_retries: 3`): 98%+ success. Envoy's own
   `/stats` shows the mechanism, not just the outcome —
   `upstream_rq_retry: 65`, `upstream_rq_retry_success: 39`,
   `upstream_rq_retry_limit_exceeded: 2` — real retries were attempted,
   most succeeded, a few genuinely exhausted all 3 attempts and still
   failed (matching the observed failure count exactly).
   `evidence/02-retry-policy.txt`.

3. **Circuit breaking (`/circuit`).** A cluster with `max_connections: 2`,
   `max_pending_requests: 2` fronting a backend with an artificial 500ms
   delay (so concurrent requests genuinely overlap instead of completing
   too fast to contend). 2 sequential requests: both succeed. 10
   concurrent requests: 4 succeed, 6 get a real `503` with
   `reset reason: overflow` — and Envoy's own counters
   (`upstream_cx_overflow: 6`, `upstream_rq_pending_overflow: 2`,
   `upstream_rq_active_overflow: 4`) confirm Envoy itself rejected them
   at the proxy, before the backend ever saw them.
   `evidence/03-circuit-breaker.txt`.

4. **mTLS (`client-sidecar` → `server-sidecar`).** A real local CA
   (`openssl`), with separate `client-sidecar` and `server-sidecar`
   identity certs it signed. The server-side Envoy listener requires a
   client certificate (`require_client_certificate: true`) and validates
   it against the CA; the client-side Envoy originates TLS with its own
   cert. Verified four ways, each a real TLS handshake:
   - Through the real path (client-sidecar → mTLS → server-sidecar →
     backend): genuine `HTTP 200` from the real backend.
   - Plaintext straight at the mTLS-only port, bypassing the
     client-sidecar entirely: connection refused — the listener only
     speaks TLS.
   - A real TLS handshake with **no** client certificate: rejected with
     TLS alert 116, "certificate required".
   - A real TLS handshake with a **self-signed cert not signed by the
     mesh CA**: rejected with TLS alert 48, "unknown ca" — proving the
     CA validation is genuinely checked, not just that *a* cert is
     present.

   `evidence/04-mtls-enforcement.txt`.

None of this needed Kubernetes, istiod, or any control plane at all —
which is itself worth being honest about: a real service mesh's value
isn't the proxy (that part is fully demonstrated here), it's the control
plane that configures hundreds of these proxies dynamically, rotates
their certs automatically, and gives you one place to see and change
mesh-wide policy. That part genuinely doesn't run here, for a reason
entirely unrelated to Istio.

## Why the Istio control plane isn't running here

Two things were checked, independently, with real tools against a real
cluster — not assumed from Project 7's result.

**1. Istio's binaries are genuinely obtainable here — unlike chaosd and
helm in Project 9.** `istioctl` ships via `istio/istio`'s actual GitHub
Releases (`istioctl-1.31.1-linux-amd64.tar.gz`), and downloaded and ran
cleanly. This is a real, useful distinction from Project 9's finding:
not every CNCF project avoids GitHub Releases — Istio does use them, the
same as k3s, cilium-cli, parca, and stress-ng before it.

**2. A real k3s cluster, real default CNI, zero registries — and pods
still can't start.** Project 7's finding involved Cilium specifically,
with CNI disabled and a registry-hosted image — leaving open the
question of whether the cgroup-delegation issue was somehow tied to
that specific setup. This project closes that gap. A **fresh** k3s v1.31
cluster was brought up with its **default flannel CNI left enabled**
(not disabled, unlike Project 7), so the node reaches `Ready` normally.
Then:

- `kubectl get pods -n kube-system` shows k3s's own bundled CoreDNS pod
  — an image from k3s's airgap bundle, zero external registry contact
  of any kind — stuck in `ContainerCreating` forever, with:
  ```
  Failed to create pod sandbox: rpc error: code = Unknown desc = failed
  to create containerd task: failed to create shim task: OCI runtime
  create failed: runc create failed: unable to start container process:
  can't get final child's PID from pipe: EOF: unknown
  ```
  (`evidence/05-cgroup-wall-confirmed-on-coredns.txt`) — the exact same
  error signature Project 7 found on Cilium's pods, now reproduced on
  the simplest possible workload, with no CNI or registry confound at
  all. This is the strongest, most minimal version of this finding in
  the series so far: it rules out CNI choice and registry access as
  contributing factors, leaving only the cgroup-delegation limitation
  itself.

- `istioctl install --set profile=minimal -y` was then run for real
  against this cluster. It applied Istio's actual CRDs and Deployment
  manifests, scheduled `istiod` (image: `docker.io/istio/pilot:1.31.1`),
  and hit the identical `FailedCreatePodSandBox` error before ever
  reaching the image-pull step
  (`evidence/06-istiod-hits-same-cgroup-wall.txt`). Worth noting:
  `docker.io` is itself one of the registries Project 7 already found
  blocked — so even if the cgroup wall weren't there, `istiod`'s image
  pull would be a second, independent blocker, same two-layer pattern
  as Project 7's Cilium finding.

The conclusion from Project 7 holds and is now better-evidenced: this
sandbox cannot create **any** pod sandbox, for **any** workload,
regardless of CNI, image source, or which Kubernetes-native tool is
asking. This isn't an Istio-specific or even a service-mesh-specific
limitation — it would block literally every project in this series that
needs a real K8s workload running, which is exactly what Projects 7 and
9 also found, each from a different angle.

## Why the data-plane demo is a legitimate substitute, not a lesser one

The sidecar proxy *is* the thing doing the actual mesh work — mTLS,
retries, circuit breaking, traffic shaping, telemetry. istiod's job is
entirely about *configuring* that proxy dynamically and automating
certificate rotation; it doesn't do any of the data-plane work itself.
Using the identical Envoy binary with static config instead of istiod's
xDS stream exercises the exact same code paths a real Istio sidecar
would use for the four features demonstrated — the only thing missing
is automatic reconfiguration without restarting Envoy, which is a real
and worth-naming gap (see below), not a simulated one.

This is also not an invented technique: statically-configured Envoy
(no control plane at all) is a normal, supported way to run Envoy in
production for simpler topologies — it's literally Envoy's own "getting
started" configuration style, the same one `front-proxy.yaml` in
Envoy's official examples repo uses.

## What this doesn't demonstrate

Being precise about the boundary, same as every other project in this
series:

- **Dynamic reconfiguration.** A real mesh updates all its sidecars'
  routing/mTLS/circuit-breaker config live, without restarting any
  proxy, by pushing new config over xDS from istiod. This demo's Envoy
  instances read their config once at startup from a static YAML file —
  changing the split percentage or breaker thresholds means editing the
  file and restarting Envoy. This is the single biggest capability gap
  between what's here and a real mesh.
- **Automatic sidecar injection.** A real mesh mutates pod specs via a
  Kubernetes admission webhook to inject the Envoy sidecar automatically.
  Here, the two Envoy instances were started by hand with explicit
  config files — there's no injection mechanism at all, since there's no
  Kubernetes workload to inject into.
- **Automatic certificate issuance and rotation.** A real mesh's CA
  (istiod's built-in CA, or SPIRE) issues and rotates short-lived
  workload certificates automatically and continuously. Here, `openssl`
  generated long-lived (365-day) certs once, by hand, via
  `gen-certs.sh` — real certs and real validation, but no automated
  lifecycle.
- **Mesh-wide observability and policy.** Istio's Kiali/Grafana
  dashboards, `AuthorizationPolicy` CRDs, and centralized telemetry
  aggregation across many services all depend on the control plane and
  Kubernetes; none of that exists here — only the two services' own
  Envoy `/stats` endpoints, read directly.
- **Multi-hop meshes.** This demo is one client-sidecar/server-sidecar
  pair and one front-proxy with three backends — a real mesh's value
  compounds with dozens or hundreds of interconnected services, which
  isn't something a single-host demo can show regardless of sandbox
  constraints.

## A smaller, genuinely new finding: not every CNCF project avoids GitHub Releases

Project 9 found that `chaosd` and `helm` both ship binaries via cloud
storage (S3, Azure Blob) rather than GitHub Releases. This project found
the opposite for Istio: `istioctl` is a real, first-class GitHub Release
asset, same pattern as k3s/cilium-cli/parca/stress-ng. The asset naming
had to be discovered empirically either way — `releases/latest/download/`
redirects don't verify the target asset exists, they just do tag
substitution, so a wrong filename guess can silently produce a working
redirect to a 404. Confirmed by probing a few candidate names with HEAD
requests rather than guessing once and assuming success.
