# Operating Systems

Load-bearing for NetSight (concurrency, memory, kernel/user-space boundary) and for reasoning about how any of your three backend/systems projects actually run.

---

## Cluster 1: Processes, Threads, Context Switching, Scheduling

### What is it?
A process is an independent execution unit with its own memory address space; a thread is a unit of execution *within* a process, sharing that address space with sibling threads. Context switching is the CPU saving one execution context (registers, program counter, stack pointer) and loading another.

### Why does it exist?
Processes give isolation (one process crashing doesn't corrupt another's memory); threads give concurrency *within* shared memory without the overhead of separate address spaces — the right choice depends on whether you need isolation or cheap shared-memory parallelism.

### How does it work?
Switching between processes is expensive (full address-space/TLB reload); switching between threads of the same process is cheaper (same address space, only registers/stack change) — this cost difference is *why* your Task Scheduler uses a worker *fleet* (likely separate processes for isolation/fault-tolerance — one worker crashing shouldn't take down others) while NetSight uses threads via pthreads for tight, shared-memory, lock-free coordination on the same captured packet stream.

### Interview Questions
- **L1:** "A process has its own address space; threads within a process share memory but have their own stack and registers. Context switching between threads is cheaper than between processes."
- **L2:** explain preemptive vs. cooperative scheduling, and name a scheduling policy (round-robin, priority-based) and its tradeoff (fairness vs. responsiveness for high-priority work).
- **L3:** explain why your NetSight worker threads sharing a lock-free ring buffer avoids the context-switch and copy overhead that a multi-process design would incur for the same workload — a direct architectural justification question.

### Resume Connection
🔴 Directly explains *why* NetSight is threaded (pthreads) while the Task Scheduler is process-based (worker fleet) — a strong "why this design" answer if you can state clearly which of your projects made which choice and why.

---

## Cluster 2: Synchronization — Mutex, Semaphore, Condition Variable, Deadlock, Starvation, Race Conditions

### What is it?
A **mutex** allows one thread into a critical section at a time. A **semaphore** generalizes this to N permits (useful for limiting concurrent access to a pool of N resources). A **condition variable** lets a thread sleep until another thread signals a state change, avoiding busy-waiting.

### How does it work?
```c
pthread_mutex_lock(&lock);
while (!condition_met) pthread_cond_wait(&cond, &lock);  // atomically unlocks + sleeps, relocks on wake
// critical section
pthread_mutex_unlock(&lock);
```
A **deadlock** requires four simultaneous conditions (mutual exclusion, hold-and-wait, no preemption, circular wait) — break any one and deadlock becomes impossible. A **race condition** is any outcome depending on unpredictable thread interleaving; it's a superset of "data race" (race conditions can happen even with correct locking, e.g., a check-then-act sequence across two separately-locked operations).

### Interview Questions
- **L1:** "A mutex is a binary lock; a semaphore allows N concurrent holders; a condition variable lets a thread wait efficiently for a state change instead of spinning."
- **L2:** name the four deadlock conditions and explain how a fixed lock-acquisition ordering across all threads eliminates circular wait specifically.
- **L3:** contrast this whole lock-based toolkit with NetSight's lock-free approach (`C.md`, Cluster 2) — explain precisely what problem class (deadlock, priority inversion under preemption) lock-free code sidesteps *by construction*, and what it costs you in return (harder correctness reasoning, ABA problem).

### DO NOT SAY THIS
"A race condition only happens without locks." — Improper lock granularity (e.g., a check-then-act split across two separately-acquired locks) can still race even with mutexes present everywhere.

### Follow-up Chain
> **Q: How would you prevent deadlock across multiple locks?**
> A: Enforce a global, consistent lock-acquisition order across all threads.
> **Follow-up: What if you can't control acquisition order because locks are acquired dynamically based on runtime data (e.g., transferring between two accounts, locking both)?**
> A: Use a tie-breaking rule independent of call order — e.g., always lock the resource with the lower memory address / ID first — so any two threads locking the same pair always agree on order regardless of which "direction" they're operating in.
> **Trap: "Using `try_lock` everywhere avoids deadlock."**
> Correct response: `try_lock` avoids *indefinite blocking* (you can back off and retry) but doesn't prevent deadlock outright — a poorly designed retry-without-backoff loop can livelock (threads perpetually failing to acquire and retrying) instead, which is a different but equally real problem.

---

## Cluster 3: Virtual Memory — Paging, Page Tables, TLB, mmap

### What is it?
Virtual memory gives each process the illusion of a large, private, contiguous address space; the OS maps virtual pages to physical frames via a **page table**, and the **TLB** (Translation Lookaside Buffer) caches recent translations to avoid a page-table walk on every memory access.

### How does it work?
`mmap()` maps a file (or a device, or shared memory) directly into a process's address space, so reads/writes to that memory region are transparently backed by the file/device without explicit `read()`/`write()` syscalls.

### Resume Connection 🔴
This is the mechanism directly underneath your NetSight claim of **zero-copy** packet capture: `TPACKET_V3` works by `mmap`-ing a ring buffer that's shared between the kernel's network driver and your user-space process — the kernel writes captured packets directly into that shared memory region, and your code reads them without the kernel needing to `copy_to_user()` the data into a separate userspace buffer. This is precisely what "zero-copy" means here, and being able to state this mechanism plainly is likely the single highest-value NetSight answer in this whole file.

### Interview Questions
- **L1:** "Virtual memory maps each process's address space to physical memory via page tables; the TLB caches translations to avoid a full page-table walk on every access."
- **L2:** explain a TLB miss's cost (page-table walk, possibly multiple levels) and why this is why the TLB matters for performance-sensitive code.
- **L3 — 🔴 NetSight-critical:** explain exactly why `mmap`-based `TPACKET_V3` capture is "zero-copy" relative to the traditional `recvfrom()`-based capture path (which requires the kernel to copy each packet from its internal buffer into a user-supplied buffer) — one fewer copy, directly reducing CPU cycles per packet at high packet rates.

### Common Mistakes
- Saying "zero-copy" without being able to explain which specific copy is eliminated and between which two memory domains
- Confusing virtual memory (address translation) with swapping (moving pages to disk) — related but distinct concepts

---

## Cluster 4: System Calls, File Descriptors, epoll, I/O Models

### What is it?
A system call is the controlled gateway from user space into kernel space (for I/O, memory, process control). A file descriptor is a small integer handle to an open resource (file, socket, pipe). `epoll` (Linux) lets one thread monitor many file descriptors for readiness, enabling high-concurrency I/O without a thread per connection.

### How does it work?
I/O models, in increasing sophistication: **blocking** (thread waits until I/O completes) → **non-blocking** (immediately returns, caller polls) → **I/O multiplexing** (`select`/`poll`/`epoll` — one thread waits on many descriptors at once) → **async I/O** (`io_uring`, kernel notifies completion, no polling at all).

### Interview Questions
- **L1:** "epoll lets a single thread efficiently monitor thousands of file descriptors for readiness, which is how Node.js's event loop achieves high concurrency without a thread per connection."
- **L2:** explain why `select`/`poll` don't scale as well as `epoll` at high connection counts (O(n) scan of all descriptors every call vs. epoll's O(1) amortized "give me only the ready ones").
- **L3:** connect this directly to your own stack: Node.js's event loop (used in Hermes and the Task Scheduler) is built on libuv, which uses epoll on Linux under the hood — explain how that lets a single-threaded Node process handle many concurrent connections without you writing any threading code yourself.

### Resume Connection
Explains the runtime mechanism under every Node.js service you've built, and is a direct sibling concept to NetSight's packet-level I/O.

### Quick Revision
- Process: isolated address space. Thread: shares address space, cheaper context switch.
- Deadlock needs all 4 conditions (mutual exclusion, hold-and-wait, no preemption, circular wait) — break one to prevent it.
- `mmap` + `TPACKET_V3` = NetSight's zero-copy mechanism — know this cold.
- `epoll` = how Node's single-threaded event loop scales to many connections.
