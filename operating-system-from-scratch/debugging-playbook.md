# Debugging Playbook

Every systematic debugging chain this course built, consolidated by
symptom. Each one is: **the question that narrows the layer**, then
**the chain**, then **the tool at each link** (full detail in
`source-code-map.md`).

The one rule underneath all of them, from file `01`: identify the
layer before picking a tool. Guessing the tool first wastes the most
time.

---

## "My program crashed"

```
application touches a bad address
   ↓
MMU walk fails (Present=0, or R/W or U/S violated)        → page table bit, file 22
   ↓
CPU raises exception 14, faulting address into CR2          → file 14
   ↓
kernel's page fault handler finds no valid mapping          → file 22
   ↓
kernel sends the process SIGSEGV
   ↓
"Segmentation fault" — or a core dump
```

**Tools:** `gdb` on the core dump or live (`bt` for a backtrace, which
only works cleanly if frame pointers weren't omitted — file `10`,
`33`); the faulting address itself is a clue (near `0` → null pointer,
file `08`; huge/garbage → wild pointer; just past a buffer → off-by-one).

---

## "My program is slow"

Start with the **USE method** (file `32`): Utilization, Saturation,
Errors, per resource, before touching code.

```
Is it on-CPU or off-CPU?                      → perf record -g (on-CPU)
                                                 or off-CPU flame graphs (file 32)
On-CPU: where in the call stack?              → perf report / flame graph
Off-CPU: waiting for what?
   ├─ a lock                                   → file 23, lock profiling
   ├─ I/O                                      → see "disk I/O is slow" below
   └─ to be scheduled (Ready, not Running)     → file 19/20, mpstat -P ALL
```

**Tools:** `perf stat`/`perf record` (file `32`), `strace -T` (per-
syscall timing, file `21`).

---

## "Disk I/O is slow"

```
application calls read()/write()
   ↓
syscall boundary (file 21)                     → strace -T
   ↓
VFS dispatch (file 18)                         → which filesystem's f_op?
   ↓
page cache — hit or miss?                      → file 22, free -h
   ↓
block layer (file 17)                          → iostat -x
   ↓
driver (file 15, 16)                           → dmesg for driver errors
   ↓
device itself                                  → smartctl (storage health)
```

---

## "A network connection seems stuck"

File `30`'s full path, read both directions:

```
NIC → DMA (file 15) → interrupt/NAPI (file 14) → driver
   → network stack (Ethernet/IP/TCP) → socket receive buffer
   → process wakes from Waiting (file 19/20) → recv()/read()
```

**Diagnosis order:**
```
Is the packet arriving at the NIC at all?      → ethtool, /proc/interrupts
Is the driver keeping up (drops)?               → ip -s link
Is the socket buffer the bottleneck?            → ss -i
Is the application just slow to read it?        → strace -T on the process
```

A socket that "hangs" is very often a process correctly in **Waiting**
(file `19`) for data that hasn't arrived — `ss -i` and
`/proc/<pid>/status` both show this state directly, which is a
different diagnosis entirely from "broken."

---

## "Two threads produce the wrong answer, sometimes"

This is file `23`'s entire chapter, compressed:

```
Reproduce more often — stress loop, more threads, load
   ↓
Use a tool, not intuition — ThreadSanitizer (-fsanitize=thread), or
  lockdep if it's a kernel race
   ↓
Shrink to the shared variable — which one is touched by two flows?
   ↓
For a hang (not wrong output): get every thread's stack
  (gdb's thread apply all bt) and look for a cycle — a deadlock
  (file 23) is diagnosed exactly this way, from the outside
```

---

## "The machine won't boot"

```
No bootable device at all?
   → check the magic number (bytes 510-511 = 0x55,0xAA, file 06)
   → check BIOS/UEFI mode matches the disk's partition scheme
     (MBR vs. GPT, file 07)

BIOS found the sector but nothing visible happens?
   → bug in your own code; disassemble and trace by hand (file 05)

Protected-mode switch causes an immediate reset (triple fault)?
   → missing cli, wrong CODE_SEG/DATA_SEG offset, near jump instead
     of far, forgot to reload segment registers (file 08)

Secure Boot refuses to boot a signed-looking binary?
   → check which link in the PK→KEK→db/dbx chain actually failed
     (file 34); mokutil manages additional trusted keys
```

---

## "My own kernel's interrupt/driver/scheduler isn't working"

```
Interrupt never fires at all?
   → PIC not remapped or still masked (file 14)

Interrupt fires once, never again?
   → ISR forgot to send EOI (file 14)

Scheduler switches to garbage, or crashes on resume?
   → stack discipline bug — pushed/popped in mismatched order, or
     switched stacks before every needed register was saved (file 20)

Port I/O writes silently do nothing?
   → wrong port address or wrong bit; re-check the actual datasheet,
     verify with a known-working read first (file 15)
```

---

## "The kernel panicked and I can't reproduce it live"

Live debugging (GDB over `qemu -s -S`, file `27`) assumes the system is
still running. For a crash that already happened:

```
Was kdump configured before the crash?
   → yes: the crash dump was captured automatically; use `crash` to
     inspect it exactly as if live (file 27)
   → no: nothing to inspect post-hoc — configure it going forward
```

---

## "A container behaves differently than I expect"

```
Visible to the host but isolated from other containers?
   → namespace question (file 29) — check `docker inspect` /
     `runc state` for actual namespace config

Killed under load with no clear application error?
   → cgroup memory limit — check `dmesg` for an OOM kill even when
     the container just reports "exited" (file 29)

Network unreachable from inside?
   → network namespace has no route to the host — check its own
     routing table, not the host's
```

---

## Cross-Reference

For *what each tool actually reveals*, see `source-code-map.md`. For
*what each term in these chains means*, see `glossary.md`. For *why
each mechanism works the way it does*, the file number next to each
step is the full lesson.
