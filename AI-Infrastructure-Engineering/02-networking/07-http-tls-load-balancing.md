# HTTP, TLS, and Load Balancing

> **The one-sentence model:** by the time you debug HTTP, DNS and TCP may already be fine. The questions become: did TLS complete, did the request reach the right backend, and did the application return a valid response in time?

---

## 1. HTTP in one page

HTTP is a request/response protocol. A client sends a method, target, headers, and optional body; the server sends a status, headers, and optional body.

```http
GET /api/v1/models HTTP/1.1
Host: api.example.com
User-Agent: curl/8.5.0
Accept: application/json
Authorization: Bearer <token>

```

```http
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 42
Cache-Control: no-store

{"models":["llama","mistral"],"count":2}
```

### Methods

| Method | Safe | Idempotent | Typical use |
|---|---|---|---|
| GET | Yes | Yes | Read |
| HEAD | Yes | Yes | Headers only |
| POST | No | No | Create / non-idempotent action |
| PUT | No | Yes | Replace |
| PATCH | No | No | Partial update |
| DELETE | No | Yes | Remove |
| OPTIONS | Yes | Yes | Capabilities / CORS preflight |

Idempotency matters for retries: retrying a POST can create duplicate resources unless the API supports an idempotency key.

### Status codes

| Code | Meaning | Debug hint |
|---|---|---|
| 200 | OK | Normal |
| 201 | Created | Check `Location` header |
| 204 | No content | Body must be empty |
| 301/308 | Permanent redirect | 308 preserves method/body |
| 302/307 | Temporary redirect | 307 preserves method/body |
| 304 | Not modified | Caching; no body |
| 400 | Bad request | Malformed syntax/body |
| 401 | Unauthorized | Missing/invalid credentials |
| 403 | Forbidden | Authenticated but not allowed |
| 404 | Not found | Wrong path, host, or router |
| 408 | Request timeout | Client too slow |
| 409 | Conflict | Concurrent/duplicate change |
| 413 | Payload too large | Body or header limit |
| 429 | Too many requests | Rate limit; check `Retry-After` |
| 500 | Internal server error | App exception |
| 501 | Not implemented | Unsupported method |
| 502 | Bad gateway | Upstream refused/invalid response |
| 503 | Service unavailable | No healthy backend / overload |
| 504 | Gateway timeout | Upstream too slow |

The distinction between 502, 503, and 504 is often the fastest way to localise a failure:

- **502** → the proxy reached the backend but got garbage or a connection failure.
- **503** → the proxy has no backend it considers healthy, or is shedding load.
- **504** → the proxy waited for the backend and gave up.

### Headers that matter

| Header | Purpose |
|---|---|
| `Host` | Virtual host routing (HTTP/1.1 requires it) |
| `Content-Type` | Body media type |
| `Content-Length` | Body size in bytes |
| `Transfer-Encoding: chunked` | Body length unknown up front |
| `Connection: keep-alive/close` | Connection reuse |
| `Authorization` | Credentials |
| `Cookie` / `Set-Cookie` | State |
| `Cache-Control`, `ETag`, `Last-Modified` | Caching |
| `Location` | Redirect target / created resource |
| `Retry-After` | When to retry (429/503) |
| `X-Forwarded-For`, `X-Forwarded-Proto`, `Forwarded` | Original client/protocol through a proxy |
| `X-Request-ID` | Correlation across services |
| `Server`, `Via` | Proxy identity (often hidden) |

### HTTP versions

| Version | Transport | Key traits |
|---|---|---|
| HTTP/1.0 | TCP | One request per connection by default |
| HTTP/1.1 | TCP | Keep-alive, pipelining (rarely used), chunked |
| HTTP/2 | TCP | Binary framing, multiplexing, header compression (HPACK), server push (deprecated) |
| HTTP/3 | QUIC/UDP | Avoids TCP head-of-line blocking, 0-RTT, built-in TLS 1.3 |

HTTP/2 multiplexes streams over one TCP connection. A lost packet still blocks all streams at the TCP layer (head-of-line blocking); HTTP/3 solves that by moving reliability into QUIC streams.

### Keep-alive

