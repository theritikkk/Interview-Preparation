# Kubernetes Networking, Services, and DNS

> **The one-sentence model:** every Pod gets its own IP and can reach every other Pod IP without NAT; a Service is a stable virtual IP with a DNAT rule to a set of ready Pod IPs; DNS is a Deployment that resolves names to those virtual IPs; and a NetworkPolicy is a firewall enforced by the CNI, not by kube-proxy.

---

## 1. The Pod network model

Kubernetes assumes a **flat Pod network**:

- Every Pod has a unique IP.
- Any Pod can reach any other Pod IP without NAT.
- Nodes can reach Pod IPs (and vice versa) without NAT.
- Agents on a node can reach all Pods on that node.

The CNI plugin implements this. It is not part of the Kubernetes binary.

```text
Pod A 10.244.1.5 ──veth── cni0/bridge ── node route / tunnel ── cni0 ──veth── Pod B 10.244.2.7
```

### CNI in one page

The Container Network Interface is a spec with `ADD`, `DEL`, `CHECK` operations. The runtime (containerd/CRI-O) calls a plugin binary with a JSON config on stdin and expects a JSON result containing the Pod IP, routes, and DNS.

Common plugins:

| Plugin | Model | Notes |
|---|---|---|
| `bridge` + `host-local` | L2 bridge per node, IPAM from a per-node subnet | Simple; needs routes between nodes |
| Calico | BGP or IP-in-IP/VXLAN; policy | Routed mode avoids overlay overhead; strong policy |
| Cilium | eBPF dataplane; policy, observability | Replaces kube-proxy in some modes; rich drop tracing |
| Flannel | VXLAN overlay | Simple; fewer policy features by itself |
| Antrea | Open vSwitch/eBPF | Policy and observability |
| Cloud CNIs (VPC CNI, Azure CNI) | Pod IPs from the cloud VPC | Integrates with cloud routing/security groups; IP exhaustion is a common failure |

Inspect the CNI config on a node:

```bash
ls /etc/cni/net.d/
cat /etc/cni/net.d/10-kindnet.conflist
```

```json
{
  "cniVersion": "0.3.1",
  "name": "kindnet",
  "plugins": [
    {"type": "ptp", "ipMasq": false, "ipam": {"type": "host-local", "ranges": [[{"subnet": "10.244.0.0/16"}]]}},
    {"type": "portmap", "capabilities": {"portMappings": true}}
  ]
}
```

The `ptp` plugin creates a veth pair and assigns the Pod IP; `portmap` implements `hostPort`; `ipMasq` controls NAT for traffic leaving the Pod CIDR.

### Same node vs cross node

Same node:

```text
Pod A eth0 ──veth── bridge ──veth── Pod B eth0
```

Cross node:

```text
Pod A ── bridge ── node route/tunnel ── bridge ── Pod B
```

Overlay (VXLAN) encapsulates the Pod packet inside a UDP packet between nodes. Routed (BGP) advertises Pod CIDRs to the network. Cloud CNIs use the VPC route table. Each model changes the MTU and the failure modes.

### MTU

| Underlay | Typical Pod MTU | Why |
|---|---:|---|
| Ethernet 1500, no overlay | 1500 | No extra header |
| VXLAN overlay | 1450 | 50 bytes VXLAN/IP/UDP overhead |
| IP-in-IP | 1480 | 20 bytes |
| WireGuard | ~1420 | Encryption overhead |

If the Pod MTU is wrong, small requests work and large responses hang. Test:

```bash
kubectl exec -it test -- ping -M do -s 1472 -c 1 10.244.2.7
kubectl exec -it test -- tracepath 10.244.2.7
```

`ping -M do -s 1472` sends 1500 bytes total. If it fails with `message too long` or hangs, the path MTU is smaller. Fix the CNI MTU or clamp MSS; do not guess.

---

## 2. Services

A Service selects Pods by label and gets a virtual IP (ClusterIP) from the service CIDR. It does not run a process; kube-proxy or an eBPF dataplane programs forwarding rules.

```yaml
apiVersion: v1
kind: Service
metadata:
  name: web
spec:
  selector:
    app: web
  ports:
    - name: http
      port: 80
      targetPort: 8080
  type: ClusterIP
```

```bash
kubectl get svc web
kubectl get endpointslices -l kubernetes.io/service-name=web -o wide
kubectl describe svc web
```

