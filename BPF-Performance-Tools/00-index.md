# BPF Performance Tools — Study Notes: Index & Roadmap

Source: *BPF Performance Tools: Linux System and Application Observability* by Brendan Gregg (Addison-Wesley, 2019) — 880 pages, 18 chapters, 5 appendices.

**Diagrams.** The `images/` folder holds 31 original SVG diagrams, organized by domain (`bpf/`, `cpu/`, `memory/`, `storage/`, `networking/`, `containers/`, `kernel/`, `languages/`, `applications/`, `observability/`) and embedded at the point in each chapter where the mental model is actually needed — not bundled at the top. See `VISUAL-AUDIT.md` for the full file-by-file audit and priority ratings — every CRITICAL, HIGH, and MEDIUM item identified is now built; only LOW-priority items (where prose already does the job) remain text-only. These are illustrative originals, not reproductions of the book's own figures.

### Visual map

| Domain | Diagrams | Chapters |
|---|---|---|
| BPF architecture | `bpf-pipeline` · `bpf-loading-pipeline` · `event-sources` · `bcc-vs-bpftrace` · `stack-walking-comparison` · `flame-graph-anatomy` · `timestamp-pair-idiom` · `power-of-two-histogram-example` · `percpu-map-race` · `tracepoint-jump-patching` | 2, 4, 5 |
| CPU | `cpu-thread-state-machine` · `cpu-vs-offcpu-flamegraph` | 6 |
| Memory | `memory-page-lifecycle` | 7 |
| Filesystems / I/O | `filesystem-io-path` · `filesystem-cache-path` | 8 |
| Storage | `io-time-timeline` | 9 |
| Networking | `network-stack` · `network-packet-lifecycle` · `tcp-connection-lifecycle` | 10 |
| Languages | `jit-uprobe-problem` · `java-stack-trace-fix` | 12 |
| Applications | `thread-pool-models` | 13 |
| Kernel | `wakeup-chain` · `hypervisor-visibility-progression` · `mutex-escalation` | 14, 16 |
| Containers | `container-namespaces-cgroups` · `kubernetes-to-kernel` | 15 |
| Observability | `investigation-loop` · `latency-heatmap-example` · `observer-effect-feedback-loop` · `prometheus-grafana-pipeline` | 3, 17, 18 |

**A note on how these notes are written.** These are original study notes — explanations, summaries, and worked examples written in my own words to teach you the concepts — not a copy of the book's text. I verified every fact, number, and tool name below directly against your uploaded copy so the details are accurate, but the prose, structure, and teaching examples are freshly written for you as a study companion. That means these notes are yours to use freely, but they're not a substitute for the book itself — where the book gives long-form rationale, worked production war-stories, or full tool source listings, I've pointed you back to the specific section rather than reproducing it.

**Given the size of this book (880 pages), I'm working through it in batches rather than all at once.** This index will be updated as each batch is finished — think of it as the map, with the finished chapters as the territory. Say "continue" any time and I'll produce the next batch.

## How the book is organized

| Part | Chapters | What it covers |
|---|---|---|
| I — Technologies | 1–5 | What BPF is, how it actually works under the hood, performance-analysis methodology, and the two tool front ends (BCC and bpftrace) |
| II — Using BPF Tools | 6–16 | One chapter per subsystem (CPUs, memory, file systems, disk I/O, networking, security, languages, applications, kernel, containers, hypervisors), each cataloging the relevant BPF tools |
| III — Additional Topics | 17–18 | Visualization front ends for BPF data (Grafana, PCP, etc.) and a "tips, tricks, and common problems" troubleshooting chapter |
| IV — Appendixes | A–E | bpftrace one-liners, a bpftrace cheat sheet, a crash course in BCC tool development, BPF programming in C, and raw BPF instructions |

