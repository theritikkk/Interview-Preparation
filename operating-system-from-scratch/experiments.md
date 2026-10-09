# Experiment Map

Every practical experiment in this course, indexed by difficulty
instead of by file order. Full instructions live in each experiment's
home file — this is the index, not a replacement. **All VM-level
experiments (Levels 3+) run in QEMU/Bochs; nothing in this course ever
touches a real disk's boot sector.**

---

## Level 1 — User-Space Observation

No compilation, no VM. Runs directly on your own machine.

- `uname -a`, `lscpu`, `dmesg | head` — the stack made visible — `01`
- `/proc/cpuinfo`'s `lm` flag; compare `uname -m` across machines — `04`
- `ps -eLf`, `cat /proc/<pid>/status` — processes and threads — `19`
- `strace -c ls`; `strace` your own `write()` call — `21`
- `cat /proc/self/maps`; `malloc(1GB)` + touch pages, watch `VmRSS` climb — `22`
- `lscpu`, `taskset -c 0`, `mpstat -P ALL 1` — SMP topology and affinity — `24`
- `grep`/`rg` the real kernel source; trace `read()` via elixir.bootlin.com — `26`
- `unshare --pid --fork --mount-proc bash`; `docker inspect` — `29`
- `tcpdump -i lo -c 5`, `ss -i` — `30`
- `perf stat ls` — `32`
- `lscpu` cache sizes; `perf stat -e branch-misses` — `33`
- `efibootmgr -v`, `mokutil --sb-state` — `07`, `34`

## Level 2 — Assembly and Binaries

`nasm`, `gcc`, `objdump`, `readelf` — no VM yet.

- Prove endianness with a C union/cast; `xxd` a real file's ELF header — `02`
- Hand-decode the book's "Hello" opcodes; compare `ndisasm` vs. `objdump` (AT&T vs. Intel) — `03`
- "Find the byte" (4 attempts), the stack push/pop example, `classify.c`'s `cmp`/`je` — `05`
- The segment-register resolution of "find the byte"; prove segment overlap — `09`
- `-m32` vs. native disassembly of the book's 3 C examples; `-O0` vs. `-O2` — `10`
- The full `gcc -E/-S/-c` → `ld -Ttext` pipeline, twice, at two load addresses — `11`
- Reproduce the entry-point bug (function order breaks it), then fix it — `12`
- A deliberate race condition + fix three ways + `-fsanitize=thread` — `23`

## Level 3 — QEMU / Bootloader

First VM experiments. Nothing here touches real hardware.

- Boot the "Hello, World!" boot sector; corrupt the magic number; remove the infinite loop — `06`
- Attempt booting the same image via OVMF (UEFI) instead of BIOS — `07`
- Full real→protected mode switch; break the far jump on purpose; corrupt the null descriptor — `08`
- Build the complete `boot/`+`kernel/`+`drivers/` tree and `make run` — `13`
- Read disk sectors via BIOS `int 0x13`; trigger and check both failure modes (carry flag, partial read) — `17`
- Build a FAT12 image on the host, read it from the book's own disk-read code — `18`
- The complete, assembled checkpoint system — `37`

## Level 4 — Kernel Modules and Linux Internals

Real Linux, not the toy OS — observing how production kernel features work.

- `lsmod`, `modinfo` on real loaded modules — `25`
- Hand-create a cgroup, set a memory limit, watch an OOM kill — `29`

## Level 5 — Kernel Debugging

- Bochs debugger: breakpoint right after `mov cr0, eax`, confirm the PE bit flipped — `08`
- GDB attached via `qemu -s -S`; finish the book's own unsolved Question 5 (`print_hex`) — `27`

## Level 6 — Custom OS Components

Building directly into your own toy kernel from file `13`. This is
where the capstone (file `38`) lives in full.

- Minimal IDT + PIC remap + one working ISR on IRQ0 — `14` / capstone stage 7
- `port_byte_in`/`out` wrappers, used for real — `15`
- Keyboard ISR (IRQ1, scancode table) — `16` / capstone stage 8
- A real ATA/IDE PIO disk driver (no BIOS available) — `17`
- PIT-driven timer tick — capstone stage 9
- Cooperative two-task switch, then preemptive (timer-driven) — `20` / capstone stage 11
- Identity-mapped paging + a vector-14 handler printing `CR2` — `22` / capstone stage 10
- A minimal FAT12 reader — `18` / capstone stage 12

## Level 7 — Advanced Systems Experiments

- TCG vs. KVM-accelerated boot timing comparison — `28`
- ASLR address comparison across runs; stack canary + PIE experiment — `31`
- Generate a real flame graph (`perf record` → `FlameGraph` scripts) — `32`
- `bpftrace` one-liners tracing live syscalls; trigger a verifier rejection on purpose — `35`
- Ring-3 syscall gate with a working TSS; user-mode program loader and shell — capstone stages 13–14

---

## A Note on Difficulty

Levels roughly track *consequence of a mistake*, not just concept
difficulty: Level 1 can't break anything; Level 3 can only "break" a
disposable VM; Level 6–7 is where real, patient debugging (file `27`)
becomes the actual skill being practiced, not a side note.