A persistent connection avoids a TCP+TLS handshake per request. But idle connections are state that middleboxes can silently drop:

- Load balancer idle timeout 60 s, client keep-alive 120 s → the client reuses a dead connection and gets a reset.
- Server keep-alive shorter than the client's → server closes first; races cause resets.

Keep client keep-alive **shorter** than the LB idle timeout, and make retries safe for idempotent requests.

---

## 2. Reading `curl -v`

```bash
curl -v https://api.example.com/health
```

```text
*   Trying 203.0.113.10:443...
* Connected to api.example.com (203.0.113.10) port 443 (#0)
* ALPN: offers h2,http/1.1
* TLSv1.3 (OUT), TLS handshake, Client hello (1):
* TLSv1.3 (IN), TLS handshake, Server hello (2):
* TLSv1.3 (IN), TLS handshake, Encrypted Extensions (8):
* TLSv1.3 (IN), TLS handshake, Certificate (11):
* TLSv1.3 (IN), TLS handshake, CERT verify (15):
* TLSv1.3 (IN), TLS handshake, Finished (20):
* SSL connection using TLSv1.3 / TLS_AES_256_GCM_SHA384 / X25519MLKEM768
* Server certificate:
*  subject: CN=api.example.com
*  start date: Oct  1 00:00:00 2026 GMT
*  expire date: Dec 30 23:59:59 2026 GMT
*  subjectAltName: host "api.example.com" matched cert's "api.example.com"
*  issuer: C=US; O=Let's Encrypt; CN=R11
*  SSL certificate verify ok.
> GET /health HTTP/1.1
> Host: api.example.com
> User-Agent: curl/8.5.0
> Accept: */*
>
< HTTP/1.1 200 OK
< Content-Type: application/json
< Content-Length: 15
< Date: Mon, 26 Oct 2026 22:20:00 GMT
<
{"status":"ok"}
```

Read it bottom-up for failures, top-down for setup:

1. **DNS:** `Trying 203.0.113.10` — did it resolve?
2. **TCP:** `Connected ... port 443` — did the handshake complete?
3. **TLS:** protocol, cipher, certificate, verify result.
4. **HTTP:** request line, status, headers.
5. **Body:** actual response.

If `Trying` never resolves → DNS. If `Trying` but no `Connected` → TCP/firewall. If `Connected` but TLS errors → certificates/protocol. If HTTP status is 5xx → backend/application.

### Timing breakdown

```bash
curl -sS -o /dev/null -w '\
dns=%{time_namelookup} connect=%{time_connect} tls=%{time_appconnect} \
ttfb=%{time_starttransfer} total=%{time_total} code=%{http_code}\n' \
https://api.example.com/health
```

```text
dns=0.012 connect=0.031 tls=0.078 ttfb=0.121 total=0.125 code=200
```

| Metric | Meaning |
|---|---|
| `time_namelookup` | DNS done |
| `time_connect` | TCP handshake done |
| `time_appconnect` | TLS handshake done |
| `time_starttransfer` | First response byte (TTFB) |
| `time_total` | Whole request |
| `http_code` | Status |

If `connect - namelookup` is large → TCP/network. If `appconnect - connect` is large → TLS. If `starttransfer - appconnect` is large → backend processing. If `total - starttransfer` is large → large/slow body.

### Bypass DNS and test a specific backend

```bash
curl --resolve api.example.com:443:10.0.0.5 https://api.example.com/health
curl -H 'Host: api.example.com' http://10.0.0.5/health
```

`--resolve` lets you test a backend before changing DNS. It also proves whether the LB or the backend is at fault.

### Force protocol versions

```bash
curl --http1.1 https://example.com
curl --http2 https://example.com
curl --http3 https://example.com
```

If HTTP/2 fails but HTTP/1.1 works, suspect an ALPN mismatch or a proxy that does not support h2 end-to-end.

---

## 3. TLS: what actually happens

TLS provides confidentiality, integrity, and authentication (via certificates).

### TLS 1.2 handshake (2-RTT)

```text
Client                                Server
  | ClientHello (versions, ciphers, SNI, ALPN)
  | ------------------------------------>
  | <------------------------------------
  | ServerHello, Certificate, ServerKeyExchange, ServerHelloDone
  | ClientKeyExchange, ChangeCipherSpec, Finished
  | ------------------------------------>
  | <------------------------------------
  | ChangeCipherSpec, Finished
  | ========== encrypted application data ==========
```

