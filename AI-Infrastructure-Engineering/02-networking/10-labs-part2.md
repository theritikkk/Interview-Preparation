# Phase 2 Labs — Part 2 (L2.5–L2.7)

> Continue only after L2.1–L2.4 pass. These labs are destructive to the VM's network namespace setup but not to the host. Snapshot before starting.

---

# Lab L2.5 — NAT, Conntrack, and a Default-Deny Firewall

## Objective

Build a three-namespace topology (client → router → server), prove that forwarding alone is not enough when the server has no return route, add masquerade NAT, add DNAT port forwarding, then enforce a default-deny forward policy with a single allowed service. Observe conntrack entries and distinguish DROP from REJECT.

**Skills:** `nftables`, NAT, conntrack, forwarding, DNAT, default deny, packet evidence.

## Prerequisites

- [NAT and firewalls](06-nat-and-firewalls.md).
- [Routing](03-routing-and-linux-networking.md), section 4.
- `nftables`, `conntrack`, `tcpdump`, `curl`, `python3`.

## Setup

```bash
sudo apt update && sudo apt install -y nftables conntrack tcpdump curl python3
```

Create namespaces and veth pairs:

```bash
sudo ip netns add client
sudo ip netns add router
sudo ip netns add server

sudo ip link add veth-c type veth peer name veth-rc
sudo ip link set veth-c netns client
sudo ip link set veth-rc netns router

sudo ip link add veth-s type veth peer name veth-rs
sudo ip link set veth-s netns server
sudo ip link set veth-rs netns router
```

Addressing:

```bash
sudo ip -n client link set lo up
sudo ip -n client addr add 10.0.1.10/24 dev veth-c
sudo ip -n client link set veth-c up
sudo ip -n client route add default via 10.0.1.1

sudo ip -n router link set lo up
sudo ip -n router addr add 10.0.1.1/24 dev veth-rc
sudo ip -n router addr add 10.0.2.1/24 dev veth-rs
sudo ip -n router link set veth-rc up
sudo ip -n router link set veth-rs up
sudo ip netns exec router sysctl -w net.ipv4.ip_forward=1

sudo ip -n server link set lo up
sudo ip -n server addr add 10.0.2.10/24 dev veth-s
sudo ip -n server link set veth-s up
# Deliberately no default route on the server yet.
```

Start a backend:

```bash
sudo ip netns exec server python3 -m http.server 8080 --bind 0.0.0.0 &
```

Check the baseline:

```bash
sudo ip -n client ping -c 2 10.0.2.10
```

Expected: 100% loss. The request reaches the server, but the reply to `10.0.1.10` has no route (the server only knows `10.0.2.0/24`).

Prove it:

```bash
sudo ip netns exec server ip route get 10.0.1.10
```

Expected: `RTNETLINK answers: Network is unreachable` or a route via nothing useful.

## Part A — forwarding without NAT is not enough

Capture on both router interfaces:

```bash
sudo ip netns exec router timeout 10 tcpdump -eni veth-rc icmp &
sudo ip netns exec router timeout 10 tcpdump -eni veth-rs icmp &
sudo ip -n client ping -c 2 10.0.2.10
```

Expected: echo requests cross `veth-rc` and `veth-rs`; no echo replies return. The server's kernel drops them because it has no route to `10.0.1.0/24`.

## Part B — masquerade

Create the NAT table and rule:

```bash
sudo ip netns exec router nft add table ip nat
sudo ip netns exec router nft 'add chain ip nat postrouting { type nat hook postrouting priority srcnat; policy accept; }'
sudo ip netns exec router nft add rule ip nat postrouting oifname "veth-rs" masquerade
```

Create a permissive forward chain for now:

```bash
sudo ip netns exec router nft add table ip filter
sudo ip netns exec router nft 'add chain ip filter forward { type filter hook forward priority filter; policy accept; }'
```

Test:

```bash
sudo ip -n client ping -c 2 10.0.2.10
sudo ip -n client curl -s -o /dev/null -w '%{http_code}\n' http://10.0.2.10:8080/
```

Expected: ping succeeds, HTTP returns `200`.

Inspect conntrack:

```bash
sudo ip netns exec router conntrack -L
```

