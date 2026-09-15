# 10. Implementing a Secure Web Server
### *(Covers the book's Chapter 10 — "Implementing a Secure Web Server")*

Chapter 7's HTTP server, plus Chapter 9's TLS wrapping, equals this chapter: the exact same `select()`-based multi-client file server, now speaking HTTPS. If you've read both of those, almost everything here will look immediately familiar — the interesting new material is entirely about **certificates**: generating one, loading it, and presenting it during the handshake.

## 10.1 Getting a certificate — the step that has to happen outside your C code

Before the server can run at all, it needs a certificate and a matching private key, in PEM format (a base64-wrapped text format you'll recognize by its `-----BEGIN CERTIFICATE-----` header). For genuine public-internet use, you'd get one issued by a real Certificate Authority. For local development and testing — exactly the situation these notes are in — you generate your own **self-signed** certificate with the `openssl` command-line tool:

```
$ openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 365 -nodes -subj "/CN=localhost"
```

Breaking that down: `-x509` says "generate a certificate, not just a signing request"; `-newkey rsa:2048` generates a fresh 2048-bit RSA key alongside it; `-keyout`/`-out` name the two output files; `-days 365` sets how long it's valid; `-nodes` ("no DES") skips encrypting the private key file itself with a passphrase — convenient for a test server that needs to start unattended, but notably **not** what you'd want for a private key protecting anything real; `-subj "/CN=localhost"` sets the certificate's subject directly from the command line instead of interactively prompting for it (`CN` = "Common Name," the field that's supposed to match the hostname clients will connect to).

This is real, standard tooling — the exact same command (with a real hostname and a CA-issued cert instead) is how production servers get their certificates configured too; the only thing specific to local testing here is the *self-signed* part.

## 10.2 Loading the certificate into the server

```c
SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());   // note: SERVER method, not client

if (!SSL_CTX_use_certificate_file(ctx, "cert.pem" , SSL_FILETYPE_PEM)
|| !SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM)) {
    fprintf(stderr, "SSL_CTX_use_certificate_file() failed.\n");
    ERR_print_errors_fp(stderr);
    return 1;
}
```

Two file loads, one context — done once at startup, then every subsequent client connection's `SSL` object (created from this `ctx`) automatically presents this same certificate during its handshake. The only server-specific change from Chapter 9's client-side `SSL_CTX_new()` call is `TLS_server_method()` instead of `TLS_client_method()` — the rest of the setup boilerplate (`SSL_library_init()`, etc.) is identical.

## 10.3 The server loop — Chapter 7's structure, with TLS bolted on at exactly two points

Compare this to `04_TCP_Client_Server.md`'s / `07_HTTP_Server.md`'s server loop and the changes are minimal and localized:

```c
struct client_info {
    socklen_t address_length;
    struct sockaddr_storage address;
    SOCKET socket;
    SSL *ssl;                          // <-- new: one SSL object per client
    char request[MAX_REQUEST_SIZE + 1];
    int received;
    struct client_info *next;
};
```
```c
// Accepting a connection: identical up through accept(), then:
client->ssl = SSL_new(ctx);
SSL_set_fd(client->ssl, client->socket);
if (SSL_accept(client->ssl) != 1) {        // <-- the actual TLS handshake, server side
    ERR_print_errors_fp(stderr);
    drop_client(client);
} else {
    printf("New connection from %s.\n", get_client_address(client));
    printf("SSL connection using %s\n", SSL_get_cipher(client->ssl));
}
```
```c
// Reading a request: SSL_read() instead of recv(), otherwise identical
int r = SSL_read(client->ssl, client->request + client->received,
                  MAX_REQUEST_SIZE - client->received);
```
```c
// Serving a file: every send() becomes SSL_write(), otherwise byte-for-byte
// identical to Chapter 7's serve_resource() — same Content-Type lookup table,
// same ".." traversal guard, same 400/404 handling
```

**`SSL_accept()`** is the server-side counterpart to the client's `SSL_connect()` from Chapter 9 — this is where the actual handshake happens: the server presents its certificate (the one loaded in §10.2), negotiates a cipher suite, and establishes the encrypted channel, all before a single byte of the actual HTTP request is read.

