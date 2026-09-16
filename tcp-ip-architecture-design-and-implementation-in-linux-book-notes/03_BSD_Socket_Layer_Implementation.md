# 3. The BSD Socket Layer Implementation
### *(Covers the book's Chapter 3 — "Sockets")*

This is where `socket()` stops being a black box. The core idea worth holding onto through all the structure names below: Linux's socket layer is deliberately split into a **generic part** (knows nothing about TCP or IP specifically — just "a socket") and a **protocol-specific part** (the actual TCP or UDP implementation) — and the entire job of this chapter's code is wiring those two parts together correctly for whatever `socket()` call you made.

## 3.1 Two structures, two jobs: `struct socket` and `struct sock`

Beginners reading kernel networking source for the first time almost always trip on this: there are **two** different "socket" structures, deliberately separated by concern.

```c
struct socket {
    socket_state            state;      // SS_UNCONNECTED, SS_CONNECTING, ...
    unsigned long            flags;
    struct proto_ops        *ops;       // <-- generic, protocol-independent operations
    struct inode            *inode;
    struct fasync_struct    *fasync_list;
    struct file             *file;      // <-- the VFS link, see §3.2
    struct sock             *sk;        // <-- the protocol-specific object
    wait_queue_head_t        wait;
    short                    type;      // SOCK_STREAM, SOCK_DGRAM, ...
};
```

```c
struct sock {
    /* ...a large number of fields covering connection state, send/receive
       queues (sk_receive_queue, sk_write_queue), memory accounting
       (sk_rmem_alloc, sk_wmem_alloc — see Chapter 9), timers, the
       destination cache, and a huge amount more... */
    struct proto            *prot;      // <-- protocol-specific operations
    unsigned short           family;    // AF_INET, AF_INET6, ...
    volatile unsigned char   state;     // TCP_ESTABLISHED, TCP_LISTEN, ...
    /* ... */
};
```

**`struct socket`** is the thin, generic, VFS-facing object — it exists identically in shape whether you opened a TCP socket, a UDP socket, or (in principle) some other address family entirely. **`struct sock`** is where the actual protocol lives — a TCP socket's `struct sock` carries an embedded `struct tcp_opt` (the TCP state machine's own data, covered fully in `11_TCP_Core_Processing_State_Machine.md`); a UDP socket's doesn't need anything nearly that elaborate. Every `struct socket` points to exactly one `struct sock` (`sk`), and that `struct sock`'s `prot` field points to a table of **protocol-specific function pointers** — this is the seam where "generic socket layer" ends and "TCP implementation" or "UDP implementation" begins.

## 3.2 The VFS connection — why a socket has a file descriptor at all

Unix's oldest, most foundational design idea is "everything is a file" — and sockets, despite not being files in any conventional sense, are deliberately made to fit that model so the exact same `read()`, `write()`, `close()`, `select()`/`poll()`, and file-descriptor-table machinery that already exists for regular files works on sockets too, with zero special-casing needed anywhere else in the kernel or in userspace tools.

```c
struct file_operations socket_file_ops = {
    .llseek  = sock_lseek,
    .read    = sock_read,
    .write   = sock_write,
    .poll    = sock_poll,
    .ioctl   = sock_ioctl,
    .mmap    = sock_mmap,
    .open    = sock_no_open,
    .release = sock_close,
    .fasync  = sock_fasync,
    /* ... */
};
```

When `socket()` runs, the kernel:
1. Allocates a `struct socket` (§3.1).
2. Allocates a special-purpose **anonymous inode**, on an internal, memory-only filesystem conventionally referred to as "sockfs" — there's no actual disk file here; this exists purely to let a socket participate in the VFS's inode/file abstractions.
3. Allocates a `struct file`, pointing its `f_op` at `socket_file_ops` above, and its private data at the `struct socket` from step 1.
4. Installs that `struct file` into the calling process's file descriptor table, via `sock_map_fd()` — and *that* installed slot number is the integer `socket()` hands back to your program.