```text
NAME   TYPE        CLUSTER-IP     EXTERNAL-IP   PORT(S)   AGE
web    ClusterIP   10.96.142.31   <none>        80/TCP    5m

NAME         ADDRESSTYPE   PORTS   ENDPOINTS                     AGE
web-abc12    IPv4          8080    10.244.1.5,10.244.2.7         5m
```

`Endpoints` (legacy) and `EndpointSlice` (modern) are the actual backends. A Service with an empty endpoint list is a selector/readiness problem, not a DNS problem.

### Service types

| Type | What it gives you | Failure notes |
|---|---|---|
| `ClusterIP` | Virtual IP reachable inside the cluster | Default; no external access |
| `NodePort` | A port on every node (default 30000–32767) forwarding to the Service | Firewall, port conflicts, `externalTrafficPolicy` |
| `LoadBalancer` | Cloud LB or MetalLB VIP in front of NodePorts | Cloud permissions, subnet tags, LB controller |
| `ExternalName` | DNS CNAME to an external name | No proxying; clients must resolve it |
| Headless (`clusterIP: None`) | No virtual IP; DNS returns Pod IPs | Client-side load balancing; no kube-proxy |
| `Ingress` | Not a Service type; L7 HTTP routing in front of Services | Controller/class/TLS/backend |

`externalTrafficPolicy: Local` preserves the client source IP and avoids a second hop, but traffic only goes to nodes with a ready endpoint. `Cluster` may SNAT and lose the source IP. Choose deliberately.

### kube-proxy modes

| Mode | How it forwards | Strengths / weaknesses |
|---|---|---|
| `iptables` | NAT rules with random selection | Ubiquitous; rule count grows with Services/endpoints; conntrack |
| `IPVS` | Kernel virtual server with real servers | Better scale, scheduling algorithms; needs IPVS modules |
| `nftables` | nftables rules (newer) | Cleaner rule management; still maturing |
| eBPF (Cilium) | Programmable dataplane, may bypass kube-proxy | Performance/observability; different debugging model |

In iptables mode, the ClusterIP is not assigned to an interface. It is a DNAT target:

```bash
sudo iptables-save -t nat | grep -A3 'KUBE-SVC'
sudo conntrack -L | grep 10.96.142.31
```

```text
-A KUBE-SERVICES -d 10.96.142.31/32 -p tcp --dport 80 -j KUBE-SVC-XXXX
-A KUBE-SVC-XXXX -m statistic --mode random --probability 0.5 -j KUBE-SEP-AAAA
-A KUBE-SVC-XXXX -j KUBE-SEP-BBBB
-A KUBE-SEP-AAAA -p tcp -m tcp -j DNAT --to-destination 10.244.1.5:8080
```

The first packet creates a conntrack entry; return traffic is reverse-NATed. If the endpoint changes mid-connection, the existing conntrack entry keeps the old destination until it times out. That is why a rolling update can leave a few stale connections.

### Service DNS

For a Service `web` in namespace `app`, the cluster DNS name is:

```text
web.app.svc.cluster.local
```

From a Pod in the same namespace, `web` works because of the search path. From another namespace, use `web.app` or the FQDN. A headless Service returns one A/AAAA record per ready Pod (and SRV records for named ports). `ExternalName` returns a CNAME.

---

## 3. CoreDNS

CoreDNS runs as a Deployment in `kube-system` behind the `kube-dns` Service (usually `10.96.0.10`). The kubelet writes `/etc/resolv.conf` in each Pod:

```text
nameserver 10.96.0.10
search app.svc.cluster.local svc.cluster.local cluster.local
options ndots:5
```

`ndots:5` means a name with fewer than 5 dots is tried as a relative name through the search list before being tried as an absolute name. `api.example.com` has 2 dots, so the resolver first tries `api.example.com.app.svc.cluster.local`, then `api.example.com.svc.cluster.local`, then `api.example.com.cluster.local`, then `api.example.com.`. This is why external DNS lookups can be slow or noisy; `ndots:2` or an FQDN with a trailing dot avoids it.

```bash
kubectl -n kube-system get deploy,svc coredns kube-dns
kubectl -n kube-system logs deploy/coredns --tail=50
kubectl -n kube-system get configmap coredns -o yaml
```

Corefile essentials:

```text
.:53 {
    errors
    health
    ready
    kubernetes cluster.local in-addr.arpa ip6.arpa {
        pods insecure
        fallthrough in-addr.arpa ip6.arpa
    }
    prometheus :9153
    forward . /etc/resolv.conf
    cache 30
    loop
    reload
    loadbalance
}
```