Expected: an ICMP or TCP flow with the original tuple and a reply tuple. For TCP, the source on the server side is `10.0.2.1`, not `10.0.1.10`.

Prove the source rewrite:

```bash
sudo ip netns exec server timeout 5 tcpdump -ni veth-s tcp port 8080
sudo ip -n client curl -s -o /dev/null http://10.0.2.10:8080/
```

Expected: packets from `10.0.2.1` to `10.0.2.10:8080`, not from `10.0.1.10`.

## Part C — DNAT port forwarding

Expose the server's 8080 as `10.0.2.1:8081`:

```bash
sudo ip netns exec router nft add chain ip nat prerouting '{ type nat hook prerouting priority dstnat; policy accept; }'
sudo ip netns exec router nft add rule ip nat prerouting iifname "veth-rc" tcp dport 8081 dnat to 10.0.2.10:8080
```

Test from the client:

```bash
sudo ip -n client curl -s -o /dev/null -w '%{http_code}\n' http://10.0.2.1:8081/
```

Expected: `200`. Conntrack shows the DNAT mapping.

## Part D — default deny

Change the forward policy to drop and allow only the web ports plus established/related:

```bash
sudo ip netns exec router nft flush chain ip filter forward
sudo ip netns exec router nft delete chain ip filter forward
sudo ip netns exec router nft 'add chain ip filter forward { type filter hook forward priority filter; policy drop; }'
sudo ip netns exec router nft add rule ip filter forward ct state established,related counter accept
sudo ip netns exec router nft add rule ip filter forward iifname "veth-rc" oifname "veth-rs" tcp dport { 8080, 8081 } counter accept
```

Test:

```bash
sudo ip -n client curl -s -o /dev/null -w '%{http_code}\n' http://10.0.2.1:8081/
sudo ip -n client ping -c 2 10.0.2.10
```

Expected: HTTP 200; ping 100% loss (ICMP is not allowed). The HTTP rule counter increments; the ICMP path hits the policy drop.

Inspect:

```bash
sudo ip netns exec router nft -a list chain ip filter forward
```

`-a` prints handles. Counters show which rules matched. This is your evidence that the packet reached the rule, not just "the firewall".

## Part E — DROP vs REJECT

Add a reject rule for ICMP before the policy:

```bash
sudo ip netns exec router nft add rule ip filter forward icmp type echo-request reject
sudo ip -n client ping -c 2 10.0.2.10
```

Expected: immediate `Destination Port Unreachable`/`Administratively Prohibited` instead of a 2-second timeout. Remove it to compare:

```bash
sudo ip netns exec router nft -a list chain ip filter forward
sudo ip netns exec router nft delete rule ip filter forward handle <handle>
```

## Why it works

- Forwarding moves a packet between interfaces; it does not add a return route. The server must know how to reach the client, or NAT must make the source look local.
- Masquerade rewrites the source to the outgoing interface's address and stores the mapping in conntrack. The reply is un-NATed on the way back.
- DNAT rewrites the destination before routing. The forward chain then sees the translated destination and must allow it.
- A default-deny policy drops everything not explicitly allowed; `ESTABLISHED,RELATED` is required for return traffic.
- DROP is silent; REJECT sends an ICMP error. The client's behavior (timeout vs immediate failure) is the observable difference.

## Troubleshooting task

1. DNAT rule exists, but its counter is zero when the client connects. Name three reasons.
2. HTTP works, but the server logs show `10.0.2.1` as the client. Why, and how would you preserve the original source?
3. After setting the forward policy to drop, HTTP fails too. What did you forget?
4. The conntrack table is full. What happens to new connections, and what are two causes?
5. The client can reach the server, but the server cannot initiate a connection to the client. Is that a bug?

### Expected answers

1. Wrong interface in the rule, wrong hook/chain, packet dropped before NAT, or traffic never reaches the router.
2. Masquerade hides the client. To preserve the source, route the server's return traffic through the router and use DNAT without masquerade, or use a proxy that forwards the client identity (e.g. `X-Forwarded-For`).
3. The `ESTABLISHED,RELATED` rule, or the allow rule for the translated port (`8080`).
4. New connections are dropped (`nf_conntrack: table full`); causes include a connection flood, too-small `nf_conntrack_max`, or long timeouts.
5. No. NAT is outbound-only by default; inbound requires DNAT/port forwarding.

