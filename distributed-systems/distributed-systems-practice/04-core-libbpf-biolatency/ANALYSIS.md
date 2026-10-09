# ANALYSIS — what's real, what's simplified, and the bug that was actually the interesting part

## The honest boundary

Everything downstream of "is BPF loadable here at all" is the real thing,
not a simulation of it:

- **Real eBPF bytecode**, compiled by a real `clang -target bpf` from real
  C source, with real `.BTF`/`.BTF.ext` sections carrying real CO-RE
  relocation records.
- **Real kernel load**: `bpf_object__load()` from the real `libbpf`
  (`github.com/libbpf/libbpf`, built from its actual source, not
  reimplemented), making the real `BPF_PROG_LOAD` syscall, going through
  this kernel's real BPF verifier.
- **Real CO-RE relocation**: at load time, libbpf actually parses this
  kernel's own `/sys/kernel/btf/vmlinux` (115,392 BTF types) and patches
  the compiled program's field offsets to match — confirmed by watching it
  happen (`BIOLATENCY_VERBOSE=1`, see below) and, independently, by the
  relocation *failing* first against a wrong guess and *succeeding* after
  the real struct names were found. A relocation that only worked by
  coincidence wouldn't explain why the wrong names failed cleanly and the
  right ones didn't.
- **Real tracepoints**: `block:block_rq_issue` / `block:block_rq_complete`
  genuinely exist in this kernel's `tracefs` and the attached program
  genuinely fires on them — proven by generating real `O_DIRECT` block I/O
  and watching a correspondingly real, non-empty, non-trivial histogram
  come out (not a canned fixture).

The one thing that's hand-rolled is `libelf.h`/`gelf.h` (the
`toolchain/libelf-shim/` directory) — and even that isn't a substitute
implementation, it's a **declarations-only header shim** in front of the
*real*, already-installed `libelf.so.1` binary. No ELF-parsing logic was
reimplemented; every function call still goes into the real library. See
below for exactly how that was verified before being trusted.

Nothing here is a kprobe-based `biolatency` pretending to be the
tracepoint-based one, and nothing here is "it compiles, so it probably
works" — every claim in this document was watched happening, not assumed.

## The libelf-dev gap, and how the shim was actually verified

`libbpf`'s build (and `bpftool`'s, and in general most of the eBPF
userspace toolchain) assumes `libelf-dev` is installed — it needs
`libelf.h`/`gelf.h` to compile against. This sandbox has the real libelf
**runtime** (`libelf1t64`, providing `libelf.so.1` — confirmed via `dpkg -l`
and `nm -D`) but not the **dev** package with the headers, and `apt` is
network-blocked here exactly the same way it was in Projects 2 and 3 (every
Ubuntu mirror returns 403). No GitHub mirror of `elfutils` was reachable
through the sandbox's proxy allowlist either.

The options at that point were: (a) give up on libbpf and hand-roll an ELF
reader too, meaning the whole "real toolchain" claim collapses into "a
replica of the toolchain," or (b) write just the missing *declarations*
(the shim is ~160 lines total across both headers) and let them resolve
against the real, already-present `.so` at link time. (b) is what's here,
and the difference matters: a hand-rolled ELF reader would mean this
project never actually exercises libbpf's real ELF-parsing code path at
all. The shim means it does — `libbpf.a` calls the exact same
`elf_begin()`/`elf_getdata()`/`gelf_getshdr()` etc. it would call against a
`libelf-dev`-provided header, against the exact same library binary.

This was **not** trusted on the strength of "it compiled, so the
declarations must be right." Every function declared in the shim was
cross-checked against the real exported symbol table first
(`nm -D libelf.so.1`), and then the shim's *behavior* was verified against
an independent parser:

```
toolchain/libelf-shim/verify_shim.c   — sections: shim-parsed vs readelf -S
toolchain/libelf-shim/verify_phdr.c   — program headers & notes: shim-parsed vs readelf -l / -n
```