Failure modes:

- CoreDNS Pods `CrashLoopBackOff`: bad Corefile, `loop` detected, missing upstream, port conflict.
- CoreDNS running but no endpoints: the `kube-dns` Service selector/labels were changed.
- NXDOMAIN for a Service: wrong name/namespace, headless Service with no ready Pods, or the Service does not exist.
- Timeouts for external names: upstream resolver unreachable, NetworkPolicy blocking egress to UDP/TCP 53, or `ndots` amplification.
- Intermittent SERVFAIL: one CoreDNS replica has a bad upstream or is CPU-throttled; check per-Pod logs and endpoints.

Test from a Pod:

```bash
kubectl run dnstest --rm -it --restart=Never --image=busybox:1.36 -- sh
# inside:
nslookup web.app.svc.cluster.local
nslookup web.app
getent hosts web.app
cat /etc/resolv.conf
```

`nslookup` bypasses NSS; `getent hosts` uses the same path as the application. If `getent` fails but `nslookup` works, suspect `/etc/hosts`, `nsswitch.conf`, or the resolver library, not DNS itself.

---

## 4. Ingress and Gateway API

A Service is L4 (IP/port). An Ingress is L7 HTTP routing: host, path, TLS, redirects, headers.

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: web
spec:
  ingressClassName: nginx
  tls:
    - hosts: ["app.example.com"]
      secretName: app-tls
  rules:
    - host: app.example.com
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: web
                port:
                  number: 80
```

The Ingress object does nothing without an ingress controller (ingress-nginx, Traefik, HAProxy, cloud LB controller). The controller watches Ingress objects, configures its proxy, and routes to Service endpoints.

```bash
kubectl get ingressclass
kubectl get ingress
kubectl describe ingress web
kubectl -n ingress-nginx logs deploy/ingress-nginx-controller --tail=50
```

Common failures:

| Symptom | Likely cause |
|---|---|
| 404 from the controller | Host/path mismatch, wrong `ingressClassName` |
| 502 | Backend connection refused, wrong `targetPort`, Pod not listening |
| 503 | No ready endpoints, Service selector mismatch |
| 504 | Backend timeout, proxy-read-timeout too low |
| TLS error | Secret missing/wrong namespace, certificate does not match host, chain incomplete |
| Redirect loop | `X-Forwarded-Proto` not honored, TLS terminated at LB but app redirects to HTTP |

Gateway API (`GatewayClass`, `Gateway`, `HTTPRoute`) is the successor model; it separates infrastructure (Gateway) from application routing. Learn Ingress first; concepts transfer.

---

## 5. NetworkPolicy

A NetworkPolicy selects Pods and defines allowed ingress/egress. Policies are **additive**: if any policy selects a Pod for a direction, the Pod becomes default-deny for that direction and only the union of allowed rules is permitted. There is no explicit deny rule and no ordering.

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: web-allow
  namespace: app
spec:
  podSelector:
    matchLabels:
      app: web
  policyTypes: ["Ingress", "Egress"]
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: ingress-nginx
        - podSelector:
            matchLabels:
              app: frontend
      ports:
        - protocol: TCP
          port: 8080
  egress:
    - to:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: kube-system
      ports:
        - protocol: UDP
          port: 53
        - protocol: TCP
          port: 53
    - to:
        - podSelector:
            matchLabels:
              app: db
      ports:
        - protocol: TCP
          port: 5432
```

**Critical:** a default-deny egress policy also blocks DNS. Always allow UDP/TCP 53 to `kube-system` (or the cluster DNS Service IP) before debugging "DNS is broken".

NetworkPolicy is enforced by the CNI. If the CNI does not support it (or the policy controller is not running), the object exists but does nothing. Verify with the CNI's tooling:

```bash
kubectl get networkpolicy -n app
kubectl describe networkpolicy web-allow -n app
# Cilium:
kubectl -n kube-system exec ds/cilium -- cilium monitor --type drop
# Calico:
calicoctl get networkpolicy -n app -o yaml
```

---

## 6. The packet walk

### Pod A → Service `web` (ClusterIP) → Pod B

