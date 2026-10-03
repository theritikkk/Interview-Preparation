# The Compiler and `gdb`: Interactive Debugging

> **Phase 10 · Debugging & Tooling — entirely `[External]`.** None of the three source
> books covers debugging tools at all; this phase exists specifically to fill that gap.
> Depends on: `systems/01_compilation_toolchain.md` §3 (object-file symbol tables — `-g`
> is what makes them rich enough for a debugger), `functions/01_functions_and_stack_frames.md`
> §6 (stack frames — what `backtrace` walks).
> Feeds into: `02_sanitizers_and_valgrind.md`, `04_debugging_workflows_labs.md` (this
> phase).
> Sources: **entirely `[External]`**, GCC and GDB's own documentation. Every compiler
> warning, linker error, and tool output quoted in this file was actually produced by
> running the commands shown, in the environment this vault was written in (GCC 13.3.0 on
> x86-64 Linux) — not reconstructed from memory. `gdb` itself was not available in that
> environment; its command reference below is drawn from GDB's own stable, long-documented
> interface rather than from a live session, and is marked as such throughout.

---

## 1. `-g`: the flag that makes everything else in this file possible

`[External]` Compiling with `-g` (`gcc -g -O0 prog.c -o prog`) embeds **debug information**
in the object/executable file — a mapping from machine instructions back to source lines,
variable names, and type information — alongside the ordinary symbol table
(`systems/01_compilation_toolchain.md` §3). Without it, a debugger can still show you raw
addresses and registers, but not source lines or variable names. **`-O0`** (no
optimization) matters just as much: an optimizer routinely reorders, inlines, or eliminates
code entirely (`systems/01_compilation_toolchain.md` §2), which can make a debugger's
"current line" tracking misleading or make a variable "optimized out" — debug builds
conventionally use `-g -O0`; only reach for higher optimization once you're past the
debugging phase.

---

## 2. `gdb`, the GNU Debugger — a command reference

`[External, GDB's documented interface]` The core workflow: run a program under `gdb`'s
control, pause it at chosen points, and inspect its state.

| Command | Effect |
|---|---|
| `gdb ./prog` | Start `gdb` on `prog` (built with `-g`) |
| `run [args]` | Start executing, optionally with command-line arguments |
| `break LOCATION` | Set a breakpoint (`break main`, `break file.c:42`, `break some_func`) |
| `continue` (`c`) | Resume after a breakpoint |
| `next` (`n`) | Execute the current line, **stepping over** function calls |
| `step` (`s`) | Execute the current line, **stepping into** function calls |
| `print EXPR` (`p`) | Evaluate and print a C expression — variables, `*ptr`, `arr[i]`, even function calls |
| `backtrace` (`bt`) | Show the full call stack — every active frame, per `functions/01_functions_and_stack_frames.md` §6 |
| `frame N` | Switch inspection to stack frame `N` (to `print` a variable local to a caller) |
| `watch EXPR` | Break whenever `EXPR`'s value changes — invaluable for "who is corrupting this variable" bugs |
| `list` | Show source around the current line |
| `quit` | Exit `gdb` |

### 🧠 Mental model: `next` vs. `step`, precisely
`[External]` Both execute the current source line. `next` treats a function call on that
line as one atomic unit — useful when you trust the callee and just want to move past it.
`step` descends **into** the callee's own first line — useful when the bug might be
*inside* that call. Habitually using `step` everywhere means wading through library
internals you don't care about; habitually using `next` everywhere means you can walk right
past the actual bug. Choosing deliberately, line by line, is the actual skill.

### A representative session, narrated

`[External, illustrative — not a literal captured transcript, since gdb wasn't available in
this vault's build environment, but this is standard gdb usage as documented]`

```
$ gdb ./demo
(gdb) break square
Breakpoint 1 at 0x1155: file main.c, line 4.
(gdb) run
Starting program: /home/user/demo
Breakpoint 1, square (x=7) at util.c:4
4           call_count++; return helper(x);
(gdb) print x
$1 = 7
(gdb) backtrace
#0  square (x=7) at util.c:4
#1  main () { at main.c:9
(gdb) next
5       }
(gdb) print call_count
$2 = 1
(gdb) continue
Continuing.
square demo: 49
[Inferior 1 (process 12345) exited normally]
```

**What each piece demonstrates:** the breakpoint fires with the *real* argument value
already visible (`x=7`) — no `printf` needed; `backtrace` shows the exact two-frame call
stack (`main` called `square`) this vault has been diagramming conceptually since Phase 2;
`next` steps over `helper`'s call without descending into it.

---

## 3. Post-mortem debugging: core dumps

