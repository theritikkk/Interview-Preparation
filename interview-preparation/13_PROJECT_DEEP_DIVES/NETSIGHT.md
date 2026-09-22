# Project Deep Dive: NetSight — Linux Network Telemetry & Traffic Analysis Engine

🔴🔴🔴 Your most technically dense project and the one most likely to draw a skeptical "did you really build this" line of questioning — because it genuinely is advanced. Being able to explain mechanism, not just name-drop terms, is what separates a strong answer here from a weak one.

---

## Architecture (Reconstructed From Resume Claims)

```
NIC → Linux Kernel (AF_PACKET/TPACKET_V3, mmap'd ring buffer)    [03_CS_FUNDAMENTALS/OS.md Cluster 3]
        │  (zero-copy: kernel writes directly into shared mmap'd memory)
        ▼
   Capture Thread(s) (pthreads) → Protocol Parser (Ethernet/VLAN/IPv4/TCP/UDP/ICMP)  [NETWORKING.md]
        │
        ├──→ Lock-free Ring Buffer (C11 atomics)         [C.md Cluster 2]
        ├──→ Treiber-Stack Memory Pool (no malloc/free in hot path)
        └──→ 1M-entry Hash-based Flow Table (TCP connection tracking)
        
   eBPF Programs (attached via BTF CO-RE, kernels 5.4–6.x, single binary)  [NEEDS VERIFICATION: attach point]
        │
        ▼
   Prometheus metrics export · ncurses live UI

Validation: ~9,700 LOC / 11 modules, Unity tests + cppcheck + Valgrind
Load test: 43,442 TCP connections @ ~1,448 conn/s, 0 failures; 60-min endurance, <0.2% memory growth, 0 FD leaks
```

---

## Component-by-Component Justification

| Component | Why it exists | Why chosen (vs. alternative) | Failure mode | Scaling |
|---|---|---|---|---|
| **AF_PACKET/TPACKET_V3** | Raw packet capture bypassing the normal socket stack | `mmap`-shared ring buffer = zero-copy (no `copy_to_user`) vs. `recvfrom()`-based capture (one copy per packet) — see `OS.md` Cluster 3 | Ring buffer full under sustained burst → packet drops at the kernel level, detectable via kernel drop counters | Line-rate at the tested throughput; further scaling would mean multiple capture threads across RX queues (RSS) — `NEEDS VERIFICATION` if implemented |
| **C11-atomic ring buffer** | Lock-free handoff between capture and processing | No blocking under contention — a stalled thread means dropped packets at high rates (`C.md` Cluster 2) | ABA problem if not mitigated — `NEEDS VERIFICATION` on your specific mitigation | Bounded by cache-line contention at very high thread counts (a real, known lock-free-structure limit worth naming) |
| **Treiber-stack memory pool** | Avoids `malloc`/`free` churn in the hot path | Heap allocation under load is slow and fragmenting (`C.md` Cluster 1) — a fixed pool with lock-free push/pop avoids both | Pool exhaustion under sustained overload — what happens then? `NEEDS VERIFICATION` | Fixed-size by design — a real scaling ceiling worth naming, tunable via pool size at startup |
| **1M-entry hash-based flow table** | O(1) average TCP connection state lookup | Hash table over a tree/list for the pure lookup-speed requirement of high-rate flow tracking | Collision-heavy degradation if the hash function or table size is poorly matched to real traffic patterns; table full at exactly 1M distinct concurrent flows — what's the eviction/replacement policy? `NEEDS VERIFICATION` | Fixed capacity (1M) — explicit, named scaling ceiling |
| **eBPF (BTF CO-RE)** | Kernel-level TCP lifecycle observability without per-kernel recompilation | CO-RE (Compile Once – Run Everywhere) reads kernel type info (BTF) at load time to adjust struct offsets automatically — the alternative (recompiling per target kernel version) is exactly what CO-RE eliminates | An eBPF program that references a struct field removed in a newer kernel would fail to load — CO-RE handles offset changes, not removed fields entirely | N/A — eBPF programs run in-kernel with minimal overhead by design |