```text
1. Pod A resolves web.app.svc.cluster.local → 10.96.142.31 (CoreDNS).
2. Pod A sends to 10.96.142.31:80. Its route sends the packet to the node
   (the Pod's default gateway is the node's bridge/gateway).
3. The node's nat PREROUTING hits KUBE-SERVICES and DNATs to a chosen
   endpoint, e.g. 10.244.2.7:8080.
4. The node routes/overlays the packet to the node hosting Pod B.
5. Pod B receives it with source 10.244.1.5:ephemeral, destination 10.244.2.7:8080.
6. Return traffic from Pod B goes to 10.244.1.5; conntrack on the first node
   reverses the DNAT so Pod A sees the reply from 10.96.142.31:80.
```

Key points:

- The destination IP in the packet is rewritten; the application sees the Pod IP/port, not the ClusterIP.
- The source IP seen by the backend is the client Pod IP, unless NAT/masquerade or a proxy changes it.
- Conntrack is per-connection; a new connection can choose a different endpoint.
- If the selected endpoint dies, existing connections may hang until timeout; new connections go to healthy endpoints.

### Pod → external IP

```text
Pod A → node → iptables MASQUERADE (if the CNI/runtime does it) → underlay → Internet
```

The reply returns to the node's public IP, then conntrack reverses the NAT. If the node has no route or the cloud NAT gateway is misconfigured, Pods have no egress.

### External → NodePort / LoadBalancer

```text
client → nodeIP:30080 → KUBE-NODEPORTS DNAT → Service → endpoint Pod
```

With `externalTrafficPolicy: Cluster`, the packet may be forwarded to another node, adding a hop and SNAT. With `Local`, only nodes with a local endpoint accept, preserving the source IP.

### Hairpin

A Pod calling its own Service IP is a hairpin connection. It needs the bridge/CNI to support hairpin mode and the NAT rules to handle the source/destination overlap. Symptoms: connection refused/timeout only when a Pod calls its own Service; direct Pod IP works.

---

## 7. Debugging ladder

Work from the name to the packet, not from the YAML to a guess.

```text
1. Does the Service exist and have the right selector/port?
2. Does the EndpointSlice contain ready Pod IPs on the right port?
3. Is the target Pod Ready? Is the app listening on that port and address?
4. Does DNS resolve the name to the ClusterIP?
5. Does a connection to the ClusterIP work from a test Pod?
6. Does a connection to the Pod IP work? If yes, the Service path is the problem.
7. Is a NetworkPolicy/CNI dropping the packet?
8. Is the packet leaving the node / arriving at the other node (tcpdump)?
9. Is conntrack/MTU/NAT corrupting it?
```

Commands:

```bash
kubectl get svc,endpoints,endpointslices -o wide
kubectl get pods --show-labels -o wide
kubectl describe svc web
kubectl describe pod web-abc
kubectl run nettest --rm -it --restart=Never --image=nicolaka/netshoot -- bash
# inside:
getent hosts web
curl -v --max-time 5 http://web.app.svc.cluster.local
nc -vz web.app 80
curl -v --max-time 5 http://10.244.2.7:8080
ip route
cat /etc/resolv.conf
```

If the ClusterIP fails but the Pod IP works, the Service/kube-proxy path is suspect. If both fail but DNS resolves, the network/policy path is suspect. If DNS fails, check CoreDNS and the resolver config.

Node-level:

```bash
sudo iptables-save -t nat | grep -E 'KUBE|10.96.142.31'
sudo ipvsadm -Ln
sudo conntrack -L | grep 10.96.142.31
sudo tcpdump -ni any host 10.244.2.7 and port 8080
sudo crictl pods
```

For a Pod's network namespace:

```bash
PID=$(sudo crictl inspectp <pod-id> | jq -r .info.pid)
sudo nsenter -t "$PID" -n ip addr
sudo nsenter -t "$PID" -n ip route
sudo nsenter -t "$PID" -n ss -lntp
```

---

## 8. Failure scenarios

### A — `Connection refused` to a ClusterIP

Fault domain: endpoint/app, not DNS. Check endpoints; if empty, selector/readiness. If present, `kubectl exec` to the Pod IP and `ss -lntp`; the app may be bound to `127.0.0.1` or the wrong port. `targetPort` mismatch is common.

### B — Timeout to a ClusterIP

Fault domain: packet drop (NetworkPolicy, CNI, firewall, wrong endpoint IP) or a black hole. Check endpoints, policy, `tcpdump` on the client and server, conntrack, and cross-node connectivity. A timeout with SYNs leaving but no SYN-ACK points to the server/policy path; no SYNs at the server points to routing/CNI.

### C — NXDOMAIN

