# Master Tool Index

*A single-page lookup across the entire BPF Performance Tools study-notes series — every tool covered in `01-introduction.md` through the appendix files, in one table. Find a tool here, then open the listed file for the full explanation, worked examples, and caveats.*

**How to use this**: scan by chapter/topic for "what would I reach for to answer X" — then go read the real file. This index deliberately strips out the war stories, the caveats, and the *why* — that's what the 28 chapter files are for. This is purely "what exists and roughly what it does."

## Ch. 4 — BCC multi-tools (`04-bcc.md`)
| Tool | What it shows |
|---|---|
| `funccount` | How often a function/tracepoint/USDT probe fires |
| `stackcount` | Stack traces leading to an event, frequency-counted |
| `trace` | Custom per-event printing (args, retval, stack) |
| `argdist` | Argument/return-value distribution (count or histogram) |

## Ch. 6 — CPUs (`06-cpus.md`)
| Tool | What it shows |
|---|---|
| `execsnoop` | New process execution |
| `exitsnoop` | Process exit: age, exit code |
| `runqlat` | Run-queue (scheduler) latency histogram |
| `runqlen` | Run-queue length, sampled |
| `runqslower` | Individual run-queue waits over a threshold |
| `cpudist` | On-CPU time per scheduling turn |
| `cpufreq` | CPU clock frequency by process |
| `profile` | Timed CPU stack sampling → flame graphs |
| `offcputime` | Off-CPU time + stack, by reason |
| `syscount` | Syscall counts/latency, system-wide or by process |
| `funccount` (on kernel funcs) | Confirms hot-vs-frequent ambiguity from `profile` |
| `softirqs` / `hardirqs` | Time in soft/hard interrupt handling |
| `smpcalls` | Cross-CPU call latency and source |
| `llcstat` | LLC cache hit ratio by process (PMC-based) |
| `cpuwalk` | Which CPU each process ran on |
| `cpuunclaimed` | Idle CPUs coexisting with runnable threads elsewhere |
| `loads` | Load averages (mostly a `kaddr()` teaching example) |

## Ch. 7 — Memory (`07-memory.md`)
| Tool | What it shows |
|---|---|
| `oomkill` | Context at the moment of an OOM kill |
| `memleak` | Outstanding (potentially leaked) allocations |
| `mmapsnoop` | Every `mmap(2)` call |
| `brkstack` | Heap-growth (`brk()`) stack traces |
| `shmsnoop` | System V shared-memory calls |
| `faults` | Page faults by user stack |
| `ffaults` | Page faults by filename |
| `vmscan` | Page-out daemon / direct-reclaim latency |
| `drsnoop` | Per-event direct-reclaim latency |
| `swapin` | Who's paying the swap-in cost |
| `hfaults` | Huge-page fault confirmation |

## Ch. 8 — File Systems (`08-file-systems.md`)
| Tool | What it shows |
|---|---|
| `opensnoop` | File opens |
| `statsnoop` | `stat(2)` family calls |
| `syncsnoop` | `sync(2)` family calls |
| `mmapfiles` | Which files get `mmap()`'d |
| `scread` | Filename for a `read(2)`, via the FD table |
| `fmapfault` | Page faults for memory-mapped files |
| `filelife` | Short-lived file lifespans |
| `vfsstat` | Per-second VFS op counts (the `vmstat` of file I/O) |
| `vfscount` | Counts across all VFS functions |
| `vfssize` | Read/write size histograms by process + target |
| `fsrwstat` | I/O split by file system type (ext4 vs. socket vs. pipe) |
| `fileslower` | Slow *synchronous* file reads/writes |
| `xfsslower` / `ext4slower` / etc. | Same, per file system |
| `filetop` | `top` for hot files by read/write bytes |
| `writesync` | Synchronous (`O_SYNC`) writes |
| `filetype` | I/O classified by inode type |
| `cachestat` | Page-cache hit ratio |
| `writeback` | Dirty-page flush latency + reason |
| `dcstat` / `dcsnoop` | Directory-cache hit ratio / per-lookup detail |
| `icstat` | Inode-cache hit ratio |
| `mountsnoop` | Mount/unmount activity |
| `xfsdist` / `ext4dist` / etc. | Per-operation latency histograms |
| `bufgrow` | Buffer-cache growth by process |
| `readahead` | Read-ahead effectiveness (used vs. wasted) |

## Ch. 9 — Disk I/O (`09-disk-io.md`)
| Tool | What it shows |
|---|---|
| `biolatency` | Device I/O latency histogram |
| `biosnoop` | Per-I/O detail: PID, latency, sector, size |
| `biotop` | `top` for disk I/O by process |
| `bitesize` | I/O size distribution by process |
| `seeksize` | Requested seek distance (sequential vs. random) |
| `biopattern` | Actual completed I/O pattern (%random/%sequential) |
| `biostacks` | Full I/O latency *with the initiating stack* |
| `bioerr` | Disk I/O errors, with cause |
| `mdflush` | Software RAID flush events |
| `iosched` | I/O scheduler queueing time |
| `scsilatency` / `scsiresult` | SCSI command latency / result codes |
| `nvmelatency` | NVMe command latency by opcode |

