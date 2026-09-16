# 5. `sk_buff` — Linux's Representation of a Network Packet
### *(Covers the book's Chapter 5 — "Linux Implementation of Network Packet")*

Every packet, in either direction, at every layer, from the moment a device driver first sees raw bytes off the wire to the moment `recv()` copies data into your application's buffer, is represented by exactly one kind of structure: the **`sk_buff`** ("socket buffer"). Understanding its design — specifically, *why* it's shaped the way it is — makes almost every later chapter's code read as obviously sensible rather than mysterious.

## 5.1 The core design problem `sk_buff` solves

A single packet gets a header added or removed at *every layer* it passes through: the application hands raw bytes to TCP, which prepends a TCP header; that goes to IP, which prepends an IP header; that goes to the device driver, which prepends a link-layer (Ethernet) header — and the exact reverse happens on receive, one header stripped at each layer going up. **The naive approach — allocate a fresh buffer and copy the data every time a header is added or removed — would mean multiple full-packet memory copies for every single packet, at every hop through the stack.** For a system moving any serious amount of network traffic, that's a genuinely severe performance cost, repeated for every packet, forever.

`sk_buff`'s answer: allocate **one buffer, generously sized with extra space at both ends**, and let each layer simply **adjust pointers** into that one buffer — "growing" into pre-reserved headroom to prepend a header, or "shrinking" by moving a pointer forward to strip one — without ever touching, moving, or copying the actual payload bytes in the middle.

```
 head                                                              end
  │                                                                  │
  ▼                                                                  ▼
  ┌──────────────┬───────────────────────────────────────────┬────────────┐
  │   headroom   │            actual packet data             │  tailroom  │
  │ (reserved    │         (data ────────────► tail)         │ (reserved  │
  │  for headers)│                                           │  for more  │
  └──────────────┴───────────────────────────────────────────┴────────────┘
                 ▲                                           ▲
                data                                        tail
```

## 5.2 The structure itself, and the fields that matter most

```c
struct sk_buff {
    struct sk_buff  *next;         // sk_buffs live in doubly linked lists
    struct sk_buff  *prev;         //  (queues) constantly throughout the stack
    struct sock     *sk;           // owning socket, once one is known
    struct timeval   stamp;        // when this packet arrived/was created
    struct net_device *dev;        // the network device this is associated with

    union {
        struct tcphdr   *th;
        struct udphdr   *uh;
        unsigned char   *raw;
    } h;                            // transport-layer header pointer

    union {
        struct iphdr    *iph;
        unsigned char   *raw;
    } nh;                           // network-layer (IP) header pointer

    union {
        struct ethhdr   *ethernet;
        unsigned char   *raw;
    } mac;                          // link-layer (Ethernet) header pointer

    unsigned int     len;          // actual data length, this buffer
    unsigned int     data_len;     // extra length in any attached fragments
    unsigned char    *head;        // start of the allocated buffer
    unsigned char    *data;        // start of the actual (current-layer) data
    unsigned char    *tail;        // end of the actual data
    unsigned char    *end;         // end of the allocated buffer

    atomic_t          users;       // reference count
    unsigned int      cloned;      // is the DATA shared with another sk_buff?

    void  (*destructor)(struct sk_buff *skb);
    /* ... considerably more: checksum state, VLAN tags, protocol field,
       priority, and more ... */
};
```

The **three header-pointer unions** (`mac`, `nh`, `h`) are worth sitting with: as one single packet travels down through the stack on transmit (or up through it on receive), the *same* `sk_buff` gets its `nh.iph` pointer set once IP has done its work, its `h.th` pointer set once TCP has done its work, and so on — each layer records "here's where *my* header starts within this buffer" without needing to know or care what any other layer did. Any later piece of code that needs "the IP header of this packet" just dereferences `skb->nh.iph`, regardless of how the packet got there.

## 5.3 The four boundary pointers, and the operations built on them

`head`/`data`/`tail`/`end` are the entire mechanism from §5.1, made concrete. A small set of primitive operations, used constantly throughout every later implementation chapter, manipulate exactly these four pointers and nothing else:

- **`skb_reserve(skb, len)`**: called once, right after allocation, before any real header-writing begins — moves both `data` and `tail` forward by `len` bytes, reserving that much headroom up front. A typical TCP send path reserves enough headroom for the largest possible combination of link-layer + IP + TCP headers this packet might ultimately need, so that every later layer's header-prepend is guaranteed to have room without any reallocation.
- **`skb_push(skb, len)`**: moves `data` *backward* by `len` bytes (into already-reserved headroom) and returns the new `data` pointer — this is exactly "prepend a header": IP's transmit path calls this to make room for the IP header it's about to write, immediately in front of whatever TCP already placed there.
- **`skb_pull(skb, len)`**: the receive-side mirror image — moves `data` *forward* by `len` bytes, "consuming" (skipping past) a header that's already been parsed, so the next layer up sees `data` pointing at exactly where *its* header begins.
- **`skb_put(skb, len)`**: moves `tail` forward by `len` bytes (into tailroom), for appending data at the end rather than the front — used far more on the transmit side when actual payload is being copied in.

