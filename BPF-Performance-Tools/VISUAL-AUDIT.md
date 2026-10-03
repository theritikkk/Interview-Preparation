# Visual Audit — BPF Performance Tools Study Notes

*A complete pass over all 28 note files and the 12 pre-existing diagrams, identifying where a diagram would materially improve understanding versus where the prose already does the job. Produced before any new diagrams were built, per the audit-first requirement.*

**Folder structure note:** the brief suggested folders mixing diagram-*type* names (architecture/, data-flow/, state-machines/) with domain names (networking/, cpu/, memory/...). Since the repository is already organized by chapter/domain, I used **domain-based folders** — `images/bpf/`, `cpu/`, `memory/`, `storage/`, `networking/`, `containers/`, `observability/`, `kernel/`, `languages/` — and let each filename self-describe its diagram type (e.g. `cpu-thread-state-machine.svg`). This keeps one consistent organizing principle instead of two competing ones.

## Audit table

| File | Concept | Existing visual? | Missing visual? | Recommended visual | Priority |
|---|---|---|---|---|---|
| 01-introduction.md | Dynamic vs. static instrumentation, cross-stack visibility | No | Partial | Already well served by the §1.5 comparison table in prose | LOW |
| 02-technology-background.md | BPF program pipeline (event → verifier → JIT → maps → user space) | **Yes** (`bpf/bpf-pipeline.svg`) | No | — | — (covered) |
| 02-technology-background.md | Five event sources (kprobe/uprobe/tracepoint/USDT/PMC) | **Yes** (`bpf/event-sources.svg`) | No | Also satisfies the "BPF probe taxonomy" request (§4.8 of the brief) | — (covered) |
| 02-technology-background.md | Flame graph anatomy | **Yes** (`bpf/flame-graph-anatomy.svg`) | No | — | — (covered) |
| 02-technology-background.md | Stack-walking technique comparison | **Yes** (`bpf/stack-walking-comparison.svg`) | No | — | — (covered) |
| 02-technology-background.md | Per-CPU maps solving the lost-update race | **Yes** (`bpf/percpu-map-race.svg`) | No | — | — (covered) |
| 02-technology-background.md | Static jump patching (tracepoint nop ↔ jump) | **Yes** (`bpf/tracepoint-jump-patching.svg`) | No | — | — (covered) |
| 03-performance-analysis.md | USE method / workload characterization / drill-down as a repeatable process | No | Yes | A small "investigation loop" diagram tailored to this chapter's 4 methodologies | HIGH |
| 04-bcc.md | BCC vs. bpftrace positioning | **Yes** (`bpf/bcc-vs-bpftrace.svg`) | No | — | — (covered) |
| 04-bcc.md | BCC's own 9-step compile/load pipeline | No | Yes | Build pipeline: source → LLVM → bytecode → bpf() → verifier → JIT | **CRITICAL** |
| 05-bpftrace.md | Entry/return timestamp-pair idiom | **Yes** (`bpf/timestamp-pair-idiom.svg`) | No | — | — (covered) |
| 05-bpftrace.md | Power-of-two histogram shape | **Yes** (`bpf/power-of-two-histogram-example.svg`) | No | — | — (covered) |
| 06-cpus.md | Thread state machine (ON-PROC / RUNNABLE / SLEEP) and which tool measures which transition | No | Yes | State machine with runqlat/offcputime/wakeuptime/profile labeled on the transitions | **CRITICAL** |
| 06-cpus.md | CPU vs. off-CPU flame graphs | **Yes** (`cpu/cpu-vs-offcpu-flamegraph.svg`) | No | — | — (covered) |
| 07-memory.md | Page life cycle (alloc → fault → RSS growth → reclaim/swap) | No | Yes | Full life-cycle diagram connecting faults/ffaults/vmscan/drsnoop/swapin/oomkill | **CRITICAL** |
| 08-file-systems.md | Generic I/O stack (app → VFS → FS → block → device) | **Yes** (`storage/filesystem-io-path.svg`) | Partially | Existing diagram doesn't branch on cache hit vs. miss | HIGH |
| 09-disk-io.md | Wait time vs. service time vs. request time | **Yes** (`storage/io-time-timeline.svg`) | No | — | — (covered) |
| 10-networking-part1.md | Generic network stack layers | **Yes** (`networking/network-stack.svg`) | No | — | — (covered) |
| 10-networking-part1.md | Full send+receive packet lifecycle with instrumentation points | No | Yes | End-to-end packet diagram, both hosts, XDP/TC/kprobe/tracepoint points marked | **CRITICAL** |
| 10-networking-part2.md | TCP connection state lifecycle (SYN backlog → ESTABLISHED → retransmit → close) | No | Yes | State diagram tying tcpconnect/tcplife/tcpretrans/tcpsynbl together | HIGH |
| 10-networking-part3.md | UDP/DNS/qdisc tools | No | No | Already served by the packet-lifecycle diagram once added to Part 1 | LOW |
| 11-security.md | Policy enforcement landscape (seccomp/Cilium/bpfilter/Landlock/KRSI) | No | Marginal | A small landscape diagram is possible but the existing table does the job | LOW |
| 12-languages-part1.md | Why uprobes can't safely target JIT'd code | No | Yes | Cause/effect diagram: two causes (relocation, no backing file) → two effects (corruption, nothing to attach) | **CRITICAL** |
| 12-languages-part2.md | Java stack trace three-act fix (broken → walkable → symbolized) | **Yes** (`languages/java-stack-trace-fix.svg`) | No | — | — (covered) |
| 12-languages-part3.md | Bash symbol-stripping debugging story | No | No | Narrative, not spatial — a diagram would add little over the prose | LOW |
| 13-applications.md | Service thread pool vs. event-worker thread pool, and why `tid` keying works/fails | No | Yes | Two small diagrams: one thread→one request vs. one thread→many requests | HIGH |
| 14-kernel.md | Wakeup dependency chain (the real tar/gzip story) | No | Yes | Three-step diagram: offcputime → wakeuptime → offcputime again, fused by offwaketime | **CRITICAL** |
| 14-kernel.md | Kernel mutex fastpath/midpath/slowpath | **Yes** (`kernel/mutex-escalation.svg`) | No | — | — (covered) |
| 15-containers.md | Namespaces vs. cgroups, no unified container ID | **Yes** (`containers/container-namespaces-cgroups.svg`, extended by `containers/kubernetes-to-kernel.svg`) | No | — | — (covered) |
| 16-hypervisors.md | Guest-visible observability shrinking across PV → PVHVM → PVHVM+SR-IOV → Nitro | No | Yes | Progression diagram, already described as a table in prose — a visual would make the trend more vivid | HIGH |
| 17-other-tools.md | Latency heat map | **Yes** (`observability/latency-heatmap-example.svg`) | No | — | — (covered) |
| 17-other-tools.md | Prometheus/Grafana/exporter observability pipeline | **Yes** (`observability/prometheus-grafana-pipeline.svg`) | No | — | — (covered) |
| 18-tips-tricks.md | Observer effect / feedback loop | No | Yes | Normal bounded chain vs. runaway feedback loop | **CRITICAL** |
| 18-tips-tricks.md | Overhead formula | No | No | Already clear as a one-line equation; a diagram would add nothing | LOW |
| appendix-a-b-oneliners-cheatsheet.md | — | No | No | Pure reference tables; no concept needs spatial explanation | LOW |
| appendix-c-bcc-development.md | STORAGE/STORE/FACTOR templating | No | Marginal | Already illustrated clearly via the actual code diff shown | LOW |
| appendix-d-e-c-bpf-instructions.md | `struct bpf_insn` field layout | No | Marginal | Already a clean table; a diagram would be redundant | LOW |
| 99-master-tool-index.md | — | No | No | Lookup table; inherently textual | LOW |
| 100-full-book-review-quiz.md | — | No | No | Q&A content; no spatial concept | LOW |

