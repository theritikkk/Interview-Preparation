# P11 — Systems Baseline: Operating Systems, Linux, Concurrency and Protocol Layers

*Prerequisite module 11 · ⏱ about 8–10 hours · assumes P08–P10 · every program and number below was **run on Linux (kernel 6.18, 1 CPU core, 4 KiB pages)** with Python 3.12 / g++ 13.3. Timing values are machine-dependent — what matters is the **pattern**; I give ranges where two runs differed.*

> **Where this unlocks the curriculum.** The curriculum's own Linux chapter (a guide to Love's *Linux Kernel Development*) lists what is **critical**: real-time scheduling classes (`SCHED_FIFO`/`SCHED_RR`), **CPU affinity** (`taskset`, `isolcpus`), **interrupts and bottom halves**, **virtual memory** (`mmap`, `mlock`), **kernel synchronisation** (spinlocks vs mutexes, atomics, barriers) and **timers** — "a cellular slot/TTI deadline (0.5 ms/1 ms) is literally a real-time timer problem". It also says **networking-stack internals are *not* necessary** unless you target Core Network. This module gives you the vocabulary and a measured feel for those topics: Lesson 24 (threads, lock-free SPSC), Lesson 23 (hot path never allocates), Lesson 27 (profiling), Lessons 20/21/33/38/39 (HARQ, RLC/PDCP, protocol traces, conformance). It is **not** a kernel course.

---

## 11.1 The big picture: a radio is a real-time program on an operating system

An SDR or modem delivers sample blocks on a **fixed schedule**; your code must finish each block **before the next arrives** (a 20 MHz LTE subframe = 1 ms = 30,720 samples = **245,760 bytes** as `cf32`). That is a *deadline*. Three things can make you miss it: (1) the OS **doesn't run you** (scheduling, interrupts, other processes), (2) **memory** stalls (page faults, cache misses, allocation), (3) **synchronisation** (waiting on a lock). The first half of this module is about those three; the second half is about the **protocol-layer vocabulary** you need to read the L2/L3 lessons.

**Per-sample budget:** $1\,\text{ms}/30{,}720=\mathbf{32.6\ ns}$ per sample. Keep that number in mind for everything below.

---

## 11.2 Programs, processes, threads, system calls 🔥

* **Process:** a running program with its own **virtual address space**, file descriptors and resources. **Thread:** an independent flow of execution *inside* a process; threads **share** the process's memory (which is why they are fast to communicate and dangerous, §11.8).
* **Context switch:** the OS stops one thread and runs another (save/restore registers, switch memory maps). It costs microseconds and pollutes caches.
* **User mode vs kernel mode:** your code runs unprivileged; to read a file, allocate pages, or wait it makes a **system call** into the kernel.

**Measured cost of a system call** (`getppid`, one million times):

**sc.cpp**
```cpp
#include <chrono>
#include <cstdio>
#include <sys/syscall.h>
#include <unistd.h>
int main() {
    const int N = 1'000'000; long s = 0;
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) s += syscall(SYS_getppid);
    auto t1 = std::chrono::steady_clock::now();
    auto t2 = std::chrono::steady_clock::now();
    volatile long k = 0; for (int i = 0; i < N; ++i) k += i;
    auto t3 = std::chrono::steady_clock::now();
    std::printf("syscall getppid: %.0f ns each ; plain loop add: %.2f ns each (s=%ld)\n",
        std::chrono::duration<double, std::nano>(t1 - t0).count() / N,
        std::chrono::duration<double, std::nano>(t3 - t2).count() / N, s);
}
```
Real output (two runs): `syscall getppid: 110 ns each ; plain loop add: 0.34 ns each` and `115 ns / 0.39 ns` — a syscall is **~300× the cost of a simple add**.
*Consequence:* if you made even **one syscall per sample at 30.72 Msps** you would need $30.72\times10^{6}\times110\,\text{ns}=\mathbf{3.4\ s}$ of CPU time **per second** — physically impossible. This is why SDR code reads **blocks** and why hot paths avoid syscalls. *(Lesson 27 teaches how to find where the time really goes.)*

---

## 11.3 Scheduling, real-time classes and CPU affinity 🔥

The scheduler decides which thread runs on which CPU, and for how long. Default Linux threads use a **fair time-sharing** policy (`SCHED_OTHER`): many processes get slices, and you are pre-empted whenever the kernel decides. **Real-time classes** give strict priority:
* **`SCHED_FIFO`** — runs until it blocks or a higher-priority RT thread appears (no time slicing);
* **`SCHED_RR`** — like FIFO but round-robin among equal priorities.

