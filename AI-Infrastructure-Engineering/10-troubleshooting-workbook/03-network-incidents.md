# Network Incidents (INC-05 – INC-07)

> **Evidence first.** Same method as the Linux incidents: write your timeline, hypotheses, and discriminating test before the analysis marker. For network incidents, the stack discipline matters — check L3 (routes/ARP), L4 (TCP state), and L7 (application) in that order, and always say what a test **would have shown if the hypothesis were wrong**.

---

## INC-05 — Reachable from one subnet, not another

### The page

> **09:30 SEV2** — A new service `svc-b` (10.20.5.11, port 8080) is unreachable from `app-a` (10.10.2.31). It is reachable from `app-c` (10.20.5.44) and from a jump host (10.10.9.9). No code changes; `svc-b` was deployed to a new subnet yesterday.

### The evidence (raw, as collected)

From `app-a`:

```bash
ip addr show eth0 | grep inet
ip route get 10.20.5.11
curl -m 5 -v http://10.20.5.11:8080/health 2>&1 | tail -4
ping -c 3 -W 2 10.20.5.11
```

```text
$ ip route get 10.20.5.11
10.20.5.11 via 10.10.2.1 dev eth0 src 10.10.2.31

$ curl -m 5 -v http://10.20.5.11:8080/health
* Trying 10.20.5.11:8080...
* connect to 10.20.5.11 port 8080 failed: Connection timed out

$ ping -c 3 -W 2 10.20.5.11
--- 10.20.5.11 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss
```

From `svc-b` itself, while `app-a` runs `curl` in a loop:

```bash
tcpdump -ni any host 10.10.2.31 and port 8080 -c 20
ip route get 10.10.2.31
cat /proc/sys/net/ipv4/conf/all/rp_filter /proc/sys/net/ipv4/conf/eth0/rp_filter
iptables -L INPUT -n -v | head
```

```text
$ tcpdump -ni any host 10.10.2.31 and port 8080 -c 20
09:31:02.114  IP 10.10.2.31.41220 > 10.20.5.11.8080: Flags [S], seq 812345678
09:31:02.114  IP 10.20.5.11.8080 > 10.10.2.31.41220: Flags [S.], seq ..., ack ...
09:31:02.118  IP 10.10.2.31.41220 > 10.20.5.11.8080: Flags [.], ack ...
... (handshake completes; then data flows normally during the manual test)

$ ip route get 10.10.2.31
10.10.2.31 via 10.20.5.1 dev eth0 src 10.20.5.11

$ cat /proc/sys/net/ipv4/conf/all/rp_filter /proc/sys/net/ipv4/conf/eth0/rp_filter
2
2
```

The curl from `app-a` **times out at connect**, but tcpdump during that same window shows **nothing arriving** (the capture above is from the manual connectivity test, not the failing one). The failing-window capture shows zero packets.

### Your turn (before the line)

- [ ] Timeline.
- [ ] Three hypotheses ranked with evidence.
- [ ] The one discriminating test.
- [ ] First 10 minutes.

---
---

# ANALYSIS — INC-05

### Timeline

`svc-b` was deployed to a new subnet (10.20.5.0/24) yesterday. Today, `app-a` (10.10.x) cannot reach it; hosts inside 10.20.x can, and one 10.10.9.x host can. The failure began with the subnet move, which is the strongest "what changed" signal.

### Hypotheses ranked

1. **A firewall/ACL between subnets blocks app-a → svc-b.** TCP connect times out with **no packets arriving** at svc-b (confirmed by tcpdump in the failing window) while ping works. Ping ICMP may be permitted by an ACL that blocks TCP 8080. The 10.10.9.9 jump host working suggests a rule that allows a specific CIDR/pair rather than the whole 10.10.0.0/16.
2. **Asymmetric routing / rp_filter drop.** `rp_filter=2` (loose) makes this less likely, and the manual test shows the return path works from 10.20.5.x. Possible but weakly supported.
3. **ARP/L2 issue on svc-b.** Rejected: svc-b is reachable from its own subnet, and ping from app-a reaches it.

### The discriminating test

`tcpdump` on svc-b **during a failing curl**: zero packets arriving proves the block is upstream (network/firewall), not at svc-b. If packets arrived but no SYN-ACK left, the problem would be local (iptables/conntrack/app). That single test splits the path in half.

### Root cause

