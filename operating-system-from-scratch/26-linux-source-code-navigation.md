# 26 — Linux Source Code Navigation

## Layer map for this file

```
┌──────────────────────────────────────────────┐
│ 1. BOOK: none — this file is a skill, not a   │
│    concept, and the book predates the tools    │
├──────────────────────────────────────────────┤
│ 2. CORE CONCEPT: why grep beats reading top to │
│    bottom, and how to actually trace a call     │
├──────────────────────────────────────────────┤
│ 3. MODERN: this whole file is Layer 3 — the    │
│    tools that make file 25's map walkable       │
└──────────────────────────────────────────────┘
```

Book coverage: **None.** File `25` gave you a map. This file is how
to actually walk it — a skill, not a concept, so it earns its place
differently from most files in this course.

## Prerequisites

Files `01`–`25`, especially `25`'s Rosetta table.

---

## [CORE CONCEPT] You Cannot Read This Top to Bottom

The Linux kernel is on the order of 30 million lines. Nobody reads it
linearly, including its own maintainers. The actual skill is **find
the ten lines that matter, fast** — which means tools, not stamina.

## [CORE CONCEPT] The Baseline: grep

`grep -rn "pattern" fs/` (or `ripgrep`'s `rg`, faster and
`.gitignore`-aware) always works, needs no setup, and is where every
kernel developer starts. Two searches solve most "where is this?"
questions:

- **Find a definition:** `grep -rn "^.*ext4_file_read_iter" fs/ext4/`
- **Find every caller:** `grep -rn "ext4_file_read_iter" --include=*.c`

## [CORE CONCEPT] Jump-to-Definition, at Kernel Scale

`ctags`/`cscope` (`make tags`, `make cscope` from the kernel source
root) index the whole tree so an editor can jump straight to a
definition or list every call site — the traditional method, still
standard. Modern IDEs backed by `clangd` (via `compile_commands.json`,
generated with tools like `bear`) do the same thing with full type
awareness.

**elixir.bootlin.com** deserves special mention: a free, zero-setup,
web-based cross-referenced browser for the actual kernel source, for
every release. Click any identifier; every definition and every call
site is a link. For following one function without building anything
locally, it's usually the fastest option available.

## [CORE CONCEPT] Tracing One Call, End to End

This is the actual exercise worth doing once, carefully, because the
method generalizes to anything:

