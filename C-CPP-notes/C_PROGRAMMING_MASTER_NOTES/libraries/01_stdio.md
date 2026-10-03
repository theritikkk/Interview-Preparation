# `<stdio.h>` — Streams, Buffering & Why the Library Looks This Way

> **Phase 7 · The Standard Library, By Design.** Depends on: `memory/01_memory_model.md`
> (regions), `preprocessing/01_macros.md` (the macro-vs-function coexistence trick this
> file's centerpiece depends on).
> Feeds into: `systems/03_file_descriptors_and_unix_io.md` (Phase 8 — the `FILE*` → fd →
> kernel chain this file sets up one side of).
> Sources: **Book C** ch.12, in full (primary — Plauger was on the committee that designed
> this header; his "Background" section is the best design-rationale material in any of the
> three books, though the scan's OCR is rough in places). **Book A** ch.21 and **Book B**
> ch.7 for practical breadth and the quick-reference API surface, respectively.

---

## 1. Why `stdio` looks the way it does — a genuine design history

`[Book-derived, C]` This is worth reading before a single function signature, because it
explains *why* the API has the shape it has, not just what that shape is. Early languages
(Plauger's own example: FORTRAN II) named the physical device directly in I/O statements —
utterly non-portable. FORTRAN IV added "logical unit numbers" as a layer of indirection,
an improvement but still clumsy. **UNIX's actual contribution was a genuinely uniform
model**: every stream — file, terminal, pipe — presents the same interface; every line
ends in a single `'\n'`; device-specific quirks are pushed to the system's edges (device
handlers, `ioctl`) instead of leaking into every program. C, built on UNIX, inherited this
model directly, including small-integer **file descriptors** issued by the OS and the
convention of three always-open streams (standard input/output/error).

**The concrete design tension the standards committee actually faced:** UNIX's raw
`open`/`read`/`write`/`close` primitives are simple but require the *caller* to manage
buffering by hand for good performance; a separate, older tradition of **buffered stream**
functions (`fopen`/`fgetc`/`fputc`, built around an opaque `FILE` object) had evolved
specifically to hide that buffering. Rather than standardize both layers, `[Book-derived, C]`
**the committee deliberately dropped the raw UNIX-style primitives from ISO C entirely** —
`open`/`read`/`write`/`close` are **not** part of the C standard at all; they're POSIX,
implemented in terms of (or alongside) `<stdio.h>`, not inside it. This is precisely the
"C language vs. C standard library vs. POSIX vs. Linux system calls" distinction the brief
insists on keeping sharp, and it's worth seeing the committee's *reasoning* rather than
just being told the rule: portability (some target platforms genuinely don't have a UNIX-
style fd model at all) and a preference for one clean mechanism over two overlapping ones
were both explicitly weighed. `systems/03_file_descriptors_and_unix_io.md` (Phase 8) covers
those POSIX-level primitives properly, on their own terms.

### 🧠 Mental model: text vs. binary streams
`[Book-derived, C]` A second real design compromise: not every platform shares UNIX's
"a byte stream is a byte stream" simplicity — MS-DOS, for instance, terminates text lines
with two bytes (`\r\n`) but expects raw byte fidelity for binary data. Standard C's answer
is the `"b"` mode flag on `fopen`: a **text stream** may translate line endings and is
permitted various platform-specific liberties (padding, a guaranteed-safe character subset);
a **binary stream** promises exact byte-for-byte fidelity for what you wrote, with only
trailing zero-byte padding permitted. On UNIX, text and binary streams behave identically
(no translation needed) — but code that ignores the `"b"` flag isn't portable, it's just
gotten lucky on one platform.

---

## 2. The `FILE` object, and why it has the members it does

`[Book-derived, C]` `FILE` is an opaque struct — the standard deliberately does not specify
its members, so you must never read or write one directly, only through the library's own
functions. Plauger's own implementation's members (paraphrased, not the literal internal
names) illustrate what a `FILE` genuinely needs to track: status/mode flags; the OS-level
file descriptor underneath; a pointer to the stream's buffer (or none); pointers marking
the buffer's current read/write position and its end; a small pushback area (for `ungetc`);
and, for a temporary file, a name to delete on close. **Every one of these fields exists to
answer one question the library must repeatedly ask itself:** "is there already a byte
ready right here in memory, or do I need to go talk to the OS?" — which is exactly the
question the next section's macro trick is built to answer as cheaply as possible.

---

## 3. The `getchar`/`putchar` macro trick — buffering made concrete

`[Book-derived, C — this is the exact mechanism the phase brief calls out by name]` Recall
from `preprocessing/01_macros.md` §4 that `getchar`/`putchar` are commonly implemented as
macros specifically to avoid function-call overhead per character. Here is *precisely* how,
paraphrased from Plauger's own implementation:

```c
#define getchar() \
    (stdin_ptr->next < stdin_ptr->read_end ? *stdin_ptr->next++ : (getchar)())

#define putchar(c) \
    (stdout_ptr->next < stdout_ptr->write_end ? (*stdout_ptr->next++ = c) : (putchar)(c))
```

**Reading this precisely:** the macro's condition checks a **cheap pointer comparison** —
"is there already a character sitting in the buffer I haven't consumed yet (or room left
to write one)?" If so, the **fast path** fires: dereference, use, advance the pointer —
no function call at all, just a couple of machine instructions. If the buffer is exhausted
(read side) or full (write side), the macro falls through to `(getchar)()` — parenthesizing
the name specifically to **suppress macro expansion and call the real underlying
function**, which does the actual, comparatively expensive work: talking to the OS to
refill or flush the buffer, then retrying.

### 🎯 Interview point
"Why does `(getchar)()` have parentheses around the function name?" — Because `getchar` is
*also* a macro (per this exact definition); writing bare `getchar()` inside the macro's own
replacement text would just re-expand the macro recursively (in practice, the preprocessor
suppresses re-expanding a macro from within its own expansion, so it wouldn't infinite-loop
— but it also wouldn't reach the real function). Wrapping the name in parentheses,
`(getchar)`, hides it from the macro-expansion pass entirely — the preprocessor only
recognizes a macro invocation as a bare identifier immediately followed by `(`, and
`(getchar)` doesn't match that pattern — so this specific spelling correctly reaches the
real, compiled `getchar` function, which every conforming library still provides
underneath the macro for exactly this reason. This is directly the pattern
`preprocessing/01_macros.md` §4's `#undef getchar` note gestured at, now shown in its
actual, load-bearing use.

### 💡 Why this design, and not something simpler
`[Synthesis]` This macro trick is a direct, textbook answer to one of the brief's "WHY"
questions — *why does `printf` use `FILE *`* and why the library bothers with all this
buffering machinery at all: because a naive implementation calling a real OS `write` (a
system call, with real, non-trivial overhead — `systems/`, Phase 8, covers this cost
concretely) for every single character would be disastrously slow for any program that
does more than trivial I/O. The buffer is what lets `getchar`/`putchar` cost almost nothing
in the overwhelmingly common case (buffer already has what you need), while still
guaranteeing correctness in the rare case (buffer needs refilling), via the real function
fallback.

---

## 4. Buffering modes — precisely, per the standard

`[Book-derived, C, standard text paraphrased]` Three modes, settable via `setvbuf`:

| Mode | Constant | Behavior |
|---|---|---|
| Unbuffered | `_IONBF` | Characters are transmitted to/from the OS as soon as possible — no batching at all. |
| Fully buffered | `_IOFBF` | Characters accumulate and transfer as one block once the buffer fills. |
| Line buffered | `_IOLBF` | Characters transfer as a block whenever a newline is written (or, on input, whenever the OS has more to offer than the buffer currently holds). |

`[Book-derived, C]` **The default mode for the three standard streams is itself a real,
useful design decision:** `stderr` is never fully buffered (so error/diagnostic output
appears promptly, even if the program crashes immediately after); `stdin`/`stdout` are
fully buffered **if and only if the implementation can determine the stream doesn't refer
to an interactive device** — which is *why* output to a terminal typically appears
line-by-line (effectively line-buffered in practice) while output redirected to a file
batches into much larger chunks, without you ever calling `setvbuf` yourself.

### 🛠 Debugging technique
A classic, real gotcha this explains directly: a program that `printf`s progress messages
but is piped into another program (`./prog | less`, say) can appear to "hang" with no
visible output — the destination isn't a terminal, so `stdout` silently became fully
buffered, and your messages are sitting in the buffer, not yet flushed. `fflush(stdout)`
after each message (or explicitly setting line-buffered mode via `setvbuf`) fixes this —
worth knowing as a debugging pattern in its own right, independent of ever reading this
paragraph again.

---

## 5. `printf`/`scanf` — the format-string model, at a conceptual level

`[Synthesis, at a level this vault treats as sufficient — full format-specifier syntax is
better looked up than memorized]` `printf`'s family walks its format string character by
character, copying ordinary characters through unchanged and, on each `%`-introduced
conversion specifier, consuming the next variadic argument
(`libraries/08_stdarg.md`, later in this phase, covers exactly how a variadic function
reaches those arguments) and formatting it according to that specifier's rules. `scanf`'s
family does the mirror-image job: matching input against the format string, converting
matched text into values written through pointer arguments.

### ⚠️ Common bug — `scanf` format/argument mismatches
`[Synthesis]` Because `scanf` writes through pointers it's handed, and neither `printf` nor
`scanf` is checked by the compiler against the format string's actual conversions on every
compiler by default (though modern GCC/Clang can and often do warn about mismatches —
`[External]`, a valuable compiler diagnostic, not a language guarantee) — passing a
non-pointer where `scanf` expects one, or mismatching a conversion specifier's expected
type against the actual argument's type, is a real, classic, and often silent bug source.
This connects directly to `preprocessing/01_macros.md` and `functions/`'s discussion of
variadic functions: there is no language-level type-checking safety net once execution
reaches a variadic function's body — `printf`/`scanf`'s format string *is* that safety
net, informally, and only as reliable as the programmer keeping it in sync with the actual
argument list.

---

## 6. ⚠️ Misconceptions to unlearn now

- **"`open`/`read`/`write`/`close` are part of standard C."** They're POSIX, not ISO C —
  the committee deliberately excluded raw UNIX-style I/O from the standard entirely (§1).
- **"`getchar()` always makes a function call."** It doesn't, in a typical implementation —
  the macro form (§3) only calls the real function when the buffer's exhausted; most calls
  are a cheap inline pointer check.
- **"stdout always flushes immediately after `printf`."** Only when it's unbuffered or
  line-buffered *and* a newline was written — redirected/piped output is commonly fully
  buffered by default, a real, practical debugging trap (§4).

---

## 7. 🎯 Interview questions

**Easy**
- Are `open`/`read`/`write`/`close` part of the C standard? → No — POSIX, not ISO C (§1).
- Which of the three standard streams is never fully buffered by default, and why? →
  `stderr` — so diagnostic output appears promptly even right before a crash (§4).

**Medium**
- Why does `getchar()`'s macro form check a pointer comparison before doing anything else?
  → It's testing whether a character is already sitting in the buffer — the cheap common
  case — before falling back to the real, comparatively expensive function call that talks
  to the OS (§3).
- Why can piped output appear to "hang" with no visible output, even though the program is
  actively calling `printf`? → Output to a non-terminal destination is typically fully
  buffered by default; the text is sitting in the buffer, not yet flushed (§4).

**Hard**
- Explain exactly why `(getchar)()`, with parentheses around the name, is necessary inside
  the `getchar` macro's own fallback path. → `getchar` is itself a macro; a bare
  `getchar()` there would be recognized as another macro invocation, while `(getchar)`
  doesn't match the "identifier immediately followed by `(`" pattern the preprocessor looks
  for, so it correctly reaches the real, compiled function instead (§3).
- Why did the ISO C committee decide to exclude raw UNIX-style I/O primitives from the
  standard entirely, rather than standardizing both layers? → Portability (not every target
  platform has a UNIX-style file-descriptor model) and a preference for one clean,
  committed-to abstraction over two overlapping, historically-accreted ones (§1).

**Systems-level**
- A program processes a large file character-by-character using `getchar()` in a tight
  loop. Why is this typically *not* the performance disaster naive reasoning might suggest?
  → The macro form's buffered fast path (§3) means the overwhelming majority of calls never
  touch the OS at all — the real cost (a `read` system call, `systems/`, Phase 8) is paid
  only once per buffer-full, amortized across potentially thousands of characters.

---

## 8. External references

- [cppreference — `<cstdio>`/`<stdio.h>`](https://en.cppreference.com/w/c) `[External]` —
  current, complete function-by-function reference, including the exact `printf`/`scanf`
  format-specifier grammar this file treats only conceptually.
- [man7.org — `stdio(3)`](https://man7.org/linux/man-pages/) `[External]` — glibc's own
  stdio overview, including its real buffering-mode defaults in practice.

## 9. 🛠 Practice

1. Write a program using `getchar()` in a loop to count characters in a large file, and
   compare its wall-clock time against a hand-rolled version using raw `read()` in small
   (e.g. 1-byte) chunks — the gap directly demonstrates §3's buffering payoff (full `read`
   coverage is Phase 8; a rough comparison is enough here).
2. Reproduce the "piped output hangs" scenario from §4's debugging box deliberately —
   `printf` progress messages in a loop with a deliberate `sleep`, piped into `cat`, with
   and without an explicit `fflush(stdout)` after each message.
3. Use `setvbuf` to force a stream into each of the three buffering modes explicitly, and
   observe (e.g. via `strace`, previewed here, covered fully in `debugging/`, Phase 10) how
   the number and size of underlying `write` calls changes between modes.