A network ACL (cloud security group or a hardware firewall between subnets) permitted `10.10.9.0/24 → 10.20.5.0/24` (the jump-host CIDR) and intra-subnet traffic, but had no rule for `10.10.2.0/24 → 10.20.5.0/24 TCP 8080`. ICMP was allowed by a separate blanket rule, which is why ping worked while TCP failed — a trap that costs people hours.

### Fix (smallest safe first)

1. Add the minimal rule: allow `10.10.2.0/24 → 10.20.5.11:8080` (source-constrained, not 0.0.0.0/0).
2. Verify with `curl` from app-a and confirm the SYN-ACK in tcpdump.
3. Audit the ACL for the other subnets that will need this service; add them deliberately, not by opening the whole range.

### Prevention

- Treat "ping works" as **not** a connectivity test for TCP services; use the actual protocol.
- When moving a service to a new subnet, run a connectivity matrix test from every caller subnet as part of the deploy.
- Alert on connection timeouts from known callers (synthetic probe per caller subnet).
- Document ACLs per subnet pair; a one-page matrix beats tribal memory.

### Lessons

- **A timeout is a silent drop; a refused connection is an active reject.** `connection refused` means something answered with RST (service down or a reject rule); a timeout means packets are disappearing (filter/drop rule, wrong route, or no listener on the wire path).
- **ICMP and TCP are filtered independently.** A passing ping proves almost nothing about a TCP service.
- **tcpdump at both ends splits the search space in half.** Packet present at the receiver → problem is local. Absent → problem is upstream.

### 5-minute walkthrough script

> "A new service in a different subnet was unreachable from one caller but fine from two others. Ping worked, TCP timed out. tcpdump on the service during a failing curl showed zero packets arriving — that put the block upstream, not at the host. The cause was a subnet ACL that allowed the jump-host CIDR and intra-subnet traffic but had no rule for the caller's subnet; ICMP was allowed by a separate rule, which is why ping passed. The fix was a source-constrained allow rule for that CIDR and port, verified with curl and a SYN-ACK in tcpdump. The lesson: ping is not a TCP connectivity test, and a timeout is a silent drop — capture on both ends to split the path."

---

## INC-06 — 1% of requests time out, randomly, after a network change

### The page

> **14:20 SEV2** — Since the weekend network change, the payment API shows p99 timeout rate of ~1% from `app-a` to `db`. p50/p95 unchanged. No errors in the app logs other than the timeouts. The DB is fine.

### The evidence (raw, as collected)

```bash
ss -ti dst 10.30.4.7:5432 | head -20
sar -n ETCP,DEV 1 5
ethtool -k eth0 | grep -E 'tcp-segmentation|generic-segmentation|generic-receive'
ip route get 10.30.4.7
tracepath -n 10.30.4.7 | head
ping -M do -s 1472 -c 3 10.30.4.7
ping -M do -s 1400 -c 3 10.30.4.7
```

```text
$ ss -ti dst 10.30.4.7:5432
... retrans:214/2148RTT... backoff... unacked...
  (several connections show retransmissions; some show 'timer:persist' and 'busy')

$ ip route get 10.30.4.7
10.30.4.7 via 10.10.2.1 dev eth0 src 10.10.2.31 mtu 1500

$ tracepath -n 10.30.4.7 | head
 1:  10.10.2.31                0.2ms
 2:  10.10.2.1                 0.4ms
 3:  10.30.4.7   asymm  2      0.9ms pmtu 1500
 ...
 (note: 'pmtu 1500' — no ICMP Frag Needed observed anywhere)

$ ping -M do -s 1472 -c 3 10.30.4.7     # 1472+28 = 1500 = exact MTU
3 packets transmitted, 0 received, 100% packet loss

$ ping -M do -s 1400 -c 3 10.30.4.7     # 1428 < 1500
3 packets transmitted, 3 received, 0% packet loss

$ ping -M do -s 1452 -c 3 10.30.4.7     # a slightly smaller probe
3 packets transmitted, 0 received, 100% packet loss
```

A weekend change note: a new tunnel/wan optimization appliance was inserted between two data centers; its interfaces are configured with **MTU 1400** internally, and it silently drops oversized frames instead of sending ICMP Frag Needed.

### Your turn (before the line)

- [ ] Timeline.
- [ ] Three hypotheses ranked with evidence.
- [ ] The one discriminating test.
- [ ] First 10 minutes.

---
---

# ANALYSIS — INC-06

### Timeline

