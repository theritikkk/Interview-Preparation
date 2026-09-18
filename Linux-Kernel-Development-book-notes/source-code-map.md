# Source Code Map

> Your guide to the Linux kernel source tree itself. This file grows with every chapter — each one adds the specific files/functions/structures it covers. Read this file's top section once now; treat the per-chapter sections as reference material to return to.

---

## How to Get the Source

```bash
git clone --depth 1 https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git ~/linux
cd ~/linux
```
A shallow clone is enough for reading current code. If you later want history (`git log`, `git blame`) on a specific file:
```bash
git fetch --unshallow   # multi-GB, be patient — or just do this once you actually need history
```

---

## Top-Level Directory Tour

| Directory | Responsibility | First relevant chapter |
|---|---|---|
| `arch/` | Architecture-specific code (x86, arm64, riscv, ...) — boot, low-level entry/exit, page table formats, context-switch assembly | Ch.1 (mentioned), Ch.3-5 (used directly) |
| `block/` | Block I/O layer — request queues, I/O schedulers | Ch.14 |
| `certs/` | Kernel module/kexec signing infrastructure | (not covered in depth) |
| `crypto/` | Cryptographic algorithm implementations used internally | (not covered in depth) |
| `Documentation/` | Formal process docs, subsystem documentation, coding style | Ch.1, Ch.20 |
| `drivers/` | The overwhelming majority of the tree's code — device drivers of every kind | Ch.17 |
| `fs/` | Filesystems and the VFS (Virtual Filesystem) abstraction layer | Ch.13 |
| `include/` | Public and internal headers; `include/linux/` holds most core data structure declarations (`task_struct`, etc.) | Every chapter |
| `init/` | Kernel boot/initialization; `init/main.c` is the closest thing to "where the kernel proper begins" after architecture-specific early boot | Ch.2 |
| `ipc/` | System V IPC — semaphores, shared memory, message queues | (touched in Ch.9-10 context) |
| `kernel/` | The core kernel: process management, scheduling, most syscall infrastructure, `kernel/sched/`, `kernel/irq/`, etc. | Ch.3, 4, 5, 7, 8, 9, 10, 11 |
| `lib/` | Generic, architecture-independent library code (some Ch.6 data structures live here) | Ch.6 |
| `mm/` | Memory management | Ch.12, 15, 16 |
| `net/` | The networking stack | (connections drawn throughout in "Connection to Systems Engineering" sections) |
| `scripts/` | Build system and developer tooling | Ch.2, 20 |
| `security/` | LSM framework, SELinux, and related | (not covered in depth) |
| `sound/` | Audio subsystem | (not covered in depth) |
| `tools/` | User-space tools shipped alongside the kernel — `perf` lives here | Ch.18 |

## Navigation Tools — How and When to Use Each

### `ripgrep` (`rg`) — your default, fastest tool
```bash
rg -n "task_struct" kernel/ include/linux/sched.h   # -n shows line numbers
rg -t c "spin_lock" kernel/                          # -t c restricts to C files
```
Use for: quick, ad-hoc searches across the whole tree. This is what you'll reach for 90% of the time.

### `git grep` — like `rg`, but respects the git tree exactly and is always available if you have the repo
```bash
git grep -n "asmlinkage long sys_"
git grep -n "SYSCALL_DEFINE" -- '*.c'
```
Use for: the same job as `rg`, when you specifically want results scoped to tracked files only, or when `rg` isn't installed.

### `git log` / `git blame` — history, not just current content
```bash
git log --oneline -- kernel/sched/core.c | head       # recent commits touching this file
git blame -L 100,130 kernel/fork.c                      # who wrote these specific lines, and when
```
Use for: understanding *why* code looks the way it does — a huge advantage of open kernel development (Ch.1) is that this history is always available.

### `ctags` / `cscope` — jump-to-definition, jump-to-references
```bash
ctags -R .          # build a tag index once (regenerate after pulling new code)
cscope -Rbq          # build a cscope database
```
Use for: editor integration (Vim, Emacs, and many others support both natively) — once built, "go to definition" and "find all callers" become instant, essential for tracing execution across a codebase this large. This course will lean on this heavily from Chapter 3 onward.

### `pahole` — inspect a compiled structure's real, compiler-decided memory layout
```bash
pahole task_struct    # requires debug info available, e.g. on a locally-built kernel with CONFIG_DEBUG_INFO
```
Use for: seeing exactly how a struct like `task_struct` is laid out in memory once the compiler applies padding/alignment — introduced properly once we reach data-structure-heavy chapters (Ch.6, 12).

### `objdump` / `readelf` / `nm` — inspect compiled binaries and modules
```bash
objdump -d my_module.ko | less    # disassemble a compiled kernel module
readelf -S my_module.ko            # inspect ELF sections
nm my_module.ko                     # list symbols
```
Use for: Chapter 17 (modules) and Chapter 18 (debugging), when you need to inspect what the compiler/linker actually produced, not just what you wrote.

### `gdb` / `crash` — live and post-mortem kernel debugging
Introduced properly in Chapter 18, where you'll set up `kgdb`/`crash` against a real (VM) kernel.

---

## Chapter 1's Contribution to This Map

Chapter 1 didn't touch specific functions yet — it oriented you to the tree's *shape*. The one concrete pointer from Chapter 1:

```bash
# Where the x86-64 syscall entry mechanism (conceptually introduced in Ch.1)
# actually lives — full detail comes in Chapter 5.
arch/x86/entry/entry_64.S            # low-level asm entry point
arch/x86/entry/common.c              # do_syscall_64() -- the C-level dispatcher
arch/x86/include/generated/uapi/asm/unistd_64.h   # syscall numbers (__NR_*)
```

*(Every subsequent chapter appends its own section here, in the same format: which files, which structures, which functions, and how they call into each other.)*