Both tools open a real ELF file through the shim's declarations and print
every section/segment/note they find; the output was diffed by hand against
`readelf`'s own, completely independent ELF parser. Confirmed matches:
13/13 program headers, 3/3 notes, and (re-run against the actual compiled
`biolatency.bpf.o` for this project) all 30 of its sections, including the
`.BTF`/`.BTF.ext`/`.rel.BTF`/`.rel.BTF.ext` ones that matter most here.
Two struct-layout risk categories were considered and ruled out rather than
assumed away:

- **Opaque types** (`Elf`, `Elf_Scn`): libelf has never exposed their
  internal layout — callers only ever hold and pass back pointers — so
  there's no struct-layout ABI risk for these at all, unlike the
  `nostd::shared_ptr` ABI problem Project 3 hit.
- **`Elf_Data`**: its handful of public fields *are* part of libelf's
  documented, stable ABI (`man elf_getdata`) and are reproduced exactly.
- **`GElf_*` types**: real elfutils aliases every `GElf_*` type straight to
  the matching `Elf64_*` type on a 64-bit build (`gelf_get*()` functions
  upconvert from 32-bit internally so callers always see the 64-bit shape)
  — the shim does the same, pulling the real `Elf64_*` definitions from
  glibc's own `<elf.h>` rather than redeclaring them.

Two enum constants (`ELF_T_BYTE`, `ELF_T_SYM`) are set by libbpf's
static-linking write path (`bpf_linker__*`, which this project's load-only
usage never calls) but never read back by anything — their exact ordinal
genuinely can't produce a wrong result even if misremembered, so they're
kept in elfutils' documented canonical order for completeness rather than
re-verified against something that never reads them.

The shim grew twice during this project, each time because a real compile
error demanded a real missing declaration — `elf_getphdrnum`/
`gelf_getphdr`/`GElf_Nhdr`/`gelf_getnote` for `libbpf`'s `usdt.c`, and
`GELF_ST_BIND`/`GELF_ST_TYPE`/`GELF_ST_INFO` for `elf.c` — never spec'd out
in advance, which is itself a small piece of evidence the shim tracks
libbpf's *actual* real usage rather than a guessed-at API surface.

## Why tracepoints, not kprobes

The canonical modern `biolatency` (in `bcc`/`libbpf-tools`) hooks
`blk_account_io_start`/`blk_account_io_done` via kprobes, keyed by the
in-flight `struct request *` itself — more precise than anything
timestamp+key based, because it survives request merges and splits exactly.

That approach isn't available here. This kernel has kprobes compiled out
entirely, confirmed three independent ways, not just inferred from one
missing file:

```
$ ls /sys/bus/event_source/devices/
breakpoint  msr  power  software  tracepoint  uprobe      # no "kprobe"

$ cat /sys/kernel/tracing/kprobe_events
cat: /sys/kernel/tracing/kprobe_events: No such file or directory

$ zcat /proc/config.gz | grep -i kprobe
# CONFIG_KPROBES is not set
CONFIG_HAVE_KPROBES=y
CONFIG_HAVE_KPROBES_ON_FTRACE=y
```

The third one is decisive — this isn't a permissions restriction or a
stripped `/proc`, it's a genuine kernel build-time choice (`HAVE_KPROBES=y`
meaning the architecture supports them, but `KPROBES` itself not set
meaning this particular kernel build doesn't include them). No kprobe-based
tool can work here regardless of privilege.

So this uses `block:block_rq_issue` / `block:block_rq_complete` tracepoints
instead — which this kernel does support — correlating in-flight requests
by `(dev, sector)` rather than by request pointer. This is exactly how
BCC's *original* `biolatency` worked before the project moved to kprobes
for better accuracy, not an invented workaround. The real trade-off it
carries: if the block layer merges two adjacent requests into one before
issue, or splits one request into two, the `(dev, sector)` key can
under- or double-count relative to a `struct request *`-keyed version.
For a learning/demo tool measuring overall latency distribution, this
doesn't change the shape of the histogram meaningfully; it would matter
for a tool trying to attribute latency to a specific high-level I/O
request with byte-for-byte precision.