The weekend change inserted a WAN/tunnel appliance with an effective MTU of 1400 between the app and DB segments. The appliance does not emit `ICMP Frag Needed`, so PMTUD cannot discover the smaller path MTU. TCP sessions that send segments larger than 1400 bytes have those segments silently dropped; after ~15 retransmissions (~15 min at default backoff, sooner with `tcp_retries2` tuned) the connection dies or stalls. Only sessions whose MSS negotiation/segmentation produced ≤1400-byte segments (small queries, header-heavy packets) work — hence ~1% of requests, size-dependent and random-looking.

### Hypotheses ranked

1. **PMTUD blackhole / MTU mismatch.** `ping -M do -s 1472` (1500-byte frames) loses 100%; `-s 1400` (1428-byte frames) passes; a probe at 1452 also fails → the path MTU is between 1428 and 1480 bytes. `tracepath` shows no ICMP Frag Needed anywhere. Retransmissions in `ss -ti` confirm in-flight stalls. Matches.
2. **TCP-level congestion/packet loss from the appliance.** Possible in general, but loss would not be so cleanly size-dependent.
3. **DB-side slowness.** Rejected: DB metrics are fine and the failure is connect/data-stall shaped, not query-time shaped.

### The discriminating test

The binary-search `ping -M do -s <size>` sweep. It measures path MTU directly and distinguishes "the path is smaller than the interface" (size-dependent loss) from "the path is uniformly lossy" (size-independent loss). Record where the cutoff sits: the largest passing size + 28 bytes of IP+ICMP headers = the path MTU.

### Root cause

A PMTUD blackhole: the path's effective MTU dropped to 1400 and the intermediate device drops oversized frames without sending the ICMP message that TCP needs to adapt. Classic causes: tunnels (GRE/VXLAN/IPsec), VPN appliances, jumbo-frame misconfigurations, and any middlebox that filters ICMP.

### Fix (smallest safe first)

1. Lower TCP MSS on the affected hosts so segments fit the path:

```bash
# on the host, clamp MSS for that destination
ip route change 10.30.4.0/24 via 10.10.2.1 dev eth0 mtu 1400 advmss 1360
```

   (Or fix the appliance interfaces to a consistent MTU, or enable clamping on the appliance.)
2. Verify: `ss -ti` retransmissions stop; p99 timeouts return to baseline; the size-sweep now passes at the new MSS.
3. Find the actual failing hop with the size sweep from each intermediate host (hop-by-hop) so the appliance is identified, not just worked around.

### Prevention

- Monitor retransmission rate (`ss -ti` retrans counters, `sar -n ETCP`) and alert on a rise — it is the earliest signature of an MTU/loss problem.
- After any network change, run a path-MTU sweep between critical pairs as part of the change's validation.
- Never filter ICMP Frag Needed (type 3 code 4) on paths that carry TCP; if you must filter ICMP, allow this one.
- Prefer MSS clamping on VPN/tunnel endpoints over relying on PMTUD across untrusted intermediaries.

### Lessons

- **MTU problems look like random slowness.** The size-dependence (large frames fail, small pass) is the tell; a binary-search `ping -M do` sweep proves it in a minute.
- **PMTUD is only as good as the ICMP path.** A middlebox that drops Frag Needed creates a blackhole that no amount of TCP tuning fixes.
- **Retransmission counters are the earliest symptom.** By the time users see timeouts, TCP has already been suffering for minutes.

### 5-minute walkthrough script

> "After a weekend network change, about 1% of database requests started timing out with no change in p50/p95. The app-level symptom was too vague, so I went to the transport layer: `ss -ti` showed retransmissions and persist timers on the affected connections. The size-dependent test clinched it: a `ping -M do` sweep lost 100% of 1500-byte frames but passed 1428-byte ones — the path MTU had dropped to about 1400. The weekend change had inserted a tunnel appliance with a 1400 MTU that silently drops oversized frames instead of sending ICMP Frag Needed, so PMTUD couldn't discover it. Mitigation was MSS clamping on the route; the real fix was the appliance MTU. Prevention: retransmission-rate alerting and an MTU sweep as part of every network change. The lesson: size-dependent loss is MTU, and PMTUD is only as good as the ICMP path."

---

## INC-07 — TLS works in browsers, fails in `curl` and one service

### The page

> **10:02 SEV2** — After a certificate renewal on `api.example.com`, the web app works, `curl https://api.example.com` fails with `unable to get local issuer certificate`, and one internal service's TLS client fails the same way. Expiry date is fine; hostname matches.

