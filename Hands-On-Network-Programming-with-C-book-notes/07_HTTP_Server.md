# 7. Building a Simple Web Server
### *(Covers the book's Chapter 7 — "Building a Simple Web Server")*

The other half of Chapter 6's exchange: a real, if deliberately minimal, static-file HTTP server — multi-client, `select()`-based, serving actual files off disk with correct content types. And, as a bonus from genuinely testing it rather than just reading it: a real compiler warning caught live in the book's own published code, which turns into a good lesson on its own.

## 7.1 Serving the right `Content-Type`

Browsers decide how to *render* a response almost entirely by trusting its `Content-Type` header, not by guessing from the content — so a server needs to get this right. The book's approach: a lookup table keyed on file extension.

```c
const char *get_content_type(const char* path) {
    const char *last_dot = strrchr(path, '.');
    if (last_dot) {
        if (strcmp(last_dot, ".css") == 0) return "text/css";
        if (strcmp(last_dot, ".csv") == 0) return "text/csv";
        if (strcmp(last_dot, ".gif") == 0) return "image/gif";
        if (strcmp(last_dot, ".htm") == 0) return "text/html";
        if (strcmp(last_dot, ".html") == 0) return "text/html";
        if (strcmp(last_dot, ".ico") == 0) return "image/x-icon";
        if (strcmp(last_dot, ".jpeg") == 0) return "image/jpeg";
        if (strcmp(last_dot, ".jpg") == 0) return "image/jpeg";
        if (strcmp(last_dot, ".js") == 0) return "application/javascript";
        if (strcmp(last_dot, ".json") == 0) return "application/json";
        if (strcmp(last_dot, ".png") == 0) return "image/png";
        if (strcmp(last_dot, ".pdf") == 0) return "application/pdf";
        if (strcmp(last_dot, ".svg") == 0) return "image/svg+xml";
        if (strcmp(last_dot, ".txt") == 0) return "text/plain";
    }
    return "application/octet-stream";
}
```

`strrchr()` (**r**everse `strchr`) finds the *last* dot in the path, correctly handling filenames like `archive.tar.gz` (you want `.gz`, not the first dot). Anything not on the list falls back to `application/octet-stream` — HTTP's "I don't know what this is, just offer it as a raw download" type, a sensible safe default rather than guessing wrong.

## 7.2 Tracking multiple clients: a linked list, not an array

