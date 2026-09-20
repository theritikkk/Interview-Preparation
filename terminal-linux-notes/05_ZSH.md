# 05 — Zsh

> **Where this fits in the stack**
> ```
> User  →  Terminal Emulator  →  PTY/TTY  →  [ ZSH ]  →  Programs  →  Syscalls  →  Kernel  →  Hardware
>                                             ^^^^^^^
>                                             this file
> ```
> This file is deliberately a **diff against file 04**, not a ground-up retelling. Everything file 04 established about fork/exec, builtins vs external commands, subshells, and `exec` applies to Zsh identically — the underlying system-call layer doesn't care which shell is issuing the calls. What's worth a dedicated file is *where Zsh's language and defaults genuinely diverge* from Bash, because several of those differences are the exact kind of thing that silently breaks a script the moment you move it between the two.

---

## 1. What is Zsh, and why was it created?

Zsh ("Z shell") was written by Paul Falstad in 1990, originally as a student project at Princeton, with the explicit goal of combining the best interactive features of `ksh`, `bash`, and `tcsh` into one shell — not to replace Bash for portable scripting, but to be a substantially nicer shell to actually *use* at a prompt. It has since grown into a fully capable scripting language in its own right, but its DNA is interactivity-first.

> The name is commonly attributed to Zhong Shao — a fellow student/TA whose account login was `zsh`, which Falstad had access to when he started the project — with the added appeal that it sorts alphabetically after every other common shell name, making it, jokingly, "the last word in shells."

---

## 2. How to think about Zsh relative to Bash

Not a redesign — an accumulation. Zsh took Bash-like syntax as a base, folded in ksh features, and then kept adding its own extensive interactive machinery (completion, globbing, prompt theming) on top. The result is a shell whose *scripting* core looks and feels close to Bash, while its *interactive* experience diverges substantially — and a handful of scripting-relevant defaults diverge too, which is where real bugs live.

---

## 3. Bash vs Zsh — comparison table

| Area | Bash | Zsh |
|---|---|---|
| Unquoted variable word-splitting | Splits on `$IFS` by default | **Does not split by default** — Section 4 |
| Array indexing | 0-indexed | **1-indexed by default** — Section 5 |
| Completion | Readline-based, functional but simpler | `compinit`/`compsys` — far more configurable, context-aware |
| Globbing | Standard glob + `globstar` shopt for `**` | Extended glob (`setopt extended_glob`): exclusion patterns, file-type qualifiers, recursive `**` natively |
| Prompt system | `PS1` with a modest escape set | Rich `%`-escape system, `RPROMPT` (right-aligned prompt), the foundation most prompt themes (Powerlevel10k, etc.) are built on |
| Plugin ecosystem | Exists, less commonly used | Extensive and central to typical setups (oh-my-zsh, zinit, antigen) |
| Startup files | `.bashrc`, `.bash_profile`, `.profile` | `.zshrc`, `.zprofile`, `.zshenv`, `.zlogin`, `.zlogout` — different set entirely, full depth in file 18 |
| Interactive extras | Basic shared history | Real-time shared history across sessions (`share_history`), spelling correction, richer directory-stack shortcuts |
| POSIX conformance | Closer by default | Diverges more by default, but includes an explicit **compatibility mode** — Section 8 |

---

## 4. The single biggest practical gotcha: word splitting

```bash
var="a b c"
for x in $var; do echo "[$x]"; done
```

**In Bash:** unquoted `$var` is split on `$IFS`, so this prints three separate lines: `[a]`, `[b]`, `[c]`.

**In Zsh, by default:** unquoted `$var` is treated as **one single word**, so this prints exactly one line: `[a b c]`.

This is not a minor stylistic quirk — it's a behavioral default divergence that silently changes what a loop iterates over, and it's the single most common reason a script that works perfectly in Bash produces wrong (not even erroring, just *wrong*) results the moment someone runs it under Zsh.

**Getting Bash-like splitting in Zsh when you actually want it:**
```zsh
for x in ${=var}; do echo "[$x]"; done   # ${=...} forces splitting for this one expansion
# or, shell-wide:
setopt shwordsplit
```

> **Common mistake**
> "Zsh is POSIX/Bash-compatible enough that word-splitting differences don't really matter in practice." They matter constantly and silently — because the failure mode isn't a syntax error, it's a loop that runs once instead of three times, with no warning at all.

---

## 5. Arrays: 1-indexed, not 0-indexed

```zsh
arr=(a b c)
echo $arr[1]        # zsh: "a" — the FIRST element is index 1
```

```bash
arr=(a b c)
echo ${arr[0]}       # bash: "a" — the FIRST element is index 0
```

Zsh can be told to use 0-indexing for compatibility (`setopt ksh_arrays`), but out of the box, index arithmetic ported directly from a Bash script will be off by one in Zsh, and vice versa.