And `drop_client()` gets one extra line compared to Chapter 7's version:
```c
void drop_client(struct client_info *client) {
    SSL_shutdown(client->ssl);   // <-- new: close the TLS session cleanly first
    CLOSESOCKET(client->socket);
    SSL_free(client->ssl);       // <-- new: free the SSL object
    /* ...unchanged linked-list removal, identical to Chapter 7... */
}
```

Everything else — the linked-list client tracking, `wait_on_clients()`'s `select()` call, the `Content-Type` lookup table, the directory-traversal guard, even the exact same repeated `%u`/`size_t` format-specifier warning from Chapter 7 (this server's `serve_resource()` is close enough to a copy-paste of Chapter 7's that the same latent bug comes along for the ride) — is unchanged. This is a genuinely good way to *learn* TLS integration: seeing exactly how little of a working, tested server's structure needs to change to add encryption makes it much less intimidating than it initially sounds.

## 10.4 Running it — a real TLS handshake, both ends, live

```
$ openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 365 -nodes -subj "/CN=localhost"
$ gcc -Wall -o https_server https_server.c -lssl -lcrypto
$ ./https_server &
Configuring local address...
Creating socket...
Binding socket to local address...
Listening...
```

**A real browser, or `curl`, connecting (skipping certificate verification with `-k`, since this is a self-signed cert a real browser would otherwise reject exactly as designed):**
```
$ curl -sk -i https://localhost:8080/
HTTP/1.1 200 OK
Connection: close
Content-Length: 69
Content-Type: text/html

<html><body><h1>Hello from the book's web server!</h1></body></html>
```

**And, in the server's own log, the full handshake-then-request sequence:**
```
New connection from 127.0.0.1.
SSL connection using TLS_AES_256_GCM_SHA384
serve_resource 127.0.0.1 /
```

**Using the book's own `https_simple.c` client from the previous chapter** (rather than `curl`) makes the round trip complete end-to-end with both halves being code you've now read in full — full transcript already shown in `09_TLS_and_HTTPS_Client.md` §9.4. Same cipher suite negotiated (`TLS_AES_256_GCM_SHA384`), same self-signed certificate presented and correctly parsed on the client side (`subject: /CN=localhost`, `issuer: /CN=localhost`) — genuinely the same handshake, viewed from both sides.

## 10.5 `tls_time_server.c` — the non-HTTP version

Just as Chapter 9 had a generic `tls_client.c` alongside the HTTP-aware `https_simple.c`, this chapter's `tls_time_server.c` is the TLS-wrapped version of Chapter 2's original one-shot time server — accept one connection, handshake, send the time, close. It needs the exact same `cert.pem`/`key.pem` pair in its working directory. If `https_server.c`'s multi-client, file-serving structure feels like a lot to take in at once, this smaller file is the "just the TLS parts, none of the HTTP or multi-client complexity" version worth reading first.

---

## Quick recap (for revision)

- A real certificate needs a real Certificate Authority; a self-signed one (generated locally with `openssl req -x509 ...`) is fine for development/testing but will (correctly) be rejected by browsers and by any client that actually verifies the chain of trust.
- Server-side setup mirrors the client's almost exactly, with one flip: `TLS_server_method()` instead of `TLS_client_method()`, and `SSL_CTX_use_certificate_file()`/`SSL_CTX_use_PrivateKey_file()` to load what the client only ever *received*.
- `SSL_accept()` is the server-side handshake call — the direct counterpart to the client's `SSL_connect()`.
- Converting a working plain-HTTP server to HTTPS is a small, localized diff: one new field (`SSL *ssl` per client), `SSL_new`/`SSL_set_fd`/`SSL_accept` at connection time, `SSL_read`/`SSL_write` replacing `recv`/`send`, and `SSL_shutdown`/`SSL_free` added to cleanup — the request-parsing and file-serving logic itself doesn't change at all.
- Testing HTTPS locally needs `curl -k` (or your browser's "proceed anyway" click) specifically because self-signed certs are *supposed* to be untrusted by default — that's the security model working as intended, not a bug to work around permanently.

Next: `11_SSH_with_libssh.md` — a different protocol, a different library, but yet another instance of the same "wrap a plain socket in something protocol-aware" pattern.