Fault domain: DNS name/namespace or CoreDNS. Test the FQDN, check the Service exists in the expected namespace, check headless endpoints, CoreDNS logs, and the Pod's `/etc/resolv.conf`. A typo in the namespace is more common than a CoreDNS bug.

### D — DNS resolves but connection fails

Fault domain: Service/endpoint/policy/app. DNS only proves the name maps to an IP. Follow the packet walk from the ClusterIP.

### E — NodePort unreachable from outside

Fault domain: node firewall/security group, kube-proxy, `externalTrafficPolicy`, or the Service. Check `ss -lntp` on the node (kube-proxy does not listen; iptables does), cloud security group, `iptables -t nat -L KUBE-NODEPORTS`, and whether the node has a local endpoint when `Local` is set.

### F — Ingress 502/503/504

Fault domain: controller → Service → Pod. 502 = connection refused/bad response; 503 = no endpoints; 504 = timeout. `kubectl describe ingress`, controller logs, endpoints, and a direct `curl` to the Service from a debug Pod.

### G — Cross-node Pod traffic fails

Fault domain: CNI/routing/overlay/MTU. Same-node works, cross-node does not. Check node routes (`ip route`), tunnel interfaces (`ip -d link`), CNI logs, cloud route tables/security groups, and MTU with `ping -M do`.

### H — Intermittent failures (one of N requests)

Fault domain: one bad endpoint, readiness flapping, conntrack, or asymmetric capacity. Check per-endpoint success (`curl` each Pod IP), endpoint readiness, Pod restarts, and node load. A Service with three endpoints and one broken Pod gives roughly 33% failures.

### I — CoreDNS CrashLoopBackOff

Fault domain: CoreDNS config/upstream/resources. `kubectl logs -n kube-system deploy/coredns --previous`, `kubectl describe`, check the Corefile for a `loop`/`forward` mistake, upstream reachability, and CPU limits. Do not scale CoreDNS to zero while debugging.

### J — External DNS slow but internal fast

Fault domain: `ndots`/search path/upstream. Use FQDNs with a trailing dot, reduce `ndots` via Pod `dnsConfig`, or fix the upstream resolver. Capture with `tcpdump port 53` to see the repeated search-list queries.

---

## 9. Command field manual

### `kubectl get endpointslices`

**Purpose:** see the actual backends of a Service.

```bash
kubectl get endpointslices -l kubernetes.io/service-name=web -o wide
kubectl get endpointslice web-abc12 -o yaml
```

**Interpretation:** `endpoints[].conditions.ready` tells you whether the Pod is included. A slice with `ports` but no endpoints means no ready Pods; a missing slice means the selector matches nothing or the controller is down.

**Mistake:** looking only at `kubectl get endpoints` (legacy) and missing the per-slice conditions.

### `kubectl run` — disposable test Pod

```bash
kubectl run nettest --rm -it --restart=Never --image=nicolaka/netshoot -- bash
```

**Why:** tests DNS/HTTP/TCP from inside the cluster without modifying the app. `--rm` cleans up; `--restart=Never` avoids a Deployment.

**Mistake:** testing from a Pod in a different namespace with a different NetworkPolicy; always state the namespace and labels.

### `kubectl port-forward`

```bash
kubectl port-forward svc/web 8080:80
```

**Why:** bypasses Ingress/NodePort and connects to a Pod through the API server. If port-forward works but the Service does not, the problem is in the Service/network path, not the app.

**Limitation:** it does not test the Service's load balancing or NetworkPolicy; it uses the API server's tunnel.

### `kubectl exec` + `getent`/`curl`/`nc`

```bash
kubectl exec -it web-abc -- getent hosts web
kubectl exec -it web-abc -- curl -v --max-time 5 http://web
kubectl exec -it web-abc -- nc -vz web 80
```

**Interpretation:** `getent` uses NSS (the app path); `curl` shows connect vs HTTP vs TLS; `nc` isolates TCP. Use `--max-time` so a black hole does not hang your session.

### `tcpdump` in a Pod namespace

```bash
PID=$(sudo crictl inspectp <pod-id> | jq -r .info.pid)
sudo nsenter -t "$PID" -n tcpdump -ni any -c 20
```

**Why:** proves whether packets leave/arrive and whether the source/destination are what you expect. Capture on both ends before concluding "the network drops it".

### `conntrack`

```bash
sudo conntrack -L -p tcp --dport 8080
sudo conntrack -S
```