(`bpftool` was cloned early on, with the idea of generating a proper
`vmlinux.h` via `bpftool btf dump file ... format c` — but `bpftool` itself
needs libelf to build, so building it would have meant solving the same
libelf-dev gap a second time for a tool this project doesn't strictly need.
A full generated `vmlinux.h` is tens of thousands of lines for a program
that touches two tracepoint structs; hand-declaring just those two,
verified against the live kernel as described below, was the better trade
for this project's scope. The clone was never built and isn't included in
this delivery.)

## The real bug: guessed CO-RE struct names were wrong, and here's why

This is the part of the project actually worth reading closely, because it
isn't a sandbox limitation — it's a genuine, reproducible fact about how
the Linux tracepoint machinery names its own generated types, that would
bite on *any* kernel built the same way, not just this one.

**The first attempt** declared the two tracepoint argument structs named
after their tracepoints — the obvious guess:

```c
struct trace_event_raw_block_rq_issue    { __u32 dev; __u64 sector; } __attribute__((preserve_access_index));
struct trace_event_raw_block_rq_complete { __u32 dev; __u64 sector; } __attribute__((preserve_access_index));
```

Field names, types, and offsets were copied verbatim from this kernel's own
`/sys/kernel/tracing/events/block/block_rq_{issue,complete}/format`, so
there was good reason to expect this to work. It compiled cleanly — clang
has no way to know the struct doesn't exist on any real kernel, since
`preserve_access_index` structs are *declared* locally, not looked up at
compile time. It even produced correct-looking `.BTF`/`.BTF.ext` sections.
It failed at the one point that actually checks the name against reality —
loading into the kernel:

```
libbpf: prog 'handle_rq_issue': BPF program load failed: -EINVAL
...
failed to resolve CO-RE relocation <byte_off> [24] struct trace_event_raw_block_rq_issue.dev (0:0 @ offset 0)
...
libbpf: failed to load object 'biolatency.bpf.o'
```

**Root-causing it** meant not trusting a second guess either — instead,
two small diagnostic tools (`toolchain/btf-tools/`) were written to ask
the kernel's real BTF directly, using libbpf's own BTF API
(`btf__parse`, `btf__find_by_name_kind`, `btf_members`) rather than
`bpftool`:

```
$ btf_probe trace_event_raw_block_rq_issue trace_event_raw_block_rq_complete
vmlinux BTF loaded, 115392 types
struct trace_event_raw_block_rq_issue: NOT FOUND (id=-2)
struct trace_event_raw_block_rq_complete: NOT FOUND (id=-2)
```

Confirmed: neither name exists anywhere among 115,392 real types. Not a
typo, not a stripped-symbol issue — genuinely absent. The follow-up tool
searched for anything *related* instead of guessing a third name:

```
$ btf_search block_rq
[kind=4] trace_event_raw_block_rq_requeue
[kind=4] trace_event_raw_block_rq_completion
[kind=4] trace_event_raw_block_rq
[kind=4] trace_event_raw_block_rq_remap
[kind=8] btf_trace_block_rq_issue        (a typedef, not the struct itself)
[kind=8] btf_trace_block_rq_complete
...
```

