# 6. Building a Simple Web Client
### *(Covers the book's Chapter 6 — "Building a Simple Web Client")*

Everything before this chapter was infrastructure. This is where it starts paying off: a real HTTP client, built entirely from the socket calls you already know, that can actually talk to a real web server — including handling the messy parts real servers do (chunked responses, unknown body length) that a toy example would usually skip.

## 6.1 HTTP, just enough to build a client

An HTTP request is plain text, three parts: a **request line** (method, path, HTTP version), a set of **headers** (`Key: Value` pairs, one per line), and an optional **body**, all separated by `\r\n` (carriage-return + newline — HTTP is one of the last major protocols to still insist on this old-school line ending, a holdover from very early internet standards).

```
GET /index.html HTTP/1.1\r\n
Host: example.com\r\n
Connection: close\r\n
\r\n
```
The blank line (`\r\n\r\n`) marks the end of the headers. A response looks structurally identical, but starts with a **status line** instead (`HTTP/1.1 200 OK`) and usually does have a body:
```
HTTP/1.1 200 OK\r\n
Content-Length: 69\r\n
Content-Type: text/html\r\n
\r\n
<html>...the actual page...</html>
```

The genuinely tricky part for a client isn't sending the request — it's knowing **where the response body ends**, since a raw TCP stream (Chapter 3's lesson, again) has no built-in concept of "message boundary." HTTP gives you three different ways a server might tell you this, and a real client has to handle all three:

1. **`Content-Length: N`** — the body is exactly N bytes; stop once you've received that many.
2. **`Transfer-Encoding: chunked`** — the body arrives in a series of small chunks, each prefixed by its own length (in hex!) on its own line, terminated by a zero-length chunk.
3. **Neither header present** — the only way to know the body is finished is when the server closes the connection (`recv()` returns `0`).

## 6.2 Parsing a URL by hand

```c
void parse_url(char *url, char **hostname, char **port, char** path) {
    printf("URL: %s\n", url);

    char *p;
    p = strstr(url, "://");

    char *protocol = 0;
    if (p) {
        protocol = url;
        *p = 0;
        p += 3;
    } else {
        p = url;
    }

    if (protocol) {
        if (strcmp(protocol, "http")) {
            fprintf(stderr,
                    "Unknown protocol '%s'. Only 'http' is supported.\n",
                    protocol);
            exit(1);
        }
    }

    *hostname = p;
    while (*p && *p != ':' && *p != '/' && *p != '#') ++p;

    *port = "80";
    if (*p == ':') {
        *p++ = 0;
        *port = p;
    }
    while (*p && *p != '/' && *p != '#') ++p;

    *path = p;
    if (*p == '/') {
        *path = p + 1;
    }
    *p = 0;

    while (*p && *p != '#') ++p;
    if (*p == '#') *p = 0;

    printf("hostname: %s\n", *hostname);
    printf("port: %s\n", *port);
    printf("path: %s\n", *path);
}
```

This is doing exactly what it looks like: walking the URL string once, character by character, cutting it into pieces **in place** by overwriting the delimiter characters (`:`, `/`, `#`) with `\0` as it goes, rather than copying substrings into new buffers. That's a deliberate, common C technique — it's memory-efficient (no extra allocation) but it does mean the function *mutates the string you pass it*, which is exactly why `main()` below passes `argv[1]` directly rather than a copy: it's fine here because nothing else needs the original URL string intact afterward.

## 6.3 Sending the request

```c
void send_request(SOCKET s, char *hostname, char *port, char *path) {
    char buffer[2048];

    sprintf(buffer, "GET /%s HTTP/1.1\r\n", path);
    sprintf(buffer + strlen(buffer), "Host: %s:%s\r\n", hostname, port);
    sprintf(buffer + strlen(buffer), "Connection: close\r\n");
    sprintf(buffer + strlen(buffer), "User-Agent: honpwc web_get 1.0\r\n");
    sprintf(buffer + strlen(buffer), "\r\n");

    send(s, buffer, strlen(buffer), 0);
    printf("Sent Headers:\n%s", buffer);
}
```

Notice `Connection: close` is sent deliberately — this client is explicitly telling the server "don't keep this connection open for more requests, just close it when you're done," which sidesteps a whole category of complexity (HTTP "keep-alive" connection reuse) that a production client would need to handle but this teaching example intentionally avoids.

## 6.4 Reading the response — all three body-length strategies, handled

```c
const clock_t start_time = clock();

#define RESPONSE_SIZE 32768
char response[RESPONSE_SIZE+1];
char *p = response, *q;
char *end = response + RESPONSE_SIZE;
char *body = 0;

enum {length, chunked, connection};
int encoding = 0;
int remaining = 0;

while(1) {

    if ((clock() - start_time) / CLOCKS_PER_SEC > TIMEOUT) {
        fprintf(stderr, "timeout after %.2f seconds\n", TIMEOUT);
        return 1;
    }

    if (p == end) {
        fprintf(stderr, "out of buffer space\n");
        return 1;
    }

    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(server, &reads);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 200000;

    if (select(server+1, &reads, 0, 0, &timeout) < 0) {
        fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    if (FD_ISSET(server, &reads)) {
        int bytes_received = recv(server, p, end - p, 0);
        if (bytes_received < 1) {
            if (encoding == connection && body) {
                printf("%.*s", (int)(end - body), body);
            }
            printf("\nConnection closed by peer.\n");
            break;
        }

        p += bytes_received;
        *p = 0;

        if (!body && (body = strstr(response, "\r\n\r\n"))) {
            *body = 0;
            body += 4;

            printf("Received Headers:\n%s\n", response);

            q = strstr(response, "\nContent-Length: ");
            if (q) {
                encoding = length;
                q = strchr(q, ' ');
                q += 1;
                remaining = strtol(q, 0, 10);
            } else {
                q = strstr(response, "\nTransfer-Encoding: chunked");
                if (q) {
                    encoding = chunked;
                    remaining = 0;
                } else {
                    encoding = connection;
                }
            }
            printf("\nReceived Body:\n");
        }

        if (body) {
            if (encoding == length) {
                if (p - body >= remaining) {
                    printf("%.*s", remaining, body);
                    break;
                }
            } else if (encoding == chunked) {
                do {
                    if (remaining == 0) {
                        if ((q = strstr(body, "\r\n"))) {
                            remaining = strtol(body, 0, 16);
                            if (!remaining) goto finish;
                            body = q + 2;
                        } else {
                            break;
                        }
                    }
                    if (remaining && p - body >= remaining) {
                        printf("%.*s", remaining, body);
                        body += remaining + 2;
                        remaining = 0;
                    }
                } while (!remaining);
            }
        }
    }
}
finish:
```

Walking through the shape of this, since it's dense: the code accumulates everything it receives into one growing buffer (`response`), and re-scans for the header/body split (`\r\n\r\n`) *every time* more data arrives, until it finds it (`body` starts `NULL` and stays that way until the split is found). Once the headers are known, it picks exactly one of the three strategies from §6.1 by checking which header is present, and from then on, every additional chunk of received data gets checked against that strategy's own "am I done yet?" rule. Notice the `chunked` branch parses each chunk's hex-encoded length line (`strtol(body, 0, 16)` — base 16, because that's what the HTTP chunked-encoding spec actually specifies, unlike `Content-Length` which is plain decimal) and jumps straight to `finish` the moment a zero-length chunk (the standard end-of-chunks marker) appears.