**Interpretation:** shows the NAT mapping and state. `conntrack -S` shows insert failures/drops; a full table can cause intermittent failures. `conntrack -F` is destructive; never run it on a production node.

### `kubectl debug`

```bash
kubectl debug -it web-abc --image=nicolaka/netshoot --target=app
kubectl debug node/node1 -it --image=busybox
```

**Why:** distroless images lack tools; `--target` shares the target container's process namespace so you can inspect it. Node debug creates a privileged Pod with the host filesystem at `/host`.

---

## 10. Checkpoint

1. What are the three guarantees of the Kubernetes Pod network model?
2. What does a CNI plugin do, and who calls it?
3. Why does a VXLAN overlay reduce the Pod MTU?
4. What is the difference between a ClusterIP and a headless Service?
5. What is an EndpointSlice, and who writes it?
6. What does kube-proxy do in iptables mode?
7. Why does a Service with no endpoints return connection refused/timeout instead of a DNS error?
8. What does `ndots:5` do to an external lookup?
9. Why does a default-deny egress policy break DNS?
10. Is a NetworkPolicy enforced by kube-proxy? Explain.
11. What is the difference between `externalTrafficPolicy: Cluster` and `Local`?
12. Why can a rolling update cause a few stale connections?
13. What does a 503 from an ingress controller usually mean?
14. How do you prove the problem is DNS rather than the Service?
15. What command shows the actual Pod IPs behind a Service?
16. Why does `kubectl port-forward` working not prove the Service works?
17. What is a hairpin connection?
18. What is the first thing to check when same-node Pod traffic works but cross-node fails?
19. Why can `nslookup` succeed while the application cannot resolve a name?
20. What evidence would you collect for a Service that works 2 out of 3 times?

### Answers

1. Every Pod has a unique IP; any Pod can reach any Pod IP without NAT; nodes can reach Pod IPs and vice versa.
2. It configures the Pod's network namespace (veth, IP, routes, policy) according to the CNI spec; the container runtime calls it during sandbox creation.
3. VXLAN adds outer Ethernet/IP/UDP/VXLAN headers (about 50 bytes), so the inner Pod MTU must be smaller to avoid fragmentation.
4. ClusterIP has a virtual IP and kube-proxy load-balances; headless has no virtual IP, and DNS returns the Pod IPs for client-side balancing.
5. A slice of Service endpoints with readiness conditions and ports; the EndpointSlice controller writes it from the Service selector and Pod readiness.
6. It watches Services/EndpointSlices and programs NAT rules (DNAT to endpoints) plus conntrack; it does not proxy packets in the data path itself.
7. DNS resolves the Service name to the ClusterIP regardless of endpoints; the connection then fails because there is no backend to DNAT to.
8. Names with fewer than 5 dots are tried with each search domain first, causing extra queries and latency before the absolute name.
9. The policy selects the Pod for egress and allows only the listed destinations; DNS to `kube-system`/the DNS Service is not allowed unless explicitly added.
10. No; kube-proxy implements Services, not policy. The CNI enforces NetworkPolicy; without CNI support, the policy is inert.
11. `Cluster` may forward to a node without a local endpoint and SNAT; `Local` only uses local endpoints and preserves the source IP, but can leave nodes idle.
12. Existing conntrack entries keep the old endpoint mapping until they expire; new connections use the new endpoint set.
13. The ingress controller has no ready backend endpoints for the matched Service (or the Service/port is wrong).
14. Resolve the name from a test Pod (`getent`/`nslookup`), then connect to the resolved IP directly; if DNS returns the right IP but the connection fails, it is not a DNS problem.
15. `kubectl get endpointslices -l kubernetes.io/service-name=<svc> -o wide` (or `kubectl get endpoints`).
16. Port-forward goes through the API server directly to a Pod, bypassing the Service's virtual IP, kube-proxy rules, and often NetworkPolicy.
17. A Pod connecting to a Service that routes back to itself; it requires hairpin NAT/mode support and can fail even when other Pods work.
18. The CNI/tunnel/routes between nodes, the Pod CIDRs, and the MTU. Capture on both nodes.
19. `nslookup` queries DNS directly; the application uses NSS (`/etc/nsswitch.conf`, `/etc/hosts`, `systemd-resolved`, or a different resolver). Check `getent hosts`.
20. Per-endpoint success, EndpointSlice readiness, Pod restarts, node load, and conntrack. Test each Pod IP directly to find the bad backend.