### TLS 1.3 handshake (1-RTT)

```text
Client                                Server
  | ClientHello + key share
  | ------------------------------------>
  | <------------------------------------
  | ServerHello + key share + EncryptedExtensions + Certificate + Finished
  | Finished
  | ========== encrypted application data ==========
```

TLS 1.3 removes RSA key exchange, static DH, and many legacy ciphers. It encrypts most of the handshake, which makes debugging harder without keys. 0-RTT resumption can replay requests, so it must only be used for idempotent operations.

### Certificates

```text
Root CA (in trust store)
  └── Intermediate CA
        └── Leaf certificate (api.example.com)
```

The server must send the leaf **and** the intermediates; the client already has the root. A missing intermediate is the most common "works in Chrome, fails in curl/Java" problem because browsers cache intermediates.

| Field | Check |
|---|---|
| Subject/SAN | Must match the hostname (SAN, not CN, is authoritative) |
| Validity | `notBefore` ≤ now ≤ `notAfter` |
| Issuer | Chains to a trusted root |
| Key usage / EKU | Server authentication |
| Signature algorithm | Not SHA-1/MD5 |
| Revocation | OCSP/CRL; often stapled |

### SNI

A server may host many certificates. The client sends the hostname in **SNI** during the handshake so the server can pick the right one. Without SNI, you get the default certificate — a common cause of "wrong certificate" when testing by IP.

```bash
openssl s_client -connect 203.0.113.10:443 -servername api.example.com
```

Always pass `-servername` when testing by IP. `curl --resolve` does this correctly.

### ALPN

ALPN negotiates the application protocol (`h2`, `http/1.1`, `h3`). A mismatch can cause the connection to succeed at TLS but fail at HTTP, or fall back to HTTP/1.1 unexpectedly.

```bash
openssl s_client -connect api.example.com:443 -alpn h2
```

### Mutual TLS (mTLS)

Both sides present certificates. Common for service mesh, internal APIs, and zero-trust. Failure modes: client cert not sent, wrong CA, expired cert, server not requesting a cert.

```bash
curl --cert client.crt --key client.key --cacert ca.crt https://internal.example.com
```

---

## 4. TLS diagnostics

### `openssl s_client`

```bash
openssl s_client -connect api.example.com:443 -servername api.example.com -showcerts </dev/null
```

```text
CONNECTED(00000003)
depth=2 C = US, O = Internet Security Research Group, CN = ISRG Root X1
verify return code: 0 (ok)
---
Certificate chain
 0 s:CN = api.example.com
   i:C = US, O = Let's Encrypt, CN = R11
 1 s:C = US, O = Let's Encrypt, CN = R11
   i:C = US, O = Internet Security Research Group, CN = ISRG Root X1
---
SSL handshake has read 3214 bytes and written 312 bytes
Verification: OK
New, TLSv1.3, Cipher is TLS_AES_256_GCM_SHA384
Server public key is 256 bit
```

- `verify return code: 0 (ok)` — chain validated.
- `depth` lines show the chain the server sent.
- If depth 1 is missing, the server is not sending the intermediate.

Test a specific protocol:

```bash
openssl s_client -connect api.example.com:443 -servername api.example.com -tls1_2 </dev/null
openssl s_client -connect api.example.com:443 -servername api.example.com -tls1_3 </dev/null
```

Inspect a certificate file:

```bash
openssl x509 -in cert.pem -noout -subject -issuer -dates -ext subjectAltName
```

```text
subject=CN = api.example.com
issuer=C = US, O = Let's Encrypt, CN = R11
notBefore=Oct  1 00:00:00 2026 GMT
notAfter=Dec 30 23:59:59 2026 GMT
X509v3 Subject Alternative Name:
    DNS:api.example.com, DNS:www.api.example.com
```

Check a remote chain without connecting to the app:

```bash
echo | openssl s_client -connect api.example.com:443 -servername api.example.com 2>/dev/null \
  | openssl x509 -noout -dates -subject -issuer
```