---

## The Named Questions From Your Own Prep Brief — Direct Answers

**"Why AF_PACKET?"** → It's the Linux mechanism for capturing raw packets at the link layer, bypassing the normal protocol stack processing — necessary when you need every packet, unprocessed, rather than what a normal socket would deliver you after kernel protocol handling.

**"What is TPACKET_V3?"** → The specific AF_PACKET ring-buffer version offering block-based (not just packet-based) delivery and better batching than TPACKET_V2 — reducing syscall overhead per packet at high rates. `NEEDS VERIFICATION`: be ready to name what TPACKET_V3 specifically improves over V2 if pushed (block-level polling instead of per-packet).

**"What does zero-copy mean here?"** → The kernel writes captured packet data directly into a `mmap`-shared memory region your process also maps — no `copy_to_user()` step moving data from a kernel buffer into a separate userspace buffer, which is the copy that a traditional `recvfrom()`-based capture path pays on every single packet. See `OS.md` Cluster 3 for the full mechanism.

**"How does packet capture actually work? What happens between NIC and user space?"** → NIC receives a frame → kernel network driver processes it → with AF_PACKET/TPACKET_V3 attached, a copy is placed directly into the `mmap`-shared ring buffer (this is the *one* copy that happens — from NIC/driver buffer into the shared ring, not into a private userspace buffer) → your userspace code polls/reads from that same shared memory region directly.

**"What is a ring buffer?"** → A fixed-size circular buffer with a read and write pointer, allowing a producer (kernel) and consumer (your capture thread) to hand off data without reallocation — full when the write pointer catches up to the read pointer, at which point new writes must wait or drop.

**"Why lock-free?"** → No thread ever blocks holding a lock across a preemption — critical at high packet rates where a stalled thread directly means dropped packets; see `C.md` Cluster 2's full treatment including the ABA problem and memory ordering.

**"How does C11 atomic memory ordering work?"** → Controls what reorderings the compiler/CPU may perform around an atomic operation — `memory_order_seq_cst` (strongest, default if unspecified) through `memory_order_relaxed` (weakest, fastest, easiest to get wrong) — see `C.md` Cluster 2 L2/L3 questions.

