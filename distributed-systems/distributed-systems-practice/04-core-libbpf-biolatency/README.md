# CO-RE/libbpf rewrite of BCC's `biolatency`

A from-scratch, CO-RE (Compile Once – Run Everywhere) rewrite of
[BCC](https://github.com/iovisor/bcc)'s `biolatency` tool: it traces every
block I/O request on the live kernel and prints a log2 histogram of request
latency, in microseconds, the same output shape as the original.

Everything here is real: a real compiled eBPF object loaded into the real
running kernel via the real `bpf()` syscall, with real CO-RE relocation
resolved against this kernel's actual BTF — not a simulation, not a
recording, not code that merely compiles. See `ANALYSIS.md` for exactly how
that was confirmed and for a genuine bug this kernel's BTF forced out into
the open along the way.

```
# as root
cd biolatency
make
./biolatency 10          # trace for 10 seconds, or omit for Ctrl-C
```

```
Tracing block I/O latency (block_rq_issue -> block_rq_complete)... for 10s.

usecs               : count    distribution
       129 -> 256        : 16       |**                                      |
       257 -> 512        : 239      |****************************************|
       513 -> 1024       : 60       |**********                              |
      1025 -> 2048       : 29       |****                                    |
      2049 -> 4096       : 1        |                                        |
     65537 -> 131072     : 13       |**                                      |
```

## Why this is harder than it sounds here

The roadmap that set this project up predicted loading real eBPF into this
sandbox's kernel was "unverified / unlikely." That turned out to be wrong —
but getting there required building most of the eBPF toolchain from parts,
because the one thing actually missing was `libelf-dev` (apt is
network-blocked, same as every other project in this series), and
`libelf-dev` is what `libbpf`, `bpftool`, and essentially every eBPF
build normally assume you have. Nothing here is a stand-in for the real
toolchain — it's the real toolchain, with the one missing piece replaced by
a hand-written, independently-verified substitute for that piece alone.

## What's here

```
04-core-libbpf/
├── biolatency/                 the actual tool
│   ├── biolatency.bpf.c        kernel-side: the CO-RE eBPF program (clang -target bpf)
│   ├── vmlinux_min.h           hand-declared CO-RE structs (not a generated vmlinux.h)
│   ├── biolatency.c            userspace: loads/attaches/reads the histogram (gcc, libbpf API)
│   └── makefile
└── toolchain/                  everything biolatency.bpf.o/biolatency are built against
    ├── libelf-shim/            hand-written libelf.h/gelf.h for the libelf-dev gap
    ├── libbpf/                 real libbpf source (github.com/libbpf/libbpf), built from here
    ├── btf-tools/               two small programs that query the kernel's real BTF directly
    └── raw-load-test/          the zero-dependency "can this sandbox load BPF at all?" probe
```

`ANALYSIS.md` is the one to read for the why behind every piece of this —
especially the libelf-shim verification methodology and the CO-RE
relocation bug this kernel's BTF exposed, which is the most interesting
thing that happened in this project.

## Build

Everything is built from source, in dependency order. From a clean checkout:

```
# 1. libbpf itself — the real github.com/libbpf/libbpf source, built with
#    NO_PKG_CONFIG=1 because there's no libelf.pc for the hand-written shim
#    to legitimately claim, and pointed at the shim's headers/lib explicitly
cd toolchain/libbpf/src
make NO_PKG_CONFIG=1 BUILD_STATIC_ONLY=1 \
  EXTRA_CFLAGS="-I$(pwd)/../../libelf-shim/include" \
  EXTRA_LDFLAGS="-L$(pwd)/../../libelf-shim/lib"

# 2. the tool itself
cd ../../../biolatency
make
```

(`toolchain/libelf-shim/` and `toolchain/btf-tools/` also have their own
small makefiles if you want to rebuild or re-run their verification
binaries — see ANALYSIS.md for what each one proves.)

## Run

```
sudo ./biolatency           # trace until Ctrl-C
sudo ./biolatency 10        # trace for 10 seconds, then print and exit
BIOLATENCY_VERBOSE=1 sudo ./biolatency 5   # + libbpf's own load/attach/CO-RE debug log
```

Needs `CAP_BPF`+`CAP_PERFMON` or `CAP_SYS_ADMIN` (in practice, root) to load
a BPF program and attach tracepoints — `biolatency` checks for root itself
and gives a clear error rather than an opaque EPERM if you forget `sudo`.

To see it actually counting something (idle disks produce an empty
histogram, correctly — "no I/O completions observed" if genuinely nothing
happened), generate I/O that bypasses the page cache while it's running:

```
dd if=/dev/zero of=/tmp/testfile bs=1M count=100 oflag=direct
sync
dd if=/tmp/testfile of=/dev/null bs=1M iflag=direct
```

## What this measures, and what it doesn't

This hooks the `block:block_rq_issue` and `block:block_rq_complete`
tracepoints and correlates them by `(dev, sector)`, not by `struct request
*`. That's a real, deliberate design choice forced by this kernel having
`CONFIG_KPROBES` compiled out (confirmed from `/proc/config.gz`, not just
inferred from a missing file) — the canonical modern `biolatency` in
`bcc`/`libbpf-tools` uses kprobes on `blk_account_io_start`/`_done` keyed by
the request pointer itself, which isn't available here. `(dev, sector)` is
the same key BCC's own *original* tracepoint-based biolatency used before
the project moved to kprobes, and it undercounts slightly when the block
layer merges requests. `ANALYSIS.md` has the full trade-off.