### The evidence (raw, as collected)

```bash
echo | openssl s_client -connect api.example.com:443 -servername api.example.com 2>/dev/null | head -30
curl -v https://api.example.com/health 2>&1 | grep -E 'subject|issuer|SSL|certificate|error' | head
curl --cacert /etc/ssl/certs/ca-certificates.crt -v https://api.example.com/health 2>&1 | tail -3
```

```text
$ openssl s_client ... | head -30
---
Certificate chain
 0 s:CN = api.example.com
   i:C=US, O=Let's Encrypt, CN=R11          <-- issuer, but the R11 cert itself is NOT in the chain
--- (chain stops after the leaf; no "1 s:" line follows)

Verify return code: 21 (unable to verify the first certificate)

$ curl -v ...
* issuer: C=US; O=Let's Encrypt; CN=R11
* SSL certificate problem: unable to get local issuer certificate
```

The browser works because it fetched the intermediate (R11) on demand (AIA fetching) or had it cached; `curl` and strict clients (Go's crypto/tls, Java's default trust manager, most SDKs) require the **server to send the full chain**.

### Your turn (before the line)

- [ ] Timeline.
- [ ] Three hypotheses ranked with evidence.
- [ ] The one discriminating test.
- [ ] First 10 minutes.

---
---

# ANALYSIS — INC-07

### Timeline

The certificate was renewed and deployed with **only the leaf certificate** installed, not the full chain (leaf + intermediate). The old (valid) certificate had the chain; the new deployment script copied only the `.crt` for the leaf. Browsers tolerate this; strict clients do not.

### Hypotheses ranked

1. **Missing intermediate in the served chain.** `openssl s_client` shows the chain contains only the leaf; `Verify return code: 21 (unable to verify the first certificate)`. The issuer name is present but the issuer certificate is absent. Matches exactly.
2. **Expired/untrusted root.** Rejected: the issuer (`R11`) chains to ISRG Root X1, present in modern trust stores; and the browser (same root store) validates fine.
3. **Hostname mismatch.** Rejected: `s` line shows `CN = api.example.com` matching, and the error is about the issuer, not the name.

### The discriminating test

`openssl s_client -showcerts` and count the chain entries. If the server sends only one certificate, the chain is incomplete. (`Verify return code: 21` is the leaf-not-chained signature; code 20 would be a root-not-trusted problem — different cause.)

### Root cause

The deployment installed the leaf only. TLS servers must send the full chain (leaf + intermediates); the root stays in the client's trust store. Browser tolerance (AIA fetching/caching) masked the fault for most users while strict clients failed.

### Fix (smallest safe first)

1. Install the full chain on the server/load balancer (leaf first, then intermediate). For nginx: `ssl_certificate /path/fullchain.pem;`. For a cloud LB, upload the chain in the certificate bundle.
2. Verify: `openssl s_client -showcerts | grep -c 'BEGIN CERT'` shows 2 (or more), and `Verify return code: 0 (ok)`.
3. Run the same check from a machine **without** the browser cache (a CI runner or container) to be sure.

### Prevention

- Validate certificates with a strict client (curl/openssl) in the deploy pipeline, not just a browser.
- Automate expiry **and chain-completeness** monitoring (`openssl s_client` from a canary host).
- Keep the renewal script producing `fullchain.pem` and assert its certificate count before deploy.
- Use a synthetic external probe with the same TLS stack as production clients.

### Lessons

- **The server sends the chain; the client sends the root.** Browsers hide this requirement; strict clients enforce it.
- **`openssl s_client -showcerts` is the diagnostic.** Count the certificates in the served chain; one leaf alone is the bug.
- **Verify return codes are diagnostic:** 21 = chain incomplete (issuer missing), 20 = root untrusted, 10 = expired, 66/67 = hostname. Read the code, don't guess.

### 5-minute walkthrough script

> "After a certificate renewal, browsers worked but curl and an internal service failed with 'unable to get local issuer certificate'. `openssl s_client -showcerts` showed the server sending only the leaf certificate — the intermediate was missing, which openssl flags as verify code 21. Browsers had fetched or cached the intermediate, which masked the fault. The fix was installing the full chain (leaf + intermediate) on the load balancer, verified by a chain count and verify code 0 from a cache-free client. Prevention: strict-client certificate validation in the deploy pipeline and chain-completeness monitoring alongside expiry. The lesson: TLS servers must send the full chain, and browsers are the wrong canary for that."
