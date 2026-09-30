# 31 — Security and Protection

## Layer map for this file

```
┌─────────────────────────────────────────────────┐
│ 1. BOOK: ring 0 exclusivity, and the null       │
│    descriptor as a defensive pattern (§4.2)     │
├─────────────────────────────────────────────────┤
│ 2. CORE CONCEPT: the exploit/defense arms race  │
│    — collecting six breadcrumbs from earlier    │
│    files into one narrative                     │
├─────────────────────────────────────────────────┤
│ 3. MODERN: KASLR, CFI, Meltdown/Spectre's       │
│    actual shape                                 │
└─────────────────────────────────────────────────┘
```

Book coverage: **Seed**. This file, like `21`, mainly collects: six
separate mentions across earlier files turn out to be one continuous
story.

## Prerequisites

Files `01`–`30`, especially `05`, `08`, `10`, `12`, `15`, `21`, `22`.

---

## The Breadcrumbs

- File `05`: `ret` unconditionally trusts the stack — the basis of ROP.
- File `08`: the null descriptor as "fail loudly on a mistake" — and
  SMEP/SMAP, named but not explained.
- File `10`: stack canaries, defending exactly the layout that file
  built.
- File `12`: PIE and ASLR, mentioned once.
- File `15`: the IOMMU, restricting what DMA-capable devices can touch.
- File `21`/`22`: Meltdown, and KPTI as its costly fix.

This file is where they connect.

---

## [BOOK] What's Actually Here

Ring 0 used exclusively, and one real defensive pattern: reserve the
first GDT entry as all zeros so a forgotten segment register fails
loudly (file `08`). That instinct — build in a way for mistakes to be
caught rather than silently exploited — is this entire file's theme,
generalized.

---

## [CORE CONCEPT] The Arms Race

![Security as an arms race: buffer overflow leads to shellcode injection, answered by the NX bit; NX leads to ROP reusing existing code, answered by stack canaries and ASLR; ASLR is defeated by information leaks, answered by control-flow integrity — each defense built on a file already covered](images/31-arms-race.svg)

Nothing here is a single fix — each defense closes one door, and each
answer opens a new one:

- **Buffer overflow → inject and run shellcode.** The oldest attack:
  overwrite the stack past its bounds, land attacker-supplied code
  where `ret` will jump.
- **NX bit, the defense.** A bit in every page table entry (file `22`'s
  PTE diagram left it out for space — it's real, and it's the reason
  this defense works) marks a page **non-executable**. Stack and heap
  pages get data-only permissions; injected shellcode can no longer be
  *run* even after it's written.
- **ROP, the counter.** Can't inject new code — so reuse code that's
  *already* there and already executable, chaining together short
  existing snippets purely by controlling what `ret` (file `05`) finds
  on the stack. The mechanism file `05` taught you, weaponized exactly
  as that file warned.
- **Stack canaries and ASLR/PIE, the defense.** A canary (file `10`)
  catches the corruption before `ret` ever executes. **ASLR** (Address
  Space Layout Randomization) randomizes where code and data load each
  run, so an attacker can't know in advance where the "existing code"
  ROP needs actually lives; **PIE** (file `12`) is what makes an
  executable *capable* of loading at a random address at all.
- **Information leaks, the counter.** A separate bug that leaks even
  one real address defeats ASLR for that entire run — once you know
  where anything is, you can compute where everything else is, since
  offsets between loaded regions don't change.
- **Control-Flow Integrity, the current defense.** Modern compilers can
  instrument every indirect call and jump to verify the target is a
  legitimate one, not an arbitrary gadget — narrowing what ROP-style
  chains can even attempt, regardless of what an attacker knows about
  addresses.

## [CORE CONCEPT] SMEP and SMAP, Finally Explained

File `08` named these without explaining them. Both are CPU features
enforcing exactly what file `21`'s "never trust a user pointer" says,
in hardware: **SMEP** (Supervisor Mode Execution Prevention) faults if
the kernel ever tries to *execute* code from a user-mode page — even by
mistake, even if a kernel bug tries to jump there. **SMAP** (Supervisor
Mode Access Prevention) does the same for ordinary reads and writes.
Both exist because kernel bugs that accidentally trust user memory
happen — the hardware backstops the discipline file `21` asked kernel
code to maintain by convention.

---

## [RELATED KNOWLEDGE] Privilege Escalation, Named as a Category

Every boundary this course has built — rings (files `08`, `21`), page
permissions (file `22`), namespaces and UID mapping (file `29`) — is a
place a bug can turn into a **privilege escalation**: code that should
be confined gaining capability it shouldn't have. A namespaced
container process finding a kernel bug to escape its user-namespace UID
mapping (file `29`) is the exact same category of failure as ring-3
code somehow executing a privileged instruction (file `21`) — different
boundary, same shape of bug.

