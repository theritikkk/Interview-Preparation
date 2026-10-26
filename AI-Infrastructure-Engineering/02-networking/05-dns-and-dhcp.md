# DNS and DHCP

> **The one-sentence model:** DNS turns names into records by walking a delegation tree (or asking someone who already did); DHCP gives a host an address, a gateway, and DNS servers before it can use any of them. Both fail in ways that look like "the network is down".

---

## 1. Why DNS is a distributed database

No single server knows every name. Responsibility is delegated:

```text
.                         root servers (13 named, hundreds of anycast instances)
└── com.                  TLD servers
    └── example.com.      authoritative servers for the zone
        ├── www.example.com.  A 203.0.113.10
        └── api.example.com.  A 203.0.113.20
```

Each zone has authoritative nameservers listed in `NS` records. The root only knows where `com.` lives; `com.` only knows where `example.com.` lives; the `example.com.` servers know the actual records.

### Recursive vs iterative

| Mode | Who | Behaviour |
|---|---|---|
| Recursive | Stub resolver → recursive resolver | "Give me the final answer; do the work" |
| Iterative | Recursive resolver → root/TLD/authoritative | "Point me to the next server" |

Your laptop runs a **stub resolver** (libc, systemd-resolved, dnsmasq). It sends a recursive query (RD=1) to a configured resolver. That resolver walks the tree iteratively, caches the result, and returns it.

```text
App → stub resolver → recursive resolver → root → com → example.com
                                  ↑
                              cache (TTL)
```

The recursive resolver is usually your ISP, corporate DNS, or a public resolver (1.1.1.1, 8.8.8.8). In Kubernetes it is CoreDNS. In a VPC it is the provider's `.2` resolver (AWS) or `168.63.129.16` (Azure).

### Caching and TTL

Every record has a TTL in seconds. The recursive resolver caches it for that long. Negative answers are cached too, for the SOA minimum (or the negative TTL).

Consequences:

- A DNS change is not visible everywhere immediately; wait for TTL.
- Lowering TTL before a migration is a standard preparation step.
- `dig` may return a cached answer from a resolver you did not expect.

---

## 2. Record types you must know

| Type | Purpose | Example |
|---|---|---|
| `A` | IPv4 address | `www.example.com. 300 IN A 203.0.113.10` |
| `AAAA` | IPv6 address | `www.example.com. 300 IN AAAA 2001:db8::10` |
| `CNAME` | Alias to another name | `www 300 IN CNAME web.example.com.` |
| `MX` | Mail exchanger + priority | `example.com. 3600 IN MX 10 mail.example.com.` |
| `TXT` | Arbitrary text | SPF, DKIM, domain verification |
| `NS` | Authoritative nameserver | `example.com. 86400 IN NS ns1.example.com.` |
| `SOA` | Zone authority, serial, timers | See below |
| `PTR` | Reverse lookup (IP → name) | `10.2.0.192.in-addr.arpa. PTR host.example.com.` |
| `SRV` | Service location (host + port) | `_https._tcp.example.com. SRV 10 5 443 web.example.com.` |
| `CAA` | Which CAs may issue certs | `example.com. CAA 0 issue "letsencrypt.org"` |
| `NAPTR` | Regex-based service mapping | Rare outside telecom/SIP |

### SOA

```text
example.com.  3600  IN  SOA  ns1.example.com. admin.example.com. (
              2024010101 ; serial
              7200       ; refresh
              3600       ; retry
              1209600    ; expire
              300 )      ; minimum / negative TTL
```

- **Serial** increases on every zone change; secondaries use it to decide whether to transfer.
- **Refresh/retry/expire** govern secondary refresh.
- **Minimum** is the negative-cache TTL (and historically the default TTL).

### CNAME rules

- A CNAME cannot coexist with other records for the same name.
- A CNAME cannot exist at the zone apex (`example.com` itself) because the apex must have SOA/NS.
- Chains are allowed but add latency; loops are fatal.

### SRV

```text
_https._tcp.example.com. 300 IN SRV 10 60 443 web1.example.com.
```

Format: `priority weight port target`. Lower priority is preferred; weight distributes load among equal priorities. Kubernetes headless services and many service-discovery systems use SRV.