![Tracing read() through the kernel: application calls read(), glibc wraps it, the syscall instruction enters through syscall_64.tbl, ksys_read and vfs_read handle the generic path, a function pointer in file_operations dispatches to the specific filesystem's read_iter, ext4 for example, then down through the block layer and driver](images/26-tracing-read-syscall.svg)

The one place this gets genuinely non-obvious is the jump from
`vfs_read()` to a *specific* filesystem's function — it isn't a direct
call at all. `fs/read_write.c` calls through `file->f_op->read_iter`,
a **function pointer** set when the file was opened, to whatever
`struct file_operations` the underlying filesystem registered (file
`18`'s VFS abstraction, in the actual source). Grepping for
`ext4_file_read_iter` by name finds its definition immediately; finding
*why* it gets called at all means first finding the `file_operations`
struct that assigns it — a different, equally standard search:
`grep -rn "read_iter" fs/ext4/*.h`.

## [CORE CONCEPT] Kconfig and the Build System

`Kconfig` files (one per directory, roughly) declare the feature flags
`make menuconfig` presents, along with their dependencies —
`CONFIG_EXT4_FS`, `CONFIG_SMP` (file `24`), and thousands more. A
`Makefile` in the same directory says which `.c` files get compiled
*only if* their matching config option is set:
`obj-$(CONFIG_EXT4_FS) += ext4.o` — the wildcard-and-pattern-rule idea
from file `13`'s Makefile, grown to select entire subsystems in or out
of a build.

## [CORE CONCEPT] `git log` and `git blame`: Finding Out Why

Source alone answers *what*; the commit that introduced a line often
answers *why* — a bug it fixed, a hardware quirk it works around, a
discussion that shaped it. `git blame path/to/file.c` names the commit
behind any given line; `git log --oneline -- path/to/file.c` gives that
file's whole history. For genuinely strange-looking code, this is
frequently the fastest way to stop being confused.

## [CORE CONCEPT] MAINTAINERS

The `MAINTAINERS` file maps subsystems to the people responsible for
them and the mailing list where changes are discussed — useful even
just as a second index into "which part of the tree owns this,"
independent of the directory structure itself.

---

## Practical Experiment

1. Clone the kernel (`git clone --depth 1
   https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git`)
   or use elixir.bootlin.com if you'd rather not download ~4 GB.
2. Reproduce the `read()` trace above yourself, one hop at a time, using
   `grep`/`rg` or elixir's cross-references — confirm each function
   name and file path.
3. Pick a second syscall from file `25`'s Rosetta table (`write`, or
   `openat`) and trace it the same way, unaided.
4. Find one `Kconfig` entry for something you recognize (`CONFIG_EXT4_FS`
   or `CONFIG_SMP`) and read its help text and dependencies.
5. `git log --oneline -- fs/ext4/file.c | wc -l` — get a feel for how
   much churn one file has actually had.
6. Search `MAINTAINERS` for a subsystem from file `25` (`ext4`, say)
   and find who's listed as responsible for it.

## Debugging

Reading kernel source *as* a debugging step follows the same layer
question file `25` ended on: `dmesg`/`strace` name a subsystem or
function; grep for that name to find the actual code path; `git blame`
the suspicious line if it's genuinely confusing; check `MAINTAINERS` if
you need to know who to ask. The skill in this file is what turns "the
kernel did something wrong" into "line 214 of this specific file did
something wrong."

## Source Code

None new — the `read()` trace and your second, self-directed trace are
this file's deliverable.

---

## Common Misconceptions

- **"You need to understand the whole kernel to read any of it."**
  You need to find the ten relevant lines — the tools in this file
  exist precisely so the other 30 million never have to be read.
- **"A function call in kernel code always goes where it looks like it
  goes."** Not when a function pointer (`file_operations`, and similar
  structs throughout the kernel) is involved — the actual target is
  decided at runtime.
- **"Comments explain why code is the way it is."** Often the commit
  message does and the code doesn't — `git blame` is frequently more
  informative than the surrounding comments.

## Interview Questions

**Basic:** Name two tools for finding where a kernel function is defined.
**Intermediate:** Why doesn't grepping for a filesystem-specific
function's name always explain *when* it gets called?
**Advanced:** What's the role of a `Kconfig` file, and how does it
connect to what actually gets compiled?
**Systems Design:** Why does Linux use function pointers
(`file_operations`) instead of a big switch statement over filesystem
type?
**Debugging:** You find a suspicious line of kernel code with no
explanatory comment — what's your next command?
**Explain what happens when...:** you search "read_iter" in the ext4
directory — what are you actually looking for, and why two different
kinds of match matter?

## Active Recall

1. Why is linear reading not a viable strategy for the kernel source?
2. What's the fastest, zero-setup way to browse the actual kernel
   source with full cross-references?
3. What kind of construct makes "who gets called" a runtime decision
   rather than a fixed call?
4. What does `git blame` answer that the source code alone doesn't?

## Answers

1. The tree is tens of millions of lines — no one, including
   maintainers, reads it start to finish; targeted search is the only
   workable approach.
2. elixir.bootlin.com.
3. A function pointer, such as a `file_operations` struct, filled in
   with a filesystem-specific implementation at runtime.
4. Why a line exists — the commit, and often its message, that
   introduced or changed it.

---

## What You Should Now Understand

- Why kernel-scale source demands tools (grep, ctags/cscope, elixir),
  not stamina.
- How to trace a syscall end to end, including the function-pointer
  indirection that makes "which filesystem" a runtime decision.
- Kconfig/Makefile as the mechanism deciding what's even compiled in.
- `git blame`/`log` as often the fastest route to understanding *why*
  code looks the way it does.

---

**Next:** file `27-kernel-debugging.md`.
