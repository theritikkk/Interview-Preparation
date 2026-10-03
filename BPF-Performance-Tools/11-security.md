# Chapter 11 — Security

*Study notes on Chapter 11 of BPF Performance Tools (Brendan Gregg, 2019). Original notes, not book text — see `00-index.md` for how these notes are written.*

> A shorter chapter (26 pages) with a different character from the subsystem chapters: it's split between **using BPF for security work** (monitoring, forensics, whitelisting) and **the security of BPF itself** (who's allowed to load a BPF program, and how that's controlled). No Traditional Tools or Strategy-first-for-the-usual-reasons section this time — the "strategy" here is really about choosing an instrumentation *point*, not diagnosing a performance problem.

## 11.1 Background

### What "security" covers here, and what BPF specifically adds

The book lists a wide net: security analysis, real-time forensics, privilege debugging, executable whitelisting, malware reverse engineering, monitoring, auditing, host/container intrusion detection (HIDS/CIDS), policy enforcement, and firewalling. **The explicit parallel drawn between security and performance engineering is worth keeping in mind for this whole chapter**: both are fundamentally about analyzing the behavior of a wide variety of software — which is exactly why so many tools in this chapter are simply *earlier chapters' tools, reused*, rather than a wholly new toolkit.

BPF answers security-relevant questions directly: which processes are executing; what network connections are being made, by whom; which privileges (capabilities) are being requested; what permission-denied errors are occurring; and whether a specific kernel/user function is being called with specific arguments — this last one is the basis of a genuinely compelling real capability worth understanding in detail.

### Zero-day vulnerability detection — a real, worked example

**The book's own example is worth walking through in full, since it's a genuinely persuasive demonstration of a capability unique to a flexible tracer like bpftrace.** A real Docker vulnerability, disclosed around the time of writing, exploited a **symlink-race attack**: calling `renameat2(2)` in a loop with the `RENAME_EXCHANGE` flag, combined with `docker cp`. Because this specific syscall-plus-flag combination is genuinely rare in normal production use (the author reports catching **zero** natural occurrences of it on his own production systems), a one-line detector is both simple and has a very low false-positive rate:
```
bpftrace -e 't:syscalls:sys_enter_renameat2 /args->flags == 2/ { time();
    printf("%s RENAME_EXCHANGE %s <-> %s\n", comm, str(args->oldname),
    str(args->newname)); }'
```
Run against a real proof-of-concept exploit, this one-liner — silent under normal conditions — immediately floods with matching output. **The general, reusable idea worth taking from this**: bpftrace's combination of a fast, easy-to-write language and access to kprobes/uprobes/tracepoints/USDT with full argument access means a **detector for a brand-new vulnerability can plausibly be written within minutes of its disclosure** — the book explicitly imagines a future where vulnerability disclosures ship with an accompanying bpftrace detection one-liner, the way network-based intrusion detection systems (the book names Snort) already share signature rules for new worms.

### Security monitoring, and BPF's own approach to safety and reliability

**BPF vs. kernel modules for monitoring, revisited from Chapter 2 with a security-specific angle**: traditional monitoring/auditing solutions built as loadable kernel modules carry real risk of introducing their own kernel bugs or vulnerabilities; BPF programs pass through the verifier and reuse existing, already-hardened kernel infrastructure. **A concrete efficiency number worth remembering**: a 2016 internal study comparing `auditd` logging against an equivalent BPF program found BPF introduced **roughly six times less overhead** — a direct, security-specific instance of the same "in-kernel summarization beats userspace processing" theme that's run through this whole book.

