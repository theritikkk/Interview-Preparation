# Full Book Review Quiz — 20 Questions, One Per Chapter

*A static copy of the interactive review quiz delivered in chat, for the archive. Each question maps to one chapter's most load-bearing idea. Answers and explanations are in the collapsed section at the end of each question.*

1. **Why does the book recommend trying a tracepoint before falling back to a kprobe for the same event, whenever both exist?**
   Tracepoints are a stable, maintained interface; kprobes can silently break across kernel versions — exactly what happened to `tcpsnoop` and `ext4dist` in real, documented cases.

2. **In a CPU flame graph, what does the x-axis (horizontal position) actually represent?**
   An alphabetical sort chosen to merge identical stacks — *not* time. Width, not position, is what's meaningful: it's proportional to a frame's share of total samples. (A flame *chart* is the one where x actually is time.)

3. **Per-CPU BPF maps solve the concurrent-counter race condition by...**
   Giving each CPU its own private copy of the map, eliminating shared state entirely — no lock needed, because there's nothing to race over.

4. **The USE Method's real advantage over "start from whatever metrics we already have" is that it...**
   Surfaces monitoring blind spots, by asking Utilization/Saturation/Errors of *every* resource whether or not an existing dashboard already covers it.

5. **Chapter 5's flagship `vfs_read()` timer keys its `@start` map by thread ID (`tid`). Why is that valid there but *not* valid for timing disk I/O in Chapter 9?**
   Disk I/O can be issued on one thread and completed on a different one (interrupt context, a kworker thread); `vfs_read()` entry and return happen on the same thread. `biolatency(8)` has to key by the `struct request` address instead.

6. **`runqlat(8)` and `runqlen(8)` both relate to CPU scheduling. What's the actual difference?**
   `runqlat` measures how long threads *wait* for CPU (the primary metric); `runqlen` measures how many threads are *waiting* (a cheaper, secondary proxy, good for explaining a `runqlat` finding).

7. **Why do most Chapter 7 memory tools trace page faults and `brk()`/`mmap()` calls instead of tracing `malloc()`/`free()` directly?**
   Allocation calls are far too frequent to trace directly without serious overhead (potentially 10x slowdown); faults and heap-growth syscalls are rarer proxy events that answer many of the same questions cheaply.

8. **Why does the book say `fileslower(8)` finding slow I/O is "probably a real application problem," in a way plain disk-latency numbers can't guarantee?**
   `fileslower` specifically targets *synchronous* I/O — I/O the application actually blocks on — unlike disk-level latency, which can reflect background work nobody's waiting on.

9. **What does "best-effort" PID/COMM attribution mean for many of Chapter 10's TCP-level tools?**
   TCP-level events don't always run in the originating process's own context, so the process shown by a kprobe deeper in the stack may not be the one that actually owns the connection — unlike syscall-level tools, which are reliable by construction.

10. **As of this book's writing, an unprivileged Linux user (no `CAP_SYS_ADMIN`) could load which BPF program types?**
    Only `BPF_PROG_TYPE_SOCKET_FILTER` and `BPF_PROG_TYPE_CGROUP_SKB` — the direct, visible reason every tool in this book requires root and lives in man-page section 8.

11. **Why can't a uprobe simply be placed directly at a JIT-compiled function's address, even with a known supplemental symbol file?**
    Two reasons: the JIT can relocate the function without telling the kernel (risking memory corruption on removal), and JIT code typically lives in anonymous mappings with no backing file at all — uprobes are fundamentally file/inode-based.

12. **What did the live before/after `bash` demonstration in Chapter 12 show about `-fno-omit-frame-pointer`?**
    It fixed the stack walk *within* bash — but the same stack still broke again the moment it crossed into an unfixed `libc`. Fixing one binary only fixes walking within that binary.

13. **Why does thread ID work as a per-request correlation key for MySQL (a service thread pool) but not for an event-worker-thread architecture like Node.js?**
    In a service thread pool, one thread handles one request start-to-finish. In an event-worker model, one thread services pieces of many concurrent requests — `tid` no longer identifies a single request.

14. **Why does BCC maintain an explicit list of banned kretprobe targets for kernel spin-lock functions?**
    Instrumenting them with a kretprobe was found to be able to deadlock the entire system — a genuine safety constraint, not just an inconvenience. (The book's own workaround: use CPU profiling instead.)

15. **Why can't a BPF tool simply filter its output "by container ID" the way it can filter by PID?**
    A container has no single kernel-level identifier — it's really just a combination of cgroups and namespaces. Tools have to read a PID namespace ID or cgroup ID directly out of kernel structs and map it back to a human container name themselves.

16. **Why does `funccount 't:xen:xen*'` return zero results on a Xen HVM guest, when it works fine on a Xen PV guest?**
    HVM code paths make native calls the hypervisor traps transparently, rather than explicit guest-initiated hypercalls — there's nothing for those tracepoints to fire on.

17. **In a latency heat map, what do the three axes (x, y, color) represent?**
    x = time, y = latency, color saturation = event count in that cell — able to reveal a persistent slow mode that a single aggregate histogram of the same data would average away.

18. **Why does the book recommend sampling CPU profiles at 99 Hz instead of a rounder number like 100 Hz?**
    A round sampling interval can fall into lockstep with periodic application timers, producing systematically wrong (0% or 100%) aliasing errors. 99 Hz avoids coinciding with typical round-number intervals.

19. **In raw C BPF (Appendix D), why does `bitehist`'s hash-map increment need `__sync_fetch_and_add()`, when bpftrace's `count()` never requires you to think about this at all?**
    `count()` uses a per-CPU map internally, eliminating the race entirely. A hand-written ordinary hash map in raw C has no such protection built in, so the atomic operation has to be added explicitly.

20. **What's the general lesson from the `tcpsnoop` story (Ch.10), restated as explicit design advice in Chapter 18?**
    "Solve the specific problem and no more." `tcpsnoop`'s eleven kprobes and deep struct usage meant nearly every kernel update was a chance for something to break — fewer kprobes and struct-field dependencies means less to break across kernel versions.

---
*For the interactive, self-graded version of this quiz with hints and per-question feedback, see the chat where it was originally delivered.*