> **Try this**
> If you have both shells installed:
> ```
> bash -c 'arr=(a b c); echo ${arr[0]}'
> zsh  -c 'arr=(a b c); echo $arr[1]'
> ```
> Both print `a` — because each is using *that shell's own* first-element index, not because the arrays behave identically.

---

## 6. Zsh's line editor: ZLE

Just as Bash's interactive line editing (arrow-key history, `Ctrl+R` search, Tab completion) is powered by **GNU Readline** — previewed in file 02 and covered in full in file 23 — Zsh has its own, **independently implemented** line editor called **ZLE** (Zsh Line Editor). It is not Readline, does not share Readline's codebase, and has its own keybinding and "widget" system for customizing editing behavior.

Practically: if you're troubleshooting why a keybinding, a completion trigger, or a history-search behavior differs between a Bash session and a Zsh session, the answer starts here — you're comparing two genuinely separate pieces of software solving the same underlying problem (giving a canonical-mode-avoiding shell its own keystroke-level editing on top of the raw terminal, per file 23's mechanics), not two configurations of the same one.

---

## 7. Why did macOS default to Zsh?

Apple switched macOS's default interactive shell from Bash to Zsh starting with **Catalina (10.15, 2019)**. The officially stated reason was licensing: modern Bash (4.x and later) is licensed under **GPLv3**, which Apple avoids shipping as part of the OS for its own legal/licensing-strategy reasons — which is exactly why Apple's bundled `bash` had been frozen at the old GPLv2-licensed **3.2** branch for over a decade (recap: files 03-04). Zsh is **MIT-licensed**, with no such obstacle, letting Apple ship a current, actively maintained version as the default going forward. Zsh was also already a mature, widely adopted choice among power users, making it a low-risk pick rather than an unusual one.

---

## 8. What does `/bin/zsh` actually mean?

Exactly what `/bin/bash` means for Bash: the path to the Zsh interpreter **binary** on disk. Nothing magical about the path itself. Two things worth knowing:

- `/etc/shells` is the system's list of "approved" login shells; `chsh -s /bin/zsh` changes *your* login shell by updating your entry to point here (full login-shell mechanics: file 18).
- Zsh includes a genuine **compatibility/emulation mode** — `emulate sh` (or being invoked *as* `sh`, e.g. via a symlink named `sh` pointing at the zsh binary) changes its option set to behave much closer to POSIX `sh`, including restoring Bash/POSIX-style word splitting. This is a real, load-bearing feature, not a curiosity — it's how Zsh can serve as a POSIX-sh-compatible interpreter when needed despite its very different interactive defaults.

---

## 9. Common misconceptions

| Misconception | Reality |
|---|---|
| "Zsh is just Bash with nicer prompts and plugins." | The interactive layer is where the *visible* differences concentrate, but word-splitting and array-indexing defaults are genuine scripting-behavior divergences, not cosmetic. |
| "A script that runs correctly in Bash will run identically in Zsh." | Not guaranteed by default — word splitting and array indexing are the two most common silent breakages; `emulate sh` narrows the gap deliberately when needed. |
| "Zsh's tab completion is just Bash's completion with more themes." | It's a substantially different, more context-aware system (`compinit`/`compsys`), independent of Readline's completion machinery entirely. |
| "ZLE and Readline are the same thing under different names." | Independently implemented line editors solving the same interactive-editing problem — not two names for one codebase. |

---

## 10. Try this

> **Try this**
> ```
> echo $SHELL
> ps -p $$ -o comm=
> ```
> `$SHELL` reports your *login* shell (set at login, from `/etc/passwd` — recap from file 03's macOS note); `ps` on your own PID reports what's *actually currently running*. These can disagree — running `zsh` from inside a `bash` login session is completely ordinary and shows up as a mismatch between the two.

---

## 11. Summary

- Zsh accumulates ksh and Bash features plus its own extensive interactive additions — it's not a from-scratch redesign.
- **Word splitting is off by default in Zsh**, on by default in Bash — the single most common source of silently-wrong ported scripts.
- **Zsh arrays are 1-indexed by default**; Bash arrays are 0-indexed.
- Zsh's interactive line editing runs on **ZLE**, an independent implementation — not Readline.
- macOS moved to Zsh in Catalina specifically over Bash's GPLv3 licensing; Zsh's MIT license removed that obstacle.
- `emulate sh` / invocation as `sh` is Zsh's real, built-in mechanism for narrowing these gaps when POSIX-style behavior is actually needed.

---

## What's next

`06_SHELL_PARSING.md` goes back to shell-agnostic ground truth: the actual order in which *any* POSIX-family shell tokenizes, quotes, expands, and globs a command line — the mechanics files 04 and 05 both assumed you already understood well enough to follow their examples.
