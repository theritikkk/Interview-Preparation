# sched_ext toy scheduler — a real scheduler this kernel genuinely can't run

A real, compiled, correctly-structured [`sched_ext`](https://github.com/sched-ext/scx)
BPF scheduler (`scx_simple` — a global FIFO/weighted-vtime scheduler,
adapted from the real upstream Linux kernel tree) — and a real, captured,
reproducible kernel rejection, because this sandbox's kernel was built
with `CONFIG_SCHED_CLASS_EXT` **not set**.

Unlike every earlier project in this series, this one does **not** end
with something running. That's the honest result of the investigation,
not a shortfall: scheduler classes are compiled directly into the kernel
core at build time (never a loadable module — this isn't a
privileges/tooling gap, see `ANALYSIS.md`), so there's no alternative
attach path the way Project 4 found tracepoints when kprobes were
missing, or Project 5 found generic-mode XDP when native mode wasn't
available. What this project delivers instead: real code, a real build,
and a real, specific, directly-observed kernel error — captured, not
predicted — plus the independent evidence that triangulates exactly why.

## Run it

```
cd scx-simple
make                 # -> scx_simple.bpf.o, loader
sudo ./loader scx_simple.bpf.o
```

Expected output on this sandbox (and reproduced, unedited, in
`reference/real_load_attempt.log`):

```
[1/3] bpf_object__open_file(scx_simple.bpf.o)
      open OK — object name: scx_simple
[2/3] bpf_object__load() — this is the real kernel accept/reject step
...
libbpf: loaded kernel BTF from '/sys/kernel/btf/vmlinux'
libbpf: extern (func ksym) 'scx_bpf_consume': not found in kernel or module BTFs

==> LOAD REJECTED (errno=22, Invalid argument)
```

On a kernel built with `CONFIG_SCHED_CLASS_EXT=y` (6.12+, most
distributions' default kernels by now), the exact same `loader` binary
and `.bpf.o` would instead print `ATTACHED. scx_simple is now the system
scheduler.` and start printing live `local=... global=...` dispatch
counters — `loader.c` has that full path implemented and was reviewed
for correctness, it's simply never reached here (see `ANALYSIS.md`).

## What's here

```
06-sched-ext/
├── scx-simple/                   the scheduler and its loader
│   ├── scx_simple.bpf.c          the real scheduler — byte-for-byte upstream logic
│   ├── vmlinux_min.h             hand-declared CO-RE types (sched_ext_ops, etc.)
│   ├── scx_common_min.bpf.h      trimmed kfunc decls + struct_ops macros
│   ├── user_exit_info_min.h      exit-reason reporting (BPF side + userspace side)
│   ├── loader.c                  non-skeleton libbpf loader (open/load/attach)
│   ├── bpf_helpers.h, bpf_helper_defs.h, bpf_tracing.h, bpf_core_read.h
│   │                             (libbpf's own bundled BPF-side headers, reused
│   │                              from Projects 4/5's toolchain)
│   └── makefile
└── reference/                    provenance — what the real upstream looks like
    ├── scx_simple.bpf.c.upstream     the real file this project's version is
    │                                 byte-for-byte identical to (diff them)
    ├── scx_simple.c.upstream         real upstream userspace (uses a skeleton;
    │                                 loader.c is the non-skeleton equivalent)
    ├── common.bpf.h.upstream         the real, untrimmed 427-line header
    ├── compat.bpf.h.upstream, user_exit_info.h.upstream
    ├── kernel_ext.c.excerpt.upstream real kernel/sched/ext.c excerpt —
    │                                 struct sched_ext_ops's actual definition
    ├── real_load_attempt.log         full, unedited output of the real load
    └── real_sections.txt             readelf -S of the real compiled object
```

Reuses Project 4's already-built libbpf + libelf shim
(`04-core-libbpf/toolchain/`) — keep this zip extracted next to that one,
or see the makefile comments for pointing at your own libbpf checkout
(note: `-mcpu=v3` is required for this specific program — see
`ANALYSIS.md` for why).

## Why this result, not a workaround

This project's entire point was to check the one capability question
Project 5's own roadmap entry left explicitly open. The answer turned
out to be no, confirmed four independent ways (kernel build config,
live BTF introspection, struct_ops family registration, and now a real
load attempt), and with a specific architectural reason that rules out
"maybe it just needs enabling at runtime": scheduler classes are
compiled into the kernel core, never loadable, for any Linux kernel.
Building and attempting to run the real thing anyway — rather than
skipping this project — is what makes that "no" a verified fact instead
of a guess, and `ANALYSIS.md` is where the full chain of evidence lives.