*Verified in Python on this machine:* `os.sched_getscheduler(0)` returned **0 (`SCHED_OTHER`)** with priority 0; the constants are `os.SCHED_FIFO = 1`, `os.SCHED_RR = 2`; `os.sched_getaffinity(0)` returned **{0}** (one CPU). `taskset` and `chrt` are installed. **Setting** a real-time policy needs privileges and can starve the rest of the system — I did *not* change it here. `[FOUNDATION ADDED — privileges and risk are general Linux knowledge]`

**CPU affinity** (`taskset -c 2 ./prog`, or `os.sched_setaffinity`) pins a thread to chosen cores; **isolcpus/cpusets** reserve cores so noisy processes stay off them — the technique the curriculum's Linux chapter recommends for its Stage 10 / Project 9 real-time pipeline ("pinning a DSP-heavy thread off the noisy cores").

**Priority inversion:** a *low*-priority thread holds a lock that a *high*-priority thread needs, while a *medium*-priority thread keeps running — so the high-priority thread effectively waits on the medium one. Mitigations: priority inheritance, and (the main reason real-time code avoids locks) **lock-free queues** (Lesson 24).

---

## 11.4 Interrupts, DMA and how samples reach your code

An SDR/FPGA moves samples to memory by **DMA** (direct memory access — the device writes RAM without the CPU) and signals completion with an **interrupt**. The kernel runs a short **top half** immediately and defers the rest to a **bottom half** (softirq/tasklet/threaded IRQ); your user-space thread is then woken to consume the data.

```
 antenna → RF front-end → ADC ──DMA──▶ kernel ring buffer ──interrupt──▶ driver ──▶ your thread (user space)
                                                  ▲ if this thread isn't scheduled in time, the buffer overflows ("O"/overrun)
```
Where deadlines are lost: interrupt latency, scheduling delay, copying, page faults (§11.5). *(The Linux chapter marks interrupts and bottom halves as critical.)*

---

## 11.5 Virtual memory, pages and page faults 🔥

* **Virtual memory:** each process sees its own address space, divided into **pages** (**4,096 bytes** here — `getconf PAGESIZE`); the OS maps them to physical RAM *on demand*.
* A **page fault** occurs when a page isn't mapped yet (**minor**: just map a zeroed page — microseconds; **major**: read from disk — milliseconds).
* **`mmap`** maps a file into the address space (used by `np.memmap`, P09); **`mlock`/`mlockall`** pin pages so they can't be swapped or faulted later — real-time code locks memory and **pre-allocates before the hot loop** (the title of Lesson 23: "the hot path must never allocate").

**pagefault.py** — touching 100 MiB of freshly allocated memory:
```python
import resource
import numpy as np

def minor_faults():
    return resource.getrusage(resource.RUSAGE_SELF).ru_minflt

a = minor_faults()
x = np.zeros(100 * 1024 * 1024 // 8)            # 100 MiB, mapped lazily
b = minor_faults()
x[:] = 1.0                                       # touch every page
c = minor_faults()
print(f"allocate: +{b - a} faults ; touch all: +{c - b} faults ; 100 MiB / 4 KiB pages = {100 * 1024 * 1024 // 4096}")
```
Real output: `allocate: +1 faults ; touch all: +50 faults ; 100 MiB / 4 KiB pages = 25600`. *Not* 25,600: **50 = 100 MiB / 2 MiB**, which is consistent with the kernel using **2 MiB transparent huge pages** for this large allocation — I inferred that from the exact match and did not inspect the kernel setting. The lesson stands either way: **allocation is lazy, and the first touch of fresh memory triggers faults** (here ~50 × per 100 MiB; with 4 KiB pages it would be tens of thousands) — so pre-touch buffers before the real-time loop.

---

## 11.6 Caches and locality 🔥 (the physical reason behind Lesson 22)

The CPU is far faster than main memory, so it keeps recently used data in small, fast **caches** and moves data in **cache lines** of **64 bytes** (16 `float`s) — an assumption I'm using from general hardware knowledge (the sandbox's `getconf` did not report cache sizes). Reading one byte pulls in its whole line, so **neighbouring data is nearly free; scattered data is expensive.**