---

## 3. The resolver stack on Linux

```text
application
   │ getaddrinfo()
   ▼
NSS (nsswitch.conf)  → files (/etc/hosts), dns, mdns, ...
   │
   ▼
stub resolver (/etc/resolv.conf or systemd-resolved)
   │
   ▼
recursive resolver
```

### `/etc/nsswitch.conf`

```text
hosts: files dns
```

Order matters. `files` first means `/etc/hosts` overrides DNS. `dns` means query the resolver. `myhostname`, `mdns`, `resolve` may appear depending on the distro.

### `/etc/resolv.conf`

```text
nameserver 127.0.0.53
search lab.local example.com
options ndots:5 timeout:2 attempts:3
```

| Directive | Meaning |
|---|---|
| `nameserver` | Resolver address; up to 3 are used |
| `search` | Append these domains to short names |
| `domain` | Single search domain (older) |
| `ndots:n` | If a name has at least n dots, try it as absolute first |
| `timeout:n` | Per-query timeout in seconds |
| `attempts:n` | Retries per nameserver |
| `rotate` | Round-robin nameservers |

`ndots:5` is common in Kubernetes and causes surprising latency: `api` (0 dots) is tried as `api.<ns>.svc.cluster.local`, `api.svc.cluster.local`, ..., before `api.` — five queries. Set `ndots:2` or use FQDN with a trailing dot.

On systemd-resolved systems, `/etc/resolv.conf` is a symlink to `/run/systemd/resolve/stub-resolv.conf` and `127.0.0.53` is a local stub. Do not edit that file expecting it to persist; use `resolvectl` or the network config.

```bash
ls -l /etc/resolv.conf
resolvectl status
```

```text
Global
       Protocols: -LLMNR -mDNS -DNSOverTLS DNSSEC=no/unsupported
resolv.conf mode: stub

Link 2 (eth0)
    Current Scopes: DNS
         Protocols: +DefaultRoute
Current DNS Server: 192.168.10.53
       DNS Servers: 192.168.10.53 1.1.1.1
        DNS Domain: lab.local
```

### `getent` vs `dig` — a critical distinction

`getent hosts name` uses NSS: `/etc/hosts`, then DNS, then other sources. `dig name` sends a DNS query directly and ignores `/etc/hosts`.

```bash
getent hosts app.lab.local
dig +short app.lab.local
```

If `getent` resolves but `dig` returns NXDOMAIN, the answer came from `/etc/hosts`. If `dig` works but the app fails, NSS may be misconfigured or the app uses a different resolver (container, chroot, static binary with c-ares).

---

## 4. Reading `dig` output

```bash
dig example.com A
```

```text
; <<>> DiG 9.18.24 <<>> example.com A
;; global options: +cmd
;; Got answer:
;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 42133
;; flags: qr rd ra; QUERY: 1, ANSWER: 1, AUTHORITY: 0, ADDITIONAL: 1

;; OPT PSEUDOSECTION:
; EDNS: version: 0, flags:; udp: 1232
;; QUESTION SECTION:
;example.com.			IN	A

;; ANSWER SECTION:
example.com.		300	IN	A	203.0.113.10

;; Query time: 12 msec
;; SERVER: 127.0.0.53#53(127.0.0.53) (UDP)
;; WHEN: Mon Oct 26 22:14:01 UTC 2026
;; MSG SIZE  rcvd: 56
```

| Part | What to read |
|---|---|
| `status:` | `NOERROR`, `NXDOMAIN`, `SERVFAIL`, `REFUSED`, `TIMEOUT` |
| `flags:` | `qr` response, `rd` recursion desired, `ra` recursion available, `aa` authoritative, `tc` truncated |
| `ANSWER SECTION` | The actual records and TTL |
| `AUTHORITY` | NS/SOA for the zone; useful for negative answers |
| `SERVER` | Which resolver answered — often not the one you think |
| `Query time` | Cache hit vs recursive lookup |
| `MSG SIZE` | If large, TCP fallback or EDNS may matter |

### Useful variants