Beej's Guide's `pollserver.c` (if you've been through those notes) tracked clients in a growable **array**. This server tracks them in a **linked list** instead — a genuinely different, equally valid design choice worth comparing:

```c
struct client_info {
    socklen_t address_length;
    struct sockaddr_storage address;
    SOCKET socket;
    char request[MAX_REQUEST_SIZE + 1];
    int received;
    struct client_info *next;
};

static struct client_info *clients = 0;

struct client_info *get_client(SOCKET s) {
    struct client_info *ci = clients;

    while(ci) {
        if (ci->socket == s)
            break;
        ci = ci->next;
    }

    if (ci) return ci;
    struct client_info *n =
        (struct client_info*) calloc(1, sizeof(struct client_info));

    if (!n) {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
    }

    n->address_length = sizeof(n->address);
    n->next = clients;
    clients = n;
    return n;
}


void drop_client(struct client_info *client) {
    CLOSESOCKET(client->socket);

    struct client_info **p = &clients;

    while(*p) {
        if (*p == client) {
            *p = client->next;
            free(client);
            return;
        }
        p = &(*p)->next;
    }

    fprintf(stderr, "drop_client not found.\n");
    exit(1);
}
```

`get_client(-1)` (called from `main()` for a brand-new connection, before it even has a real socket number yet) is a nice small trick: passing a socket number that can never match a real client (`-1`) guarantees the `while(ci)` search fails, so `get_client()` falls through to allocating a fresh node every time it's asked for `-1` — one function doing double duty as both "find an existing client" and "make me a new one," depending on what you ask it to find.

`drop_client()`'s pointer-to-pointer trick (`struct client_info **p = &clients;`) is the standard idiom for removing a node from a singly-linked list without needing any special-case code for "removing the very first node" versus "removing a node in the middle" — `p` always points at *the pointer that currently points at the node we're checking*, whether that's the list's head (`clients` itself) or some earlier node's `next` field, so the same one line (`*p = client->next;`) correctly patches either case.

**A genuine design note worth carrying forward**: this server's `create_socket()` function does **not** call `setsockopt(..., SO_REUSEADDR, ...)` — unlike Beej's Guide, which set this on every single server example. That omission is deliberate on the book's part, not an oversight: Chapter 13 (`13_Pitfalls_and_Tips.md`) specifically revisits this exact question with two named example programs, `server_reuse.c` and `server_noreuse.c`, to let you feel the difference directly. Flagging it here so it doesn't feel like an inconsistency when you hit that chapter later.

## 7.3 Waiting on everyone at once

```c
fd_set wait_on_clients(SOCKET server) {
    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(server, &reads);
    SOCKET max_socket = server;

    struct client_info *ci = clients;

    while(ci) {
        FD_SET(ci->socket, &reads);
        if (ci->socket > max_socket)
            max_socket = ci->socket;
        ci = ci->next;
    }

    if (select(max_socket+1, &reads, 0, 0, 0) < 0) {
        fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }

    return reads;
}
```

Exactly the pattern you've now seen several times: walk every known client, add each one's socket to the set, track the highest fd seen (`select()`'s own requirement), call `select()` once, return the result. The only new wrinkle versus earlier chapters is that the client list is a linked list being walked instead of an array being iterated — the *concept* (build the set fresh every time, from whatever the current live-client bookkeeping structure is) is identical either way.

## 7.4 Serving a file, byte-correct headers included

```c
void serve_resource(struct client_info *client, const char *path) {

    printf("serve_resource %s %s\n", get_client_address(client), path);

    if (strcmp(path, "/") == 0) path = "/index.html";

    if (strlen(path) > 100) {
        send_400(client);
        return;
    }

    if (strstr(path, "..")) {
        send_404(client);
        return;
    }

    char full_path[128];
    sprintf(full_path, "public%s", path);

    FILE *fp = fopen(full_path, "rb");

    if (!fp) {
        send_404(client);
        return;
    }

    fseek(fp, 0L, SEEK_END);
    size_t cl = ftell(fp);
    rewind(fp);

    const char *ct = get_content_type(full_path);

#define BSIZE 1024
    char buffer[BSIZE];

    sprintf(buffer, "HTTP/1.1 200 OK\r\n");
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "Connection: close\r\n");
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "Content-Length: %u\r\n", cl);
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "Content-Type: %s\r\n", ct);
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "\r\n");
    send(client->socket, buffer, strlen(buffer), 0);

    int r = fread(buffer, 1, BSIZE, fp);
    while (r) {
        send(client->socket, buffer, r, 0);
        r = fread(buffer, 1, BSIZE, fp);
    }

    fclose(fp);
    drop_client(client);
}
```

Two security-relevant lines worth stopping on: `if (strstr(path, ".."))` — rejecting any path containing `..` — is a **directory traversal** guard. Without it, a request for `/../../../etc/passwd` would happily walk right out of the `public/` folder and read arbitrary files elsewhere on the server's disk. This is a genuinely real, historically common web-server vulnerability class, being defended against in eleven characters of C. And `if (strlen(path) > 100)` guards the fixed-size `full_path[128]` buffer a few lines later against overflowing — simple, but exactly the kind of check that's easy to skip and expensive to regret.

**And, live, a bug I actually found running this exact code** (not something the book got wrong on purpose to teach you — a genuine, tiny portability issue):
```
web_server.c: In function 'serve_resource':
web_server.c:250:39: warning: format '%u' expects argument of type 'unsigned int',
but argument 3 has type 'size_t' {aka 'long unsigned int'} [-Wformat=]
    sprintf(buffer, "Content-Length: %u\r\n", cl);
                                     ~^       ~~
```
`cl` is declared `size_t`, which is a 64-bit unsigned type on this (and most modern) 64-bit systems, but the format specifier used is `%u`, which expects a 32-bit `unsigned int`. On the vast majority of real files (anything under about 4GB) this happens to work anyway, because the actual numeric value fits fine in the bytes `printf` reads — but it's undefined behavior by the C standard, and exactly the kind of latent bug that can misbehave in genuinely confusing ways on a different compiler, a different platform, or (eventually) a file large enough for the mismatch to actually matter. The fix, if you're typing this yourself: `%zu` is the correct, portable format specifier for a `size_t` — or explicitly cast: `(unsigned)cl`. This is a nice, real-world example of exactly why `-Wall` (which is how this warning surfaced) is worth compiling with even for "simple" example code, a point that'll come back explicitly in Chapter 13.

## 7.5 Running it — real multi-part test

```
$ mkdir public && echo '<html><body><h1>Hello from the book'"'"'s web server!</h1></body></html>' > public/index.html
$ gcc -Wall -o web_server web_server.c
$ ./web_server &
Configuring local address...
Creating socket...
Binding socket to local address...
Listening...
```

**A successful request:**
```
$ curl -s -i http://localhost:8080/
HTTP/1.1 200 OK
Connection: close
Content-Length: 69
Content-Type: text/html

<html><body><h1>Hello from the book's web server!</h1></body></html>
```

**A missing file — the 404 path, for real:**
```
$ curl -s -i http://localhost:8080/missing.html
HTTP/1.1 404 Not Found
Connection: close
Content-Length: 9

Not Found
```

And, tying this directly back to the previous chapter: this is exactly the server the `web_get.c` client's real run in `06_HTTP_Client.md` was actually talking to — one book's two chapters, proven to genuinely interoperate, not just independently plausible.

## 7.6 `web_server2.c` — the same server, one global variable removed

The book includes a second version that's behaviorally identical but refactored to avoid the `static struct client_info *clients = 0;` file-scope global — every function that used to read/write `clients` directly now takes a `struct client_info **client_list` parameter instead, and the list itself lives as a local variable inside `main()`.

```c
// before: every function just reaches out to the global
static struct client_info *clients = 0;
struct client_info *get_client(SOCKET s) { /* uses `clients` directly */ }

// after: the list is passed explicitly, everywhere
struct client_info *get_client(struct client_info **client_list, SOCKET s) {
    struct client_info *ci = *client_list;
    /* ... */
}

int main() {
    struct client_info *client_list = 0;   // lives here now, not at file scope
    /* ...pass &client_list into every helper that needs it... */
}
```

Functionally, running either version produces identical output — this refactor doesn't fix a bug, it fixes a *design smell*. Global mutable state (`clients` was reachable and modifiable from anywhere in the file, by anything, without the call signature giving any hint of that) is harder to reason about, test in isolation, or safely reuse (imagine wanting two independent servers, each with its own client list, in one program — the global-variable version can't do that; the parameter-passing version can, trivially). Worth internalizing as a general practice, well beyond networking: **a function's signature should tell you what state it touches.**

---

## Quick recap (for revision)

- Content-Type is decided by file extension via a lookup table — get this wrong and browsers render (or refuse to render) the response incorrectly, even if the bytes themselves are fine.
- This server tracks clients in a linked list (vs. Beej's Guide's growable array) — different data structure, identical underlying "walk everyone, build one `fd_set`, `select()` once" strategy.
- `if (strstr(path, ".."))` is a real, necessary directory-traversal guard — never serve files from user-supplied paths without one.
- **Real, live-caught bug**: `sprintf(..., "%u", cl)` where `cl` is `size_t` is a genuine format-specifier mismatch on 64-bit systems — use `%zu` for `size_t` values. Caught here specifically *because* the code was compiled with `-Wall`, not glossed over.
- This server deliberately omits `SO_REUSEADDR` — intentional, and revisited head-on in Chapter 13.
- `web_server2.c` is the identical server with client-list state passed explicitly instead of living in a global — a clean illustration of why doing that is worth the extra parameter.

Next: `08_Sending_Email_SMTP.md` — using the same raw-socket skills to talk directly to a mail server.