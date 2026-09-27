# 30 — Networking From The Terminal

> **Where this fits in the stack**
> ```
> Shell → curl → socket() → DNS → connect() → TCP → kernel networking stack → NIC → internet
>          ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
>          this file
> ```
> File 27 covered `AF_UNIX` sockets — local-only, file-path-addressed. This file covers the *other* major socket family, `AF_INET`/`AF_INET6` — IP-address-and-port-addressed, and the one most people mean when they say "socket" without qualification — traced through a single, complete `curl` request.

---

## 1. The full trace: `curl https://example.com`

**Shell:** parses the line, looks up `curl` via `PATH` (file 07), `fork`s and `execve`s it (file 08) — nothing about this differs from launching any other program.

**`curl` (the program):** an entirely ordinary userspace program — not part of the kernel, not privileged in any special sense. It happens to know the HTTP protocol and how to drive the socket API; nothing more.

**`socket(AF_INET, SOCK_STREAM, 0)`:** the syscall (file 09) that creates a new, unconnected socket and returns a **file descriptor**. A socket fd fits into the exact same fd/VFS-adjacent abstraction as any other fd (file 10) — it just comes with additional, socket-specific syscalls (`connect`, `bind`, `listen`, `accept`, `send`, `recv`) for setting it up in the first place.

**DNS resolution:** before `curl` can connect to `example.com`, it needs an actual IP address. This typically happens via a library call — `getaddrinfo()` — which itself, underneath, either reads `/etc/hosts` directly or sends its own UDP packets to a configured DNS server and parses the response.

> **Under the hood**
> DNS resolution *feels* like one function call, but it's itself constructing, sending, and receiving ordinary socket traffic underneath — file 09's `printf`-wraps-`write` lesson, generalized once more. `getaddrinfo()` is a library-level convenience wrapping genuine network I/O, not a special kernel operation.

**`connect(fd, address, addrlen)`:** initiates a TCP connection to the now-known IP address and port (443 for HTTPS). This is where the three-way handshake (SYN/SYN-ACK/ACK — named, not deep-dived; full TCP mechanics are out of scope here) happens entirely inside the **kernel's own TCP implementation**. `curl`'s code simply blocks inside this one syscall until the connection is established or fails.

**TCP / kernel networking stack:** once connected, `curl` writes its HTTP request bytes via ordinary `write()`/`send()` (file 09) to the socket fd. The kernel's TCP/IP stack handles all packetization, sequencing, and retransmission-on-loss, entirely transparently — `curl`'s own code just sees "I wrote some bytes; they'll arrive reliably, in order." The same theme as everywhere else in this handbook: a simple userspace abstraction, backed by substantial kernel work underneath to make that simplicity hold.

**NIC (network interface card):** the actual hardware that physically transmits the resulting packets. The kernel's networking stack hands finished, properly formatted packets down to the NIC driver, which performs the final hardware-level transmission.

**Internet:** beyond your own machine — routers, ISPs, and the rest — genuinely out of scope for an OS/shell-focused handbook; acknowledged only as "where the packets go next."

---

## 2. `curl` is not part of the kernel

Worth stating with full explicitness, tying directly back to file 09's syscall-vs-library theme and file 07's shell-is-not-the-kernel theme: `curl` is just another userspace program, exactly like `ls` or `grep`. It happens to link against networking-capable libraries (or call socket-family syscalls directly) and understand the HTTP protocol — a purely application-level, userspace concept the kernel has **no knowledge of whatsoever**. The kernel sees bytes flowing over a TCP connection; it has no idea any of it constitutes "HTTP." Everything protocol-aware about `curl`'s behavior is `curl`'s own code, running in user mode (file 09), making ordinary syscalls exactly like any other program would.

> **Interview answer**
> "`curl` is an ordinary, unprivileged userspace program. It makes the same kind of syscalls — `socket`, `connect`, `write`, `read` — that any networked program would. The kernel handles TCP/IP; it has no concept of HTTP at all. That's entirely `curl`'s own application-level logic, layered on top of a connection the kernel just sees as a byte stream."

---

## 3. Sockets fit the same fd model

Once connected, a socket fd behaves largely like an ordinary file or pipe fd for `read()`/`write()` purposes (file 10, file 13) — the real differences are in the special syscalls used to **create and configure** it in the first place (`socket`/`connect`/`bind`/`listen`/`accept`), not in how you use it once you already have a connected fd in hand.

---

## 4. `AF_UNIX` vs `AF_INET` — completing file 27's comparison

| | `AF_UNIX` (file 27) | `AF_INET`/`AF_INET6` (this file) |
|---|---|---|
| Addressed by | A local file path | An IP address + port |
| Can cross a network | Never | Yes |
| Typical use | Local-only IPC — tmux's client-server link, many database client/server setups (Postgres's default socket), Docker's own daemon socket | Anything needing to reach another machine |

---

## 5. Common misconceptions

| Misconception | Reality |
|---|---|
| "`curl` is somehow privileged, or part of the kernel, because it 'does networking.'" | Ordinary unprivileged userspace program — the same kind of syscalls any networked program makes. |
| "DNS resolution is a special, privileged kernel operation." | Ordinary UDP traffic (or local file reads), entirely userspace/library-level work. |
| "The kernel understands HTTP." | The kernel handles TCP/IP; HTTP is entirely an application-level concept, layered on top by the program itself. |

---

## 6. Try this

> **Try this**
> ```
> strace -e socket,connect,sendto curl -s https://example.com > /dev/null
> ```
> Watch the actual syscalls fire, in order — a direct continuation of file 09's `strace` preview.

> **Try this**
> ```
> cat /etc/resolv.conf
> ```
> Your configured DNS servers — exactly what `getaddrinfo()` actually queries.

---

## 7. Summary

- `curl https://example.com` is `socket()` → DNS resolution (itself ordinary UDP traffic) → `connect()` (kernel-handled TCP handshake) → `write()`/`read()` over the connection → kernel TCP/IP stack → NIC.
- `curl` is an ordinary userspace program throughout — the kernel has no concept of HTTP at all, only TCP/IP bytes.
- A connected socket fd behaves like any other fd for read/write purposes; the special syscalls are only for setup.
- `AF_UNIX` (file 27) and `AF_INET`/`AF_INET6` (this file) are the two major socket families — local-only-by-construction versus capable of reaching another machine.

---

## What's next

`31_DEBUGGING_TOOLS.md` begins the final tier: now that most of the system has been covered, it's time to learn how to observe all of it yourself — `strace`, `ps`, `lsof`, `ss`, and the rest, one tool at a time.