```bash
dig +short example.com
dig +short example.com AAAA
dig example.com MX
dig @1.1.1.1 example.com          # ask a specific resolver
dig @ns1.example.com example.com +norecurse   # ask authoritative, no recursion
dig +trace example.com            # iterative walk from the root
dig -x 203.0.113.10               # reverse lookup
dig +noall +answer example.com    # answers only
dig +dnssec example.com           # request DNSSEC records
```

`+trace` is the best way to see delegation and spot a broken authoritative server. It ignores your configured resolver and queries the root directly.

### Status codes

| Status | Meaning | Likely cause |
|---|---|---|
| `NOERROR` | Query succeeded (answer may be empty) | Normal |
| `NXDOMAIN` | Name does not exist | Typo, deleted record, wrong zone |
| `SERVFAIL` | Resolver failed | Authoritative down, DNSSEC failure, broken zone |
| `REFUSED` | Resolver refuses to answer | ACL, querying an authoritative server for a zone it does not host |
| `FORMERR` | Malformed query | Rare; broken client/EDNS |
| timeout | No response | Firewall, wrong resolver, network |

`NOERROR` with zero answers is not NXDOMAIN. It can mean the name exists but has no record of that type (e.g. AAAA-only host queried for A).

---

## 5. Negative answers and caching

When a name does not exist, the authoritative server returns NXDOMAIN with an SOA in the authority section. The resolver caches the negative answer for `min(SOA minimum, SOA TTL)` — commonly 300 s.

Why this matters: you create a record, but clients keep getting NXDOMAIN for five minutes because a resolver cached the negative answer. `dig` against the authoritative server shows the truth; `dig @resolver` shows the cache.

```bash
dig @192.168.10.53 newapp.lab.local
dig @ns1.lab.local newapp.lab.local +norecurse
```

If the authoritative server has it and the resolver does not, flush the resolver cache:

```bash
sudo resolvectl flush-caches
# or restart the resolver (dnsmasq/unbound/bind)
```

You cannot flush public resolver caches. Wait out the TTL.

---

## 6. DNS failure patterns

### NXDOMAIN

- Typo in the name.
- Record not created, or created in the wrong zone.
- Wrong search domain appended (short name resolved as `app.lab.local.example.com`).
- Split-horizon DNS: internal resolver has it, external does not.

Check the exact query `dig` sends, including the trailing dot:

```bash
dig app.lab.local
dig app.lab.local.
```

A trailing dot makes the name absolute and disables the search list. If the dotted version works and the undotted one does not, your search/ndots config is the problem.

### SERVFAIL

- Authoritative servers unreachable or timing out.
- DNSSEC validation failure (expired signature, missing DS).
- Broken zone (CNAME loop, bad NS).
- Resolver overloaded.

`dig +trace` shows where it stops. `dig +cd` disables DNSSEC checking on the resolver (for diagnosis only) — if it then works, you have a DNSSEC problem.

### Timeout

- UDP 53 blocked by firewall.
- Resolver IP wrong or unreachable.
- Fragmented response dropped (large answers over UDP).
- Route missing.

Test with TCP:

```bash
dig +tcp @192.168.10.53 example.com
```

DNS uses UDP 53 for most queries and TCP 53 for large responses and zone transfers. If UDP works but TCP does not, large responses and AXFR fail.

### Works for `dig`, fails for the app

- App uses a different resolver (container `/etc/resolv.conf`, JVM DNS cache, Go pure resolver).
- `/etc/hosts` differs inside a container.
- App caches DNS forever (JVM `networkaddress.cache.ttl`).
- IPv6 `AAAA` returned but no IPv6 route; app tries AAAA first and times out.
- Search domain/ndots differs.

Check what the process actually sees:

```bash
cat /proc/<pid>/root/etc/resolv.conf
cat /proc/<pid>/root/etc/hosts
```

### Works for `ping`, fails for `curl`

`ping` resolves the name, then sends ICMP. `curl` resolves, connects TCP, does TLS, sends HTTP. DNS is only the first step. Use `curl -v` to see where it stops.

---

## 7. DNSSEC in one page

DNSSEC signs DNS records so a resolver can verify they were not tampered with. It does **not** encrypt queries.

Chain of trust:

