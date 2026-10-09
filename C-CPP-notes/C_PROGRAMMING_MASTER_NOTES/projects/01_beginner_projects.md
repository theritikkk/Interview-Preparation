# Beginner Projects

> **Phase 12 · Projects & Interview Preparation.** These five projects use only material
> from Phases 1–2 (fundamentals, functions, the preprocessor) — no pointers beyond basic
> array indexing are required, deliberately, so they can be attempted immediately after
> `fundamentals/` and `functions/`/`preprocessing/` without waiting for Phase 3.
> Each project follows the same structure: requirements, architecture, concepts exercised,
> implementation strategy, pitfalls, testing, and extension ideas — per the task brief's
> explicit request for this depth, not just an exercise list.

---

## 1. Calculator

**Requirements:** read an arithmetic expression from the command line or standard input
(`3 + 4 * 2`), evaluate it respecting operator precedence, print the result.

**Architecture:** a `tokenize` pass (split the input into numbers and operator characters)
feeding a two-stack or recursive-descent evaluator.

**Concepts exercised:** `fundamentals/03_expressions_and_operators.md` §8's precedence
table (your evaluator must *implement* what that table only documents); `fundamentals/04_control_flow.md`
§3's `switch` for dispatching on operator character; `libraries/02_stdlib.md` §5's
`strtod`/`strtol` for parsing numbers with real error detection, instead of `atoi`/`atof`.

**Implementation strategy:** start with no precedence at all (left-to-right evaluation of
`+`/`-` only), confirm it works, then add `*`/`/` with proper precedence, then parentheses.
Building up this way isolates exactly where a bug was introduced.

**⚠️ Pitfalls:**
- Using `atoi`/`atof` and silently accepting garbage input (`libraries/02_stdlib.md` §5) —
  use `strtol`/`strtod` and check `endptr`.
- Division by zero — undefined behavior for integers (`advanced/01_undefined_behavior.md`
  §3); check before dividing, don't just let it crash.
- Off-by-one errors in the tokenizer at the end of the input string.

**Testing:** a table of input strings paired with known-correct results, including edge
cases: a single number, a negative number, division, deeply nested parentheses, division
by zero (confirm it's caught, not crashed on).

**Extension ideas:** variables (`x = 3`, then use `x` later), functions (`sqrt(4)`), a
proper recursive-descent parser replacing the two-stack approach.

---

## 2. String Manipulation Toolkit

**Requirements:** a small command-line tool supporting several string operations
(reverse, uppercase/lowercase, count vowels, check palindrome) selected by an argument.

**Architecture:** `main` dispatches on `argv[1]` (the operation name) to a corresponding
function, each taking the input string (`argv[2]`) and performing one transformation.

**Concepts exercised:** `memory/04_arrays_and_strings.md`'s pointer-based string idioms
(§6) — this project is where `while (*s++ = *t++);`-style thinking becomes personally
familiar rather than just read; `fundamentals/04_control_flow.md` §7's `argc`/`argv`
pattern-marching idiom for dispatch.

**Implementation strategy:** write each operation as its own small function operating
in-place on a `char` buffer where possible (reverse, case conversion) and via a separate
output buffer where in-place isn't natural (building a count). Resist the urge to use
`<string.h>` functions for the core logic — the point of this project is writing the
pointer-arithmetic versions yourself.

**⚠️ Pitfalls:**
- Reversing a string in place correctly requires two pointers (one from each end) moving
  toward each other — an easy off-by-one if you don't stop exactly when they cross or meet.
- Modifying a string literal passed directly as `argv[2]` is fine (argv strings are
  writable on POSIX systems, unlike the `char *p = "literal"` case from
  `memory/04_arrays_and_strings.md` §4) — but don't assume this is portable to every
  possible C environment without checking.

**Testing:** empty string, single character, a string that's already a palindrome, mixed
case input for the case-conversion operations.

**Extension ideas:** Unicode awareness (well beyond this vault's scope — a good "why is
this actually hard" research exercise), a simple Caesar-cipher encode/decode operation.

---

## 3. File Copier

**Requirements:** copy one file to another, given two filenames as command-line
arguments — `mycp source.txt dest.txt`.

**Architecture:** open the source for reading, create/open the destination for writing,
copy in a fixed-size buffer loop, close both.

**Concepts exercised:** `libraries/01_stdio.md`'s `fopen`/`fread`/`fwrite`/`fclose` for a
first version. Once `systems/03_file_descriptors_and_unix_io.md` is reached, **revisit this
exact project** with raw `open`/`read`/`write`/`close` — the phase project there explicitly
asks for this, and having a working `stdio` version already in hand makes the comparison
direct.

**Implementation strategy:** `fopen` the source in `"rb"` mode (binary — per
`libraries/01_stdio.md` §1, this matters for portability even though it's a no-op on
UNIX-like systems) and the destination in `"wb"`; loop on `fread` into a buffer, `fwrite`
the same amount out, until `fread` returns 0.

**⚠️ Pitfalls:**
- Forgetting to check `fopen`'s return for `NULL` (a missing source file, or an
  unwritable destination directory) before using the result.
- Using `fgetc`/`fputc` one byte at a time for a large file — technically correct, but a
  direct, measurable demonstration of `libraries/01_stdio.md` §4's buffering argument when
  compared against a `fread`/`fwrite`-in-chunks version.
- Not checking `fwrite`'s return value — like `write` (`systems/03_file_descriptors_and_unix_io.md`
  §3), it can transfer less than requested (e.g., a full disk).