`[External]` A **core dump** is a file capturing a crashed process's memory at the moment
it died — letting you debug *after the fact* instead of needing to reproduce the crash live
under `gdb`. On Linux, `ulimit -c unlimited` (in the shell, before running the program)
enables core files; `gdb ./prog core` then opens both together, with `backtrace` immediately
showing exactly where the crash happened. `[Needs verification: exact core-file naming and
location is distribution-configurable — e.g. via `/proc/sys/kernel/core_pattern` — check
your own system if a crash doesn't produce a core file where expected.]`

### 🎯 Interview point
"Your program crashed in production and you can't reproduce it locally. What now?" — A
core dump (if one was captured) plus the original binary (with its debug symbols, or a
matching separate debug-info file) is often the *only* way to get a real backtrace after
the fact — this is precisely why production systems commonly enable core dumps even though
they're disabled by default on many distributions.

---

## 4. `addr2line`: a lighter-weight complement

`[External]` Given a binary and a raw address (from a crash log, a stack trace printed
without symbol resolution, or manual `objdump` inspection —
`03_static_and_binary_inspection.md`, this phase), `addr2line -e prog -f -C 0xADDR`
translates it back to a function name and source line, using the same debug information
`gdb` reads. Useful specifically when a full interactive debugging session isn't practical
— resolving one address from a log file, say — and `gdb` would be overkill.

---

## 5. ⚠️ Misconceptions to unlearn now

- **"Debugging works the same at any optimization level."** `-O2`/`-O3` can reorder,
  inline, or eliminate code and variables entirely, making source-line and variable
  correspondence unreliable — debug with `-O0` (§1).
- **"`next` and `step` are interchangeable, just different keys for 'go to the next
  line.'"** They differ specifically in whether a function call on that line is entered or
  skipped — the choice materially affects what you can observe (§2).
- **"A crash with no local reproduction is undebuggable."** A core dump plus the original
  binary is frequently enough for a full post-mortem backtrace (§3).

---

## 6. 🎯 Interview questions

**Easy**
- What does `-g` do? → Embeds debug information (source-line and variable mapping) in the
  compiled output (§1).
- What's the difference between `next` and `step` in `gdb`? → `next` steps over a function
  call on the current line; `step` steps into it (§2).

**Medium**
- Why is `-O0` conventionally paired with `-g` during debugging? → Optimization can reorder
  or eliminate code, breaking the correspondence between source lines/variables and what's
  actually running, which undermines what debug info is trying to provide (§1).
- What does a `watch` breakpoint let you find that an ordinary `break` doesn't? → Exactly
  which line of code modifies a given variable, by pausing execution the moment its value
  changes — useful when you know *what* is being corrupted but not *where* (§2).

**Hard**
- Explain why a core dump is sometimes the *only* practical way to debug a production
  crash. → The exact conditions that triggered the crash may not be reproducible locally
  (timing, specific input, system state); a core dump captures the actual crashed process's
  memory, letting `gdb` reconstruct a real backtrace after the fact without needing to
  reproduce anything live (§3).

**Systems-level**
- A `gdb` session shows a variable as `<optimized out>`. What's happening, and what's the
  fix? → The compiler's optimizer (likely `-O2`/`-O3`) determined the variable's value
  could be tracked entirely in a register or eliminated, so no stable memory location
  exists for the debugger to read; recompiling that file at `-O0` (at least temporarily,
  for debugging) restores full variable visibility (§1).

---

## 7. External references

- [GDB User Manual](https://www.gnu.org/software/gdb) `[External, verify the current
  documentation URL at point of use]` — the authoritative, complete command reference,
  including the many commands beyond this file's core set (conditional breakpoints,
  reverse debugging, remote debugging).
- [man7.org — `core(5)`](https://man7.org/linux/man-pages/) `[External]` — core-dump file
  format and the `core_pattern` configuration referenced in §3.
- [man7.org — `addr2line(1)`](https://man7.org/linux/man-pages/) `[External]`.

## 8. 🛠 Practice

1. Build the two-file `main.c`/`util.c` program from `systems/01_compilation_toolchain.md`
   §10 with `-g -O0`, and step through it under `gdb`: set a breakpoint on the helper
   function, inspect its argument, `backtrace` to confirm the call chain, and `watch` a
   global variable it modifies.
2. Deliberately compile the same program at `-O2` and compare what `gdb` can and can't show
   you about local variables versus the `-O0` build.
3. Trigger a segfault (`int *p = 0; *p = 1;`) with `ulimit -c unlimited` set first, then
   open the resulting core file in `gdb` and confirm `backtrace` immediately identifies the
   crash site — no live reproduction needed.