```text
root KSK
  └── signs DS for com.
        └── com. signs DS for example.com.
              └── example.com. signs its records with ZSK
```

| Record | Purpose |
|---|---|
| `DNSKEY` | Public keys for the zone |
| `DS` | Delegation signer; hash of the child's key, stored in the parent |
| `RRSIG` | Signature over a record set |
| `NSEC`/`NSEC3` | Proof that a name does not exist |

Failure modes:

- Expired signature → SERVFAIL.
- Missing DS at the parent → insecure delegation (no validation).
- Clock skew on the signer → signatures invalid.
- Algorithm mismatch after a rollover.

Diagnose:

```bash
dig +dnssec example.com
dig DS example.com
delv example.com
```

In an interview, know the chain, that SERVFAIL can be a DNSSEC failure, and that validation happens at the recursive resolver, not the stub.

---

## 8. DHCP: how a host gets on the network

A host with no address cannot use IP. DHCP gives it:

- IP address and subnet mask
- Default gateway
- DNS servers
- NTP servers, domain name, boot options, and more

### DORA

```text
Client                                      Server
  | Discover (broadcast, 0.0.0.0:68 → 255.255.255.255:67)
  | ------------------------------------------------------->
  | <-------------------------------------------------------
  | Offer (proposed IP, lease time, options)
  | ------------------------------------------------------->
  | Request (I accept this offer)
  | <-------------------------------------------------------
  | ACK (lease confirmed, options)
```

| Step | Source → Destination | Purpose |
|---|---|---|
| Discover | `0.0.0.0:68` → broadcast:67 | Find a server |
| Offer | server:67 → client | Propose an address |
| Request | client → broadcast:67 | Accept a specific offer (so other servers know) |
| ACK | server:67 → client | Confirm lease and options |

Why Request is broadcast: multiple servers may have offered; the broadcast tells all of them which offer was accepted, so the others release their reservations.

### Lease lifecycle

```text
Lease = 24h (example)
T1 = 50% = 12h   → renew (unicast to the original server)
T2 = 87.5% = 21h → rebind (broadcast to any server)
Expiry = 24h     → address released; client must DORA again
```

A client keeps using the address while renewing. If renewal fails at T2, it tries any server. At expiry it must stop using the address.

### DHCP relay

DHCP Discover is broadcast and routers do not forward broadcasts. On a routed network, a **relay agent** (on the router or a Linux host) forwards the request unicast to the DHCP server, putting the client's subnet in `giaddr` so the server can pick an address from the right pool.

```text
Client --broadcast--> Router (relay, giaddr=10.20.0.1) --unicast--> DHCP server
```

Without a relay, clients on remote subnets get no address. This is a classic "new VLAN has no DHCP" incident.

### DHCPv6 and SLAAC

IPv6 hosts can configure themselves without DHCP:

- **SLAAC:** router advertisements provide a prefix; the host forms an address (EUI-64 or privacy).
- **DHCPv6:** used for additional options, or when the network sets the M/O flags in the RA.
- **RA flags:** `M` = use DHCPv6 for addresses; `O` = use DHCPv6 for other options.

Do not assume IPv6 is static just because IPv4 is DHCP.

### Conflicts and rogue servers

- **Duplicate address:** two hosts have the same IP, often from a static address inside a DHCP pool. Reserve the static range outside the pool, or create a reservation.
- **Rogue DHCP:** an unauthorised server hands out wrong gateways/DNS. Use DHCP snooping on switches and monitor for offers from unexpected servers.
- **Starvation:** an attacker requests thousands of leases. DHCP snooping + port security limits this.

### Where the client stores state

| System | Lease file / tool |
|---|---|
| ISC dhclient | `/var/lib/dhcp/dhclient.leases` |
| NetworkManager | `/var/lib/NetworkManager/*.lease` |
| systemd-networkd | `/run/systemd/netif/leases/` |
| Windows | `ipconfig /all` |
| macOS | `ipconfig getpacket en0` |

Linux `dhclient` logs to syslog/journal:

```bash
journalctl -u NetworkManager --since '10 min ago' | grep -i dhcp
```

---

## 9. Command field manual

