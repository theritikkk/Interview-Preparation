# 06 — Shell Parsing

> **Where this fits in the stack**
> ```
> User  →  Terminal Emulator  →  PTY/TTY  →  [ SHELL PARSER ]  →  fork/exec  →  ...
>                                             ^^^^^^^^^^^^^^^^
>                                             this file
> ```
> Files 04-05 used quoting, `$(...)`, and globbing in examples without ever pinning down the *exact order* the shell processes them in. That order is not incidental detail — it's the deterministic pipeline that makes quoting behavior predictable rather than mysterious. This file is that pipeline, applied line by line to eight concrete commands.

---

## 1. Why the order matters

The same character can mean three different things depending on *when* in this pipeline it's examined. A `$` inside single quotes is inert; the identical `$` inside double quotes triggers expansion; that expansion's *result*, if unquoted, can then be split into multiple words and *then* treated as a glob pattern. None of this is magic or inconsistency — it's one fixed pipeline, applied mechanically, every single time. Once you have the order memorized, every quoting "surprise" becomes predictable in advance.

---

## 2. The shell's actual processing order

```
PHASE 1 — Tokenize the raw input line into words
          (quoted whitespace does NOT split a word; unquoted whitespace does)

PHASE 2 — For each word, apply expansions IN THIS EXACT ORDER:
   2a. Brace expansion            {a,b,c}  {1..5}
   2b. Tilde expansion            ~
   2c. Parameter / command /       $var  ${var}  $(cmd)  `cmd`  $(( expr ))
       arithmetic expansion        (evaluated left to right as encountered)
   2d. Word splitting              splits the UNQUOTED RESULT of step 2c on $IFS
   2e. Pathname expansion (glob)   matches the UNQUOTED RESULT of step 2d against
                                    real filenames on disk

PHASE 3 — Quote removal
          (strip any quote characters that survived this far, having done their job)
```

> **Remember**
> Word splitting (2d) and globbing (2e) act on the **results of expansion**, not on your original typed text. Your original quoting already decided the word boundaries back in Phase 1 — Phase 2d/2e is a *separate, later* pass that only touches what expansion just produced, and only if that particular expansion wasn't quoted.

---

## 3. Traced examples

### `echo hello`

No `$`, no quotes, no glob characters anywhere. Phase 1 produces two words, `echo` and `hello`. Every sub-step of Phase 2 finds nothing to do. Phase 3 has no quotes to remove.

**Result:** `argv = ["echo", "hello"]` — two arguments.

### `echo "hello world"` vs `echo hello world`

```bash
echo "hello world"   # ONE argument containing a space
echo hello world      # TWO separate arguments
```

Phase 1 is where this is decided, before any expansion runs at all: the shell tracks quote state while tokenizing, so the space *inside* the double quotes never counts as a word separator. `"hello world"` is one token from the very first pass. Phase 3 then strips the surrounding `"` characters, leaving the argument `hello world` — one string, containing a space, as a single `argv` element.

### `echo 'hello world'`

Same tokenization outcome as the double-quoted version (one word, space preserved). The difference between `'` and `"` doesn't show up in *this* example — there's nothing inside to expand either way — but it's the entire subject of the next pair.

### `echo "$HOME"` vs `echo '$HOME'`

```bash
echo "$HOME"    # prints your actual home directory, e.g. /home/alice
echo '$HOME'    # prints the literal four characters: $HOME
```

**Double quotes** permit Phase 2c (parameter expansion) to run *inside* them — `$HOME` is replaced with its value — but they suppress Phase 2d (word splitting) on that result, so even if `$HOME` contained a space, it would still arrive as one argument.

**Single quotes** suppress *everything* in Phase 2, with no exceptions — `$` has no special meaning at all inside `'...'`. Phase 3 strips the quote marks and what's left is exactly the literal text that was between them: the four characters `$HOME`, unexpanded.

> **Interview answer**
> "Double quotes allow variable, command, and arithmetic expansion but suppress the word-splitting and globbing that would otherwise apply to the result. Single quotes suppress every kind of expansion — nothing inside single quotes is special, not even a backslash."

### `echo $(pwd)`

