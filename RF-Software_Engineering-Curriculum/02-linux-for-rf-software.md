[← Table of Contents](README.md)

---

# 2. Linux, specifically for this track (mapped to *Linux Kernel Development*, 3rd ed.)

You already own this book. Here's exactly where to spend deep effort versus skim, mapped to Love's chapters.

**CRITICAL — master these:**
- **Process scheduling, especially real-time classes** (`SCHED_FIFO`/`SCHED_RR`) — Ch. *Process Scheduling*. This is the theory behind why your DSP thread doesn't get pre-empted at the wrong moment.
- **CPU affinity/isolation** (`taskset`, `isolcpus`, cpusets) — practical extension of the scheduling chapter; pinning a DSP-heavy thread off the noisy cores is a real technique you'll use in Stage 10/Project 9.
- **Interrupts and bottom halves** (softirqs, tasklets, threaded IRQs) — Ch. *Interrupts and Interrupt Handlers*. IQ sample delivery from an SDR/FPGA is interrupt- or DMA-driven; this chapter is the mental model for that path.
- **Memory management fundamentals** — virtual memory, page allocation, `mmap`, avoiding page faults on a real-time path (`mlock`/`mlockall`) — Ch. *Memory Management*.
- **Kernel synchronization** — spinlocks vs. mutexes, atomics, memory barriers — Ch. *Kernel Synchronization Introduction* + *Kernel Synchronization Methods*. This maps almost directly onto the lock-free ring buffers you'll write in user space between DSP pipeline stages.
- **Timers and time management** — Ch. *Timers and Time Management*. A cellular slot/TTI deadline (0.5ms/1ms) is literally a real-time timer problem.

**USEFUL — read once, don't need to derive from scratch:**
- Kernel modules & device drivers — you'll more often read/debug someone else's SDR or FPGA driver than write one from scratch, so understanding the shape (probe/init, ioctl, mmap-to-userspace) matters more than authoring one.
- The kernel/user boundary & syscall overhead — relevant when a job description mentions kernel-bypass datapaths (DPDK-style), which shows up directly in real postings for L1/L2 RAN-side roles.
- Virtual Filesystem chapter — skim, general OS literacy only.

**NOT NECESSARY for this track — skip or skim only:**
- Filesystem internals (ext4, block I/O deep mechanics)
- Networking stack internals — unless you specifically target Core Network (N3/N6) software, which is a different track from PHY/baseband
- Deep power-management internals — unless you land on a mobile SoC power team specifically
- Kernel build-system internals beyond "compile and load a module to test something"

---

[← 1. Career Map (reference)](01-career-map.md) · [3. The Roadmap →](03-roadmap-overview.md) · [Table of Contents](README.md)