## Verification

```bash
sudo ip -n client curl -s -o /dev/null -w 'direct=%{http_code}\n' http://10.0.2.10:8080/
sudo ip -n client curl -s -o /dev/null -w 'dnat=%{http_code}\n' http://10.0.2.1:8081/
sudo ip netns exec router nft list ruleset
sudo ip netns exec router conntrack -L | head
```

Expected: `direct=200`, `dnat=200`, a ruleset with nat/filter tables, and conntrack entries showing both flows. Ping should fail under default deny.

## Cleanup

```bash
sudo pkill -f 'http.server 8080'
sudo ip netns del client
sudo ip netns del router
sudo ip netns del server
```

## Write-up template

```markdown
## L2.5 — NAT and firewalls

**Topology:** client 10.0.1.10 → router 10.0.1.1/10.0.2.1 → server 10.0.2.10
**Without NAT:** evidence (route get, tcpdump)
**With masquerade:** conntrack entry + server-side source IP
**DNAT:** rule + curl result
**Default deny:** ruleset + counters
**DROP vs REJECT:** client-observed difference
**Root cause of the failure I injected:** ...
```

---

# Lab L2.6 — HTTP, TLS Chains, and Load Balancing

## Objective

Build a three-level certificate chain (root → intermediate → leaf), serve it with `openssl s_server`, and prove why serving only the leaf fails in `curl` while a full chain succeeds. Test hostname verification, SNI, HTTP status codes, and a simple L7 load balancer with a failing backend.

**Skills:** X.509, SAN, chain building, SNI, `openssl s_client`, `curl -v`, 502/503/504, Nginx upstreams.

## Prerequisites

- [HTTP, TLS, load balancing](07-http-tls-load-balancing.md).
- `openssl`, `curl`, `python3`, optional `nginx`.

## Setup

```bash
mkdir -p ~/labs/l2.6 && cd ~/labs/l2.6
```

### 1. Root CA

```bash
openssl req -x509 -newkey rsa:2048 -nodes \
  -keyout root.key -out root.crt -days 3650 \
  -subj "/CN=Lab Root CA" \
  -addext "basicConstraints=critical,CA:TRUE" \
  -addext "keyUsage=critical,keyCertSign,cRLSign"
```

### 2. Intermediate CA

```bash
openssl req -newkey rsa:2048 -nodes \
  -keyout int.key -out int.csr -subj "/CN=Lab Intermediate CA"

cat > int.ext <<'EOF'
basicConstraints=critical,CA:TRUE,pathlen:0
keyUsage=critical,keyCertSign,cRLSign
subjectKeyIdentifier=hash
authorityKeyIdentifier=keyid,issuer
EOF

openssl x509 -req -in int.csr -CA root.crt -CAkey root.key -CAcreateserial \
  -out int.crt -days 1825 -extfile int.ext
```

### 3. Leaf certificate

```bash
openssl req -newkey rsa:2048 -nodes \
  -keyout leaf.key -out leaf.csr -subj "/CN=app.lab.test"

cat > leaf.ext <<'EOF'
basicConstraints=critical,CA:FALSE
keyUsage=critical,digitalSignature,keyEncipherment
extendedKeyUsage=serverAuth
subjectAltName=DNS:app.lab.test
EOF

openssl x509 -req -in leaf.csr -CA int.crt -CAkey int.key -CAcreateserial \
  -out leaf.crt -days 365 -extfile leaf.ext
```

Verify the chain offline:

```bash
openssl verify -CAfile root.crt -untrusted int.crt leaf.crt
```

Expected: `leaf.crt: OK`.

Inspect the SAN:

```bash
openssl x509 -in leaf.crt -noout -subject -issuer -dates -ext subjectAltName
```

Expected: `subject=CN = app.lab.test`, `issuer=CN = Lab Intermediate CA`, and `DNS:app.lab.test`.

## Part A — serve only the leaf

```bash
openssl s_server -accept 8443 -cert leaf.crt -key leaf.key -WWW &
```

Test:

```bash
curl -v --cacert root.crt --resolve app.lab.test:8443:127.0.0.1 \
  https://app.lab.test:8443/
```

Expected failure:

```text
curl: (60) SSL certificate problem: unable to get local issuer certificate
```

Why: the server sends only the leaf; curl has the root but not the intermediate, so it cannot build the chain. Browsers may fetch the intermediate via AIA or have it cached, which is why "it works in Chrome" is not proof.

Check with `openssl s_client`:

```bash
openssl s_client -connect 127.0.0.1:8443 -servername app.lab.test -showcerts </dev/null
```

Expected: only certificate 0 (the leaf) in the chain, and `verify return code: 20 (unable to get local issuer certificate)` or similar.

## Part B — serve the full chain

Stop the server (Ctrl-C or `kill %1`) and restart with the intermediate:

```bash
openssl s_server -accept 8443 -cert leaf.crt -key leaf.key -cert_chain int.crt -WWW &
```

Test again:

```bash
curl -sS --cacert root.crt --resolve app.lab.test:8443:127.0.0.1 \
  https://app.lab.test:8443/ | head
```

Expected: HTML directory listing (or `200`), no certificate error. `openssl s_client` now shows depth 0 (leaf) and depth 1 (intermediate); `verify return code: 0 (ok)`.

## Part C — hostname and SNI

Wrong hostname:

```bash
curl -v --cacert root.crt --resolve wrong.lab.test:8443:127.0.0.1 \
  https://wrong.lab.test:8443/
```

Expected: hostname mismatch (`subjectAltName` does not contain `wrong.lab.test`).

Test SNI explicitly:

```bash
openssl s_client -connect 127.0.0.1:8443 -servername app.lab.test </dev/null | grep -E 'subject=|Verification|verify return'
openssl s_client -connect 127.0.0.1:8443 -noservername </dev/null | grep -E 'subject=|Verification|verify return'
```

The first sends SNI; the second does not. A real multi-tenant server would return a different certificate without SNI. `openssl s_server` with a single cert may still return the same cert, which is a useful limitation to note.

## Part D — HTTP status codes

Run a status server on loopback:

```bash
cat > status_server.py <<'PY'
from http.server import BaseHTTPRequestHandler, HTTPServer

class H(BaseHTTPRequestHandler):
    def do_GET(self):
        code = {"/502": 502, "/503": 503, "/504": 504}.get(self.path, 200)
        body = f"{code}\n".encode()
        self.send_response(code)
        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)
    def log_message(self, *args):
        pass

HTTPServer(("127.0.0.1", 8080), H).serve_forever()
PY
python3 status_server.py &
```

```bash
for p in / /502 /503 /504; do
  printf '%s -> ' "$p"
  curl -s -o /dev/null -w '%{http_code}\n' "http://127.0.0.1:8080$p"
done
```

Expected:

```text
/ -> 200
/502 -> 502
/503 -> 503
/504 -> 504
```

This is only a simulation. In a real incident, the proxy log and backend timing tell you which one occurred.

## Part E — optional L7 load balancing with Nginx

```bash
sudo apt install -y nginx
mkdir -p ~/labs/l2.6/backend1 ~/labs/l2.6/backend2
echo 'backend-1' > ~/labs/l2.6/backend1/index.html
echo 'backend-2' > ~/labs/l2.6/backend2/index.html

python3 -m http.server 8081 --directory ~/labs/l2.6/backend1 &
python3 -m http.server 8082 --directory ~/labs/l2.6/backend2 &
```

Create `/etc/nginx/conf.d/lab.conf`:

```nginx
upstream lab_backend {
    server 127.0.0.1:8081 max_fails=2 fail_timeout=5s;
    server 127.0.0.1:8082 max_fails=2 fail_timeout=5s;
}

server {
    listen 8088;
    location / {
        proxy_pass http://lab_backend;
    }
}
```

```bash
sudo nginx -t
sudo systemctl reload nginx
for i in $(seq 1 6); do curl -s http://127.0.0.1:8088/; done
```

Expected: alternating `backend-1` and `backend-2` (round robin).

Kill backend 2:

```bash
pkill -f 'http.server 8082'
for i in $(seq 1 6); do curl -s -o /dev/null -w '%{http_code} ' http://127.0.0.1:8088/; done; echo
```

