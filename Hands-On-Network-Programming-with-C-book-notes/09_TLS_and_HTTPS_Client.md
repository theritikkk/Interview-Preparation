# 9. TLS, OpenSSL, and a Real HTTPS Client
### *(Covers the book's Chapter 9 — "Loading Secure Web Pages with HTTPS and OpenSSL")*

Every socket program so far sends plain, readable bytes over the wire — anyone watching the network traffic could read every request and response. This chapter adds a layer that scrambles the bytes so only the two endpoints can read them: **TLS** (Transport Layer Security — the modern name; you'll still hear people say "SSL," its predecessor, informally to mean the same thing).

## 9.1 What TLS actually buys you, in plain terms

Three separate guarantees, worth keeping distinct because they solve different problems:

- **Encryption**: an eavesdropper watching the raw network traffic sees scrambled noise, not your data.
- **Integrity**: if even one bit of the encrypted data is tampered with in transit, the receiving end detects it and rejects the message, rather than silently accepting corrupted/altered data.
- **Authentication**: via **certificates**, the client gets cryptographic proof of *who it's actually talking to* — not just "someone claiming to be example.com," but someone who can prove it, backed by a chain of trust rooted in a small set of well-known Certificate Authorities your OS/browser already trusts.

All three matter together: encryption alone doesn't help if you're encrypting a conversation with an *impostor* — authentication is what stops that. This is why "https://" sites show a padlock: the browser did all three checks and is telling you so.

## 9.2 OpenSSL, and the shape every TLS program shares

**OpenSSL** is the (extremely widely used) library that does the actual cryptographic heavy lifting — you're not implementing TLS yourself, you're calling into a library that already has. Every TLS-using program in this chapter shares the same skeleton, which is worth memorizing as its own pattern, layered *on top of* the plain socket setup you already know:

```c
#include <openssl/ssl.h>

int main(int argc, char *argv[]) {

    printf("OpenSSL version: %s\n", OpenSSL_version(SSLEAY_VERSION));

    return 0;
}
```

```
$ gcc -Wall -o openssl_version openssl_version.c -lssl -lcrypto
$ ./openssl_version
OpenSSL version: OpenSSL 3.0.13 30 Jan 2024
```

Note the **`-lssl -lcrypto`** on the compile line — every program in this chapter needs both of OpenSSL's libraries linked in, something easy to forget and get a wall of "undefined reference" linker errors from if you do.

The real pattern, once you get past this one-liner sanity check:

```c
SSL_library_init();
OpenSSL_add_all_algorithms();
SSL_load_error_strings();

SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());   // or TLS_server_method() on the server side
```

`SSL_CTX` ("SSL context") is a factory/configuration object — you make one of these once, then create individual `SSL` objects (one per connection) from it. This mirrors the socket/`addrinfo` relationship from earlier chapters: one reusable "how do I want connections configured" object, many individual per-connection objects created from it.

## 9.3 Wrapping a plain socket in TLS

The genuinely elegant part of how OpenSSL integrates with the sockets API you already know: **you still do the normal `getaddrinfo()`/`socket()`/`connect()` dance exactly as before** — TLS wraps *around* an ordinary, already-connected socket, rather than replacing any of that:

```c
SSL *ssl = SSL_new(ctx);
SSL_set_fd(ssl, socket_peer);      // socket_peer is a normal, already-connect()ed socket
if (SSL_connect(ssl) == -1) {      // THIS is the actual TLS handshake
    fprintf(stderr, "SSL_connect() failed.\n");
    ERR_print_errors_fp(stderr);
    return 1;
}
```

From this point on, **`SSL_read()`/`SSL_write()` replace `recv()`/`send()`** — same idea, same general calling convention, but operating on the encrypted channel instead of the raw socket. Everything else you've learned (checking return values, handling partial transfers, the `select()`-based multiplexing pattern) carries over unchanged; only the two data-transfer function names change.

## 9.4 Inspecting the server's certificate — a real handshake, real output

Once `SSL_connect()` succeeds, you can pull the certificate the server presented and look at who it claims to be:

```c
X509 *cert = SSL_get_peer_certificate(ssl);
if (!cert) {
    fprintf(stderr, "SSL_get_peer_certificate() failed.\n");
    return 1;
}

char *tmp;
if ((tmp = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0))) {
    printf("subject: %s\n", tmp);
    OPENSSL_free(tmp);
}

if ((tmp = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0))) {
    printf("issuer: %s\n", tmp);
    OPENSSL_free(tmp);
}

X509_free(cert);
```

`X509` is the standard certificate format's name in OpenSSL's API (named after the ITU-T standard that defines it). **Subject** is who the certificate is *for*; **issuer** is who signed/vouches for it — for a real public website, these are normally different (the issuer is a Certificate Authority like Let's Encrypt or DigiCert); for a **self-signed** certificate (which is exactly what I generated to test this chapter, since I don't have a real domain to get a CA-signed cert for), subject and issuer are identical — the certificate is vouching for itself, which is fine for local testing but is exactly what browsers warn you about ("not secure," "certificate not trusted") if you try it on the open web, since anyone can generate a certificate claiming to be anyone.

**Real run — a genuine TLS handshake, against a local test server I set up (full server-side details in `10_HTTPS_Server.md`), with a real, complete HTTP exchange happening over the encrypted channel:**

```
$ gcc -Wall -o https_simple https_simple.c -lssl -lcrypto
$ ./https_simple localhost 8080
Configuring remote address...
Remote address is: 127.0.0.1 http-alt
Creating socket...
Connecting...
Connected.

SSL/TLS using TLS_AES_256_GCM_SHA384
subject: /CN=localhost
issuer: /CN=localhost
Sent Headers:
GET / HTTP/1.1
Host: localhost:8080
Connection: close
User-Agent: https_simple

Received (17 bytes): 'HTTP/1.1 200 OK
'
Received (19 bytes): 'Connection: close
'
Received (20 bytes): 'Content-Length: 69
'
Received (25 bytes): 'Content-Type: text/html
'
Received (2 bytes): '
'
Received (69 bytes): '<html><body><h1>Hello from the book's web server!</h1></body></html>
'

Connection closed by peer.

Closing socket...
Finished.
```

Two things worth calling out in that real output: `TLS_AES_256_GCM_SHA384` — the actual cipher suite the handshake negotiated (AES-256 for the bulk encryption, GCM for integrity, SHA-384 for the handshake's own hashing) — and `subject: /CN=localhost` / `issuer: /CN=localhost` being identical, exactly confirming this is a self-signed certificate as described above. Also notice each `SSL_read()` call here returned a *different-sized* chunk (17 bytes, then 19, then 20...) rather than the whole response at once — the same "a stream doesn't preserve your write boundaries" lesson from Chapter 3, now demonstrated over an encrypted channel instead of a plain one.

## 9.5 Server Name Indication (SNI) — one certificate question this code answers

```c
if (!SSL_set_tlsext_host_name(ssl, hostname)) {
    fprintf(stderr, "SSL_set_tlsext_host_name() failed.\n");
    ERR_print_errors_fp(stderr);
    return 1;
}
```

This one call, made *before* `SSL_connect()`, tells the server which hostname you're trying to reach — sent as part of the handshake, before encryption is even established. Why does a client connecting by IP address need to also say the hostname? Because a single IP address commonly hosts **many** different HTTPS websites (this is the norm today, not an edge case — shared hosting, CDNs, and load balancers all rely on it), each with its own certificate. Without SNI, the server would have to guess which certificate to present before it even knows what site you want; with it, the server picks the right certificate for the right site up front. Skip this call and connect to a real multi-tenant HTTPS server, and you'll typically get *some* certificate back — just not necessarily the one for the site you actually meant to reach.

## 9.6 `tls_get_cert.c` — a minimal, focused certificate inspector

Strip the HTTP request/response handling out of `https_simple.c` entirely, and you're left with a tiny, genuinely useful utility: connect, handshake, print the certificate, done.

```
$ gcc -Wall -o tls_get_cert tls_get_cert.c -lssl -lcrypto
$ ./tls_get_cert localhost 8080
Configuring remote address...
Remote address is: 127.0.0.1 http-alt
Creating socket...
Connecting...
Connected.

SSL/TLS using TLS_AES_256_GCM_SHA384
subject: /CN=localhost
issuer: /CN=localhost

Closing socket...
Finished.
```

This is a genuinely practical pattern to keep in your toolbox: a stripped-down "just show me the cert" program is exactly the kind of thing worth having on hand when debugging a real certificate problem (expired cert, wrong hostname, unexpected issuer) — smaller and more focused than reaching for a full browser or `openssl s_client` every time.

## 9.7 `tls_client.c` — the generic (non-HTTP) version

The book also includes a version that's the direct TLS-wrapped analogue of Chapter 3's interactive `tcp_client.c` — same `select()`-multiplexed "watch the socket and my own keyboard" loop, same everything, just with `SSL_read()`/`SSL_write()` standing in for `recv()`/`send()`, and no HTTP-specific request-building at all. If you've followed the pattern in §9.3–9.4, there's genuinely nothing new in this file beyond what you've already seen — it's included in the book mainly to show that TLS-wrapping composes with *any* stream-based protocol, not just HTTP.

---

## Quick recap (for revision)

- TLS gives you three separate guarantees: encryption (nobody eavesdrops), integrity (no silent tampering), authentication (proof of identity via certificates) — remember they're separable, since "encrypted" alone doesn't mean "verified who you're talking to."
- TLS wraps an already-connected plain socket — you still do `getaddrinfo`/`socket`/`connect` exactly as before, then hand the connected socket to `SSL_set_fd()`.
- `SSL_read()`/`SSL_write()` replace `recv()`/`send()` after the handshake — same stream-not-messages behavior as plain sockets underneath.
- Self-signed certificates have identical subject and issuer — fine for local testing, rightly distrusted by browsers on the open web.
- `SSL_set_tlsext_host_name()` (SNI) tells a server which hostname you want *before* encryption starts, letting one IP serve the correct certificate for one of many possible sites.
- Compile with `-lssl -lcrypto` — forgetting these produces a wall of linker errors that has nothing obviously to do with a missing flag if you don't already know to look for it.

Next: `10_HTTPS_Server.md` — the server side of the exact handshake you just ran, self-signed certificate generation included.