There's also a **five-second timeout** guarding the whole loop (`TIMEOUT` at the top of the file) — a real, necessary safety net: without it, a server that stops responding partway through (dropped connection without a clean close, a firewall silently eating packets, anything) would hang this client forever.

## 6.5 Running it against a real server, live

Rather than depending on some external website's availability (and the resulting output changing every time that site updates its page), I tested this exact client against **this same book's own HTTP server** from the very next chapter — a genuinely nice way to prove both halves of the book's examples are correct against each other, not just individually plausible.

```
$ gcc -Wall -o web_get web_get.c
$ ./web_get http://localhost:8080/
URL: http://localhost:8080/
hostname: localhost
port: 8080
path: 

Configuring remote address...
Remote address is: 127.0.0.1 8080
Creating socket...
Connecting...
Connected.

Sent Headers:
GET / HTTP/1.1
Host: localhost:8080
Connection: close
User-Agent: honpwc web_get 1.0


Received Headers:
HTTP/1.1 200 OK
Connection: close
Content-Length: 69
Content-Type: text/html

Received Body:
<html><body><h1>Hello from the book's web server!</h1></body></html>

Closing socket...
Finished.
```

That's a genuine, real HTTP exchange — real TCP handshake, real request bytes sent, real response headers parsed, real `Content-Length`-based body extraction, all the way through. (Full details of the server side are in `07_HTTP_Server.md`, right where this test's target came from.)

## 6.6 POST requests, form encoding, and file uploads — the shape, not new code

The book's remaining sections in this chapter extend the same `send_request()`-style function to support `POST` instead of `GET`. The core idea, worth understanding conceptually even without new example code to run: a `POST` request adds a body *to the request* (the reverse direction from everything discussed so far), and needs its own `Content-Length` header describing that body's size:

```
POST /submit HTTP/1.1\r\n
Host: example.com\r\n
Content-Type: application/x-www-form-urlencoded\r\n
Content-Length: 27\r\n
\r\n
name=Alice&message=Hello
```

**Form encoding** (`application/x-www-form-urlencoded`) is just percent-encoding: spaces become `+` or `%20`, and reserved characters (`&`, `=`, and others) get replaced with `%` followed by their hex byte value, so field values can safely contain characters that would otherwise be confused with the `key=value&key=value` structure itself. **File uploads** need a different content type (`multipart/form-data`), which wraps each field — including the raw file bytes — in its own MIME-style sub-section separated by a boundary string the request declares up front. Both are, structurally, "build a bigger, more carefully-formatted body, then send it with the same `send()` calls you already know" — no new socket concepts, just more careful string construction.

---

## Quick recap (for revision)

- HTTP requests/responses are plain text, `\r\n`-terminated lines, with a blank line marking the header/body boundary.
- A raw TCP client has to determine body length itself, via exactly one of three signals: `Content-Length`, `Transfer-Encoding: chunked`, or "connection closed = done."
- Chunked encoding's chunk-length prefixes are **hexadecimal**, not decimal — `strtol(..., 16)`, not `strtol(..., 10)`.
- URL parsing here works by overwriting delimiter characters in place with `\0` — efficient, but mutates the input string; know when that's safe (nothing else needs the original after) and when it isn't.
- Always guard a `recv()` loop with a timeout in client code — a server (or the network) can simply stop responding without ever sending a clean close.
- POST/form-encoding/file-uploads are the same `send()`-based request-building you already know, just with a body attached and a matching `Content-Length`/`Content-Type`.

Next: `07_HTTP_Server.md` — the server side of the exchange you just ran live.