**stride.py** — sum 1,000,000 `float32` values, but spaced `stride` elements apart:
```python
import time
import numpy as np

M = 1_000_000                                  # elements summed in every test
big = np.ones(M * 64, dtype=np.float32)        # 256 MB; one cache line = 64 bytes = 16 floats

def best(f, reps=5):
    b = 1e9
    for _ in range(reps):
        t0 = time.perf_counter(); f(); b = min(b, time.perf_counter() - t0)
    return b

print(f"{'stride':>7} {'bytes between':>14} {'time (ms)':>10} {'ns/element':>11}")
for s in (1, 2, 4, 8, 16, 32, 64):
    v = big[::s][:M]
    t = best(lambda: v.sum())
    print(f"{s:>7} {4 * s:>14} {t * 1e3:>10.2f} {t / M * 1e9:>11.2f}")
```
Measured (three runs; the ranges show the run-to-run spread):

| Stride | Bytes between elements | ns per element |
|---|---|---|
| 1 | 4 | **0.21–0.31** |
| 4 | 16 | 0.53–0.72 |
| 16 | 64 (one element per cache line) | 2.1–2.4 |
| 64 | 256 | **6.5–7.8** |

The arithmetic is identical (1,000,000 additions) yet cost per element rises **25–30×**, because each strided element pulls in a whole cache line of which only 4 bytes are used. *Meaning:* **time follows the number of cache lines touched, not the number of operations.** This is why Lesson 22 compares array-of-structs with struct-of-arrays and why IQ buffers are kept contiguous (P08, P10).

*(An earlier attempt of mine — comparing `sum(axis=0)` with `sum(axis=1)` on a 2-D array — did not show the effect cleanly because NumPy vectorises both orders; test one idea at a time.)*

---

## 11.7 Time, timers and jitter 🔥

* **Wall clock** (`time.time`) can jump (NTP, manual changes); a **monotonic clock** (`time.perf_counter`, C++ `steady_clock`) only moves forward — **use it for measuring intervals and deadlines**.
* **Timer wake-ups are not exact.** Asking for a 1 ms sleep returns *at least* 1 ms later:

**jitter.py**
```python
import time
import numpy as np

d, t_prev = [], time.perf_counter()
for _ in range(3000):
    time.sleep(0.001)                            # ask for 1 ms
    t = time.perf_counter(); d.append((t - t_prev) * 1e3); t_prev = t
d = np.array(d)
print(f"requested 1.000 ms: mean {d.mean():.3f}  p50 {np.percentile(d, 50):.3f}  p99 {np.percentile(d, 99):.3f}  max {d.max():.3f} ms ;"
      f" intervals > 1.2 ms: {(d > 1.2).sum()} of 3000")
```
Three runs on this (non-real-time, virtualised) machine: mean **1.09 ms**, median **1.084–1.087 ms**, p99 **1.18–1.22 ms**, **max 1.43, 1.57 and 2.00 ms**, with **14, 31 and 44 of 3,000** intervals above 1.2 ms. So a "1 ms" timer is typically ~0.09 ms late and occasionally **0.6–1.0 ms late** — against a 1 ms budget that is a missed deadline. *(Percentiles, not the mean, expose this — P05 §5.10.)*

**Thread wake-up latency** — two threads handing a token back and forth through a condition variable:

**pingpong.cpp**
```cpp
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <thread>
int main() {
    std::mutex m; std::condition_variable cv; int turn = 0; const int N = 20000;
    std::thread other([&] {
        for (int i = 0; i < N; ++i) { std::unique_lock<std::mutex> l(m); cv.wait(l, [&] { return turn == 1; }); turn = 0; cv.notify_one(); }
    });
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) { std::unique_lock<std::mutex> l(m); turn = 1; cv.notify_one(); cv.wait(l, [&] { return turn == 0; }); }
    auto t1 = std::chrono::steady_clock::now();
    other.join();
    std::printf("condition-variable round trip between two threads: %.2f us average (includes 2 thread wake-ups)\n", std::chrono::duration<double, std::micro>(t1 - t0).count() / N);
}
```
Real output: **3.3–4.1 µs per round trip** over two runs (two wake-ups, so ~1.7–2 µs each) on this one-core VM. Fine for control, far too slow to put in a per-sample path (**32.6 ns** budget).

### Orders of magnitude (use as a mental map)

| Event | Time |
|---|---|
| a simple arithmetic operation (measured) | **~0.3 ns** |
| a sequential cache-friendly element (measured, stride 1) | ~0.3 ns |
| a cache-unfriendly element (measured, stride 64) | ~6.5–7.8 ns |
| main-memory access | ~100 ns `[FOUNDATION ADDED — typical]` |
| a system call (measured) | **~110–115 ns** |
| thread wake-up round trip (measured) | **~3–4 µs** |
| a "1 ms" timer's typical lateness (measured) | ~90 µs, worst ~0.6–1 ms |
| a disk read | 100 µs – ms `[FOUNDATION ADDED — typical]` |