Expected: some `502` responses until `max_fails=2` marks the backend down, then `200` from backend 1. This is passive health checking. A proper active check (`health_check` in Nginx Plus, or HAProxy `option httpchk`) detects failure sooner.

## Why it works

- A certificate chain must include every intermediate between the leaf and a trusted root. The server sends the chain; the client already has the root.
- SAN is authoritative for hostname verification; CN alone is ignored by modern clients.
- SNI tells the server which certificate/vhost to use. Testing by IP without SNI often returns the wrong cert.
- L7 load balancers parse HTTP, so they can route by path/host and return 502/503/504 when the upstream fails. Passive checks infer health from real requests; active checks probe separately.

## Troubleshooting task

1. `curl --cacert root.crt` fails with "unable to get local issuer" but `openssl verify` says OK. Why?
2. `curl` works with `-k` but fails without it. What are the possible causes?
3. The browser works but Java fails with a certificate error. What is the usual cause?
4. Nginx returns 502 for one backend but 200 for the other. Is the network broken?
5. A request works by IP but returns 404 by hostname. What do you check?
6. After enabling HTTPS, the app redirects in a loop. What header/config do you check?

### Expected answers

1. The server is not sending the intermediate; the chain is incomplete on the wire even though the files are correct locally.
2. Self-signed/untrusted CA, missing intermediate, hostname mismatch, expired cert, or wrong SNI. `-k` disables all verification and hides which.
3. Java does not use the system trust store the same way and may not fetch intermediates; the server must send the full chain and the CA must be imported.
4. No. One upstream is down/misconfigured; Nginx returns 502 for that backend. Check its logs and health.
5. `Host` header/vhost routing, SNI/certificate, or the load balancer rule.
6. `X-Forwarded-Proto` and the app's trusted-proxy setting; the app thinks the request is HTTP and redirects to HTTPS again.

## Verification

```bash
openssl verify -CAfile root.crt -untrusted int.crt leaf.crt
echo | openssl s_client -connect 127.0.0.1:8443 -servername app.lab.test 2>/dev/null | grep 'verify return code'
curl -sS --cacert root.crt --resolve app.lab.test:8443:127.0.0.1 -o /dev/null -w '%{http_code}\n' https://app.lab.test:8443/
```

Expected: `leaf.crt: OK`, `verify return code: 0 (ok)`, `200`.

## Cleanup

```bash
pkill -f 'openssl s_server'
pkill -f status_server.py
pkill -f 'http.server 808'
sudo rm -f /etc/nginx/conf.d/lab.conf
sudo systemctl reload nginx 2>/dev/null
rm -rf ~/labs/l2.6
```

Do not delete the system CA store. If you imported `root.crt` anywhere, remove it.

## Write-up template

```markdown
## L2.6 — TLS and HTTP

**Chain:** root → intermediate → leaf
**Leaf-only result:** curl error + s_client chain depth
**Full-chain result:** verify code + curl status
**Hostname test:** ...
**SNI test:** ...
**Status codes:** 200/502/503/504 mapping
**LB observation:** round robin + failure behavior
**Root cause of the chain failure:** ...
```

---

# Lab L2.7 — Packet Capture Capstone

## Objective

Capture and annotate ARP, DNS, TCP, TLS, and ICMP traffic; use filters to isolate a flow; correlate packets with `ss` and `conntrack`; and diagnose a synthetic black hole from a pcap alone.

**Skills:** `tcpdump` filters, pcap analysis, `tshark`, handshake/teardown reading, fault-domain isolation.

## Prerequisites

- Labs L2.1–L2.6.
- `tcpdump`, `tshark`, `dnsutils`, `curl`, `openssl`, `nftables`.

## Setup

```bash
mkdir -p ~/labs/l2.7 && cd ~/labs/l2.7
sudo apt update && sudo apt install -y tcpdump tshark dnsutils curl openssl nftables netcat-openbsd
```

Use a single terminal for captures and a second for traffic. Synchronise time if you have multiple hosts; on one host it is automatic.

## Part A — ARP

