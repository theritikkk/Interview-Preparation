# 03 — Shells Overview

> **Where this fits in the stack**
> ```
> User  →  Terminal Emulator  →  PTY/TTY  →  [ SHELL ]  →  Programs  →  Syscalls  →  Kernel  →  Hardware
>                                             ^^^^^^^^^
>                                             this file
> ```
> Files 01-02 got you to the point where a process is sitting on the PTY slave, reading whatever you type. This file is about *that process*: what a shell fundamentally is, why it's separate software rather than a kernel feature, and the landscape of shells this handbook keeps referring back to.

---

## 1. What is a shell?

**L1 — intuition:** a shell is the program that turns what you type into things actually happening.

**L2 — systems detail:** a shell is an **ordinary user-space program** — no different in kind from `ls` or `vim` as far as the kernel is concerned — whose specific job is to read a line of text, interpret it according to a defined grammar (Section 5), and act on it. Most of the time, "acting on it" means launching *another* program to do the real work (`fork`+`exec`, file 08); sometimes the shell does the work itself, without creating any new process at all (builtins, file 07).

> **Common mistake**
> "The shell is part of the kernel" or "the shell and the kernel are basically the same layer." No — the shell is exactly as much a userspace program as `ls` is. It has no special privileges the kernel grants it for being "the shell." Everything it does — launching programs, changing your directory's *appearance* to you, handling `Ctrl+C` — it does using the same public system-call interface any program can use (file 09). The only thing distinguishing a shell from any other program is what it was *written to do* with that interface.

---

## 2. Why does the shell exist?

Before there's a shell, you have a kernel that can run programs (via `execve`) and a human who wants to run them. Something has to bridge that gap: take typed text, figure out *which* program that refers to, hand it any arguments, connect its input/output somewhere sensible, and report back what happened. That bridging job — and the accompanying need to *chain* programs together, *repeat* commands programmatically, and *automate* sequences of them — is what a shell is for.

Put differently: the shell is the **translation layer** between a human's typed intent ("run `ls`, filter the output through `grep`, save the result to a file") and the kernel primitives that actually make that happen (`fork`, `execve`, `pipe`, `dup2`, `open`). Every one of those kernel calls exists whether or not a shell is involved — the shell's entire reason for existing is to spare you from calling them yourself, by hand, every single time.

---

## 3. What does a shell actually do?

A working list, each item pointing at the file that goes deep on it:

- **Present a prompt** and read a line of input.
- **Parse** that line — tokenize it, handle quoting, expand variables/globs/command-substitutions (file 06).
- **Decide**: is this a builtin (runs inside the shell's own process) or an external command (needs `fork`+`exec`)? (file 07, folding in your PATH-lookup and builtin-vs-external content.)
- **Set up the execution environment** for anything it launches: working directory, environment variables, open file descriptors, redirections (files 10-13, 17).
- **Wait** for foreground children, report their exit status (files 08, 43-gotchas-file).
- **Maintain its own persistent state** across commands: current directory, shell variables, functions, aliases, a job table (files 15-18).
- **Provide a programming language** — conditionals, loops, functions, arithmetic — for scripting, not just interactive use (file 04, 42).
- **React to signals** appropriately and manage job control (files 14-16).
- **Read startup files** at the right moments to configure all of the above before you ever type a command (file 18).

Notice how much of this list is *state the shell keeps around between commands* (current directory, variables, job list). That persistence, sitting in **one long-lived process**, is exactly why `cd` can't be an ordinary external program — covered precisely in file 07.

---

## 4. The shell landscape

| Shell | Full name / origin | Character |
|---|---|---|
| **sh** | Historically "the Bourne shell." Today, "`sh`" almost never refers to the original binary — it refers to *whatever POSIX-compliant shell a given system points `/bin/sh` at.* | The name of a **standard interface**, not one specific program, on modern systems. |
| **dash** | Debian Almquist Shell | Small, minimal, deliberately POSIX-focused, and fast to start. On Debian/Ubuntu, `/bin/sh` is a symlink to `dash` specifically because system scripts (boot scripts, package-manager hooks) run constantly and don't need Bash's interactive-feature overhead. |
| **bash** | Bourne-Again SHell (GNU) | The most common interactive/scripting shell on Linux. A large superset of POSIX sh, with extensive extensions (Section 6). |
| **zsh** | Z shell | A further superset, layering in extensive globbing, a powerful completion framework, and prompt theming on top of Bash/ksh-like foundations. Default shell on macOS since Catalina (2019). |
| **ksh** | Korn shell | Historically influential — many features you'd call "Bash extensions" actually originated in ksh. Still common in enterprise Unix environments. |
| **fish** | Friendly Interactive SHell | Deliberately **not** POSIX-compatible. Prioritizes interactive usability (autosuggestions, syntax highlighting out of the box) with a genuinely different scripting grammar from the sh family. |

> **Try this**
> ```
> ls -l /bin/sh
> ```
> On Debian/Ubuntu, this typically resolves to `dash`. On macOS, it's a `bash` binary running in a restricted, more POSIX-compliant mode. This single symlink target has real consequences — see Section 7.

---

## 5. Three things people conflate: language, interpreter, execution environment

- **Shell language** — a specification: the grammar for tokens, quoting, expansions, and control structures (`if`/`while`/`for`/`case`, pipelines, redirections). This is a *set of rules*, not software.
- **Shell interpreter** — the actual program (`/bin/bash`, `/bin/dash`, `/bin/zsh`) that reads text conforming to some shell language and executes it. This is *software implementing* a language.
- **Command execution environment** — the ambient state a launched program actually runs inside: its working directory, its environment variables, its inherited file descriptors, its `umask`, its resource limits. The shell **constructs** this environment for every command it runs (files 10, 17, 21).

Why the distinction earns its keep: "Bash" is really two things people mean interchangeably — a *language* (Bash's specific grammar, including all its extensions) and an *interpreter* (the one program that reads and executes that grammar; there isn't a competing "second implementation of the Bash language" the way there are multiple POSIX-sh-compliant shells). POSIX sh, by contrast, is a language with *several* interpreters (`dash`, `bash --posix`, `ksh`, and others) that all implement it — which is precisely why "is this script POSIX sh or is it Bash" is a meaningful, load-bearing question, covered next.

---

## 6. POSIX

**What POSIX is:** IEEE Std 1003.1, "Portable Operating System Interface" — a formal standard whose "Shell and Utilities" volume defines a specific shell command language and a specific set of standard utility programs, precisely so that scripts and programs written against it behave identically across compliant systems.

**What POSIX standardizes for shells:**
- Core grammar: variables, quoting rules, `if`/`while`/`until`/`for`/`case`, pipelines, the basic redirection operators (`>`, `>>`, `<`, `2>`, etc.)
- Basic parameter expansion (`${var}`, `${var:-default}`, and similar)
- A defined set of **special built-in utilities** (`cd`, `exit`, `export`, `.`, `set`, and others) that must be implemented as shell builtins, not external programs
- Exit-status conventions (0 = success, non-zero = failure)

**What POSIX does *not* standardize** — every one of these is a Bash/Zsh/ksh **extension**, absent from strict POSIX sh:

| Extension | Present in |
|---|---|
| Arrays (`arr=(a b c)`) | Bash, Zsh, ksh — not POSIX sh |
| `[[ ... ]]` extended test | Bash, Zsh, ksh — not POSIX sh (which only has `[ ... ]` / `test`) |
| Process substitution `<(...)` / `>(...)` | Bash, Zsh, ksh — not POSIX sh |
| Brace expansion `{1..5}`, `{a,b,c}` | Bash, Zsh — not POSIX sh |
| `local` inside functions | Extremely widely supported (including dash), but technically not in the POSIX base spec |
| Arithmetic C-style `for (( ; ; ))` | Bash, Zsh, ksh — not POSIX sh |
| Here-strings `<<<` | Bash, Zsh, ksh — not POSIX sh |
| `**` globstar (recursive glob) | Bash (with `shopt -s globstar`), Zsh — not POSIX sh |
| Associative arrays | Bash 4+, Zsh — not POSIX sh, not older Bash |

> **Interview answer**
> "POSIX defines a portable baseline shell language and a set of required builtins so scripts behave the same across compliant shells. It deliberately does *not* include most of what makes Bash or Zsh pleasant to use interactively — arrays, `[[ ]]`, process substitution — those are extensions specific to those shells, absent from strict POSIX sh implementations like `dash`."

**Why this is not a trivia question — it's a real, common failure mode:** a script starting with `#!/bin/sh` that uses a Bash array or `[[ ]]` will run correctly on a system where `/bin/sh` happens to be `bash`, and **fail or misbehave** on a system where `/bin/sh` is `dash` — which is exactly Debian and Ubuntu's default. "Works when I run `bash script.sh` directly, breaks when I run `./script.sh`" is, more often than any other single cause, precisely this: the shebang line lied about what language the script is actually written in. This is a recurring scenario in file 37 and a gotcha in file 42 — you're seeing the root cause here first.

---

## 7. Common misconceptions

| Misconception | Reality |
|---|---|
| "The shell is part of the kernel." | Ordinary userspace program, using the same public syscall interface any program uses. |
| "sh, bash, dash, and zsh are basically all the same thing with different names." | `sh` names an *interface* multiple different programs implement; `bash`, `dash`, and `zsh` are genuinely different codebases with different feature sets, and `dash` in particular deliberately omits most Bash extensions. |
| "`#!/bin/sh` means the script will run in Bash." | It means the script will run in *whatever `/bin/sh` is symlinked to on this machine* — `dash` on Debian/Ubuntu, a restricted-mode `bash` on macOS. Not the same thing, and not guaranteed to behave like ordinary Bash. |
| "POSIX defines everything a modern shell script needs." | POSIX defines a portable *baseline*. Arrays, `[[ ]]`, process substitution, and a long list of other conveniences are extensions on top of it, not part of it. |

---

## 8. macOS note

macOS ships `zsh` as the default interactive shell since Catalina (2019) — Apple's own stated reason at the time was Bash's GPLv3 licensing, which is also why the `bash` Apple bundles is frozen at the old GPLv2-licensed 3.2 branch, missing over a decade of upstream Bash features if you go looking for them. macOS's `/bin/sh` is a `bash` binary running in a restricted, more POSIX-compliant mode (not `dash`) — the opposite implementation choice from Debian/Ubuntu, but aimed at the same goal: giving scripts a smaller, more portable subset than full interactive Bash.

---

## 9. Summary

- A shell is an ordinary userspace program, not a kernel feature — its entire job is translating typed human intent into `fork`/`exec`/`pipe`/`dup2` calls on your behalf.
- "sh" names a standard *interface*; `dash`, `bash --posix`, and others *implement* it, while `bash`, `zsh`, `ksh`, and `fish` are distinct programs with their own (larger, non-portable) feature sets.
- Language (the grammar), interpreter (the program executing it), and execution environment (the state handed to each launched command) are three separable concepts worth keeping apart.
- POSIX standardizes a portable baseline; arrays, `[[ ]]`, process substitution, brace expansion, and associative arrays are all extensions outside it — and assuming they're portable is a real, common source of "works here, breaks there" bugs.

---

## What's next

`04_BASH.md` takes Bash specifically — the shell you'll spend the most time in — and goes deep: not a scripting tutorial, but Bash's variables, expansions, quoting, and control structures always tied back to what system-level behavior each one triggers.