**Every chapter in Part II follows the same skeleton**, which is worth knowing up front so you can navigate efficiently rather than reading linearly: **Background** (the subsystem's fundamentals + what BPF specifically can/can't see there) → **Traditional Tools** (the pre-BPF tools you'd already reach for) → **BPF Tools** (the real content — one subsection per tool, each with example output, what it means, and how it's built) → **One-Liners** → **Optional Exercises** → **Summary**. Once you're comfortable with that shape, Part II works well as a *reference* you dip into when you actually need, say, disk I/O tools — you don't need to read Chapters 6–16 cover to cover in one sitting the way Part I rewards being read straight through.

## Chapter-by-chapter status

| # | Chapter | Pages (approx.) | Notes file | Status |
|---|---|---|---|---|
| 1 | Introduction | 14 | `01-introduction.md` | ✅ done |
| 2 | Technology Background | 51 | `02-technology-background.md` | ✅ done |
| 3 | Performance Analysis | 18 | `03-performance-analysis.md` | ✅ done |
| 4 | BCC | 44 | `04-bcc.md` | ✅ done |
| 5 | bpftrace | 52 | `05-bpftrace.md` | ✅ done |
| 6 | CPUs | 60 | `06-cpus.md` | ✅ done |
| 7 | Memory | 34 | `07-memory.md` | ✅ done |
| 8 | File Systems | 64 | `08-file-systems.md` | ✅ done |
| 9 | Disk I/O | 48 | `09-disk-io.md` | ✅ done |
| 10 | Networking | 100 | `10-networking-part1.md`, `part2.md`, `part3.md` | ✅ done |
| 11 | Security | 26 | `11-security.md` | ✅ done |
| 12 | Languages | 72 | `12-languages-part1.md`, `part2.md`, `part3.md` | ✅ done |
| 13 | Applications | 44 | `13-applications.md` | ✅ done |
| 14 | Kernel | 36 | `14-kernel.md` | ✅ done |
| 15 | Containers | 18 | `15-containers.md` | ✅ done |
| 16 | Hypervisors | 20 | `16-hypervisors.md` | ✅ done |
| 17 | Other BPF Performance Tools | 18 | `17-other-tools.md` | ✅ done |
| 18 | Tips, Tricks, and Common Problems | 14 | `18-tips-tricks.md` | ✅ done |
| A–E | Appendixes | ~50 | `appendix-a-b-oneliners-cheatsheet.md`, `appendix-c-bcc-development.md`, `appendix-d-e-c-bpf-instructions.md` | ✅ done |

## Suggested study path

1. **Read Part I in full, in order (Chapters 1–5).** This is the conceptual foundation — kprobes, uprobes, tracepoints, USDT, PMCs, flame graphs, the BCC/bpftrace split — that every single tool in Part II assumes you already understand. Skipping it means every later chapter will keep sending you to the Glossary.
2. **Then treat Part II as a reference, prioritized by what you actually work with.** If you run Linux servers, start with Chapter 6 (CPUs) and Chapter 9 (Disk I/O) — they introduce patterns (histograms, latency distributions) reused everywhere else. If you're specifically interested in networking or a particular language runtime, jump straight to Chapter 10 or 12.
3. **Read Chapter 18 (Tips, Tricks, and Common Problems) earlier than its position suggests** — ideally right after Part I. It's short, and it will save you real debugging time on your very first custom bpftrace script (missing symbols, missing stack traces, event frequency/overhead gotchas).
4. **Appendix C (BCC Tool Development) and Appendix B (bpftrace Cheat Sheet)** are worth keeping open in a tab once you start writing your own tools rather than just running the built-in ones.

---
**The full study-notes series is now complete** — all 18 chapters and all 5 appendixes, 28 files covering the entire 880-page book, plus `99-master-tool-index.md`: a single-page lookup table of every tool across every chapter, for fast "what do I reach for" scanning. Nothing further is planned unless you want a specific chapter revisited, expanded, or turned into something else (a consolidated quiz, flashcards, etc.).