```bash
GW=$(ip route | awk '/^default/{print $3; exit}')
IFACE=$(ip route | awk '/^default/{print $5; exit}')
echo "GW=$GW IFACE=$IFACE"
sudo ip neigh flush all
sudo timeout 10 tcpdump -eni "$IFACE" arp -w arp.pcap &
sleep 1
ping -c 1 "$GW"
wait
```

Analyse:

```bash
tshark -r arp.pcap -Y 'arp' -T fields -e arp.opcode -e arp.src.proto_ipv4 -e arp.dst.proto_ipv4 -e eth.src -e eth.dst
```

Expected: opcode 1 (request) with destination `0.0.0.0`/broadcast MAC, then opcode 2 (reply) with the gateway MAC. If you only see requests, the gateway is not replying (or the capture missed the reply).

## Part B — DNS

```bash
sudo timeout 10 tcpdump -ni any port 53 -w dns.pcap &
sleep 1
dig example.com A >/dev/null
wait
```

Analyse:

```bash
tshark -r dns.pcap -Y 'dns' -T fields -e dns.flags.response -e dns.qry.name -e dns.a -e dns.flags.rcode
```

Expected: a query (`response=0`) and a response (`response=1`) with `dns.a` populated and `rcode=0` (NOERROR). If the response is missing, the resolver or path is the problem; if `rcode=3`, the name is NXDOMAIN.

## Part C — TCP handshake and teardown

```bash
timeout 10 nc -l 9000 &
sudo timeout 10 tcpdump -ni lo -c 20 'tcp port 9000' -w tcp.pcap &
sleep 1
printf 'hello\n' | nc 127.0.0.1 9000
wait
```

Analyse:

```bash
tshark -r tcp.pcap -Y 'tcp' -T fields -e frame.time_relative -e ip.src -e tcp.srcport -e tcp.dstport -e tcp.flags.str -e tcp.seq -e tcp.ack -e tcp.len
```

Expected: SYN, SYN-ACK, ACK, PSH-ACK with payload, ACK, FIN-ACK, ACK. Identify which side closed first by the first FIN.

## Part D — TLS ClientHello and SNI

Start the TLS server from L2.6, or generate a quick self-signed cert:

```bash
openssl req -x509 -newkey rsa:2048 -nodes -keyout key.pem -out cert.pem -days 1 \
  -subj "/CN=app.lab.test" -addext "subjectAltName=DNS:app.lab.test"
timeout 10 openssl s_server -accept 8443 -cert cert.pem -key key.pem -WWW &

sudo timeout 10 tcpdump -ni lo -c 40 'tcp port 8443' -w tls.pcap &
sleep 1
curl -k --resolve app.lab.test:8443:127.0.0.1 https://app.lab.test:8443/ -o /dev/null
wait
```

Analyse the SNI:

```bash
tshark -r tls.pcap -Y 'tls.handshake.extensions_server_name' \
  -T fields -e tls.handshake.extensions_server_name
```

Expected: `app.lab.test`. The ClientHello is plaintext even though the application data is encrypted. You can see the certificate subject and negotiated cipher, but not the HTTP body.

Check the handshake:

```bash
tshark -r tls.pcap -Y 'tls.handshake' -T fields -e tls.handshake.type -e tls.handshake.version
```

Expected types: 1 (ClientHello), 2 (ServerHello), 11 (Certificate), 16 (ClientKeyExchange, TLS 1.2 only), 20 (Finished). TLS 1.3 encrypts more of the handshake, so fewer fields are visible.

## Part E — ICMP

```bash
sudo timeout 10 tcpdump -ni any icmp -w icmp.pcap &
sleep 1
ping -c 3 1.1.1.1 >/dev/null
wait
tshark -r icmp.pcap -Y 'icmp' -T fields -e ip.src -e ip.dst -e icmp.type -e icmp.seq
```

Expected: type 8 (echo request) and type 0 (echo reply), matching sequence numbers. If only requests appear, replies are filtered or the host is down.

## Part F — a black hole

Create a drop rule for a test port:

```bash
sudo nft add table ip lab
sudo nft 'add chain ip lab output { type filter hook output priority 0; policy accept; }'
sudo nft add rule ip lab output tcp dport 9998 drop

sudo timeout 12 tcpdump -ni lo 'tcp port 9998' -w blackhole.pcap &
sleep 1
nc -vz -w 5 127.0.0.1 9998 || true
wait
```