## This batch's scope

Of the items marked **CRITICAL**, this pass built and embedded all seven that required a *new* diagram (the eighth CRITICAL candidate — BPF probe taxonomy — turned out to already be fully covered by the existing `event-sources.svg`, so no redundant diagram was created for it, per the audit's own redundancy check).

**Built this batch:**
1. `bpf/bpf-loading-pipeline.svg` — BCC/bpftrace compile-and-load pipeline (Ch. 4)
2. `languages/jit-uprobe-problem.svg` — why uprobes can't safely target JIT'd code (Ch. 12)
3. `cpu/cpu-thread-state-machine.svg` — thread states and which tool measures which transition (Ch. 6)
4. `memory/memory-page-lifecycle.svg` — the full page life cycle (Ch. 7)
5. `networking/network-packet-lifecycle.svg` — end-to-end send/receive path with instrumentation points (Ch. 10)
6. `kernel/wakeup-chain.svg` — the real tar/gzip wakeup-chain investigation (Ch. 14)
7. `observability/observer-effect-feedback-loop.svg` — bounded overhead vs. a runaway feedback loop (Ch. 18)

**Built in the follow-up HIGH-priority batch (all five now complete):**
8. `observability/investigation-loop.svg` — the USE-method/workload-characterization/drill-down loop (Ch. 3)
9. `storage/filesystem-cache-path.svg` — cache-hit-vs-miss branch on the file system I/O path (Ch. 8)
10. `networking/tcp-connection-lifecycle.svg` — TCP connection state lifecycle (Ch. 10 Part 2)
11. `applications/thread-pool-models.svg` — service thread pool vs. event-worker thread (Ch. 13)
12. `kernel/hypervisor-visibility-progression.svg` — guest-visibility shrinking across hypervisor generations (Ch. 16)

**Built in the final MEDIUM-priority batch (all seven now complete):**
13. `bpf/percpu-map-race.svg` — shared-map lost-update race vs. per-CPU map's no-race design (Ch. 2 §2.3.7)
14. `bpf/tracepoint-jump-patching.svg` — static jump patching: disabled no-op vs. enabled jump-to-trampoline (Ch. 2 §2.7)
15. `storage/io-time-timeline.svg` — wait time vs. service time vs. request time on one I/O (Ch. 9)
16. `languages/java-stack-trace-fix.svg` — the Java stack trace three-act fix: broken → walkable → symbolized (Ch. 12 §12.3.5)
17. `kernel/mutex-escalation.svg` — kernel mutex fastpath/midpath/slowpath escalation (Ch. 14)
18. `containers/kubernetes-to-kernel.svg` — extending the namespaces/cgroups diagram up through the container runtime to Kubernetes (Ch. 15)
19. `observability/prometheus-grafana-pipeline.svg` — BPF program → ebpf_exporter → Prometheus → Grafana (Ch. 17.3)

**Deliberately not built (LOW priority):** items where the existing prose, table, or code example already does the job as well as or better than a diagram would — per the brief's own core rule, these were left as text.

## Technically ambiguous items requiring human review

None found. No existing diagram or newly-audited concept surfaced a factual inconsistency in the prose during this pass.

## Visual Coverage Summary

- **Existing diagrams (before this task):** 12
- **New diagrams (CRITICAL batch + HIGH batch + MEDIUM batch):** 19
- **Total diagrams:** 31
- **Chapters/files with at least one diagram:** 2, 3, 4, 5, 6, 7, 8, 9, 10 (Parts 1–2), 12 (Parts 1–2), 13, 14, 15, 16, 17, 18 — 19 of 28 files
- **Chapters still primarily text:** 1, 10 (Part 3), 11, 12 (Part 3), both appendix pairs (A/B, C, D/E), and the master index/quiz — all by deliberate choice (LOW-priority items in the audit table above), none of them CRITICAL, HIGH, or MEDIUM
- **Highest-value diagrams:** the CPU thread state machine (ties four tools across two chapters together), the wakeup chain (makes a three-tool real investigation concrete), the network packet lifecycle (the single most cross-cutting diagram requested), and the hypervisor visibility-shrinking progression (turns a prose table into an immediately legible trend)
- **Any technically ambiguous diagrams requiring human review:** none

## MEDIUM-priority items — status

All seven items originally deferred as MEDIUM priority have now been built (see "Built in the final MEDIUM-priority batch" above). Every CRITICAL, HIGH, and MEDIUM item identified in the audit table is now covered by a diagram; only LOW-priority items remain text-only, by the audit's own deliberate judgment that prose already does the job as well as or better than a diagram would.