---

## [MODERN SYSTEM] KASLR and KPTI

**KASLR** applies ASLR's idea to the kernel itself — randomizing where
the kernel loads in memory, so a bug that leaks a kernel address
doesn't hand an attacker a fixed, universal map. File `22` already
named **KPTI** (kernel page-table isolation) as Meltdown's fix; now the
connection is explicit: KPTI trades real performance (more `CR3`
switches, file `22`) specifically to keep kernel memory out of the
user-mode page tables almost entirely — closing the exact channel
Meltdown used.

## [MODERN SYSTEM] What Meltdown and Spectre Actually Are

Both exploit **speculative execution** (file `24`'s microarchitecture
territory): a CPU executes instructions ahead of a security check,
before knowing whether that check will pass, purely for speed. If the
check ultimately fails, the *architectural* results are discarded — but
the speculative execution still touched the cache, and cache access
timing is measurable. An attacker times how long specific memory
addresses take to access, and that timing difference — nothing else —
reveals data that was never supposed to be readable at all. Named here
at the conceptual level intentionally; no more detail than that is
appropriate or needed.

---

## Practical Experiment

1. `cat /proc/cpuinfo | grep -o 'smep\|smap\|nx'` — confirm your own CPU
   reports these features.
2. Run the same program twice: `cat /proc/self/maps` — compare the
   stack/library addresses between runs; ASLR should show them differing.
3. Compile with and without `-fstack-protector` and `-fPIE -pie`; compare
   the binaries' behavior on a deliberately corrupted buffer (file
   `10`'s canary experiment, revisited with the security framing).
4. `cat /sys/devices/system/cpu/vulnerabilities/*` on a real Linux
   machine — the kernel's own live report of which speculative-execution
   mitigations are active and which vulnerabilities (if any) remain
   unmitigated on your specific CPU.

## Debugging

A crash with a *suspicious-looking* but not obviously wrong stack trace
is worth treating as a security question, not just a bug: if `ret`'s
target doesn't correspond to any real call site in the source, that's
exactly the signature a ROP chain leaves behind, not ordinary
corruption. `dmesg` reporting a stack canary failure ("stack smashing
detected," file `10`) is the defense in this file's arms race actually
firing — treat it as a real finding, not noise to suppress.

## Source Code

None new — the ASLR comparison and the canary/PIE experiment above use
tools already built in files `10` and `12`.

---

## Common Misconceptions

- **"NX made buffer overflows harmless."** It made *one specific
  technique* (inject-and-run) much harder — ROP exists precisely
  because the underlying overflow bug is still there.
- **"ASLR alone is sufficient protection."** One information leak
  defeats it for that run — it raises the bar, it doesn't remove the
  need for the bug itself to be fixed.
- **"Meltdown and Spectre are software bugs."** They're
  microarchitectural — a side effect of a legitimate performance
  optimization (speculative execution) having an observable side channel.

## Interview Questions

**Basic:** What does the NX bit prevent?
**Intermediate:** Why does ROP exist as a technique, given NX already
exists?
**Advanced:** How does a single information leak defeat ASLR for an
entire process's remaining lifetime?
**Systems Design:** Why does KPTI cost real performance, and why was
that considered an acceptable trade-off?
**Debugging:** A crash's return address doesn't correspond to any real
call site in the source — what does that suggest?
**Explain what happens when...:** a program protected by a stack canary
suffers a buffer overflow — trace it from the overflow to "stack
smashing detected"?

## Active Recall

1. What defensive pattern did file `08`'s null descriptor already
   demonstrate?
2. Order these correctly in the arms race: ROP, ASLR, NX bit, shellcode
   injection.
3. What do SMEP and SMAP each prevent?
4. What's the shared mechanism behind Meltdown and Spectre?

## Answers

1. Failing loudly on a mistake (an unset segment register) rather than
   allowing silent misbehavior.
2. Shellcode injection → NX bit → ROP → ASLR (and stack canaries).
3. SMEP: the kernel executing code from a user-mode page. SMAP: the
   kernel reading or writing user-mode memory directly.
4. Speculative execution touching the cache before a security check
   resolves, leaving a timing side channel that reveals data that
   should never have been readable.

---

## What You Should Now Understand

- Six separate mentions across earlier files, now one continuous arms
  race with a clear order and clear reasons.
- SMEP and SMAP, finally explained rather than just named.
- KASLR and KPTI as this course's ASLR and Meltdown threads, resolved.
- The actual shape of Meltdown/Spectre, at the right level of detail.

---

**Next:** file `32-performance-and-observability.md`.