**"What is a Treiber stack? ABA problem?"** → A lock-free stack built on CAS-based push/pop (`C.md` Cluster 2's code example); the ABA problem is when a value changes A→B→A between a thread's read and its CAS, making the CAS succeed despite the world having changed underneath it — **`NEEDS VERIFICATION`: confirm and state your actual mitigation (tagged pointers, hazard pointers, or an honest acknowledgment if none is implemented) rather than claiming a specific technique you haven't verified.**

**"What is cache-line contention? False sharing?"** → Cache-line contention: multiple threads' atomic operations on the same cache line (even different variables within it) force repeated cache-coherency traffic between cores, degrading performance even without a "real" logical conflict. False sharing is the specific case of *logically unrelated* variables happening to share a cache line, causing this contention despite the threads not actually needing to coordinate — a subtle, easy-to-introduce performance bug in exactly the kind of tightly-packed, high-throughput data structures NetSight uses.

**"Why use a hash table for flows? How are TCP connections tracked?"** → O(1) average lookup by a flow key (typically the 5-tuple: source/dest IP, source/dest port, protocol) is essential at high connection rates — tracking TCP state (SYN seen, established, FIN/RST seen) per flow by observing packet flags and sequence numbers on the wire, without OS help, since AF_PACKET delivers raw packets, not connection state.

**"What is eBPF? BTF? CO-RE?"** → eBPF: a sandboxed, verified bytecode VM running inside the Linux kernel, letting you attach small programs to kernel events (tracepoints, kprobes) without writing a kernel module. BTF (BPF Type Format): kernel-embedded type/struct-layout metadata. CO-RE: eBPF programs use BTF at load time to automatically adjust for struct-field offset differences between kernel versions, so one compiled binary works across kernels 5.4–6.x without recompilation per target — see `03_CS_FUNDAMENTALS`/`C.md` cross-references and be ready to state this exact CO-RE mechanism precisely, since it's a very specific, checkable claim.

**"How does an eBPF program communicate with user space?"** → Typically via a BPF map (a kernel-resident key-value store the eBPF program writes into and userspace reads from) or via `perf_event`/ring-buffer-style streaming for event-by-event delivery. `NEEDS VERIFICATION`: confirm which mechanism NetSight actually uses before stating it as fact.

**"Why does this require Linux?"** → AF_PACKET, TPACKET_V3, and eBPF/BTF/CO-RE are all Linux-kernel-specific mechanisms — there's no direct equivalent API surface on other operating systems, making this project inherently platform-specific by the nature of the exact low-level primitives it's built on.

**"What happens under packet bursts? How would packet loss be detected?"** → The ring buffer can fill faster than the consumer drains it under a sustained burst exceeding capture-thread throughput; Linux exposes kernel-level drop counters (via `getsockopt` with `PACKET_STATISTICS` on the AF_PACKET socket) that would reveal drops even if your application-level counting doesn't catch them — a strong answer names this kernel-level counter specifically rather than only describing application-level symptoms.

**"How did you validate 43,442 connection attempts? How did you test memory growth?"** → `NEEDS VERIFICATION` on the exact load-generation tool/script; the memory-growth test methodology matters precisely (see the Claim Defense Sheet below — the same rigor point raised in `C.md`'s follow-up chain about your endurance test).

**"How would you profile the system?"** → `perf` for CPU-level hotspot profiling (which function/line is consuming cycles), Valgrind's Massif for heap profiling (though its overhead makes it unsuitable for a true line-rate capture test — worth naming this limitation), and the kernel drop counters above for capture-level loss — a well-rounded answer names tools appropriate to *different layers* of the system rather than one tool for everything.

---

## Quantitative Claim Defense Sheet

### "43,442 TCP connection attempts at ~1,448 conn/s, 0 failures"
- **What generated this load?** `NEEDS VERIFICATION` — likely a custom load-generation script or an existing tool (e.g., something built on top of raw sockets or a benchmarking tool) opening many rapid TCP connections against a target.
- **What does "0 failures" measure precisely?** Presumably: every connection attempt NetSight should have tracked was correctly observed/counted in the flow table — be ready to state the exact success criterion (e.g., flow-table entry count matching connection attempts) rather than a vague "it worked."
- **What would happen at 10x the rate?** An honest answer: the ring buffer and flow table both have finite capacity (1M entries) — at some multiple of tested throughput, either kernel-level drops (ring buffer full) or flow-table capacity limits would become the binding constraint; know which one you'd expect to hit first given your actual sizing.

### "~9,700 lines of C across 11 independently testable modules"
- **What makes a module "independently testable"?** Presumably each module (e.g., ring buffer, memory pool, flow table, parser) has its own Unity test suite exercising it in isolation, without needing the full capture pipeline running — a real, meaningful engineering practice worth being able to describe concretely (which modules, what each one's tests actually check) rather than asserting the count alone.

### "60-minute endurance test, <0.2% memory growth, 0 FD leaks"
- **The exact rigor point from `C.md`:** 0.2% isn't 0% — you need to know from your own data whether memory usage plateaus early (consistent with normal warm-up: pool pre-allocation, buffer growth to steady state) or grows roughly linearly across the full 60 minutes (which would suggest a slow leak the test window was simply too short to make more visible). Don't let "under 0.2%" stand in for "no leak" without being able to describe the growth *shape*, not just the endpoint number.

### Quick Revision
- Zero-copy = one specific eliminated copy (kernel→mmap'd shared memory, not kernel→private userspace buffer) — state it precisely.
- ABA-problem mitigation in your Treiber stack: verify and state your actual technique, don't assert a plausible-sounding one you haven't confirmed.
- CO-RE's mechanism is specific and checkable (BTF-driven offset adjustment at load time) — know it precisely, it's an easy claim to probe.
- The 0.2% memory-growth number needs its *shape* (plateau vs. linear) known from your own data, not just cited as a pass/fail threshold.