Every one of these is, deliberately, **just pointer arithmetic** — no memory copy, no allocation, and (crucially for the interrupt/softIRQ-context concerns from `01_Kernel_Basics_and_TCPIP_System_Calls.md`) no operation that could possibly sleep or fail due to memory pressure once the buffer itself already exists.

## 5.4 Reference counting and cloning — the same bytes, several owners

A single `sk_buff`'s underlying data buffer is often legitimately needed by more than one part of the stack simultaneously. The canonical example, directly relevant to `08_TCP_Send.md`: **TCP must keep a copy of every segment it sends, in case retransmission is needed**, while *also* handing that exact same data off to IP, then the device driver, for actual transmission. Copying the whole payload just to satisfy this would defeat the entire point of §5.1's design.

The fix is **`skb_clone()`**: allocate a brand-new `sk_buff` *structure* (the metadata — pointers, header offsets, and so on), but have it point at the **same underlying data buffer** as the original, with a shared reference count (`users`) tracking how many `sk_buff` structures currently point at that one buffer. TCP keeps its own clone sitting in the retransmit queue; a second clone (or the "original") goes down through IP and the device driver for actual transmission and eventually gets freed once the hardware is done with it — the underlying payload bytes themselves are **never duplicated**, only referenced twice. Contrast this with **`skb_copy()`**, a genuinely full, deep copy (structure *and* underlying data) — needed only when something is actually going to *modify* the data independently of whatever else references it, which cloning's shared-buffer model would make unsafe.

## 5.5 Why this matters for reading every later chapter

Once you've internalized §5.1–5.4, a huge fraction of what looks like arbitrary complexity in the receive/send-path chapters (`06`–`08`) resolves into three predictable moves, repeated over and over: **reserve headroom up front, push/pull pointers as layers are added or stripped, clone (never deep-copy) whenever the same data legitimately needs more than one simultaneous owner.** There's genuinely very little more to the core `sk_buff` model than that — the complexity in later chapters is almost entirely about *when* and *why* each layer performs these moves, not about the primitive operations themselves.

## 5.6 What's changed since kernel 2.4.20

- **The three header-pointer unions (`mac`/`nh`/`h`) were replaced with offset-based accessor functions** (`skb_network_header()`, `skb_transport_header()`, `skb_mac_header()`, and their `skb_reset_*` / `skb_set_*` counterparts) during the 2.6 series, specifically for 64-bit-pointer-size efficiency and cleaner separation between "where the header is" and "how to interpret it." The *concept* — each layer records where its header starts within the shared buffer — is completely unchanged; only the concrete field-vs-function-call mechanics differ.
- **Paged / fragmented data (`skb_shared_info`, the `frags[]` array)**: modern `sk_buff`s can reference payload data living in separate memory pages rather than requiring every byte to sit in one contiguous `head`-to-`end` buffer — the foundation for zero-copy techniques like `sendfile()` and `splice()`, letting large transfers avoid even the one copy this chapter's simpler model still requires (e.g., getting data from a file's page cache into the packet in the first place).
- **GSO/GRO (Generic Segmentation/Receive Offload)**: a single `sk_buff` can now represent something considerably larger than one wire-sized packet, carrying metadata that tells the device driver "please segment this into several real packets yourself, in hardware" (transmit side, GSO) or, conversely, several small received packets can be coalesced into one larger `sk_buff` before being handed up the stack (receive side, GRO) — both are genuine performance optimizations with no real analogue in the one-`sk_buff`-per-wire-packet model this chapter describes.
- **XDP (eXpress Data Path)**, added in kernel 4.8, lets a userspace-supplied eBPF program process a packet at the very earliest possible point — often directly against the driver's raw receive buffer, **before a full `sk_buff` has even been allocated at all** — specifically to make the fastest possible packet-drop/redirect/forward decisions bypass this chapter's entire structure for the cases that can afford to.

---

## Quick recap (for revision)

- One buffer, generous headroom/tailroom at both ends, pointer arithmetic instead of copies as headers are added/removed at each layer — this single design decision is *why* the networking stack can be as fast as it is.
- `skb_reserve`/`skb_push`/`skb_pull`/`skb_put` are the four primitive operations behind essentially every header manipulation in every later chapter — memorize what each one moves and in which direction.
- `mac`/`nh`/`h` are per-layer "where does my header start" pointers into the *same* shared buffer — not separate copies of anything.
- `skb_clone()` (shared data, separate metadata, refcounted) is how TCP keeps a retransmit-queue copy without duplicating payload bytes; `skb_copy()` is a genuine full copy, needed only when independent modification requires it.
- Modern kernels replace the raw header-pointer unions with accessor functions, and add paged data, GSO/GRO, and (for the most performance-critical paths) XDP's ability to bypass `sk_buff` allocation entirely — none of which change the fundamental headroom/pointer-arithmetic model this chapter teaches.

Next: `06_Packet_Movement_Across_Layers.md` — watching a complete `sk_buff`, in both directions, move through the whole stack.