This is exactly why `select()`/`poll()` (the entire subject of the previous study pack's multiplexing chapters) work identically on sockets and regular files/pipes — as far as the VFS and the scheduler's wait-queue machinery are concerned, a socket genuinely *is* just another kind of file, dispatching through `sock_read`/`sock_write`/`sock_poll` instead of a filesystem's own read/write implementation, but going through the exact same generic entry points to get there.

## 3.3 Protocol family registration — how `AF_INET` finds TCP and UDP

```c
struct net_proto_family {
    int          family;    // AF_INET, AF_INET6, AF_UNIX, ...
    int          (*create)(struct socket *sock, int protocol);
};

static struct net_proto_family *net_families[NPROTO];
```

Every address family the kernel supports registers itself, once, at boot (or module-load) time, into this `net_families` table — `AF_INET` registers `inet_family_ops`, whose `create` method is `inet_create()`. When your application calls `socket(AF_INET, SOCK_STREAM, 0)`, the generic `sys_socket()` path looks up `net_families[AF_INET]` and calls its `create()` function — **this is the exact moment "some generic socket" becomes "specifically an IPv4 socket."**

`inet_create()` itself then has a second-level dispatch, this time on `type` (`SOCK_STREAM` vs. `SOCK_DGRAM`) *and* the requested protocol:

```c
struct proto_ops {
    int   family;
    int   (*release)(struct socket *sock);
    int   (*bind)(struct socket *sock, struct sockaddr *addr, int addr_len);
    int   (*connect)(struct socket *sock, struct sockaddr *addr, int addr_len, int flags);
    int   (*accept)(struct socket *sock, struct socket *newsock, int flags);
    int   (*listen)(struct socket *sock, int len);
    int   (*sendmsg)(struct socket *sock, struct msghdr *msg, int size, struct scm_cookie *scm);
    int   (*recvmsg)(struct socket *sock, struct msghdr *msg, int size, int flags, struct scm_cookie *scm);
    /* ... */
};

struct proto_ops inet_stream_ops = {
    .family    = PF_INET,
    .bind      = inet_bind,
    .connect   = inet_stream_connect,
    .accept    = inet_accept,
    .listen    = inet_listen,
    .sendmsg   = tcp_sendmsg,       // <-- straight into TCP's own code
    .recvmsg   = sock_common_recvmsg,
    /* ... */
};

struct proto_ops inet_dgram_ops = {
    .family    = PF_INET,
    .bind      = inet_bind,
    .connect   = udp_connect,
    /* no listen/accept — connectionless, exactly per the previous study
       pack's UDP chapters */
    .sendmsg   = udp_sendmsg,
    .recvmsg   = udp_recvmsg,
    /* ... */
};
```

`inet_create()` sets `sock->ops` to `&inet_stream_ops` for a `SOCK_STREAM` request, or `&inet_dgram_ops` for `SOCK_DGRAM` — **and this single assignment is the entire reason `send()` on a TCP socket runs completely different code than `send()` on a UDP socket, despite both being reached through the exact same generic `sys_socket()`/`sock_sendmsg()` entry point.** The generic layer never has an `if (is_tcp) ... else ...` anywhere — it just calls `sock->ops->sendmsg(...)`, and the function-pointer table itself, chosen at `socket()` time, is what actually determines TCP vs. UDP behavior.

## 3.4 `struct proto` — the second table, one level deeper

Alongside `proto_ops` (generic socket-layer operations), there's a second table specifically for TCP/UDP-family internals:

```c
struct proto {
    void   (*close)(struct sock *sk, long timeout);
    int    (*connect)(struct sock *sk, struct sockaddr *uaddr, int addr_len);
    int    (*disconnect)(struct sock *sk, int flags);
    struct sock *(*accept)(struct sock *sk, int flags, int *err);
    int    (*ioctl)(struct sock *sk, int cmd, unsigned long arg);
    int    (*setsockopt)(struct sock *sk, int level, int optname, char *optval, int optlen);
    void   (*keepalive)(struct sock *sk, int valbool);
    /* ... */
    char   name[32];
};

struct proto tcp_prot = { .name = "TCP", .close = tcp_close, .connect = tcp_v4_connect, /* ... */ };
struct proto udp_prot = { .name = "UDP", .close = udp_close, .connect = udp_connect, /* ... */ };
```

Practically: `proto_ops` is "the table the generic VFS/socket layer calls into"; `proto` is "the table TCP/UDP-specific code, and code that already knows it's dealing with an INET socket, calls into" — a second layer of indirection that exists because a fair amount of code (the whole `AF_INET`-family layer, not just the outermost VFS-facing shell) still wants to stay protocol-agnostic between TCP and UDP for as long as possible, only dropping down to protocol-specific behavior at the last necessary moment.

## 3.5 Putting it together: the full path of a `socket()` call

```
 sys_socketcall(SYS_SOCKET, ...)
        │
        ▼
 sys_socket()
        │
        ▼
 sock_create()  ──►  allocate struct socket
        │
        ▼
 net_families[AF_INET]->create()   =  inet_create()
        │
        ├─► allocate struct sock, struct tcp_opt (if SOCK_STREAM)
        ├─► sock->ops   = &inet_stream_ops   (or &inet_dgram_ops)
        └─► sk->prot    = &tcp_prot          (or &udp_prot)
        │
        ▼
 sock_map_fd()  ──►  allocate inode + struct file on sockfs,
                      install into this process's fd table
        │
        ▼
 return the fd number to userspace
```

Every later chapter's "here's what `bind()`/`connect()`/`send()` does" walkthrough picks up *after* this — by the time you call any of those, `sock->ops` and `sk->prot` are already correctly wired to either the TCP or UDP implementation, and the generic dispatch is simply `sock->ops->whatever(...)`.

## 3.6 What's changed since kernel 2.4.20

- **Network namespaces** (mainlined starting around kernel 2.6.24, and heavily built out afterward) thread a `struct net *` argument through essentially every one of these structures and function calls today — `net_families` in a modern kernel is conceptually per-namespace, not a single global table, because a namespace-isolated container needs its *own* independent view of sockets, routing tables, and interfaces. This is the single kernel feature underpinning every container runtime's network isolation (Docker, Kubernetes pods, and so on) — none of which existed in any form resembling today's containers when this book was written.
- **`proto_ops`/`proto` still exist, in essentially the same conceptual role**, though the exact field lists have grown and shifted considerably over 20+ years of feature additions (eBPF socket filtering hooks, `SO_REUSEPORT` load-balancing logic, and far more).
- **`sockfs` is still exactly this same idea** — sockets are still VFS-integrated anonymous-inode objects today, for the same "make `select()`/`poll()`/`read()`/`write()` just work" reasons described in §3.2.
- **eBPF** now lets userspace attach verified, sandboxed programs at multiple points in this exact dispatch chain (`SO_ATTACH_BPF` for classic socket filtering, and considerably more via the modern XDP/TC-eBPF hooks covered conceptually in `16_Netfilter_Framework.md` and `17_SoftIRQ_Implementation.md`'s "what's changed" sections) — a capability with no 2.4.20-era equivalent at all.

---

## Quick recap (for revision)

- `struct socket` = generic, VFS-facing; `struct sock` = protocol-specific (TCP's carries the whole state machine; UDP's doesn't need to).
- Sockets get a file descriptor by being wired into the VFS as anonymous-inode objects on `sockfs`, dispatching through `socket_file_ops` — this is *why* `read()`/`write()`/`select()` work on them identically to regular files.
- `net_families[AF_INET]->create()` = `inet_create()` — this single lookup is where "a generic socket request" becomes "specifically IPv4."
- `sock->ops` (a `proto_ops` table — `inet_stream_ops` vs. `inet_dgram_ops`) and `sk->prot` (a `proto` table — `tcp_prot` vs. `udp_prot`) are both chosen once, at `socket()` time, based on `SOCK_STREAM` vs. `SOCK_DGRAM` — and that one-time choice is the entire mechanism behind TCP and UDP behaving completely differently despite sharing every generic entry point above it.
- Network namespaces are the biggest structural change since this chapter's era — nearly every structure discussed here now carries an implicit or explicit per-namespace scope that didn't exist in 2.4.20 at all.

Next: `04_Connection_Setup_Kernel_Implementation.md` — `bind()`, `listen()`, `connect()`, and `accept()`, now that you know what `sock->ops` and `sk->prot` actually are.