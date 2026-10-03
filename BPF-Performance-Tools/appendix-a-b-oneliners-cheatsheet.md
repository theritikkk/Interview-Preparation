# Appendixes A & B — bpftrace One-Liners and Cheat Sheet

*Study notes on Appendixes A–B of BPF Performance Tools (Brendan Gregg, 2019). Original notes, not book text — see `00-index.md` for how these notes are written.*

> These two appendixes are pure reference material — not new concepts, but a curated, all-in-one-place version of things scattered across Chapters 6–14. Appendix A collects one-liners chapter by chapter (each one is a real idiom you've already met in context in the corresponding chapter file); Appendix B is a single-page syntax cheat sheet for the whole bpftrace language. Treat this file as the thing you actually keep open in a tab while writing your own scripts.

## Appendix A — bpftrace One-Liners, organized by chapter

**Chapter 6 (CPUs)**
```
# New processes with arguments
bpftrace -e 'tracepoint:syscalls:sys_enter_execve { join(args->argv); }'

# Syscall count by process
bpftrace -e 'tracepoint:raw_syscalls:sys_enter { @[pid, comm] = count(); }'

# Sample running process name at 99 Hz
bpftrace -e 'profile:hz:99 { @[comm] = count(); }'

# Sample user-level stacks at 49 Hz, for one PID
bpftrace -e 'profile:hz:49 /pid == 189/ { @[ustack] = count(); }'

# Trace new threads via pthread_create()
bpftrace -e 'u:/lib/x86_64-linux-gnu/libpthread-2.27.so:pthread_create {
    printf("%s by %s (%d)\n", probe, comm, pid); }'
```

**Chapter 7 (Memory)**
```
# Count heap expansion (brk()) by code path
bpftrace -e 'tracepoint:syscalls:sys_enter_brk { @[ustack, comm] = count(); }'

# Count page faults by process
bpftrace -e 'software:page-fault:1 { @[comm, pid] = count(); }'

# Count user page faults by user-level stack
bpftrace -e 'tracepoint:exceptions:page_fault_user { @[ustack, comm] = count(); }'

# Count vmscan operations by tracepoint
bpftrace -e 'tracepoint:vmscan:* { @[probe]++; }'
```

**Chapter 8 (File Systems)**
```
# Trace opens with process name
bpftrace -e 't:syscalls:sys_enter_open { printf("%s %s\n", comm, str(args->filename)); }'

# Distribution of read() request sizes
bpftrace -e 'tracepoint:syscalls:sys_enter_read { @ = hist(args->count); }'

# Distribution of read() bytes actually read (and errors)
bpftrace -e 'tracepoint:syscalls:sys_exit_read { @ = hist(args->ret); }'

# Count VFS calls by function
bpftrace -e 'kprobe:vfs_* { @[probe] = count(); }'

# Count ext4 tracepoints
bpftrace -e 'tracepoint:ext4:* { @[probe] = count(); }'
```

**Chapter 9 (Disk I/O)**
```
# Count block I/O tracepoints
bpftrace -e 'tracepoint:block:* { @[probe] = count(); }'

# Block I/O size histogram
bpftrace -e 't:block:block_rq_issue { @bytes = hist(args->bytes); }'

# Block I/O request user stacks
bpftrace -e 't:block:block_rq_issue { @[ustack] = count(); }'

# Block I/O type flags (rwbs)
bpftrace -e 't:block:block_rq_issue { @[args->rwbs] = count(); }'

# Block I/O errors with device and type
bpftrace -e 't:block:block_rq_complete /args->error/ {
    printf("dev %d type %s error %d\n", args->dev, args->rwbs, args->error); }'

# Count SCSI opcodes / result codes
bpftrace -e 't:scsi:scsi_dispatch_cmd_start { @opcode[args->opcode] = count(); }'
bpftrace -e 't:scsi:scsi_dispatch_cmd_done { @result[args->result] = count(); }'

# Count SCSI driver functions
bpftrace -e 'kprobe:scsi* { @[func] = count(); }'
```

**Chapter 10 (Networking)**
```
# Count accept(2)/connect(2) by PID and process
bpftrace -e 't:syscalls:sys_enter_accept* { @[pid, comm] = count(); }'
bpftrace -e 't:syscalls:sys_enter_connect { @[pid, comm] = count(); }'

# Socket send/receive bytes by on-CPU PID and process
bpftrace -e 'kr:sock_sendmsg,kr:sock_recvmsg /retval > 0/ {
    @[pid, comm, retval] = sum(retval); }'

# Count TCP send/receives, and bytes as histograms
bpftrace -e 'k:tcp_sendmsg,k:tcp*recvmsg { @[func] = count(); }'
bpftrace -e 'k:tcp_sendmsg { @send_bytes = hist(arg2); }'
bpftrace -e 'kr:tcp_recvmsg /retval >= 0/ { @recv_bytes = hist(retval); }'

# Count TCP retransmits by type and remote host (IPv4)
bpftrace -e 't:tcp:tcp_retransmit_* { @[probe, ntop(2, args->saddr)] = count(); }'

# UDP send bytes as a histogram
bpftrace -e 'k:udp_sendmsg { @send_bytes = hist(arg2); }'

# Count transmit kernel stacks
bpftrace -e 't:net:net_dev_xmit { @[kstack] = count(); }'
```

**Chapter 11 (Security)**
```
# Count security (LSM) hook calls for one PID
bpftrace -e 'k:security_* /pid == 1234/ { @[func] = count(); }'

# Trace PAM session starts
bpftrace -e 'u:/lib/x86_64-linux-gnu/libpam.so.0:pam_start {
    printf("%s: %s\n", str(arg0), str(arg1)); }'

# Trace kernel module loads
bpftrace -e 't:module:module_load { printf("load: %s\n", str(args->name)); }'
```

**Chapter 13 (Applications)**
```
# Sum malloc() requested bytes by user stack (high overhead)
bpftrace -e 'u:/lib/x86_64-linux-gnu/libc-2.27.so:malloc { @[ustack(5)] = sum(arg0); }'

# Trace kill() signals: sender, target, signal number
bpftrace -e 't:syscalls:sys_enter_kill { printf("%s -> PID %d SIG %d\n",
    comm, args->pid, args->sig); }'

# Count libpthread mutex-lock / condition-variable functions, for exactly one second
bpftrace -e 'u:/lib/x86_64-linux-gnu/libpthread.so.0:pthread_mutex_*lock {
    @[probe] = count(); } interval:s:1 { exit(); }'
bpftrace -e 'u:/lib/x86_64-linux-gnu/libpthread.so.0:pthread_cond_* {
    @[probe] = count(); } interval:s:1 { exit(); }'
```

**Chapter 14 (Kernel)**
```
# Count syscalls by resolved function name (via sys_call_table)
bpftrace -e 'tracepoint:raw_syscalls:sys_enter {
    @[ksym(*(kaddr("sys_call_table") + args->id * 8))] = count(); }'

# Count kernel functions matching a name pattern
bpftrace -e 'kprobe:attach* { @[probe] = count(); }'

# Time vfs_read() as a histogram (the flagship Ch.5 idiom)
bpftrace -e 'k:vfs_read { @ts[tid] = nsecs; } kr:vfs_read /@ts[tid]/ {
    @ = hist(nsecs - @ts[tid]); delete(@ts[tid]); }'

# Frequency-count an argument / a return value
bpftrace -e 'kprobe:func1 { @[arg0] = count(); }'
bpftrace -e 'kretprobe:func1 { @[retval] = count(); }'

# Sample kernel stacks at 99 Hz, excluding idle
bpftrace -e 'profile:hz:99 /pid/ { @[kstack] = count(); }'

# Count context-switch stacks (kernel + user + process)
bpftrace -e 't:sched:sched_switch { @[kstack, ustack, comm] = count(); }'

# Count work-queue requests by function name
bpftrace -e 't:workqueue:workqueue_execute_start { @[ksym(args->function)] = count() }'
```

**What this list is really worth taking away as a whole**: nearly every one-liner here is one of a handful of recurring shapes — count-by-key, histogram-of-a-value, stack-trace-as-key, or the entry/return timestamp pair — applied to a different event source. Once those shapes are second nature (Chapter 5's real job), writing a new one-liner for an event you haven't traced before is mostly a matter of picking the right probe type and plugging it into the shape that fits your question.

## Appendix B — bpftrace Cheat Sheet

**Synopsis**
```
bpftrace -e 'probe /filter/ { action; }'
```

### Probes

| Probe | Meaning |
|---|---|
| `BEGIN`, `END` | Program start and end |
| `tracepoint:syscalls:sys_enter_execve` | `execve(2)` syscall |
| `tracepoint:syscalls:sys_enter_open` | `open(2)` syscall (also trace `openat(2)` separately — see Ch.1) |
| `tracepoint:syscalls:sys_exit_read` | `read(2)` syscall return (one variant) |
| `tracepoint:raw_syscalls:sys_enter` | All syscalls |
| `block:block_rq_insert` | Queue a block I/O request |
| `block:block_rq_issue` | Issue a block I/O request to the storage device |
| `block:block_rq_complete` | Block I/O completion |
| `sock:inet_sock_set_state` | Socket state change |
| `sched:sched_process_exec` | Process execution |
| `sched:sched_switch` | Context switch |
| `sched:sched_wakeup` | Thread wakeup event |
| `software:faults:1` | Page faults |
| `hardware:cache-misses:1000000` | Once every 1,000,000 LLC cache misses |
| `kprobe:vfs_read` | `vfs_read()` kernel function entry |
| `kretprobe:vfs_read` | `vfs_read()` kernel function return |
| `uprobe:/bin/bash:readline` | `readline()` entry, from `/bin/bash` |
| `uretprobe:/bin/bash:readline` | `readline()` return, from `/bin/bash` |
| `usdt:path:probe` | A USDT probe from `path` |
| `profile:hz:99` | Sample on all CPUs at 99 Hz |
| `interval:s:1` | Run on one CPU, once per second |

### Probe aliases

| Alias | Full name | | Alias | Full name |
|---|---|---|---|---|
| `t` | tracepoint | | `U` | usdt |
| `k` | kprobe | | `kr` | kretprobe |
| `p` | profile | | `s` | software |
| `h` | hardware | | `u` | uprobe |
| `ur` | uretprobe | | `i` | interval |

### Built-in variables

| Var | Meaning | | Var | Meaning |
|---|---|---|---|---|
| `comm` | On-CPU process name | | `username` | Username string |
| `pid`, `tid` | On-CPU PID, thread ID | | `uid` | User ID |
| `cpu` | CPU ID | | `kstack` | Kernel stack trace |
| `nsecs` | Time, nanoseconds | | `ustack` | User stack trace |
| `elapsed` | Nanoseconds since program start | | `probe` | Current full probe name |
| `arg0..N` | `[uk]probe` arguments | | `func` | Current function name |
| `args->` | Tracepoint arguments | | `$1..$N` | CLI args, integer |
| `retval` | `[uk]retprobe` return value | | `str($1)...` | CLI args, string |
| `cgroup` | Current cgroup ID | | `curtask` | Pointer to the current `task_struct` |

### Actions

| Action | Meaning |
|---|---|
| `@map[key1, ...] = count()` | Frequency count |
| `@map[key1, ...] = sum(var)` | Sum a variable |
| `@map[key1, ...] = hist(var)` | Power-of-two histogram |
| `@map[key1, ...] = lhist(var, min, max, step)` | Linear histogram |
| `@map[key1, ...] = stats(var)` | Count, average, and total in one line |
| `min(var)`, `max(var)`, `avg(var)` | Min, max, average |
| `printf("format", var0..varN)` | Print variables (use `print()` for aggregations/maps) |
| `kstack(num)`, `ustack(num)` | Print `num` lines of the kernel/user stack |
| `ksym(ip)`, `usym(ip)` | Kernel/user symbol string from an instruction pointer |
| `kaddr("name")`, `uaddr("name")` | Kernel/user address from a symbol name |
| `str(ptr[, len])` | String from an address |
| `ntop([af,] addr)` | IP address to string |

### Asynchronous actions

| Action | Meaning |
|---|---|
| `printf("format", var0..varN)` | Print variables |
| `system("format", var0..varN)` | Run a shell command line |
| `time("format")` | Print a formatted timestamp |
| `clear(@map)` | Clear a map — delete all keys |
| `print(@map)` | Print a map's contents |
| `exit()` | Exit the program |

### Command-line switches

| Switch | Meaning |
|---|---|
| `-e` | Trace this probe description (given directly on the command line) |
| `-l` | List matching probes instead of tracing them |
| `-p PID` | Enable USDT probes on this PID |
| `-c 'command'` | Invoke this command, and trace until it exits |
| `-v`, `-d` | Verbose and debug output |

---
*Next up: Appendix C (BCC Tool Development) — the promised full walkthrough of building a real BCC tool from scratch, referenced back to throughout Chapters 4 and 13.*