## Ch. 10 — Networking (`10-networking-part1/2/3.md`)
| Tool | What it shows |
|---|---|
| `sockstat` | High-level socket event rates |
| `sofamily` / `soprotocol` | New connections by address family / protocol |
| `soconnect` / `soaccept` | Per-connection detail + decoded errno |
| `socketio` / `socksize` | Socket I/O counts / byte-size histograms |
| `sormem` | Receive-buffer usage vs. configured limit |
| `soconnlat` | Connection latency + calling code path |
| `so1stbyte` | Connect-to-first-byte latency |
| `tcpconnect` / `tcpaccept` | New active / passive TCP connections |
| `tcplife` | Full TCP session lifespan + throughput |
| `tcptop` | `top` for TCP throughput by process |
| `tcpretrans` | TCP retransmits, with state |
| `tcpsynbl` | SYN backlog size vs. limit |
| `tcpwin` | Congestion window over time (CSV, for plotting) |
| `tcpnagle` | Nagle-related transmit delay |
| `udpconnect` | New connected-UDP sessions |
| `gethostlatency` | DNS resolution latency |
| `ipecn` | Inbound ECN congestion signals |
| `superping` | Kernel-level ICMP RTT (validates `ping` itself) |
| `qdisc-fq` (+ 6 siblings) | Per-qdisc-scheduler queue latency |
| `netsize` | Packet size before/after segmentation offload |
| `nettxlat` | Device transmit-queue latency |
| `skbdrop` | Unusual packet drops + stack |
| `skblife` | `sk_buff` object lifespan |
| `ieee80211scan` | WiFi scan duration |
| `solisten` / `tcpstates` / `tcpdrop` / `sofdsnoop` | Listen detail / every TCP state transition / TCP-specific drops / FD-passing over Unix sockets |

## Ch. 11 — Security (`11-security.md`)
| Tool | What it shows |
|---|---|
| `elfsnoop` | ELF execution, tamper-resistant (mount+inode) identity |
| `modsnoop` | Kernel module loads |
| `bashreadline` | Interactively typed shell commands |
| `shellsnoop` / `ttysnoop` | Live shell/TTY session mirroring |
| `eperm` | Permission-denied syscalls |
| `tcpreset` | TCP RSTs (port-scan signature) |
| `capable` | Linux capability checks (for whitelisting) |
| `setuids` | UID-changing syscalls (login/sudo tracing) |
| `signals` / `killsnoop` | All signals / `kill(2)`-specific with sender |
| `deadlock` | Potential mutex lock-order-inversion deadlocks |

## Ch. 12 — Languages (`12-languages-part1/2/3.md`)
| Tool | What it shows |
|---|---|
| `jnistacks` | Stacks leading into JNI object creation |
| `javastat` / `javathreads` / `javacalls` / `javaflow` / `javagc` / `javaobjnew` | Java: per-second summary / thread lifecycle / method calls / call flow / GC events / object allocation |
| `bashfunc` / `bashfunclat` (ad hoc, this book's own worked examples) | Bash function calls and their latency |

## Ch. 13 — Applications (`13-applications.md`, MySQL as the example)
| Tool | What it shows |
|---|---|
| `threadsnoop` | New thread creation |
| `threaded` | On-CPU thread spread (how parallel is it *really*) |
| `offcpuhist` | Off-CPU time as a histogram, not just a sum |
| `ioprofile` | I/O syscalls with full calling stack |
| `mysqld_qslower` | Slow queries, with the query string |
| `mysqld_clat` | Command latency by command type |
| `pmlock` / `pmheld` | Lock wait time / lock hold time (user-level) |
| `naptime` | `nanosleep()` calls — finds forgotten debug sleeps |

## Ch. 14 — Kernel (`14-kernel.md`)
| Tool | What it shows |
|---|---|
| `wakeuptime` | Who woke a blocked thread, and from where |
| `offwaketime` | Blocked *and* waker stacks, fused (chain graphs) |
| `mlock` / `mheld` | Kernel mutex wait / hold time |
| `kmem` / `kpages` | Slab / page allocator activity |
| `slabratetop` | Slab allocation *rate* by cache name |
| `numamove` | NUMA page migration count + cost |
| `workq` | Work-queue execution latency by function |
| `inject` | Fault injection via `bpf_override_return()` |
| `criticalstat` | Long IRQ-disabled critical sections |

## Ch. 15 — Containers (`15-containers.md`)
| Tool | What it shows |
|---|---|
| `runqlat --pidnss` | Run-queue latency, broken out per container |
| `pidnss` | CPU switches *between* containers |
| `blkthrot` | cgroup block-I/O hard-limit throttling |
| `overlayfs` | OverlayFS read/write latency, filterable by container |

## Ch. 16 — Hypervisors (`16-hypervisors.md`)
| Tool | What it shows |
|---|---|
| `xenhyper` | Xen hypercall counts |
| `cpustolen` | Stolen CPU time histogram |
| `kvmexits` | Guest VM exit latency by reason (host-side) |

## Ch. 17 — Not command-line tools (`17-other-tools.md`)
Vector/PCP, Grafana/PCP, Cloudflare `ebpf_exporter` + Prometheus, `kubectl-trace` — GUIs and orchestration layered on top of everything above, not separate tools to learn.

---

**Reading this table alone won't teach you anything** — it's a lookup, not a lesson. When you find a tool here that sounds relevant, open its chapter file: that's where the actual latency numbers, the real production stories, the overhead caveats, and the "here's the bug this caught" narratives live. This index just gets you to the right door faster.