`pwd` runs in a forked subshell (file 04's command-substitution mechanics), its output is captured with trailing newlines stripped, and that captured text replaces `$(pwd)` — this is Phase 2c. Because the expansion is **unquoted**, Phase 2d now applies: the captured path is split on `$IFS`. If your current directory's path contains no spaces, this is invisible. If it contains a space (`/home/alice/My Documents`), the single path silently becomes **two separate arguments** to `echo` — and if any resulting piece happens to contain a glob character, Phase 2e would go further and expand it against real filenames.

> **Common mistake**
> "Command substitution just inserts the text back in — nothing else happens to it." Only true if you quote it. `echo $(pwd)` is exposed to word splitting and globbing on its result; `echo "$(pwd)"` is not. **Quoting command substitutions is the default you should reach for**, not an occasional precaution.

### `` echo `pwd` `` (backticks)

Identical semantics to `$(pwd)` — same fork, same capture, same exposure to Phase 2d/2e when unquoted here. The practical difference is nesting: nesting backticks requires escaping the inner ones with backslashes (`` `echo \`pwd\`` ``), while nesting `$(...)` is simply `$(echo $(pwd))` with no escaping needed at all. This is the entire reason modern scripts essentially never use backticks anymore — both forms are POSIX, but `$(...)` composes cleanly and backticks don't.

### `echo *.txt`

No `$`, so Phase 2a-2d have nothing to do with this word specifically. Phase 2e (globbing) is where everything happens: `*.txt` is matched against real filenames in the current directory. If matches exist (`a.txt`, `b.txt`), the single token `*.txt` is **replaced** by the (typically alphabetically sorted) list of matches, each becoming its own argument — `echo` never sees the literal string `*.txt` at all in that case.

> **Common mistake**
> "If a glob pattern matches nothing, it just expands to nothing (an empty argument list)." **Not the default.** If `*.txt` matches no files, Bash's default behavior is to leave the pattern **completely unchanged** — `echo *.txt` in a directory with no `.txt` files prints the literal string `*.txt`. Getting "expands to nothing on no match" requires explicitly opting in with `shopt -s nullglob`; getting "error out on no match" requires `shopt -s failglob`. Assuming either of these without setting them is a genuine, recurring scripting bug.

---

## 4. Escaping

```bash
echo \$HOME          # literal $HOME — backslash suppresses the $ that follows it
echo hello\ world     # ONE argument "hello world" — backslash suppresses the space
echo "say \"hi\""     # backslash lets a literal " survive inside a double-quoted string
```

**Inside single quotes, backslash is not special at all** — no character is, which is exactly what "single quotes suppress everything" means taken to its logical end. This creates a genuinely famous small puzzle: how do you put a literal single quote *inside* a single-quoted string, given that nothing (including a backslash) can escape the closing quote? You can't, directly — the standard idiom is to end the quoted section, insert an escaped or differently-quoted single quote, and resume:

```bash
echo 'it'\''s'         # end quote, escaped single quote, resume quote → it's
echo 'it'"'"'s'         # end quote, double-quoted single quote, resume quote → it's
```

**Bonus — ANSI-C quoting**, `$'...'`, processes backslash escape sequences (`\n`, `\t`, `\\`, and others) the way many programming languages do, which plain single or double quotes do not:

```bash
echo $'line one\nline two'    # an actual embedded newline, not the two literal characters \n
```

---

## 5. Brace and tilde expansion, concretely

The ordered list in Section 2 named these before command substitution ever runs — here's what that looks like:

```bash
echo {1..3}            # 1 2 3 — brace expansion, pure text generation, no filesystem involved
echo {a,b,c}.txt         # a.txt b.txt c.txt
echo ~                  # your home directory — tilde expansion
echo ~/projects          # /home/alice/projects
```

Brace expansion happens purely as text generation — it doesn't check whether any resulting filename exists, unlike globbing. That's a meaningful distinction: `echo {a,b,c}.txt` prints all three names *whether or not those files exist*; `echo *.txt` only ever produces names that genuinely exist on disk (or, per Section 3's globbing note, falls back to the literal pattern if nothing matches).

---

## 6. Common misconceptions

| Misconception | Reality |
|---|---|
| "Quotes always do the same thing." | Single and double quotes suppress fundamentally different amounts of Phase 2 — double quotes still allow `$` expansion, single quotes allow nothing. |
| "Word splitting happens on what I typed." | It happens on the **result of expansion**, in Phase 2d — your own typed quoting already fixed word boundaries back in Phase 1, before any expansion ran. |
| "An unmatched glob just disappears." | Bash's default is to leave it as the literal, unexpanded pattern text — `nullglob`/`failglob` are opt-in changes to that behavior. |
| "Backticks and `$(...)` are just two spellings of the same thing, pick whichever." | Functionally equivalent for simple cases, but backticks nest badly (manual backslash-escaping required) while `$(...)` nests cleanly — which is why virtually no modern script uses backticks. |

---

## 7. Try this

> **Try this**
> ```bash
> mkdir "test dir"
> cd "test dir"
> touch a.txt
> cd ..
> printf '%s\n' $(echo "test dir")
> ```
> Watch this print **two lines** instead of the one directory name you'd expect — the unquoted command substitution's result got word-split on the space inside `test dir`. Quote it (`printf '%s\n' "$(echo "test dir")"`) and it prints correctly as one line.

> **Try this**
> ```bash
> cd /tmp && mkdir empty_test && cd empty_test
> echo *.txt
> ```
> With no `.txt` files present, this prints the literal string `*.txt` — not nothing — demonstrating the default globbing behavior from Section 3 directly.

---

## 8. Summary

- The shell's word-processing pipeline is fixed and ordered: tokenize → brace expansion → tilde expansion → parameter/command/arithmetic expansion → word splitting → pathname expansion (globbing) → quote removal.
- Double quotes allow `$` expansion but block word splitting and globbing on the result; single quotes block everything, with no exceptions, including backslash.
- Word splitting and globbing act on **expansion results**, never on your original quoted text.
- Unquoted `$(...)` is exposed to both word splitting and globbing on its captured output — quoting it (`"$(...)"`) is the default you want, not a special case.
- Bash's default on a non-matching glob is to leave the literal pattern text unchanged, not to produce an empty result.

---

## What's next

`07_COMMAND_EXECUTION.md` is the synthesis chapter: it takes a complete command line, runs it through everything files 04-06 established, and continues past parsing into `PATH` lookup, builtin-vs-external resolution, `fork`, `execve`, and back — the full 20-plus-step trace your original brief asked for, for `ls -lah /tmp`.
