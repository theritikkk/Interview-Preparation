# Source Code and Tool Map

Every tool this course actually used, grouped by job, with what it
reveals and where it was introduced. Treat this as the toolbox index —
`glossary.md` is the concepts, this is the commands.

---

## Assembling and Disassembling

| Tool | Reveals | Example (from this course) | File |
|---|---|---|---|
| `nasm` | Assembles `.asm` → raw binary or object | `nasm boot_sect.asm -f bin -o boot_sect.bin` | `03` |
| `ndisasm` | Disassembles raw binary back to NASM-style assembly | `ndisasm -b16 hi.bin` | `03` |
| `objdump -d` | Disassembles an object/binary, AT&T syntax by default | `objdump -D -b binary -m i8086 hi.bin` | `03`, `11` |
| `xxd` / `hexdump -C` | Raw hex + ASCII view of any file | `xxd findbyte.bin \| head -2` | `02`, `05` |

## Compiling and Linking

| Tool | Reveals | Example | File |
|---|---|---|---|
| `gcc -E` | Preprocessor output only (still C) | `gcc -E basic.c -o basic.i` | `11` |
| `gcc -S` | Generated assembly (human-readable `.s`) | `gcc -S basic.c -o basic.s` | `10`, `11` |
| `gcc -c` | Object file (preprocess+compile+assemble) | `gcc -ffreestanding -c basic.c -o basic.o` | `11` |
| `ld` | Links object file(s) into a binary; resolves relative→absolute addresses | `ld -o kernel.bin -Ttext 0x1000 kernel_entry.o kernel.o --oformat binary` | `11`, `12` |
| `make` | Runs only what's stale, by dependency timestamp | `make run` | `13` |

## Inspecting Object Files and Executables

| Tool | Reveals | Example | File |
|---|---|---|---|
| `readelf -h` | ELF header: entry point, type, machine | `readelf -h kernel_entry.o` | `12` |
| `readelf -S` | Section table (link/debug view) | `readelf -S kernel_entry.o` | `12` |
| `readelf -l` | Program headers / segments (runtime-load view) | `readelf -l a.out` | `12` |
| `file` | Identifies a file's format at a glance | `file bootx64.efi` → PE executable | `04`, `34` |
| `nm` | Lists symbols in an object file | `nm kernel.o` | `12` |
| `size` | Section sizes (`.text`/`.data`/`.bss`) | `size kernel.bin` | `13` |
| `strings` | Printable text embedded in a binary | `strings kernel.bin` | — |

## Running and Inspecting a Live Process

| Tool | Reveals | Example | File |
|---|---|---|---|
| `gdb` | Interactive debugging: breakpoints, stepping, memory/register inspection | `target remote localhost:1234` after `qemu -s -S` | `08`, `27` |
| `strace` | Every syscall a process makes, with arguments and return value | `strace -c ls` | `21`, `26` |
| `ltrace` | Library calls a process makes | — | `21` (mentioned) |
| `cat /proc/<pid>/maps` | A process's virtual address space, region by region | `cat /proc/self/maps` | `10`, `22` |
| `cat /proc/<pid>/status` | Process state, memory stats | `grep VmRSS /proc/self/status` | `19`, `22` |

## Booting and Emulating

| Tool | Reveals | Example | File |
|---|---|---|---|
| `qemu-system-i386`/`-x86_64` | Boots a raw disk image in an emulator | `qemu-system-i386 -fda boot_sect.bin` | `06` |
| `qemu -s -S` | Freezes the guest CPU, opens a GDB stub on port 1234 | `qemu -s -S -fda os-image` | `08`, `27` |
| Bochs debugger | `b`, `s`, `info registers`, `sreg`, `x` — breakpoint/step/inspect | `b 0x7c50` then `c` | `06`, `08`, `27` |
| `qemu -enable-kvm` | Hardware-accelerated execution instead of software emulation | `qemu-system-x86_64 -enable-kvm <image>` | `06`, `28` |

## Kernel and System Observability

| Tool | Reveals | Example | File |
|---|---|---|---|
| `dmesg` | Kernel's own boot/runtime log ring buffer | `dmesg -w` (live stream) | `01`, `25`, `27` |
| `cat /proc/interrupts` | Per-CPU interrupt counts, by device | — | `14`, `24`, `30` |
| `cat /proc/cpuinfo` | Per-core CPU details, including feature flags | `grep -o 'lm\|vmx\|svm' /proc/cpuinfo` | `04`, `28` |
| `lscpu` | Summarized CPU topology (cores, threads, cache, NUMA) | — | `04`, `24`, `33` |
| `lsmod` / `modinfo` | Loaded kernel modules and their details | — | `25` |
| `perf stat` | Hardware counters: instructions, cache misses, branch mispredicts | `perf stat ls` | `32`, `33` |
| `perf record` / `report` | Sampling profiler; builds a call-stack histogram | `perf record -g ./prog` | `32` |
| `ftrace` / `trace-cmd` | Dynamic tracing of a running kernel, no rebuild | — | `27` |
| `bpftrace` | High-level scripting language compiling to eBPF | `bpftrace -e 'tracepoint:syscalls:sys_enter_openat {...}'` | `32`, `35` |
| `bpftool prog list` | Every eBPF program currently loaded | — | `35` |

## Filesystem, Storage, and Boot Configuration

| Tool | Reveals | Example | File |
|---|---|---|---|
| `mkfs.fat -F 12` | Creates a FAT12 image | `mkfs.fat -F 12 floppy.img` | `18` |
| `fdisk -l` | MBR-style partition listing | — | `07`, `17` |
| `gdisk -l` / `parted` | GPT-style partition listing | — | `07` |
| `efibootmgr -v` | UEFI boot entries and current `BootOrder` | — | `07`, `34` |
| `mokutil --sb-state` | Whether Secure Boot is currently active | — | `34` |
| `tpm2_pcrread` | Current TPM PCR values (measured boot evidence) | — | `34` |

## Networking

| Tool | Reveals | Example | File |
|---|---|---|---|
| `tcpdump` | Captures packets at the interface, below the socket layer | `tcpdump -i lo -c 5` | `30`, `35` |
| `ss -i` | Live socket statistics, including wait states | — | `30` |
| `ip link` / `ethtool` | Interface and driver-level counters | `ethtool -S eth0` | `30` |

## Containers and Namespaces

| Tool | Reveals | Example | File |
|---|---|---|---|
| `unshare` | Creates a process in a new namespace | `unshare --pid --fork --mount-proc bash` | `29` |
| `docker history` | A built image's actual layer stack | — | `29` |
| `docker inspect` / `runc state` | A running container's actual namespace/cgroup config | — | `29` |

---

## Where Real Source Lives

| Thing | Where to look |
|---|---|
| Linux kernel source | `git clone --depth 1 https://git.kernel.org/.../linux.git`, or [elixir.bootlin.com](https://elixir.bootlin.com) for zero-setup cross-referenced browsing |
| x86 ISA documentation | Intel SDM / AMD64 Architecture Programmer's Manual |
| GCC/Clang output | [Compiler Explorer](https://godbolt.org) for instant, multi-config comparison |
| This course's own Rosetta table | `25-linux-kernel-architecture.md` — every toy-OS piece, mapped to its real source directory |