---

## 11.8 Concurrency: threads, data races, atomics, locks 🔥 (preview of Lesson 24)

Two threads writing the same memory without coordination cause a **data race** (undefined behaviour). The classic bug is the **lost update**: `counter++` is really *read → add → write*; if two threads interleave those three steps, one update overwrites the other.

**race.cpp** — each thread adds 5,000,000 to a shared counter (three versions):
```cpp
#include <atomic>
#include <cstdio>
#include <mutex>
#include <thread>

constexpr int N = 5'000'000;

volatile int plain_counter = 0;        // volatile only stops the optimiser; it does NOT make this thread-safe
std::atomic<int> atomic_counter{0};
int locked_counter = 0;
std::mutex m;

int main() {
    std::thread a1([] { for (int i = 0; i < N; ++i) plain_counter = plain_counter + 1; });
    std::thread a2([] { for (int i = 0; i < N; ++i) plain_counter = plain_counter + 1; });
    a1.join(); a2.join();
    std::printf("plain  : %d (expected %d)\n", (int)plain_counter, 2 * N);

    std::thread b1([] { for (int i = 0; i < N; ++i) atomic_counter.fetch_add(1, std::memory_order_relaxed); });
    std::thread b2([] { for (int i = 0; i < N; ++i) atomic_counter.fetch_add(1, std::memory_order_relaxed); });
    b1.join(); b2.join();
    std::printf("atomic : %d (expected %d)\n", atomic_counter.load(), 2 * N);

    std::thread c1([] { for (int i = 0; i < N; ++i) { std::lock_guard<std::mutex> g(m); ++locked_counter; } });
    std::thread c2([] { for (int i = 0; i < N; ++i) { std::lock_guard<std::mutex> g(m); ++locked_counter; } });
    c1.join(); c2.join();
    std::printf("mutex  : %d (expected %d)\n", locked_counter, 2 * N);
    return 0;
}
```
```bash
g++ -std=c++17 -O2 -pthread race.cpp -o race && ./race
```
**Real results (3 runs; the correct total is 10,000,000):**

| Version | Run 1 | Run 2 | Run 3 |
|---|---|---|---|
| plain `volatile int` | **5,000,000** | **6,476,316** | **5,381,016** |
| `std::atomic<int>` | 10,000,000 | 10,000,000 | 10,000,000 |
| `std::mutex` + `lock_guard` | 10,000,000 | 10,000,000 | 10,000,000 |

*Even on a **single** CPU core* (so the threads never ran truly in parallel) the plain counter lost **up to half** of the updates: the scheduler can pre-empt a thread between the read and the write. Also notice **`volatile` is not a fix** — it only stops the compiler optimising the variable away; it gives no atomicity or ordering.

**ThreadSanitizer finds it deterministically:** compile with `-fsanitize=thread -g -pthread`; real output began `WARNING: ThreadSanitizer: data race ... Write of size 4 ... SUMMARY: ThreadSanitizer: data race race.cpp:15`.

**What the fixes mean**
* **`std::atomic<T>`** makes a single operation indivisible (hardware support); `memory_order` controls how other memory operations are ordered around it (`relaxed` = no ordering; `release`/`acquire` = publish/consume — the pair Lesson 24 uses for its SPSC ring buffer).
* **`std::mutex`**: only one thread holds the lock; others **block** (sleep) — safe, but blocking means unpredictable delays (priority inversion, §11.3).
* **Cost (measured, single thread, 10 M increments):** atomic ≈ **62 ms**, uncontended mutex ≈ **60 ms**, i.e. about **6 ns per operation**. Against the **32.6 ns per-sample budget** that is already ~18 % for *one* such operation (and a contended lock costs far more because the waiting thread sleeps and wakes — §11.7), so real-time pipelines exchange **blocks** and use carefully designed lock-free queues rather than locking per sample.
* **Deadlock:** two threads each hold one lock and wait for the other's — avoid by always taking locks in the same order.

**Single-producer / single-consumer (SPSC) queue** — the structure Lessons 23–24 build: a fixed-size circular buffer where *one* thread only writes and *one* thread only reads, so no lock is needed, only atomic read/write indices:

```
 producer → [ s0 | s1 | s2 | s3 | ... ] ← consumer         write_index (atomic)  read_index (atomic)
              circular buffer, pre-allocated once; full/empty decided by comparing the two indices
```
**Python note:** the *Global Interpreter Lock* means CPU-bound pure-Python threads do not run Python code in parallel; NumPy releases the GIL inside long operations. `[FOUNDATION ADDED — general Python knowledge; I could not demonstrate parallel speedup because this machine has one core.]`

---

## 11.9 The Linux command line you will actually use

*Verified mini-lab* (real output shown; `xxd`, `strace`, `perf`, `tmux`, `ssh`, `valgrind` were **not installed** here, so those are listed but unverified):

```bash
nproc                                   # 1  -> number of CPUs
getconf PAGESIZE                        # 4096
ls -l                                   # sizes: tone.cf32 = 64 bytes (8 samples x 8 B), x.cu8 = 4 bytes
od -A d -t f4 -N 16 tone.cf32           # first 4 floats = I0,Q0,I1,Q1 -> 7 0 6 1
od -A d -t u1 x.cu8                     # raw bytes of a cu8 file   (od is available; xxd was not)
cut -d' ' -f1 log.txt | sort | uniq -c | sort -rn     # count first words: 2 alpha / 1 gamma / 1 beta
grep -c alpha log.txt                   # 2    (count matching lines)
wc -l < log.txt                         # 4    (count lines)
chmod 640 run.sh; ls -l run.sh          # -rw-r-----   (owner rw, group r, others none)
seq 1 10 | tail -3                      # 8 9 10   (pipe: stdout of one command -> stdin of the next)
bash -c 'time sleep 0.2'                # real 0m0.202s
X=42 sh -c 'echo $X'; false; echo $?    # 42, then exit status 1 (0 = success)
ps -o pid,ni,cls,comm -p $$             # a process: PID, nice value 0, class TS (time sharing)
taskset -c 0 python3 script.py          # run pinned to CPU 0 (tool present; try on a multi-core box)
```
**Core ideas:** everything is a file; commands read **stdin**, write **stdout/stderr**; `|` pipes, `>` redirects; **exit status** 0 = success; **permissions** `rwx` for owner/group/others (`chmod`); `ps`/`top` show processes (`NI` = nice value, `CLS` = scheduling class: `TS` = time-sharing, `FF` = FIFO real-time). **Git** (installed) tracks your work. Also learn: `ssh` (remote shell), `tmux` (persistent terminals), `strace` (trace syscalls), `perf` (profiling — Lesson 27), `man <cmd>` (manual) — *unverified here*.

---

## 11.10 Protocol layers: the vocabulary of Lessons 20, 21, 33, 38, 39 🔥

Radio protocols are **layered**: each layer provides a *service* to the one above and uses the *service* of the one below. Data moves down on the sender and up on the receiver:

```
 SENDER                                                   RECEIVER
 Layer N+1   SDU ─────────────────────────────────────▶  SDU    (the service data unit is what the layer above hands you)
 Layer N     [header | SDU]  = PDU  ═════ over the link ═▶ strip header, deliver SDU upward
 Layer N-1   [header' | PDU]  ...
```
* **SDU** (service data unit): the payload received from the layer above. **PDU** (protocol data unit): what this layer sends = **header + SDU**. *Encapsulation* adds a header per layer; *decapsulation* removes it.
* A **header** is a few fields — flags, a **sequence number (SN)**, length — packed into bits/bytes. **Byte order:** multi-byte header fields are sent **big-endian ("network byte order")** (P08 §8.6).
* The **3GPP stack** (Lessons 20, 21, 33) names its layers **PHY / MAC / RLC / PDCP / RRC** (+ SDAP in NR); each does one job (RLC: segmentation and ARQ; PDCP: ciphering, header compression and sequence numbers; RRC: connection control — per Lesson 21). The **TCP/IP** stack uses application / transport / network / link / physical. Same idea, different names.
* The curriculum's Linux chapter says **kernel networking internals are not needed** for the PHY/baseband track — what you need is *this vocabulary* and the ability to read a trace.

### A toy protocol in code (verified)
Pack a header with bit operations, **segment** an SDU into PDUs, shuffle them, **reassemble** by SN, then compute **ARQ** behaviour. *(The 16-bit header here is a **toy**, not a 3GPP format.)*