### Common TLS errors

| Error | Meaning | Fix |
|---|---|---|
| `certificate has expired` | `notAfter` in the past | Renew and reload |
| `unable to get local issuer certificate` | Missing intermediate or untrusted root | Send full chain / install CA |
| `self-signed certificate` | Not issued by a trusted CA | Use a trusted cert or add CA (internal only) |
| `hostname mismatch` | SAN does not match | Correct cert or SNI/Host |
| `certificate verify failed` | Generic chain/validation failure | Inspect chain, time, CA |
| `sslv3 alert handshake failure` | No shared cipher/protocol | Align TLS versions/ciphers |
| `wrong version number` | Talking TLS to a plaintext port | Fix port/protocol |
| `connection reset` during handshake | Firewall/LB, mismatched mTLS | Capture, check LB logs |
| `unexpected eof while reading` | Server closed mid-handshake | Backend crash, timeout, protocol mismatch |

`curl -k` disables verification. Use it only to prove the server is otherwise reachable; never in production scripts.

### Certificate expiry monitoring

```bash
echo | openssl s_client -connect api.example.com:443 -servername api.example.com 2>/dev/null \
  | openssl x509 -noout -enddate
```

```text
notAfter=Dec 30 23:59:59 2026 GMT
```

Alert at 30/14/7 days. Automation (cert-manager, ACME) can still fail silently if renewal is broken; monitor the actual served certificate, not just the file on disk.

---

## 5. Load balancing

### L4 vs L7

| | L4 | L7 |
|---|---|---|
| Sees | IP, port, TCP/UDP | HTTP method, path, host, headers |
| TLS | Passthrough or terminate | Usually terminates |
| Routing | Per connection | Per request |
| Performance | Higher | Lower (parsing) |
| Health checks | TCP connect | HTTP status/body |
| Sticky sessions | Source IP | Cookie |
| Use | TCP databases, generic TLS | HTTP APIs, microservices |

L4 with TLS passthrough cannot inspect or route on HTTP; it also cannot add `X-Forwarded-For`. L7 terminates TLS, so the backend sees plaintext HTTP and must trust the proxy's headers.

### Algorithms

| Algorithm | How | When |
|---|---|---|
| Round robin | Rotate | Homogeneous backends |
| Weighted RR | Proportional | Mixed capacity |
| Least connections | Fewest active | Long-lived/variable requests |
| Least response time | Fastest + fewest | Latency-sensitive |
| IP hash | Client IP → backend | Session affinity without cookies |
| Consistent hash | Key → ring | Cache locality |
| Random two choices | Pick two, choose less loaded | Large pools, avoids herding |

### Health checks

- **Passive:** infer health from real traffic (5xx, timeouts). No extra load, slower detection.
- **Active:** probe `/healthz` on an interval. Faster, but the probe must reflect real ability to serve.
- **TCP check:** only proves the port accepts; a deadlocked app still passes.
- **HTTP check:** status 200 and optionally a body match; better, but can be gamed by a shallow endpoint.

Bad health checks cause outages: a `/healthz` that returns 200 while the database connection is down keeps a broken backend in rotation. A check that is too aggressive removes healthy slow backends.

### Timeouts and retries

```text
client → LB: client timeout / idle timeout
LB → backend: connect timeout, read timeout, idle timeout
backend → dependency: its own timeouts
```

Rules:

- Set every timeout explicitly; defaults are often infinite or too long.
- Client timeout > LB timeout > backend timeout, or clients give up first and retries pile up.
- Retry only idempotent requests, or use an idempotency key.
- Cap retries and add jitter to avoid retry storms.
- A 502/504 may be a timeout, not a crash.

### Sticky sessions

Cookie-based affinity is more reliable than source-IP hashing (NAT breaks IP affinity). But stickiness hides state problems and complicates scaling and failover. Prefer stateless services with shared state.

### Proxy headers

```text
X-Forwarded-For: client, proxy1, proxy2
X-Forwarded-Proto: https
X-Forwarded-Host: api.example.com
Forwarded: for=203.0.113.5;proto=https;host=api.example.com
```