Analyse:

```bash
tshark -r blackhole.pcap -Y 'tcp.flags.syn == 1' -T fields -e frame.time_relative -e tcp.flags.str -e tcp.seq
```

Expected: repeated SYNs at roughly 0, 1, 3, 7 seconds with no SYN-ACK. This is the signature of a DROP/black hole (or a dead host). If you instead see SYN → RST, the port is closed or actively rejected.

Remove the rule:

```bash
sudo nft delete table ip lab
```

## Why it works

- `tcpdump` captures at the interface; `tshark` decodes the pcap with protocol dissectors.
- ARP is broadcast request/unicast reply; no reply means L2 resolution failed.
- DNS query/response correlation uses the transaction ID; `rcode` tells you NXDOMAIN/SERVFAIL.
- TCP flags and sequence numbers reveal the state machine without application logs.
- TLS ClientHello is plaintext; SNI, ALPN, and certificate metadata are visible even when payloads are encrypted.
- A black hole produces retransmitted SYNs with no response; a closed port produces RST.

## Troubleshooting task

Given only these pcap summaries, name the fault domain and the next check:

1. SYN at t=0, 1, 3, 7 s; no SYN-ACK; no ICMP.
2. SYN → RST immediately.
3. SYN → SYN-ACK → ACK → PSH → ACK → RST at t=60 s.
4. DNS query repeated 3 times, no response; TCP to the same resolver's IP:53 also times out.
5. TLS ClientHello, ServerHello, then a TLS alert `unknown_ca`; TCP stays open briefly.
6. ARP request for the gateway repeated; no reply; same-VLAN host also unreachable.

### Expected answers

1. DROP/black hole or host down; check firewall counters, route, and capture at the destination.
2. Closed port or REJECT; check `ss -lntp` and firewall rules.
3. Idle timeout/reset after 60 s; check LB/NAT/conntrack timeout and keepalive.
4. DNS path/firewall problem, not the name; check UDP/TCP 53 and the resolver.
5. TLS trust/chain failure; inspect the server chain and the client trust store.
6. L2/VLAN/switch problem; check the access port VLAN and bridge FDB.

## Verification

Produce an annotated pcap set and a one-page flow diagram:

```text
ARP: request (who-has GW) → reply (GW is-at MAC)
DNS: query id=0x... A example.com → response NOERROR A ...
TCP: SYN → SYN-ACK → ACK → PSH(hello) → ACK → FIN-ACK → ACK
TLS: ClientHello(SNI=app.lab.test) → ServerHello → Certificate → ...
ICMP: echo request type 8 → echo reply type 0
Black hole: SYN t=0,1,3,7 → no response
```

Checklist:

- [ ] Every capture has a filter and a packet count/time limit.
- [ ] You can name the first failing layer from each pcap.
- [ ] You can explain why a pcap on one host may not show the return path.
- [ ] You removed the nftables test table.

## Cleanup

```bash
pkill -f 'openssl s_server'
pkill -f 'nc -l 9000'
sudo nft delete table ip lab 2>/dev/null
rm -f ~/labs/l2.7/*.pcap ~/labs/l2.7/*.pem
```

## Write-up template

```markdown
## L2.7 — Packet capture capstone

**ARP:** request/reply MACs, what a missing reply means
**DNS:** transaction ID, rcode, answer
**TCP:** flags in order, who closed first
**TLS:** SNI, certificate subject, negotiated version
**ICMP:** types/sequence
**Black hole:** retransmit timings, fault domain
**Next check for each failure:** ...
**Capture mistake I made:** ...
```

---

## Phase 2 completion

You are done with the labs when you can:

- Design and verify a VLSM plan without a calculator.
- Explain same-VLAN vs cross-VLAN traffic with a capture.
- Read every TCP state in `ss` and say what the kernel is waiting for.
- Diagnose DNS with `dig` and `getent`, and explain negative caching.
- Write a default-deny nftables ruleset with NAT and prove it with counters and conntrack.
- Build and debug a certificate chain.
- Read a pcap and name the failing layer.

Then take [the Phase 2 checkpoint](11-checkpoint.md).