**proto_clean.py**
```python
import math
import struct
import random
import numpy as np

# ---- a TOY 16-bit header: [Poll:1][Reserved:3][SN:12]  (NOT a real 3GPP format)
def pack_hdr(poll, sn):
    word = (poll & 1) << 15 | (sn & 0x0FFF)
    return struct.pack('>H', word)            # '>' = big-endian = "network byte order"

def unpack_hdr(b):
    (word,) = struct.unpack('>H', b)
    return (word >> 15) & 1, word & 0x0FFF

h = pack_hdr(1, 2500)
print("header bytes:", h.hex(), "-> (poll, sn) =", unpack_hdr(h))
print("little-endian would give:", struct.pack('<H', (1 << 15) | 2500).hex())

# ---- segmentation and reassembly of one SDU (service data unit)
sdu = bytes((i * 7) % 256 for i in range(1000))
MAX_PAYLOAD = 300
pdus = []
for sn, off in enumerate(range(0, len(sdu), MAX_PAYLOAD)):
    payload = sdu[off:off + MAX_PAYLOAD]
    last = off + MAX_PAYLOAD >= len(sdu)
    pdus.append(pack_hdr(1 if last else 0, sn) + payload)
overhead = sum(len(p) for p in pdus) - len(sdu)
print("PDUs:", len(pdus), "sizes:", [len(p) for p in pdus], "overhead bytes:", overhead, f"({overhead / len(sdu) * 100:.1f}%)")

rx = pdus[:]
random.Random(3).shuffle(rx)                  # out-of-order arrival
buf = {}
for p in rx:
    _, sn = unpack_hdr(p[:2])
    buf[sn] = p[2:]
print("reassembled equals original despite reordering:", b''.join(buf[k] for k in sorted(buf)) == sdu)
lost = unpack_hdr(rx[0][:2])[1]
got = {unpack_hdr(p[:2])[1] for p in rx[1:]}
print(f"if the first-arriving PDU (sn={lost}) were lost, the receiver sees a gap at sn:", [k for k in range(4) if k not in got])

# ---- window / throughput arithmetic
rate, pdu_bits, rtt = 10e6, 1000 * 8, 10e-3
t_tx = pdu_bits / rate
print(f"T_tx = {t_tx * 1e3:.2f} ms ; bandwidth-delay product = {rate * rtt / 8 / 1e3:.1f} kB")
print(f"stop-and-wait utilisation = {t_tx / (t_tx + rtt) * 100:.2f}% ; window for 100% = {math.ceil((t_tx + rtt) / t_tx)} PDUs ({(t_tx + rtt) / t_tx:.2f} exact)")
for W in (1, 4, 14):
    print(f"  window {W:>2}: utilisation {min(1, W * t_tx / (t_tx + rtt)) * 100:5.1f}%")

# ---- ARQ with random loss: attempts per PDU and tail latency
rng = np.random.default_rng(0)
p_loss, n, timer = 0.1, 200_000, 15e-3
attempts = rng.geometric(1 - p_loss, n)       # transmissions until success
delay = (attempts - 1) * timer + rtt / 2
print(f"mean transmissions per PDU: {attempts.mean():.4f} (theory 1/(1-p) = {1 / (1 - p_loss):.4f})")
p50, p99 = np.percentile(delay, [50, 99]) * 1e3
print(f"delay ms: p50 {p50:.1f}  p99 {p99:.1f}  max {delay.max() * 1e3:.1f}  mean {delay.mean() * 1e3:.2f}")
print("P(>=3 attempts) =", round(float((attempts >= 3).mean()), 4), " theory p^2 =", round(p_loss ** 2, 4))
```
Real output:
```
header bytes: 89c4 -> (poll, sn) = (1, 2500)
little-endian would give: c489
PDUs: 4 sizes: [302, 302, 302, 102] overhead bytes: 8 (0.8%)
reassembled equals original despite reordering: True
if the first-arriving PDU (sn=3) were lost, the receiver sees a gap at sn: [3]
T_tx = 0.80 ms ; bandwidth-delay product = 12.5 kB
stop-and-wait utilisation = 7.41% ; window for 100% = 14 PDUs (13.50 exact)
  window  1: utilisation   7.4%
  window  4: utilisation  29.6%
  window 14: utilisation 100.0%
mean transmissions per PDU: 1.1099 (theory 1/(1-p) = 1.1111)
delay ms: p50 5.0  p99 35.0  max 80.0  mean 6.65
P(>=3 attempts) = 0.0101  theory p^2 = 0.01
```
**What each part teaches**
1. **Packing:** `0x89C4` = `1000 1001 1100 0100` → poll bit 1, SN = `0x9C4` = 2500; sending it little-endian would reorder the bytes to `c489`, a classic protocol-implementation bug.
2. **Segmentation & reassembly (RLC's job):** a 1000-byte SDU became 4 PDUs (300+300+300+100 payload bytes), costing **8 header bytes = 0.8 %** overhead. **SNs let the receiver restore order and *detect a gap*** (a missing SN) — the trigger for a retransmission request.
3. **ARQ and window size:** **stop-and-wait** sends one PDU then waits a round-trip time (RTT). With a 10 Mbit/s link and 10 ms RTT, one 1000-byte PDU takes only 0.8 ms to send, so utilisation is $0.8/(0.8+10)=\mathbf{7.4\%}$. The **bandwidth–delay product** (12.5 kB) says how much data must be "in flight"; a **sliding window** of **14 PDUs** keeps the link 100 % busy. *(This is why windows and "HARQ processes" exist — Lesson 20.)*
4. **Loss and tail latency (ties to P05):** with 10 % PDU loss each PDU needs on average $1/(1-p)=1.11$ transmissions (simulated 1.1099), but **delay has a heavy tail**: median **5 ms**, but **p99 = 35 ms** and a worst case of 80 ms when a retransmission timer (15 ms) expires repeatedly; **P(≥ 3 attempts) = p² = 1 %**. Mean 6.65 ms hides this — exactly the "report percentiles" lesson of P05 and the **timer-and-retransmission** analysis you will do on real traces (Lesson 38).
5. **State machines and timers:** protocol entities are **finite-state machines** (idle/connected/inactive…) driven by messages and **timers** (a retransmission timer, a reordering timer). When you read a trace, ask: *which state was the entity in, which timer fired, which message was expected?*
6. **Reading a trace:** every line carries a **timestamp**, a layer, a direction and a message; the diagnostic skill is to **align events across layers by time** and find the first deviation from the expected sequence (Lesson 38). Conformance testing (Lesson 39) checks that exact sequence against the spec.

*Link to your college notes:* **encapsulation** is what Mobile IP does when it tunnels (IP-in-IP adds a 20-byte header: a 1000-byte packet becomes **1020 bytes, +2.0 %**, Unit II Ch. 11); GSM's **LAPDm** and GPRS's **GTP tunnel** are the same layering idea; the **SS7** signalling stack (Unit I Ch. 4) is a layered protocol too; and ARQ/retransmission is the "diversity vs ARQ" comparison (Unit I Ch. 7).

---

## 11.11 Common mistakes ⚠️
1. **Measuring with the wall clock** (`time.time`) instead of a monotonic clock.
2. **Using the mean** to judge a real-time system; check p99/p99.9/max (P05).
3. **Making a syscall or allocation per sample** in a hot path (110 ns vs a 32.6 ns budget).
4. **Assuming `volatile` means thread-safe.**
5. **Unsynchronised shared writes** — even "benign-looking" counters lose updates (5–6.5 M instead of 10 M above).
6. **Holding a lock while doing slow work**, or taking locks in inconsistent order (deadlock).
7. **Forgetting the first-touch page-fault cost** — pre-touch and pre-allocate buffers.
8. **Ignoring cache lines:** strided or scattered access can cost 25–30× per element.
9. **Running as a real-time process carelessly** — a spinning `SCHED_FIFO` thread can freeze the machine.
10. **Treating a toy header as a real one**, or ignoring byte order in protocol code.
11. **Using stop-and-wait over a high-latency link** (7.4 % utilisation in the example).
12. **Reading a trace without aligning timestamps and states.**

---

## 11.12 Practice problems (answers at the end)

1. How many 4 KiB pages are in 100 MiB? How many 2 MiB pages? Why did the measurement show 50 faults?
2. At 30.72 Msps `cf32`, how many bytes arrive per 1 ms subframe? What is the per-sample time budget?
3. A syscall costs 110 ns. What fraction of one core would one syscall per sample at 30.72 Msps need?
4. Two threads each execute `counter = counter + 1` five million times on a shared `int`. Explain why the final value can be less than 10 million (use the read–modify–write steps). Give two fixes and one detection tool.
5. Why is `volatile` not a fix for a data race?
6. Describe **priority inversion** with three threads and one remedy.
7. Stop-and-wait: RTT 20 ms, PDU 1500 bytes, link 20 Mbit/s. Find $T_{tx}$, utilisation, and the window needed for 100 %.
8. Decode the big-endian header bytes `89 c4` using the toy format (poll, SN). What would a little-endian reader see?
9. With PDU loss probability 0.2, what is the mean number of transmissions per PDU and P(≥ 3 attempts)?
10. A 2500-byte SDU is segmented with a 2-byte header and 600-byte maximum payload. How many PDUs, and what is the overhead percentage?
11. Write one shell pipeline that prints how many distinct words appear in the first column of `log.txt`.
12. Stride 16 vs stride 1 on `float32`: how many cache lines does each element touch, and by roughly what factor did per-element time rise in the measurement?
13. *Think:* a 1 ms timer on non-real-time Linux is sometimes ~1 ms late. List four things (from §11.3–11.5) you could do to reduce that jitter.

## 11.13 Active recall
1. Process vs thread; what do threads share? 2. What does a system call cost relative to an add? 3. Name the two real-time scheduling classes and what affinity does. 4. Describe interrupt → DMA → driver → user thread. 5. What is a page fault and why pre-touch memory? 6. Why is time proportional to cache lines touched? 7. Monotonic vs wall clock. 8. Why report percentiles for timers? 9. What is a data race and how can the three fixes differ? 10. What is an SPSC queue? 11. Define SDU, PDU, header, SN. 12. Why do sliding windows exist? 13. What is "network byte order"?

## 11.14 Self-check — ready for P12 and P13 if you can…
- [ ] explain why a hot path avoids syscalls, allocation and locks, with numbers;
- [ ] read `ps`/`taskset`/`chrt` output and describe scheduling class and affinity;
- [ ] reproduce and explain a lost-update race and its fixes;
- [ ] compute bandwidth–delay product, stop-and-wait utilisation and window size;
- [ ] explain encapsulation, segmentation/reassembly, ARQ and timers, and read a header's bits;
- [ ] use the basic Linux commands above without looking.

---

## ✅ Answer key

1. 100 MiB / 4 KiB = **25,600** pages; 100 MiB / 2 MiB = **50**. The measured **50** matches 2 MiB huge pages (my inference — the kernel setting was not inspected). 2. 30,720 samples × 8 bytes = **245,760 bytes**; budget 1 ms / 30,720 = **32.6 ns**. 3. 30.72×10⁶ × 110 ns = **3.38 s per second ≈ 338 % of a core** — impossible; read blocks instead. 4. The statement is *load, add, store*; thread B can load the same old value before thread A stores, so A's increment is overwritten ("lost update"). Fixes: `std::atomic` (indivisible increment) or a `std::mutex`; detect with **ThreadSanitizer** (`-fsanitize=thread`). 5. `volatile` only forces the compiler to keep memory accesses; it provides **no atomicity and no inter-thread ordering**. 6. Low-priority L holds a lock; high-priority H blocks on it; medium M (not needing the lock) pre-empts L, so H waits behind M. Remedy: **priority inheritance** (L temporarily runs at H's priority) or avoid the lock with a lock-free queue. 7. $T_{tx}=1500\times8/20\times10^{6}=\mathbf{0.6\ ms}$; utilisation $=0.6/(0.6+20)=\mathbf{2.9\%}$; window $=\lceil20.6/0.6\rceil=\lceil34.3\rceil=\mathbf{35}$ PDUs. 8. `0x89C4` = `1000 1001 1100 0100` → poll = **1**, SN = **0x9C4 = 2500**; a little-endian reader would read the word as `0xC489` → poll = 1, SN = 0x489 = **1161**. 9. $1/(1-0.2)=\mathbf{1.25}$ transmissions; $P(\ge3)=p^{2}=\mathbf{0.04}$. 10. ⌈2500/600⌉ = **5 PDUs** (600×4 + 100); overhead 5 × 2 = 10 bytes = **0.4 %**. 11. `cut -d' ' -f1 log.txt | sort -u | wc -l`. 12. Stride 1 touches ¹⁄₁₆ of a line per element; stride 16 touches **one whole line per element** (16× more lines per element); measured per-element time rose from ~0.2–0.3 ns to ~2.1–2.4 ns, **about 8–10×** (and 25–30× by stride 64). 13. E.g.: run the thread under **`SCHED_FIFO`** with a high priority; **pin it** to an isolated core (`taskset`/`isolcpus`); **`mlockall`** and **pre-touch/pre-allocate** all buffers; avoid locks/syscalls/allocation in the loop (use lock-free queues, block-sized I/O); keep other work off that core.

---

*Next: **P12 — Bridge to your college notes** (which parts of your Unit I/II Wireless Communication notes already cover what the RF curriculum assumes, and where the new curriculum goes deeper), then **P13 — Readiness exam**.*