Trust only headers added by your own proxy. If the app trusts `X-Forwarded-For` from the internet, clients can spoof their IP. Configure the app/proxy to trust a specific CIDR and to overwrite, not append blindly.

### Nginx example

```nginx
upstream app {
    least_conn;
    server 10.0.0.5:8080 max_fails=3 fail_timeout=10s;
    server 10.0.0.6:8080 max_fails=3 fail_timeout=10s;
    keepalive 32;
}

server {
    listen 443 ssl;
    server_name api.example.com;

    ssl_certificate     /etc/ssl/api/fullchain.pem;
    ssl_certificate_key /etc/ssl/api/privkey.pem;
    ssl_protocols       TLSv1.2 TLSv1.3;

    location / {
        proxy_pass http://app;
        proxy_http_version 1.1;
        proxy_set_header Host              $host;
        proxy_set_header X-Real-IP         $remote_addr;
        proxy_set_header X-Forwarded-For   $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;

        proxy_connect_timeout 5s;
        proxy_send_timeout    30s;
        proxy_read_timeout    30s;
    }
}
```

`fullchain.pem` must include the leaf and intermediates. A common outage is deploying only the leaf; browsers work, Java/curl clients fail.

### HAProxy example

```haproxy
frontend fe_https
    bind *:443 ssl crt /etc/haproxy/certs/api.pem alpn h2,http/1.1
    default_backend be_app

backend be_app
    balance leastconn
    option httpchk GET /healthz
    http-check expect status 200
    default-server inter 3s fall 3 rise 2
    server app1 10.0.0.5:8080 check
    server app2 10.0.0.6:8080 check
```

HAProxy's `crt` file must contain the certificate, key, and chain in the right order. `fall 3 rise 2` means three failures to mark down, two successes to mark up — tune to avoid flapping.

---

## 6. HTTP failure patterns

### 502 Bad Gateway

The proxy could not get a valid response from the backend:

- Backend not listening / connection refused.
- Backend closed the connection mid-response.
- Backend sent malformed HTTP.
- TLS between proxy and backend failed.
- Backend crashed.

Check proxy error logs first; they usually name the upstream and reason.

### 503 Service Unavailable

- No healthy backends.
- All backends at max connections.
- Rate limiting/load shedding.
- Maintenance mode.

Check health-check status and backend capacity.

### 504 Gateway Timeout

- Backend accepted the connection but did not respond in time.
- Backend is blocked on a dependency (database, external API).
- Timeout too short for a legitimately slow endpoint.

Distinguish from 502 by checking whether the backend received the request (its logs) and how long it ran.

### Connection reset after keep-alive

Client reuses a connection the server/LB already closed. Common when:

- LB idle timeout < client keep-alive.
- Server restart.
- Backend scaled down.

Fix: client keep-alive shorter than the shortest idle timeout; retry idempotent requests on a fresh connection.

### Redirect loops

`301` to HTTPS, but the proxy forwards `http` internally and the app redirects again. Check `X-Forwarded-Proto` and the app's trusted-proxy config. `curl -v` shows the chain; `--max-redirs` prevents infinite loops.

### 413 Payload Too Large

A proxy body limit (Nginx `client_max_body_size`, HAProxy `tune.maxrewrite`, cloud LB limits) rejects the upload before the app sees it. The app logs nothing; the proxy logs 413.

### 499 (Nginx-specific)

Client closed the connection before the server responded. Usually a client timeout, not a server error, but it signals slow responses.

---

## 7. Debugging matrix

| Symptom | First check | Then |
|---|---|---|
| Name does not resolve | `dig`, `getent hosts` | DNS server, search domain |
| `Connection refused` | `ss -tulpn` on backend | Wrong port, app down, REJECT |
| Timeout | `tcpdump` for SYN/SYN-ACK | Firewall, route, SG/NACL |
| TLS handshake failure | `openssl s_client -servername` | Chain, expiry, SNI, protocol |
| 502 | Proxy error log | Backend listening, protocol |
| 503 | Health-check status | Backend capacity, health endpoint |
| 504 | Backend logs/timing | Dependency latency, timeout |
| 401/403 | Credentials, token expiry | Auth service, policy |
| 404 | Host header, path, router | Wrong backend/pool |
| 429 | `Retry-After` | Rate limit, client behavior |
| Works by IP, not name | SNI/Host/vhost | Certificate SAN, LB rule |
| Works locally, not via LB | `--resolve` to backend | Proxy headers, timeouts |
| Intermittent reset | Keep-alive/timeout alignment | LB idle, server keep-alive |