### `dig` — query DNS

**Purpose:** ask a specific DNS server a specific question and see the full response.

```bash
dig example.com
dig +short example.com
dig @1.1.1.1 example.com A
dig +trace example.com
dig -x 203.0.113.10
```

**Why:** it exposes status codes, flags, TTLs, and the answering server. `nslookup` hides too much; `host` is fine for a quick answer but not for diagnosis.

**Options:**

- `+short` — answers only, script-friendly.
- `@server` — bypass the configured resolver.
- `+norecurse` — ask authoritative servers directly.
- `+tcp` — force TCP.
- `+dnssec` — request signatures.
- `+noall +answer` — clean answer section.
- `-x` — reverse lookup; builds the `in-addr.arpa` name.

**Mistake:** testing only with `dig` when the application uses NSS. Test with `getent hosts` as well.

### `getent hosts` — resolve through NSS

```bash
getent hosts example.com
getent ahosts example.com
```

`ahosts` returns all address families and is closer to what `getaddrinfo()` does. This is the command that matches application behaviour.

### `resolvectl` — systemd-resolved

```bash
resolvectl status
resolvectl query example.com
resolvectl flush-caches
resolvectl statistics
```

`query` shows which link/resolver answered and whether the answer was cached. `statistics` shows cache hit ratio and failures.

### `host` / `nslookup` — quick checks

```bash
host example.com
host -t MX example.com
nslookup example.com 1.1.1.1
```

Useful on minimal systems where `dig` is not installed. `nslookup` is interactive if run without arguments; avoid it in scripts.

### `tcpdump` — see the query

```bash
sudo tcpdump -ni any port 53
```

```text
10.0.0.5.41234 > 192.168.10.53.53: 42133+ A? app.lab.local. (30)
192.168.10.53.53 > 10.0.0.5.41234: 42133 1/0/0 A 192.168.10.20 (46)
```

- `+` means recursion desired.
- `A?` is the question.
- `1/0/0` = answers/authority/additional.
- If you see queries but no responses, the resolver or firewall is the problem.
- If you see no queries, the app is not using DNS (cached, `/etc/hosts`, or a different namespace).

### `dhclient` / `nmcli` — DHCP

```bash
sudo dhclient -v eth0
sudo dhclient -r eth0          # release
nmcli device show eth0 | grep -i dhcp
nmcli con mod "System eth0" ipv4.method auto
```

`dhclient -v` prints DORA in the foreground. `-r` releases the lease. On NetworkManager systems, prefer `nmcli`; running `dhclient` behind NetworkManager's back causes conflicts.

### `ip addr` — confirm the lease

```bash
ip -br addr show eth0
ip route show default
cat /etc/resolv.conf
```

A DHCP lease that lacks a default route or DNS is a partial lease; check the server's options.

---

## 10. Worked scenarios

### Scenario A — new VM has no IP

```bash
ip -br addr show eth0
```

```text
eth0  UP  52:54:00:aa:bb:cc <BROADCAST,MULTICAST,UP,LOWER_UP>
```

No `inet`. Evidence:

```bash
sudo tcpdump -eni eth0 port 67 or port 68
sudo journalctl -u NetworkManager --since '5 min ago' | grep -i dhcp
```

If you see Discover but no Offer, the server/relay is not answering. If you see Offer but no ACK, the server rejected the request. If you see nothing, the interface is not in the right VLAN/bridge, or DHCP is disabled on the port.

### Scenario B — DHCP works, DNS does not

```bash
ip route show default
cat /etc/resolv.conf
dig @192.168.10.53 example.com
```

If the route is present but `resolv.conf` points at `127.0.0.53` and systemd-resolved has no upstream, the DHCP option 6 was not applied. Check `resolvectl status` and the DHCP client logs.

If `dig @server` works but plain `dig` fails, the configured resolver is wrong.

### Scenario C — internal name resolves externally, not internally

Split-horizon DNS: the public zone says `app.example.com → 203.0.113.10`; the internal zone should say `10.0.0.20`. If internal clients get the public IP, they may route out and back in (hairpin) or fail.

```bash
dig @192.168.10.53 app.example.com
dig @1.1.1.1 app.example.com
```

