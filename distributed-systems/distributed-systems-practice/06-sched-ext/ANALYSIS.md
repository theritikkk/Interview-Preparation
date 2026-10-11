# ANALYSIS — a real scheduler, a real kernel that can't run it, and the real error that proves it

## The honest headline, stated first

Every project in this series so far (4: CO-RE/libbpf, 5: XDP/TC) found a
way to make the real thing actually run in this sandbox. This one
doesn't, and that's not a cop-out — it's the actual, rigorously-confirmed
answer to the question Project 5's own roadmap entry left open
("`sched_ext` has its own kernel-support question
(`CONFIG_SCHED_CLASS_EXT`) not yet checked"). This sandbox's kernel is
built with `CONFIG_SCHED_CLASS_EXT` **not set**, scheduler classes are
always compiled directly into the kernel core (never a loadable module,
for any kernel, anywhere — the scheduler initializes before module
loading exists), and there is no BPF attach mechanism that bypasses a
scheduler class that was never compiled in. This is a hard, unconditional
wall, unlike every earlier "looks missing, turns out there's a real
path" finding in this series (libelf-dev, kprobes, native XDP mode).

What makes this still a real project rather than a shrug: a complete,
real, correctly-structured sched_ext scheduler was written, compiled for
real by `clang -target bpf`, produces a real ELF object with real
`struct_ops` program sections and real `BTF`/`BTF.ext` CO-RE data, and was
then actually run against this live kernel through a real userspace
loader using libbpf's real load path — not a simulation of what loading
would do. The kernel's/libbpf's real, specific rejection was captured
directly, not predicted. That rejection, and the independent evidence
that triangulates exactly *why* it happens, is this project's actual
payload.

## Three independent confirmations that `CONFIG_SCHED_CLASS_EXT` is off

Three different tools, three different angles, same answer — this
wasn't taken on a single data point:

**1. The kernel's own build config, read directly.**
```
$ zcat /proc/config.gz | grep SCHED_CLASS_EXT
# CONFIG_SCHED_CLASS_EXT is not set
```

**2. Live BTF introspection, using Project 4's own `btf_search` tool**
(reused here unmodified — see "Tooling reused from earlier projects"
below) **— not just the one struct, the entire type namespace:**
```
$ btf_search sched_ext
vmlinux BTF loaded, 124395 types
'sched_ext': 0 matches

$ btf_search scx_bpf
vmlinux BTF loaded, 124395 types
'scx_bpf': 0 matches
```
Zero matches for `sched_ext` among 124,395 real BTF types, and zero for
the `scx_bpf_*` kfunc-name prefix. `struct task_struct` *is* found by the
same tool (ruling out a BTF-tooling failure) — the live kernel simply
never compiled in a single sched_ext-related type, function, or kfunc.

**3. A real struct_ops family registration check — the most precise of
the three.** `BPF_MAP_TYPE_STRUCT_OPS` is a *generic* BPF mechanism
shared by several unrelated kernel subsystems (TCP congestion control,
HID-BPF, kernel self-tests, sched_ext). The kernel auto-generates a
`bpf_struct_ops_<family>` BTF type for every struct_ops family it
actually registers at boot. Querying for all of them on this kernel:
```
$ btf_search bpf_struct_ops
...
[bpf_struct_ops] id=36953 name=bpf_struct_ops_bpf_dummy_ops
[bpf_struct_ops] id=41104 name=bpf_struct_ops_tcp_congestion_ops
'bpf_struct_ops': 9 matches
```
Exactly two real families are registered — `bpf_dummy_ops` (kernel
self-test scaffolding) and `tcp_congestion_ops` (this kernel really does
support real BPF-based TCP congestion control, confirmed separately via
`/proc/sys/net/ipv4/tcp_available_congestion_control` listing `bbr`
alongside `reno`/`cubic`). **No `bpf_struct_ops_sched_ext_ops` entry
exists.** This is the single most useful fact in this whole
investigation: it rules out the vaguer, less certain explanation
("maybe struct_ops as a BPF mechanism doesn't work in this sandboxed
kernel at all") in favor of the precise one that was actually
predicted ("struct_ops itself is fully functional here — it's
specifically the sched_ext family that was never registered, because
the Kconfig option that registers it was off at build time").

A fourth, weaker signal, included for completeness but not relied on:
`sysctl kernel.sched_ext_ops` doesn't exist either — consistent with, but
not independent proof of, the above, since that sysctl is documented to
appear only *while a BPF scheduler is actively loaded*, which was never
going to be true here regardless of cause.

## Why this is a harder wall than every earlier "missing" finding

Every previous "this looks unavailable" moment in this series turned out
to have a real path around it:

| Project | Looked missing | Actually was |
|---|---|---|
| 4 | `libelf-dev` (needed to build libbpf) | Not installable via apt, but a small hand-written shim satisfying libbpf's real `.so` symbol needs built and worked — genuinely equivalent, not a stand-in |
| 4 | kprobe attach | This kernel has `CONFIG_KPROBES` unset too — but tracepoints (a different, real attach mechanism) were available and used instead |
| 5 | Native (driver-mode) XDP | Unavailable on both interfages for different driver-level reasons — but generic (SKB-mode) XDP, a different real attach mode, worked |

Each of those was "mechanism A is blocked, but mechanism B reaches the
same real kernel capability." Scheduler classes don't have a mechanism
B: a `sched_class` (`CFS`, `RT`, `deadline`, and — when compiled in —
`ext`) is wired into the kernel's core scheduler dispatch table at
**build time**, not discovered or loaded at runtime the way a kprobe,
tracepoint, or network hook is. Confirmed two further ways, specifically
to rule out "maybe it's just not loaded yet, like a module":

- `/lib/modules` does not exist in this container at all — there is no
  loadable-module mechanism present here whatsoever, so "maybe it loads
  as a module later" was never on the table even hypothetically.
- `CONFIG_SCHED_CLASS_EXT` is a plain bool (`y`/not set) in this
  kernel's config, not a tristate (`y`/`m`/not set) — the kernel's own
  Kconfig system doesn't offer "build as a module" as an option for this
  feature, consistent with every other scheduling-class config in the
  same file (`CONFIG_FAIR_GROUP_SCHED=y`, `CONFIG_RT_GROUP_SCHED=y` — all
  bool, never tristate). Scheduler classes, as a category, are simply not
  a thing the Linux kernel supports loading after boot, for architectural
  reasons (the scheduler itself has to exist before `init` can run, long
  before module-loading infrastructure is available) — this isn't a
  sandbox-specific restriction.

## Getting real, authentic reference material — not a guess

The upstream [`sched-ext/scx`](https://github.com/sched-ext/scx) project
is the real, canonical place sched_ext schedulers come from, and it's
genuinely reachable from this sandbox (`git clone` through the GitHub
proxy succeeded, confirming network reachability isn't the obstacle
here either). But a shallow clone of its current `main` branch turned
out to be the wrong source: the project has evolved into a
Rust-userspace-heavy collection of production schedulers
(`scx_bpfland`, `scx_rusty`, `scx_layered`, ...) built on a 1,274-line
shared BPF header assuming the project's own Meson/Rust build tooling —
no minimal, single-file C reference remains in the current tree, and a
`--depth 1` clone has no git history to fall back to for an older,
simpler version.

The actual source used instead: a **sparse, read-only checkout of the
real Linux kernel tree itself** (`torvalds/linux`, tag `v6.12` — the
exact release sched_ext shipped in), narrowed with
`git sparse-checkout` to just `tools/sched_ext/` (the kernel's own
in-tree example schedulers, before they were later split out into the
separate `scx` project) and `kernel/sched/ext.c` /
`include/linux/sched/ext.h` (the real struct definitions). This is
strictly more authoritative than the external project for this
purpose — it's the kernel's own historical copy of the exact ABI a
6.12-era kernel exposes, including `tools/sched_ext/scx_simple.bpf.c`,
`tools/sched_ext/scx_simple.c`, and the full `scx/common.bpf.h` of that
era (427 lines — already far more tractable than the current project's
1,274). All of it is kept, verbatim, in `reference/*.upstream` in this
delivery, specifically so every claim in this document about what's
"real" vs. "adapted" can be checked by diffing against it directly
rather than taken on faith.

**`scx_simple`** (the scheduler this project builds on) was chosen
because it's the real project's own "start here" example: a global
FIFO/weighted-vtime scheduler with no CPU-affinity logic, no cgroup
support, no topology awareness — the smallest scheduler in the tree that
still demonstrates the complete real lifecycle (`init` → `select_cpu` →
`enqueue` → `dispatch` → `running`/`stopping` → `enable` → `exit`).

## What's byte-for-byte real vs. what's a hand-written stand-in

Same CO-RE methodology as Project 4's `vmlinux_min.h`, applied to a much
higher-stakes case — see `scx-simple/vmlinux_min.h`'s own header comment
for the full mechanics, but the short version: CO-RE relocation doesn't
care whether a local struct's field layout came from a generated
`vmlinux.h` or was typed by hand, only whether the (struct name, field
name) pairs are spelled correctly — the compiler-time offsets are
discarded and replaced with the *target* kernel's real offsets at load
time.

- **`scx-simple/scx_simple.bpf.c` — byte-for-byte identical to the real
  upstream file**, except for one `#include` line (swapping the real
  `scx/common.bpf.h` — itself dependent on a full generated `vmlinux.h`
  this sandbox cannot produce, since there's no sched_ext BTF to generate
  it *from* — for a hand-written minimal equivalent). Every scheduling
  decision — the DSQ choice in `select_cpu`, the vtime-vs-FIFO branch in
  `enqueue`, the vtime bookkeeping in `running`/`stopping` — is the real
  project's real logic, unmodified. Diff it against
  `reference/scx_simple.bpf.c.upstream` to confirm.
- **`scx-simple/vmlinux_min.h` — hand-written, but every field name,
  type, and relative ordering copied from the authentic kernel source**,
  not guessed: `struct sched_ext_entity` from
  `include/linux/sched/ext.h`, `struct sched_ext_ops` and
  `struct scx_exit_info` from `kernel/sched/ext.c` (the struct itself
  lives in the `.c` file, not a header — confirmed by `grep`, not
  assumed). Only the 9 of ~25 real `sched_ext_ops` fields this toy
  scheduler actually sets are declared — struct_ops field resolution in
  libbpf is by-name against the kernel's own BTF, so a local struct that
  is a strict subset of the real one is normal, the same pattern as
  Project 4 declaring 2 fields out of a much larger real tracepoint
  struct. One genuine mistake happened and was caught here, worth
  recording: an early draft of `SCX_DSQ_LOCAL` guessed
  `SCX_DSQ_FLAG_BUILTIN | 0`, which is actually the *different* constant
  `SCX_DSQ_INVALID` — going back to
  `include/linux/sched/ext.h` directly (rather than trusting the first
  guess) found the real value, `SCX_DSQ_FLAG_BUILTIN | 2`.
- **`scx-simple/scx_common_min.bpf.h` — a deliberately trimmed, not
  wholesale-copied, stand-in** for the real 427-line `common.bpf.h`: the
  5 `scx_bpf_*` kfunc declarations and the `BPF_STRUCT_OPS`/
  `SCX_OPS_DEFINE` macros are copied verbatim (signatures included); the
  dozens of other real kfuncs (cpumask, `bpf_list`/`bpf_rbtree`, cgroup,
  iterator helpers) are cut because `scx_simple.bpf.c` never calls them —
  including them would mean hand-declaring kernel types
  (`struct cgroup`, `struct bpf_cpumask`, ...) purely to satisfy unused
  signatures.
- **`scx-simple/user_exit_info_min.h` — one real, stated simplification**:
  the real header's resizable debug-dump buffer
  (`scx_bpf_dump()`-authored crash dumps, sized at runtime from the real
  `sched_ext_ops.exit_dump_len`) is left out entirely. This scheduler
  never calls `scx_bpf_dump()`, and the dump buffer has no bearing on
  whether the program loads — keeping `kind`/`exit_code`/`reason`/`msg`
  (the part that actually answers "why did the scheduler exit") and
  cutting the rest was a judgment call to avoid reproducing machinery
  that serves no purpose here, not an oversight.
- **`scx-simple/loader.c` — a real, non-skeleton userspace loader**,
  written against libbpf's generic object API
  (`bpf_object__open_file()` / `bpf_object__load()` /
  `bpf_map__attach_struct_ops()`) rather than a `bpftool`-generated
  skeleton (`scx_simple.bpf.skel.h`, what the real upstream
  `scx_simple.c` uses) — this sandbox has no `bpftool` (same situation
  as every earlier project in this series; building it was judged not
  worth it for one loader). `bpf_map__attach_struct_ops()` is libbpf's
  direct, skeleton-free equivalent of what the real code gets from the
  `SCX_OPS_ATTACH()` macro — not a reimplementation of struct_ops
  attachment, the actual real API for it.

## The build itself found a second, unrelated, real toolchain fact

Compiling `scx_simple.bpf.c` with the same `clang -target bpf` invocation
Projects 4 and 5 used crashed the LLVM backend outright — not a verifier
rejection, a compiler ICE:
```
scx_simple.bpf.c:156:2: error: unsupported atomic operation, please use 64 bit version
fatal error: error in backend: Cannot select: ... AtomicCmpSwap<...(s32)...>
...
clang: error: clang frontend command failed with exit code 70
```
The real upstream `UEI_RECORD` macro uses
`__sync_val_compare_and_swap()` on a 32-bit `int kind` field — a real,
unmodified line from the real project, not something this adaptation
introduced. The BPF instruction set's 32-bit atomic compare-and-exchange
instruction (as opposed to the 64-bit version, which compiled fine
without any special flag) requires BPF ISA v3 or later; clang's default
`-target bpf` CPU doesn't enable it. Adding `-mcpu=v3` (now in
`scx-simple/makefile`'s `BPF_CFLAGS`) fixed it immediately and
permanently. Checked against the real `scx` project's own build files —
they set `-mcpu=v3` (or `v4`, depending on the compiler-detection logic)
unconditionally for exactly this class of reason, confirming this isn't
an invented workaround but the same real fix the upstream project
already applies. Noted here because it's a genuine, previously-untested
fact about this sandbox's clang/BPF toolchain combination, in the same
honest-findings spirit as Project 5's checksum/framing discoveries —
just one that turned out to be about the compiler, not the kernel.

## The real load attempt, and what it actually proves

```
$ ./loader scx_simple.bpf.o
[1/3] bpf_object__open_file(scx_simple.bpf.o)
      open OK — object name: scx_simple
[2/3] bpf_object__load() — this is the real kernel accept/reject step
...
libbpf: struct_ops init: struct sched_ext_ops(type_id=63) simple_ops found at offset 0
...                                                        (this is parsing the
                                                             object's OWN local
                                                             BTF, not the kernel's —
                                                             see below)
libbpf: object 'scx_simple': failed (-22) to create BPF token from '/sys/fs/bpf', skipping optional step...
libbpf: loaded kernel BTF from '/sys/kernel/btf/vmlinux'
libbpf: extern (func ksym) 'scx_bpf_consume': not found in kernel or module BTFs

==> LOAD REJECTED (errno=22, Invalid argument)
```
(Full, unedited output: `reference/real_load_attempt.log`. Reproduced
identically across multiple runs and a from-scratch `make clean && make`
rebuild — not a one-off fluke.)

**What actually happened, precisely:** `bpf_object__load()` proceeds in
stages. It first parses the compiled object's *own* BTF to understand
the local `struct sched_ext_ops` layout (the `type_id=63 ... found at
offset 0` line — this is bookkeeping against the object file being
loaded, not a kernel check, and would print identically on any kernel).
It then loads the **live kernel's real BTF** from
`/sys/kernel/btf/vmlinux` — succeeding, because this kernel does export
BTF (`CONFIG_DEBUG_INFO_BTF=y`, confirmed back in Project 4) — and tries
to resolve every `__ksym`-declared extern against it. The very first one
it checks, `scx_bpf_consume`, is reported **not found in kernel or
module BTFs**, and the whole load aborts there with `EINVAL`. This is
libbpf's own kfunc-resolution preflight check failing, *before* a single
`BPF_PROG_LOAD` syscall is even issued for any of the struct_ops
programs — so there's no BPF verifier log to show (the verifier never
got the chance to run), and `dmesg` is correspondingly silent (checked;
nothing there). That's not a gap in the evidence: libbpf's own message
already names the precise, correct root cause — this kernel's live BTF
has no record of `scx_bpf_consume`, or any other `scx_bpf_*` kfunc,
because the subsystem that registers them at boot was never compiled
in — which is exactly, word-for-word, the same conclusion the three
independent confirmations above reached by different routes. Four ways
of asking the same question, one answer, from the kernel's build config,
its live BTF contents, its struct_ops family registration, and now a
real, live load attempt.

## Tooling reused from earlier projects

`btf_search` and `btf_probe` (Project 4's permanent BTF-introspection
tools) were used here completely unmodified — no new tooling had to be
built to investigate this project's central question, which is itself a
small piece of evidence for how generally useful that toolchain turned
out to be beyond the one project it was built for.

## What this project does *not* demonstrate, stated plainly

- **The scheduler never actually ran.** No task was ever scheduled by
  this code; there is no "it mostly works" story to tell. The entire
  deliverable is: a correct, compilable, honestly-documented program,
  and a real, specific, reproducible kernel rejection.
- **The verifier itself never evaluated these BPF programs.** The
  rejection happens one layer up, in libbpf's own preflight kfunc
  resolution — a real and decisive stopping point, but not the same
  thing as "the verifier rejected the program's logic." On a kernel with
  `CONFIG_SCHED_CLASS_EXT=y`, these exact `.bpf.o` bytes would proceed
  past this point and would then face real verifier scrutiny this
  project never got to observe.
- **No cgroup support, no CPU-affinity/topology awareness, no
  preemption** — `scx_simple` is deliberately the real project's
  smallest complete example, not a production scheduler; see its own
  real upstream doc comment (reproduced at the top of
  `scx-simple/scx_simple.bpf.c`) for what it explicitly doesn't attempt
  even on a kernel where it *would* run.