---

## 8. Command field manual

### `curl`

**Purpose:** send HTTP requests and show the full negotiation.

```bash
curl -v https://api.example.com/health
curl -sS -o /dev/null -w '%{http_code} %{time_total}\n' https://api.example.com
curl --resolve api.example.com:443:10.0.0.5 https://api.example.com/health
curl -H 'Host: api.example.com' http://10.0.0.5/health
```

- `-v` verbose (stderr), `-sS` silent but show errors.
- `-o /dev/null` discard body, `-w` write timing.
- `-I` HEAD, `-X` method, `-d` body, `-H` header.
- `-k` insecure (diagnosis only), `--cacert` custom CA.
- `--max-time` total timeout, `--connect-timeout` connect timeout.
- `--retry` retries transient errors; combine with `--retry-connrefused`.

**Mistake:** using `curl -k` to "fix" a certificate problem. It hides the failure and teaches nothing.

### `openssl s_client`

**Purpose:** inspect the TLS handshake and certificate chain.

```bash
openssl s_client -connect api.example.com:443 -servername api.example.com -showcerts </dev/null
```

- `-servername` sends SNI; required for virtual hosts.
- `-showcerts` prints the chain.
- `-tls1_2`/`-tls1_3` force a version.
- `-alpn h2` tests protocol negotiation.
- `-verify_return_error` makes verification failures fatal (useful in scripts).

**Mistake:** testing by IP without SNI and concluding the certificate is wrong.

### `openssl x509`

```bash
openssl x509 -in cert.pem -noout -text
openssl x509 -in cert.pem -noout -dates -subject -issuer -ext subjectAltName
```

Read SANs, not just CN. Modern clients ignore CN when SAN is present.

### `ss` and `tcpdump`

```bash
ss -tulpn | grep -E ':80|:443'
sudo tcpdump -ni any 'tcp port 443'
```

Use them to confirm the listener and whether the handshake reaches the host.

### Proxy logs

- Nginx: `/var/log/nginx/error.log`, `access.log` with upstream status/time.
- HAProxy: `show stat` on the stats socket, or the configured log format.
- Envoy: access log with `response_flags` (`UF` = upstream connect fail, `UO` = overflow, `UT` = upstream timeout).
- Cloud LB: access logs + target health reason.

The proxy usually knows more than the backend does. Read its error log before changing anything.

---

## 9. Worked scenarios

### Scenario A — certificate works in the browser, fails in `curl`

```bash
curl https://api.example.com/
```

```text
curl: (60) SSL certificate problem: unable to get local issuer certificate
```

```bash
openssl s_client -connect api.example.com:443 -servername api.example.com -showcerts </dev/null
```

If the chain shows only the leaf, the server is not sending the intermediate. Browsers may have cached it; curl/Java do not. Fix the server config to serve `fullchain.pem` (leaf + intermediates).

### Scenario B — 502 after deploying a new backend

Facts: the LB returns 502; the backend is "running".

Evidence:

```bash
curl --resolve api.example.com:443:10.0.0.5 https://api.example.com/health
ss -tulpn | grep 8080
journalctl -u myapp --since '10 min ago'
```

Likely causes: app bound to `127.0.0.1`; app speaks HTTPS but the LB expects HTTP; health check path 404; app crashed after start.

### Scenario C — intermittent 504 under load

Facts: p99 latency spikes, 504s at the LB, backend CPU normal.

Evidence:

- Backend logs show requests taking 30+ s.
- Database connection pool exhausted.
- LB `proxy_read_timeout` is 30 s; backend takes 31 s.
- Retries amplify load.

Fixes: reduce backend latency, increase pool size, set a realistic timeout, add load shedding, and avoid retry storms. Raising the timeout alone just moves the failure.

### Scenario D — HTTP works, HTTPS fails

```bash
curl -v http://api.example.com/     # 200
curl -v https://api.example.com/    # connection refused or reset
```