Compare. The fix is the internal zone, not the client.

### Scenario D — intermittent SERVFAIL

Evidence:

```bash
dig +trace app.example.com
dig @ns1.example.com app.example.com +norecurse
dig +dnssec app.example.com
```

- Authoritative server times out → check NS records and reachability.
- `+dnssec` shows expired signatures → check expiry and clock.
- Works with `+cd` → DNSSEC validation failure.

Do not "fix" it by switching clients to 8.8.8.8; that hides the authoritative problem and breaks split-horizon.

### Scenario E — Kubernetes pod cannot resolve a service

```bash
kubectl exec -it pod -- cat /etc/resolv.conf
kubectl exec -it pod -- getent hosts kubernetes.default.svc.cluster.local
kubectl -n kube-system logs deploy/coredns --tail=50
```

Common causes: CoreDNS down, NetworkPolicy blocking UDP/TCP 53, wrong `dnsPolicy`, `ndots`/search path, or the Service has no endpoints (DNS returns NXDOMAIN for a headless service with no ready pods). See [Kubernetes](../04-containers-and-kubernetes/README.md).

---

## 11. Common mistakes

| Mistake | Why it hurts | Fix |
|---|---|---|
| Testing only with `dig` | App uses NSS/hosts/different resolver | Also `getent hosts` |
| Forgetting the trailing dot | Search domains change the query | Use FQDN in diagnostics |
| Editing `/etc/resolv.conf` on systemd-resolved | Overwritten | Use `resolvectl`/network config |
| Raising TTL right before a migration | Slow rollback | Lower TTL days before |
| Assuming NXDOMAIN is always a typo | Could be negative cache or split-horizon | Query authoritative vs resolver |
| Ignoring negative caching | New record invisible for TTL | Wait or flush internal cache |
| Using UDP only for DNS tests | Large responses need TCP | `dig +tcp` |
| Forgetting DHCP relay | Remote VLAN gets no leases | Configure `giaddr` relay |
| Static IP inside DHCP pool | Duplicate address | Reserve or exclude range |
| Blaming DNS for a TCP failure | Name resolved, connect failed | `curl -v`, `ss`, `tcpdump` |
| IPv6 AAAA with no route | Slow timeouts | Check `curl -6`/`-4`, fix v6 or prefer v4 |

---

## 12. Checkpoint

1. What is the difference between recursive and iterative DNS?
2. Which record type delegates a zone, and which one points to the next zone in the chain?
3. What does `NXDOMAIN` mean, and how long is it cached?
4. Why can `dig` fail while `getent hosts` succeeds?
5. What are the four DHCP messages, and why is Request broadcast?
6. What are T1 and T2 as percentages of the lease?
7. Why does a routed network need a DHCP relay?
8. What does `dig +trace` do differently from plain `dig`?
9. `SERVFAIL` on a name that works with `+cd`. What is the likely cause?
10. What does `ndots:5` do, and why does it hurt in Kubernetes?
11. Where does systemd-resolved listen, and why does `resolv.conf` point there?
12. A host has an IP but cannot reach the internet. Which two DHCP options do you check?

### Answers

1. Recursive asks one resolver for the final answer; iterative queries each delegation level.
2. `NS` delegates; `DS` chains DNSSEC trust. (The parent's `NS` record points to the child's servers.)
3. The name does not exist; cached for the SOA minimum/negative TTL.
4. `getent` uses NSS and may read `/etc/hosts`; `dig` queries DNS directly.
5. Discover, Offer, Request, ACK. Request is broadcast so all offering servers learn which offer was accepted.
6. T1 = 50%, T2 = 87.5%.
7. Broadcasts do not cross routers; the relay forwards to the server with `giaddr`.
8. It performs iterative queries from the root, ignoring the configured resolver.
9. DNSSEC validation failure.
10. Names with fewer than 5 dots are tried with each search suffix first, causing extra queries/latency.
11. `127.0.0.53:53`; it is a stub resolver that forwards to per-link upstreams.
12. Default gateway (option 3) and DNS servers (option 6).

If you miss more than three, redo sections 2, 4, and 8 before Lab L2.4.