**The actual mechanism**: the kernel's `TRACE_EVENT`/`DECLARE_EVENT_CLASS`/
`DEFINE_EVENT` macros (`include/trace/events/block.h`) generate one
`struct trace_event_raw_<X>` per **event class**, shared by every
tracepoint defined against that class — not one struct per tracepoint.
`block_rq_issue`, `block_rq_insert`, and `block_rq_merge` are all
`DEFINE_EVENT(block_rq, ...)` against a class literally named `block_rq` —
so the generated struct is `trace_event_raw_block_rq`, named after the
*class*, and the `_issue` suffix (the individual tracepoint's own name)
never appears in the struct name at all. Likewise `block_rq_complete` and
`block_rq_error` are both `DEFINE_EVENT(block_rq_completion, ...)`, so
their shared struct is `trace_event_raw_block_rq_completion`.

Confirmed directly by dumping both real structs' fields from the kernel's
BTF and checking they match the tracefs format files exactly:

```
$ btf_probe trace_event_raw_block_rq trace_event_raw_block_rq_completion
struct trace_event_raw_block_rq: FOUND, id=23768, size=64
  [1] dev      byte=8   type_id=66
  [2] sector   byte=16  type_id=84
  ...
struct trace_event_raw_block_rq_completion: FOUND, id=23767, size=48
  [1] dev      byte=8   type_id=66
  [2] sector   byte=16  type_id=84
  ...
```

Byte offsets 8 and 16 for `dev`/`sector` in both — exactly matching the
tracefs `format` files' `offset:8`/`offset:16`, just filed under the event
class's name rather than the individual tracepoint's name. Renaming the two
structs in `vmlinux_min.h` to `trace_event_raw_block_rq` and
`trace_event_raw_block_rq_completion` (and nothing else — field names,
types, and the `preserve_access_index` attribute were already correct) was
the entire fix. The program loaded and ran correctly on the first attempt
after that change, and has reproduced the same result across multiple runs
and a full container restart (fresh kernel boot, same kernel build) since.

This is worth calling out because it's a genuinely easy mistake that has
nothing to do with this being a constrained sandbox: anyone hand-declaring
a CO-RE struct for a tracepoint that happens to share its event class with
other tracepoints — which is common; `DECLARE_EVENT_CLASS` exists
specifically to let related tracepoints share one struct and one
pretty-printer — will hit exactly this if they guess the struct name from
the tracepoint's own name instead of checking. A generated `vmlinux.h`
sidesteps it by construction (bpftool reads the real BTF and never has to
guess), which is also, honestly, the strongest argument *for* generating
one on a production project; hand-declaring is a reasonable trade only when
the alternative (building bpftool here) costs more than verifying two
struct names directly against the target's BTF — which is exactly what
`btf_probe`/`btf_search` make cheap to do.

## Why `vmlinux_min.h` is hand-declared, and why that's still real CO-RE

A full generated `vmlinux.h` (`bpftool btf dump file /sys/kernel/btf/vmlinux
format c`) would be on the order of 30,000+ lines for a program that
touches exactly two struct fields. `vmlinux_min.h` instead hand-declares
just the two structs this program actually reads, pulling the base integer
typedefs and BPF UAPI enums/flags from the real system
`<linux/types.h>`/`<linux/bpf.h>` (so nothing there is hand-transcribed —
only the two tracepoint-specific structs, which never had a UAPI header to
begin with, are typed by hand).

This is a legitimate, intentional CO-RE pattern, not a shortcut unique to
this project — and the mechanism is why: CO-RE relocation does not care
whether a `preserve_access_index` struct's definition came from a generated
`vmlinux.h` or was typed by a human, and it does **not** require the local
definition to match the target kernel's real layout, size, or field order.
At compile time every field access becomes a relocation record tagged with
just the *(struct name, field name)* pair; at load time libbpf looks that
pair up in the target's real BTF and patches in the real offset, discarding
whatever offset the local declaration implied. The offsets written in
`vmlinux_min.h` are structurally irrelevant — provably so, since the actual
patched offsets (8 and 16, logged by libbpf below) came from the kernel's
BTF, not from the header. What has to be correct is only the spelling of
the two names and which fields exist — which is exactly the thing that was
wrong initially, and exactly the thing `btf_probe`/`btf_search` check
directly instead of assuming.

```
libbpf: CO-RE relocating [24] struct trace_event_raw_block_rq: found target candidate [23768] struct trace_event_raw_block_rq in [vmlinux]
libbpf: prog 'handle_rq_issue': relo #0: patched insn #6 (ALU/ALU64) imm 0 -> 8
libbpf: prog 'handle_rq_issue': relo #1: patched insn #14 (ALU/ALU64) imm 8 -> 16
```

## Build-chain quirks worth knowing about

- **`NO_PKG_CONFIG=1 BUILD_STATIC_ONLY=1`** for libbpf's own build: there's
  no `libelf.pc` for the hand-shimmed libelf (pkg-config has no idea it
  exists), so letting libbpf's Makefile consult pkg-config at all would
  just fail; `EXTRA_CFLAGS`/`EXTRA_LDFLAGS` point it at the shim's
  `include/`/`lib/` directly instead. `BUILD_STATIC_ONLY=1` skips building
  `libbpf.so`, which this project never needs (the loader links
  `libbpf.a` statically) and which would need the shim to also provide a
  working `.so`-compatible link surface for no benefit.
- **`clang -target bpf` and multiarch headers**: compiling the BPF object
  needs `-I/usr/include/x86_64-linux-gnu` explicitly — clang's `bpf` target
  doesn't search Debian/Ubuntu's multiarch include path by default the way
  its native x86_64 target does, and `<linux/bpf.h>` pulls in
  `<asm/types.h>`, which only exists under that multiarch directory here.
  Without it: `fatal error: 'asm/types.h' file not found`.
- **`tracefs`/`debugfs` aren't mounted by default** in this sandbox and
  have to be mounted fresh every session (`mount -t tracefs tracefs
  /sys/kernel/tracing`, same for `debugfs`) — confirmed this is a
  per-boot/per-session state, not a one-time setup, by watching it come back
  unmounted after a container restart mid-project.

## What was actually verified

- `raw_load_test` (zero dependencies, hand-encoded `BPF_PROG_LOAD` syscall):
  confirms this sandbox can load *some* BPF program into the kernel at
  all, independent of libbpf/libelf/CO-RE working — step zero, run before
  investing in anything else. Re-confirmed after a full container restart.
- `biolatency.bpf.o` compiles cleanly with real `.BTF`/`.BTF.ext` CO-RE
  sections present (`readelf -S`).
- `biolatency` (the userspace loader) loads the object, and libbpf's own
  verbose log (`BIOLATENCY_VERBOSE=1`) shows the real CO-RE relocation
  happening and succeeding against the real kernel BTF (excerpted above).
- Both tracepoints attach successfully (`bpf_program__attach`, no manual
  "tracepoint" plumbing needed — libbpf reads the `SEC()` name itself).
- Running against real `O_DIRECT` block I/O (`dd ... oflag=direct` /
  `iflag=direct`, which bypasses the page cache and therefore genuinely
  reaches the block layer) produces a real, non-empty, plausibly-shaped
  latency histogram — most completions in the few-hundred-microsecond
  range with a small slower tail, consistent with a shared virtio block
  device under light load.
- **Reproduced across three independent runs**, including one after a full
  container restart (fresh kernel boot of the same kernel build) and one
  after rebuilding the *entire* toolchain from scratch
  (`make clean && make` on libbpf itself, then biolatency) — this isn't a
  result that only happened once by chance.

## Honest scope — what this doesn't do

- **No per-device or per-direction (read/write) breakdown** — BCC's real
  `biolatency` supports `-D` (per-disk histograms) and `-Q`/`-F` flags for
  queued-time and per-flag breakdowns; this version prints one overall
  histogram, matching the simplest invocation of the original tool, not
  its full flag surface.
- **No periodic/interval printing** (`biolatency -i 1`, print every N
  seconds) — this runs for a fixed duration or until Ctrl-C and prints
  once at the end. Straightforward to add (read-and-zero the map on a
  timer instead of only at exit) but wasn't needed to demonstrate the
  real end-to-end CO-RE path, which was this project's actual point.
- **`(dev, sector)` correlation, not `struct request *`** — see "Why
  tracepoints, not kprobes" above for the precision trade-off this
  carries under request merges/splits. This is a kernel-capability
  constraint (no kprobes), not a choice that was available and skipped.
- **Histogram only in-kernel, read once at exit** — there's no live
  terminal UI; output is the same static, cron-friendly text format BCC's
  own tool produces.