Check:

```bash
ss -tulpn | grep 443
openssl s_client -connect api.example.com:443 -servername api.example.com
```

If nothing listens on 443, the TLS terminator is not configured. If it listens but resets, check the certificate/key match and the proxy's TLS config. If it works from inside but not outside, check the firewall/security group for 443.

### Scenario E — works by IP, not by name

```bash
curl http://203.0.113.10/health       # 200
curl http://api.example.com/health    # 404
```

The server uses virtual hosts. By IP, it serves the default vhost; by name, it routes to the wrong backend or no backend. Check the `Host` header and the LB/vhost rules. For TLS, check SNI and certificate SAN.

### Scenario F — GPU inference endpoint times out only for large prompts

Facts: small prompts return in 200 ms; long prompts hit a 60 s LB timeout.

This is not an HTTP bug. The backend is doing more work than the timeout allows. Options: streaming responses, asynchronous job API, longer timeout for that route, or admission control. Measure tokens/sec and queue time; do not just raise the timeout. See [AI infrastructure](../09-ai-infrastructure/README.md).

---

## 10. Common mistakes

| Mistake | Why it hurts | Fix |
|---|---|---|
| Testing TLS by IP without SNI | Wrong certificate | Use `-servername` / `--resolve` |
| Deploying leaf cert only | Clients without cached intermediate fail | Serve fullchain |
| `curl -k` in production | Disables verification | Fix trust chain |
| Ignoring SAN | CN-only checks fail | Validate SAN |
| Client keep-alive > LB idle | Random resets | Shorter client keep-alive |
| Retrying POST blindly | Duplicate writes | Idempotency keys, retry policy |
| Health check `/healthz` always 200 | Broken backend stays in rotation | Deep health check |
| One global timeout | Slow endpoints always 504, fast ones wait too long | Per-route timeouts |
| Trusting `X-Forwarded-For` from anyone | IP spoofing, bad rate limits | Trust only proxy CIDRs |
| Blaming TLS for a TCP timeout | Handshake never started | `curl -v`, `tcpdump` |
| Confusing 502/503/504 | Wrong remediation | Read proxy logs and timing |
| No `Host` header on HTTP/1.1 | Default vhost/404 | Always send Host |

---

## 11. Checkpoint

1. What is the difference between 502, 503, and 504?
2. Which curl timing metric isolates TLS handshake time?
3. Why does a missing intermediate break curl but not Chrome?
4. What does SNI do, and why does testing by IP fail without it?
5. What is the difference between L4 and L7 load balancing?
6. Why should client keep-alive be shorter than the LB idle timeout?
7. What does `X-Forwarded-Proto` tell the backend, and why can trusting it be dangerous?
8. Why is a TCP health check weaker than an HTTP health check?
9. What does `openssl s_client -showcerts` reveal that `curl -v` does not?
10. A service returns 200 by IP but 404 by hostname. What is the likely cause?
11. Why can HTTP/2 still suffer head-of-line blocking?
12. What is the risk of 0-RTT in TLS 1.3?

### Answers

1. 502 = bad response/connection from upstream; 503 = no healthy upstream/overload; 504 = upstream timeout.
2. `time_appconnect - time_connect`.
3. Browsers may cache intermediates/AIA-fetch; curl relies on the served chain and local trust store.
4. SNI carries the hostname in the TLS handshake so the server picks the right certificate/vhost.
5. L4 routes connections by IP/port; L7 parses HTTP and can route by host/path/header and terminate TLS.
6. Otherwise the client reuses a connection the LB already closed, causing resets.
7. It tells the app the original scheme (https) so it generates https URLs; trusting client-supplied values enables spoofing/redirect issues.
8. TCP connect only proves the port accepts; the app can be deadlocked or unable to reach its dependencies.
9. The full certificate chain the server sends, including missing intermediates.
10. Virtual-host routing: the default vhost answers by IP, the named vhost is misconfigured or routes elsewhere.
11. All streams share one TCP connection; a lost TCP segment blocks delivery to all streams.
12. Replay: early data can be replayed by an attacker, so it must be limited to idempotent requests.

If you miss more than three, redo sections 2, 3, and 5 before Lab L2.6.