**Testing:** copy a small text file and `diff` the result against the original; copy an
empty file; copy a binary file (confirm `"b"` mode matters on platforms where it does);
attempt to copy a nonexistent source and confirm your error handling fires.

**Extension ideas:** a progress indicator for large files; preserving the source file's
permissions (`[External]`, needs `stat`/`chmod`, beyond this vault's core scope).

---

## 4. Word Counter

**Requirements:** read text (from a file or standard input) and report line, word, and
character counts — a `wc`-alike.

**Architecture:** a single-pass scan tracking three counters, with a small state machine
(in-word vs. not-in-word) for word counting specifically.

**Concepts exercised:** `libraries/04_ctype_and_locale.md` §1's `isspace`/`isalpha` for
robust whitespace/word-boundary detection instead of hand-rolled ASCII range checks (the
exact misconception that file warns against); the `getchar()`-in-a-loop idiom from
`systems/03_file_descriptors_and_unix_io.md` §4.

**Implementation strategy:** one `getchar()` loop, one pass. Track "was the previous
character part of a word" as a boolean; a word boundary (and hence a word-count increment)
happens on the **transition** from non-word to word, not on every word character.

**⚠️ Pitfalls:**
- Counting a run of spaces as multiple words — the classic bug this project exists to
  catch; the transition-based counting approach above avoids it.
- Passing a value from `getchar()` into `isspace`/`isalpha` after first narrowing it to
  `char` — `libraries/04_ctype_and_locale.md` §2's exact warning: `getchar`'s `int` return
  is already in the correct domain; don't narrow it unnecessarily.

**Testing:** empty input, a single word, multiple consecutive spaces between words, a file
with no trailing newline, a file that's only whitespace.

**Extension ideas:** longest-word tracking, a per-line word-count histogram, reading
multiple files and reporting a total.

---

## 5. 🎯 What these five projects have in common

`[Synthesis]` Every one of them is, underneath its specific domain, the same shape: **read
input character by character or token by token, maintain a small amount of state, make a
decision per character/token.** Internalizing that shared structure — not memorizing five
separate programs — is the actual skill these beginner projects exist to build, and it's
exactly the shape `projects/02_intermediate_projects.md`'s data-structure projects and
`03_systems_projects.md`'s larger tools build on next.

## 6. 🛠 Before moving on

Complete at least the Calculator and the File Copier before starting
`projects/02_intermediate_projects.md` — between them they exercise precedence-aware
parsing and real I/O error handling, both load-bearing for what comes next.
