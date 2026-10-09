# Project Deep Dive: NetSight — Linux Network Telemetry & Traffic Analysis Engine

🔴🔴🔴 **Verified directly against your source code** (`NetSight-main.zip`). Two findings below are genuinely important corrections, not just filled-in blanks — read the corrections section before anything else.

---

## Corrections From the Earlier (Pre-Verification) Draft — Read This First

### 1. Your flow table is NOT lock-free — it uses a `pthread_rwlock`
This is the single most important correction in this whole knowledge base. Your resume says: *"Implemented lock-free concurrent data structures from scratch — a C11-atomic ring buffer, a Treiber-stack memory pool, and a 1M-entry hash-based flow table — to process traffic without locks or blocking."* That sentence groups all three as lock-free. The code shows two of them genuinely are (`ringbuf.c`'s header comment literally says *"Lock-free MPSC ring buffer (C11 _Atomic)"*, and `mempool.c` is pure CAS with no lock) — **but `flow.c` calls `pthread_rwlock_wrlock`/`unlock` around every insert, and a read lock around lookups.** That's a real lock. A well-designed one — reader-writer locks let concurrent lookups proceed while serializing only writes, which is a perfectly reasonable choice for a table that's read far more often than written — but it is not lock-free, and if you say it is, a systems-minded interviewer who reads your code (exactly like I just did) will catch it immediately.

**The corrected, honest, still-strong answer:** *"The ring buffer and memory pool are genuinely lock-free — CAS-based, no blocking. The flow table uses a reader-writer lock deliberately, since lookups vastly outnumber inserts in this workload, and a rwlock lets concurrent readers proceed without contention while only serializing the rarer write path."* This is a better interview answer than the original, because it shows you made a deliberate, justified tradeoff rather than lock-free-everywhere dogma.

### 2. Your Treiber stack has no ABA mitigation — and that's worth saying plainly
`mempool.c`'s free-list push/pop is a plain `atomic_compare_exchange` on a raw pointer — no tagged pointers, no generation counter, no hazard pointers. If asked "how do you prevent ABA," the honest answer is: **you don't, explicitly** — and the defensible engineering justification is that this is a bounded, internal-only memory pool (blocks are only ever recycled within this one system, never freed to the OS or exposed externally), which narrows the practical ABA window considerably compared to a general-purpose lock-free stack, even though it isn't formally eliminated. Claiming a mitigation you don't have is far riskier than stating this plainly — this is exactly the kind of honest, technically precise answer this whole prep system exists to help you give.

### 3. A documentation/code mismatch worth fixing in your repo
`docs/explanatiion.md` describes the flow table as "a fixed-size array of FLOW_TABLE_SIZE (65,536) buckets, each a singly-linked chain... (separate chaining, not open addressing)." The actual code in `flow.c` implements **open addressing with linear probing** (`idx = (idx + 1) & t->mask`), including what looks like Robin Hood–style displacement tracking (`dist_stored`) during resize. Trust the code, not that doc — and consider fixing the doc, since an interviewer who reads both would notice the contradiction.

---

## Architecture (Verified)

```
NIC → Linux Kernel (AF_PACKET/TPACKET_V3, mmap'd ring buffer, zero-copy)
        ▼
   Capture threads (pthreads) → Protocol Parser (Ethernet/VLAN/IPv4/TCP/UDP/ICMP)
        │
        ├──→ Lock-free MPSC Ring Buffer (C11 _Atomic) — genuinely lock-free
        ├──→ Treiber-Stack Memory Pool (plain CAS, NO ABA mitigation — see above)
        └──→ Flow Table: open-addressing hash table, pthread_rwlock-guarded —
             NOT lock-free (see above). Dynamically sized via flow_table_create(capacity)
             — FLOW_TABLE_SIZE=65536 is a fallback/minimum default, not a hard cap;
             the real configured capacity (flow_max_entries=1,000,000, config file) is
             what actually drives the resume's "1M-entry" figure once rounded to a
             power of 2 and resized as needed.

eBPF (BTF CO-RE, tcp_tracer.bpf.c):
   kprobe/tcp_v4_connect, kretprobe/tcp_v4_connect,
   kprobe/tcp_close, kprobe/tcp_retransmit_skb,
   kprobe/inet_csk_accept, kretprobe/inet_csk_accept
   → tcp_events map: BPF_MAP_TYPE_PERF_EVENT_ARRAY (streams events to userspace)
   → active_socks / active_accepts maps: BPF_MAP_TYPE_HASH, keyed by pid_tgid,
     storing the socket pointer between a kprobe's entry and its kretprobe's
     return — because the original function arguments are gone by the time
     the return probe fires. A genuinely elegant, specific technique worth
     naming precisely if asked "how does your eBPF program track state across
     a function call?"

Prometheus export · ncurses UI

Validation (VALIDATION.md, real measured data):
  L4 Burst: target 1,500 conn/s → actual 1,448.1 conn/s, 43,442 connections, 0 failures
  1-hour endurance: RSS 316.16 MB (T0) → 316.66 MB (T60), +500kB, 0.16% variance
    — CRITICALLY: growth plateaued between T30–T60 (+60kB in the final 30 min),
    consistent with normal pool/buffer warm-up stabilizing, not a continuous leak
```

---

## The Named Questions — Now With Verified Answers

**"Why lock-free? Walk me through your lock-free flow table."** → Correct this one immediately, don't let it stand: *"The ring buffer and memory pool are lock-free. The flow table specifically uses a reader-writer lock, because it's read far more often than written, and a rwlock lets concurrent lookups proceed without contention."* See Correction #1 above for the full framing.

**"What is a Treiber stack? ABA problem?"** → Explain the mechanism correctly (CAS-based push/pop on the free-list head), then state plainly that this implementation has no explicit ABA mitigation, and give the bounded-internal-pool justification from Correction #2. Do not claim tagged pointers or hazard pointers — they aren't there.

**"How are TCP connections tracked?"** → Verified precisely: kprobes on `tcp_v4_connect`/`tcp_close`/`tcp_retransmit_skb`/`inet_csk_accept`, with a kretprobe pairing on `tcp_v4_connect` and `inet_csk_accept` specifically to capture the *return value* (success/failure, the accepted socket) — and a `BPF_MAP_TYPE_HASH` keyed by `pid_tgid` bridging state between a kprobe's entry and its kretprobe's return, since raw function arguments aren't available anymore once the return probe fires.

**"How does an eBPF program communicate with user space?"** → Verified: `BPF_MAP_TYPE_PERF_EVENT_ARRAY` for the actual event stream (`tcp_events`) — the older, `perf`-buffer-based mechanism, not the newer `BPF_MAP_TYPE_RINGBUF`. Worth knowing this distinction precisely if asked why one over the other (PERF_EVENT_ARRAY is more broadly compatible across older kernels; RINGBUF, added in kernel 5.8+, is more efficient but less portable back to your stated 5.4 floor — a very plausible, specific reason for the choice, worth confirming against your own reasoning if you recall it, otherwise stating as the likely portability rationale).

**"Why a hash table for flows? What's the collision strategy?"** → Verified: open addressing with linear probing (`(idx + 1) & mask`), not separate chaining — and the resize path shows Robin Hood–style displacement bookkeeping (`dist_stored`), which is a more sophisticated technique than plain linear probing alone (Robin Hood hashing minimizes variance in probe distance across entries, keeping worst-case lookups more consistent). If you recall implementing this deliberately, that's a strong, specific answer; if the `dist_stored` field's exact role is fuzzy, that's worth re-reading `flow.c`'s resize function before an interview, since it's a genuinely impressive detail to be able to explain.

**"How did you validate 43,442 connection attempts? How did you test memory growth?"** → Now fully verified and quotable: L4 burst test — target 1,500 conn/s, actual 1,448.1 conn/s sustained, 43,442 total connections, 0% failure, over a 30-second window. Memory: 1-hour endurance run, RSS 316.16→316.66 MB, and — the precise, rigorous point — growth **plateaued** in the second half of the run (+60kB from T30–T60 vs. the full +500kB from T0–T60), which is real evidence against a continuous leak, not just a small enough number to hand-wave past.

**"What's the actual max flow-table capacity, and how does 65,536 relate to 1,000,000?"** → A likely sharp follow-up now that the two constants coexist in your codebase: `FLOW_TABLE_SIZE=65536` is a compile-time fallback/minimum (`FLOW_TABLE_MIN_CAPACITY`-adjacent default) — the table is actually created via `flow_table_create(capacity)` with a runtime `capacity` argument, rounded up to a power of 2, and can resize. The "1M-entry" resume figure corresponds to the `flow_max_entries=1000000` config value, which is what's actually passed in as capacity in the real running configuration.

### Quick Revision
- The flow table uses a `pthread_rwlock`, not lock-free CAS — say this plainly and explain why the tradeoff is reasonable, don't imply it's lock-free like the other two structures.
- The Treiber stack has zero ABA mitigation — state this honestly, with the bounded-internal-pool justification, rather than claiming tagged pointers or hazard pointers you don't have.
- Collision resolution is open addressing + linear probing with Robin Hood-style displacement on resize — not separate chaining (your own `docs/explanatiion.md` says chaining; the code says otherwise — trust the code).
- eBPF uses `PERF_EVENT_ARRAY` (not `RINGBUF`) for events, and a `HASH` map to bridge kprobe/kretprobe state via `pid_tgid`.
- Memory-growth data plateaus in the second half of the endurance test — real evidence against a leak, and a specific number (+60kB in the final 30 min) worth citing directly.