**A subtle, important reliability property worth understanding precisely**: BPF's output buffers and maps have finite capacity, and **can be exceeded** under extreme load — meaning events can genuinely go unrecorded. This isn't just a performance footnote here; it's a real **security** concern, since an attacker could deliberately try to flood a system with events specifically to evade logging or policy enforcement built on top of BPF. The book's stated requirement, worth remembering as a design principle for any security-relevant BPF tooling you might build: **BPF is aware when these limits are exceeded and can report that condition to user space — any real security solution must capture and act on that overflow signal itself**, to satisfy non-repudiation requirements (i.e., "we can prove we weren't just silently blind during an attack"). A specific, practical mitigation the book suggests: a dedicated **per-CPU counting map** (Chapter 2 §2.3.7's concurrency-safe pattern, reused here for a security purpose) for just counting important events — since a fixed-size per-CPU counter can never silently drop an increment the way a keyed map or the perf output ring buffer can under extreme load, you can pair it with the (potentially lossy) perf-buffer/detailed-event path to guarantee you at least always know the *true count*, even if some individual event *details* were dropped under load.

### Policy enforcement technologies built on BPF — a landscape overview

Explicitly flagged as outside this book's teaching scope, but worth being able to name and roughly place:

| Technology | What it does |
|---|---|
| **seccomp** | Runs (classic) BPF programs to make syscall allow/deny decisions; can kill the calling process, return an error, or hand a complex decision off to a user-space helper process for a verdict |
| **Cilium** | Transparent container/workload network security and load balancing, combining BPF programs at XDP, cgroup, and `tc` (traffic control) layers |
| **bpfilter** | A proof-of-concept aiming to eventually replace `iptables` with BPF entirely, with a transition path that converts existing `iptables` rulesets |
| **Landlock** | A BPF-based security module for fine-grained access control (e.g., restricting file system access via a BPF inode map updatable from user space) |
| **KRSI** (Kernel Runtime Security Instrumentation) | A newer Google-developed LSM for extensible auditing/enforcement, using a dedicated `BPF_PROG_TYPE_KRSI` program type |

**A real, forward-looking enforcement capability worth understanding**: `bpf_send_signal()` (landing in Linux 5.3, just after this book's writing) lets a BPF program send a signal (like `SIGKILL`) **directly from kernel context**, without needing seccomp as an intermediary — extending the earlier zero-day-detection one-liner into something that doesn't just *detect* an exploit attempt but can **terminate it immediately**:
```
bpftrace --unsafe -e 't:syscalls:sys_enter_renameat2 /args->flags == 2/ {
    time(); printf("killing PID %d %s\n", pid, comm); signal(9); }'
```
**A real, important caveat the book states plainly, worth taking seriously rather than treating this as a casual trick**: this specific example kills *every* process using `RENAME_EXCHANGE` with that flag, with **no way to distinguish a genuinely malicious process from an innocent one** that happens to trigger the same pattern — a blunt instrument, offered explicitly as a *temporary* workaround until proper patching, not a permanent security control. **Before `bpf_send_signal()` existed**, the same enforcement idea could only be done indirectly via bpftrace's `system()` (calling out to `kill -9` from user space, after the kernel event was already relayed up through the perf buffer) — introducing a real, **measurable lag between detection and enforcement** that `bpf_send_signal()`'s direct-from-kernel-context signal specifically eliminates.

### 11.1.2 Unprivileged BPF users — the current (2019) restriction

**As of Linux 5.2, a user without `CAP_SYS_ADMIN` can only load `BPF_PROG_TYPE_SOCKET_FILTER` or `BPF_PROG_TYPE_CGROUP_SKB` programs** — everything else in this entire book (every kprobe, uprobe, tracepoint, and USDT-based tool) requires elevated privilege. This is enforced directly in the `bpf(2)` syscall's own kernel source (`kernel/bpf/syscall.c`), and its practical, visible consequence is **why every single tool in this book lives in section 8 of the Unix man pages** — the section reserved for superuser/system-administration tools. BCC tools without sufficient privilege report "Need super-user privileges to run"; bpftrace checks for UID 0 specifically and reports "bpftrace currently only supports running as the root user" if not. The book flags this as an active area of ongoing kernel development (discussed at the LSFMM 2019 conference), with container environments specifically named as the motivating use case — being able to run BPF observability tools *from inside* a container without full host access.

### 11.1.3 Configuring BPF security — the actual sysctls

Worth having as a reference table, since these are real, checkable settings on any system you're securing:

| sysctl | Purpose |
|---|---|
| `kernel.unprivileged_bpf_disabled` | Disables unprivileged access entirely when set to 1 — **a one-way switch**: once set to 1, it cannot be set back to 0 |
| `net.core.bpf_jit_enable` | Enables the JIT compiler (0=disabled/default in the book's Linux 5.2 reference, 1=enabled, 2=enabled with debug traces written to the kernel log — explicitly flagged as debug-only, never production) |
| `net.core.bpf_jit_harden` | Additional JIT hardening against JIT-spraying attacks, at a performance cost (0=off/default, 1=unprivileged users only, 2=all users) |
| `net.core.bpf_jit_kallsyms` | Exposes compiled BPF JIT images via `/proc/kallsyms` for privileged-user debugging (automatically disabled if `bpf_jit_harden` is on) |
| `net.core.bpf_jit_limit` | A byte limit on module memory the JIT can consume; once exceeded, further unprivileged requests fall back to the interpreter (if compiled in) |

**A genuinely important, concrete security-vs-performance connection worth remembering**: `net.core.bpf_jit_enable` isn't purely a performance knob — `CONFIG_BPF_JIT_ALWAYS_ON` was added specifically as a **mitigation for the Spectre v2 speculative-execution vulnerability**, permanently forcing the JIT on and compiling the (comparatively more exploitable) BPF *interpreter* out of the kernel entirely. The book notes both Netflix and Facebook run with JIT enabled by default in production. **A portability caveat worth knowing**: the JIT is architecture-specific — x86_64 and arm64 are described as feature-complete and genuinely production-battle-tested, while support for other architectures (ppc64, s390x, sparc64, mips64, riscv) may be comparatively less mature.

### 11.1.4 Strategy — choosing an instrumentation point for something not already covered

A short, three-step decision order for security-instrumenting some activity none of this book's existing tools already cover:
1. Check whether a **tracepoint or USDT probe** already exists for it.
2. Check whether it goes through an **LSM (Linux Security Module) hook** — these all begin with `security_`, and (as noted in §11.1.2's policy-enforcement table) are specifically designed as security-relevant instrumentation points, making them unusually good, semantically meaningful kprobe targets even though they're technically still kprobes rather than a fully stable interface.
3. Fall back to ordinary **kprobes/uprobes** on the raw implementation code.

## 11.2 BPF Tools

Twelve tools, several of them recaps of tools from earlier chapters used here for a security lens rather than a performance one, plus several genuinely new ones written specifically for this chapter.

### 11.2.1, 11.2.7, 11.2.9 — Recaps: `execsnoop`, `opensnoop`, `tcpconnect`/`tcpaccept`

**`execsnoop(8)`** (Ch. 6) — new process execution, directly useful for spotting suspicious software (the book's own minimal example: a process named `a.out` executing from `/tmp`, exactly the kind of location/naming pattern worth flagging). **A real, important limitation restated here in a security context, not just a performance one**: `execsnoop` only sees `execve(2)`-based execution — a **buffer-overflow attack that injects and runs new instructions into an already-running process never calls `execve()` at all**, and is therefore fundamentally invisible to this tool, worth remembering as a real, named blind spot rather than assuming process-execution tracing catches all forms of "new code running." **`opensnoop(8)`** (Ch. 8) — file opens, useful for understanding malware behavior and general file-usage monitoring (a real example shows `polkitd` reading `/etc/passwd` and various `/proc/PID/status` files — routine, legitimate privilege-checking activity, shown here specifically as an example of what *normal* baseline activity looks like, useful for comparison). **`tcpconnect(8)`/`tcpaccept(8)`** (Ch. 10) — new TCP connections, useful since most network-based attacks involve at least one connection; real examples show a suspiciously-named `a.out` process connecting outbound, and a burst of inbound `sshd` connections arriving roughly every 200ms — read directly as a plausible SSH brute-force signature. **The book explicitly reinforces why these specific tools, rather than a full packet-capture approach, are the right default here**: tracing TCP *session* events rather than every individual *packet* is what keeps overhead low enough to run continuously — the same overhead lesson from Chapter 10, now stated as a security-tooling design principle specifically.

### 11.2.2 `elfsnoop` — ELF binary execution, with tamper-resistant identity verification

Traces `load_elf_binary()` — the single kernel function **all** ELF program execution must pass through — printing the interpreter (for scripts), filename, mount point, inode number, and result. **The specific security value of printing mount point *and* inode together, rather than just the path, is worth understanding precisely**: an attacker could create a malicious binary with the *same displayed path* as a trusted system binary (the book even notes this could be disguised further using control characters so the fake path visually matches) — but **cannot spoof the (mount, inode) pair**, since that's a direct, low-level identity of the actual underlying file being executed, not a string that can be forged at the display layer. This is a genuinely good example of choosing what to *log* based on what an adversary can and can't fake, not just what's convenient to print.

### 11.2.3 `modsnoop` — kernel module loads, a rootkit vector

Traces `do_init_module()`, printing the module name and the loading process/user/UID. **Framed explicitly around a specific threat model**: loading kernel modules is one of the ways rootkits install themselves, making this event specifically worth monitoring regardless of whether it's tied to any particular performance question. A `module:module_load` tracepoint also exists and is used by this chapter's own one-liners (§11.3) as the more modern, stable alternative to the kprobe shown in the main tool source.

### 11.2.4–11.2.6 `bashreadline`, `shellsnoop`, `ttysnoop` — shell and terminal forensics

**`bashreadline(8)`** traces `bash`'s own `readline()` function (via a uretprobe), logging every interactively-typed command system-wide — including shell built-ins and even **commands that failed** (the book's own example shows a typo'd `eccho hi` being logged just as faithfully as a successful `ls`). **The book candidly notes two real, named limitations worth remembering rather than assuming this tool sees "every command run on the system"**: it can't see commands typed into a *different* shell program (an attacker could deliberately install and use their own minimal shell — the book's own hypothetical example name is a "nanoshell" — specifically to evade this exact kind of logging), and on some Linux distributions `bash` is built to call `readline()` from a separate `libreadline` shared library instead of containing it directly, requiring the tool's uprobe target to be adjusted accordingly (with a forward-pointer to Chapter 12's fuller treatment of this exact kind of "where does the real function actually live" problem).

**`shellsnoop(8)`** mirrors a target shell session's output live, by tracing `write(2)` calls to `STDOUT`/`STDERR` from the target PID **and all of its child processes** (tracked via `sched_process_fork`, matching the book's own general "know your event's true scope" theme) — the child-tracking part is specifically necessary to also capture the *output of commands run inside that shell* (e.g., `date`'s own output), not just the shell's own prompt text. A genuinely fun, real feature worth knowing about: a `-r` **replay mode** emits a small shell script (a sequence of `echo`/`sleep` pairs) that, when run later, reproduces the **original session's output with its original timing preserved** — the book's own one-word reaction to watching this ("It's a little spooky") is worth remembering as a nice, human data point about just how complete this kind of forensic reconstruction can feel. Traced this tool's own history back to a 2004 original, itself inspired by an older Unix tool called `ttywatcher` — and the book notes that early version was referenced in a 2005 Phrack ezine article specifically as a security forensics technique, a small but real piece of evidence that this genre of tool has a long, serious history in the security community specifically, not just as a performance-analysis curiosity.

**`ttysnoop(8)`** is the closely related **TTY/PTS device** version — mirroring everything written to a given terminal device (`tty_write()`), useful for watching a suspicious login session live rather than reconstructing it after the fact. **The book's own origin story here is a genuinely great, concrete illustration of exactly this use case actually happening**: as a working sysadmin, the author used an ancestor of this tool to watch, in real time, a non-root intruder on a production system downloading privilege-escalation exploits, compiling them, and running them — unsuccessfully — noting (with evident amusement) that the most annoying part of the whole incident was watching the intruder use the `pico` text editor instead of `vi`. The tool's own name and inspiration trace back even further, to a program called `cuckoo.d`, itself named after Cliff Stoll's *The Cuckoo's Egg* — a genuinely famous, real-world account of tracking a state-sponsored hacker through exactly this kind of terminal-session monitoring in the 1980s, cited directly in the book's footnotes as "a more exciting story of TTY snooping." A structural detail worth remembering: the bpftrace version **requires** a device argument and exits immediately with a usage message if none is given — not just for user-friendliness, but because tracing *every* TTY device at once would create **feedback loop** with the tool's own terminal output (a nice, small echo of the exact `perf record`-tracing-its-own-writes feedback loop from Chapter 8's traditional-tools section).

### 11.2.8 `eperm` — permission-denied syscalls, and a genuinely clever numeric-to-symbol lookup

Counts syscalls failing specifically with `EACCES` ("permission denied") or `EPERM` ("operation not permitted") — errors worth flagging on their own merits for security review, grouped by process and syscall name. **The technique used to resolve a bare syscall number into a readable name is worth remembering as a distinct, reusable trick from the one Chapter 9's `syscount(8)` used**: rather than maintaining a separate syscall-ID-to-name lookup table (`syscount`'s approach), `eperm(8)` reads the address stored in the kernel's own `sys_call_table` at the relevant index, and resolves *that address* to a kernel symbol name via `ksym()` — using the kernel's own dispatch table as the source of truth rather than a separately-maintained list that could drift out of sync with it.

### 11.2.10 `tcpreset` — TCP resets as a port-scan signal

Traces `tcp_v4_send_reset()`, printing local/remote address and port for every RST sent. **The specific security logic worth understanding**: a port scan probes many ports in quick succession, including closed ones — closed ports respond with a TCP RST — so a burst of RSTs from one local host to one remote host's varying source ports, all within the same second, is a strong, simple signature. The book's own example output shows exactly this pattern (a wide, essentially unrelated spread of local ports, one shared remote host, all within the same one-second timestamp) and explicitly reads it as "this looks like a port scan." **A real, tested detail worth remembering**: the book confirmed against a Linux 4.15 kernel using `nmap` that SYN, FIN, NULL, and Xmas scan types **all** provoke RST responses from the Linux TCP/IP stack, meaning this one tool has genuinely broad coverage across several different port-scanning techniques, not just the most basic one.

### 11.2.11 `capable` — Linux capability usage, for whitelist construction

Traces `cap_capable()` — the kernel function that decides whether the current task actually holds a given [POSIX capability](https://man7.org/linux/man-pages/man7/capabilities.7.html) — printing UID, process, the specific capability requested (both numerically and by name, via a hand-maintained lookup table baked into the bpftrace source), and whether that particular check is one that writes to the audit log. **The stated primary use case is whitelist construction**: watching what capabilities a given application *actually* uses in practice is a direct, empirical way to build a minimal, justified whitelist for a hardened deployment, rather than guessing or over-granting broad privileges "just in case." Real example output shows `ssh`/`sshd` checking `CAP_SETUID`/`CAP_SETGID`/`CAP_SYS_CHROOT` during a login, and (with `-KU` for combined kernel+user stacks) a genuinely detailed example trace showing `bash`'s own `read_history()`/`load_history()` call chain checking `CAP_DAC_READ_SEARCH` on startup — real, concrete, mundane capability usage made visible for exactly the kind of baseline-establishing work whitelisting requires. **Origin worth noting**: built specifically at the request of a member of Netflix's platform security team, wanting exactly this kind of visibility — a small, direct data point that this tool answers a real, felt operational need, not just a hypothetical one.

### 11.2.12 `setuids` — privilege-escalation syscalls, directly useful for login/sudo tracing

Traces the `setuid(2)`/`setresuid(2)`/`setfsuid(2)` family — the syscalls a process uses to actually **change its effective user ID**, which is exactly what happens during a `sudo` invocation or an SSH login completing authentication. The book's own real example shows a `sudo` invocation's full sequence of `setresuid()` calls shuffling real/effective/saved UIDs back and forth as it transitions from UID 1000 to UID 0 and back — directly observable, step by step, rather than inferred. **Origin worth noting on its own, since it predates BPF entirely**: the author's original version of this exact tool (`setuids.d`, written in DTrace in 2004) was built specifically because it reliably caught `login`, `su`, and `sshd` all performing this same underlying UID-change operation — i.e., **this one syscall family is a genuinely reliable, implementation-independent way to observe "a privilege change just happened," regardless of which specific higher-level authentication tool caused it.**

## 11.3 BPF One-Liners

```
# BCC: count LSM security-hook calls for one PID
funccount -p 1234 'security_*'

# bpftrace: same
bpftrace -e 'k:security_* /pid == 1234/ { @[func] = count(); }'

# BCC: trace PAM (pluggable authentication module) session starts
trace 'pam:pam_start "%s: %s", arg1, arg2'

# bpftrace: same, via a uprobe on libpam directly
bpftrace -e 'u:/lib/x86_64-linux-gnu/libpam.so.0:pam_start { printf("%s: %s\n",
    str(arg0), str(arg1)); }'

# BCC / bpftrace: trace kernel module loads via the tracepoint
trace 't:module:module_load "load: %s", args->name'
bpftrace -e 't:module:module_load { printf("load: %s\n", str(args->name)); }'
```

**The `funccount -p 1234 'security_*'` one-liner's own example output is worth a second look as a small, concrete illustration of how much security-relevant activity is happening even in ordinary, non-malicious execution**: tracing one ordinary process's LSM hook calls shows a real mix — `security_task_setpgid`, `security_inode_alloc`, `security_file_permission` (13 times), `security_file_ioctl` (34 times) — none of it inherently suspicious, but all of it now directly, individually observable, which is exactly the kind of baseline visibility a whitelist or an anomaly-detection system needs to be built from.

## 11.4 Summary

BPF supports a genuinely wide range of security work — real-time forensics, privilege/capability whitelisting, port-scan and vulnerability detection, and session monitoring — largely by reusing tools and techniques already introduced for performance analysis in earlier chapters, pointed at a different question. The chapter's own two-sided framing is worth keeping as the real takeaway: BPF is both a **tool you can use for security work** and a **subsystem with its own security model** (the privileged-only restriction, JIT hardening, overflow-awareness) that's worth understanding on its own terms before building anything security-critical on top of it.

---

## Quick-reference glossary (this chapter's terms)

- **LSM (Linux Security Module) hooks** — kernel functions beginning with `security_`, called for security-mediated operations; recommended as unusually durable kprobe targets per this chapter's own strategy.
- **Capability (POSIX capability)** — a fine-grained unit of root-equivalent privilege (e.g., `CAP_SYS_ADMIN`, `CAP_SETUID`); `capable(8)` traces individual capability checks for whitelist-building.
- **Non-repudiation** — the property that a security log can't have silently missed events without that gap itself being detectable; the reason BPF's overflow/drop signals must be captured, not ignored.
- **`bpf_send_signal()`** — a Linux 5.3+ helper letting a BPF program send a signal directly from kernel context, removing the detection-to-enforcement lag of a `system()`-based workaround.

## Check your understanding

1. In the Docker `renameat2`/`RENAME_EXCHANGE` example, what specifically made this a good candidate for a fast, low-false-positive BPF-based detector?
2. Why does the book insist that a real security monitoring system built on BPF must specifically capture and act on BPF's own overflow/drop signals, rather than just trusting that events were logged?
3. `elfsnoop(8)` prints both a mount point and an inode number for every executed file, not just its path. What specific attack does this defend against, and why does printing the path alone not provide the same protection?
4. Why is `kernel.unprivileged_bpf_disabled` described as a "one-time shot," and what does that imply for how it should be used operationally?
5. `bashreadline(8)` is described as having two real, distinct limitations. What are they, and how does each one represent a different kind of evasion an attacker could use?

<details>
<summary>Answers</summary>

1. Because the specific combination of syscall and flag (`renameat2(2)` with `RENAME_EXCHANGE`) was genuinely rare in normal, legitimate use — the author observed zero natural occurrences of it on his own production systems — meaning a detector for that exact pattern would very rarely fire on legitimate activity (a low false-positive rate) while reliably firing whenever the actual exploit technique was in use.
2. Because BPF's output buffers and maps have finite capacity and can be exceeded under extreme load, meaning events can genuinely go unrecorded — and an attacker could deliberately try to trigger exactly this kind of overflow (flooding the system with events) specifically to evade logging or enforcement. If the monitoring system doesn't detect and act on that overflow condition itself, it could end up "blind" during exactly the period an attacker most wants it to be, with no record that the gap even happened — violating non-repudiation.
3. It defends against an attacker creating a malicious binary that displays the *same path* as a trusted system binary (potentially even using control characters to make a different real path *display* identically) in order to impersonate it. Printing the path alone wouldn't catch this, since the path string itself could be forged or made to look identical to the real one — but the underlying (mount, inode) pair is a much lower-level, harder-to-fake identifier of the actual file on disk being executed, so it stays accurate even if the displayed path has been spoofed.
4. Because once `kernel.unprivileged_bpf_disabled` is set to 1, attempting to set it back to 0 will be rejected by the kernel — it can only be tightened, never loosened again, for the remainder of that boot. Operationally, this implies it should be set deliberately and early (e.g., at boot, once you're sure nothing legitimate on that system still needs unprivileged BPF access), since there's no way to reverse the decision without rebooting.
5. First, it can't see commands typed into any shell program other than `bash` itself — an attacker could deliberately use a different, untraced shell specifically to evade this logging. Second, some Linux distributions build `bash` so that its `readline()` implementation actually lives in a separate `libreadline` shared library rather than inside the `bash` binary itself, meaning a uprobe targeting `/bin/bash:readline` directly would simply never fire on such a system unless retargeted at the correct library. The first is a deliberate evasion technique an attacker could exploit; the second is more of an environment-specific blind spot that could accidentally leave logging silently non-functional even with no malicious intent involved.

</details>
