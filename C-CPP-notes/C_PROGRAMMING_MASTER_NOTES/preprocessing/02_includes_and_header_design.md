# The Preprocessor: `#include` & Header Design

> **Phase 2 · Functions, Scope & the Preprocessor.** Depends on:
> `01_macros.md`, `functions/01_functions_and_stack_frames.md` §3 (declaration vs.
> definition, external variables).
> Feeds into: `03_conditional_compilation.md` (include guards use `#if`/`#ifndef`),
> `systems/02_linking_and_loading.md` (Phase 5 — header discipline is what keeps linking
> from breaking).
> Sources: **Book B** §4.5 (header design philosophy) and §4.11.1 (`#include` mechanics).

---

## 1. What `#include` actually does

`[Book-derived, B]` A line of the form

```c
#include "filename"
#include <filename>
```

is replaced, in the preprocessed text, by the **entire contents of `filename`** — a pure
textual paste, exactly matching the mental model from `fundamentals/01_program_structure.md`
§3. The two forms differ only in *where the preprocessor looks first*:

- `"filename"` — searching typically starts wherever the *including* source file itself is
  located; if not found there, falls back to the same implementation-defined search used
  for `<filename>`.
- `<filename>` — searching follows an implementation-defined path (conventionally, the
  compiler's standard system/library include directories).

**Convention, not a hard rule:** `"..."` for your own project's headers, `<...>` for
system/library headers — this is what nearly every real codebase does, and following it
makes intent legible even though the language itself doesn't force the distinction.

An included file may itself contain further `#include` lines — nesting is normal and
expected (every standard header you `#include` typically pulls in several others itself).

---

## 2. Why `#include` is the right tool for sharing declarations

`[Book-derived, B]` `#include` is the standard way to tie together the declarations a
multi-file program needs to agree on. Concretely, it **guarantees every file that includes
a given header sees identical declarations** — eliminating an entire class of bug where two
files disagree about a function's signature or a variable's type because someone
hand-copied a declaration into two places and let them drift apart. The cost: when a header
changes, **every file that includes it must be recompiled** — this is precisely the
dependency tracking a build system (`make`, etc.) exists to automate (full treatment in
`systems/01_compilation_toolchain.md`, Phase 5).

### 🧠 Mental model: what actually belongs in a header

`[Synthesis, directly following the declaration/definition split established in
`functions/01_functions_and_stack_frames.md` §3]` A header should contain **declarations**,
not **definitions** — with narrow, deliberate exceptions:

| Belongs in a header | Why |
|---|---|
| Function prototypes (`int strindex(char *, char *);`) | Declares the interface; the linker later matches this against the one real definition, wherever it lives. |
| `extern` variable declarations | Same reasoning, for shared external variables. |
| `#define` macros, `typedef`s, `struct`/`enum` type definitions | These don't allocate storage the way a variable definition does — every including file needs the *same* type definition, and (unlike a variable) there's no "storage" to duplicate, so this is safe and necessary to repeat. |
| `static inline` function definitions (`[External]`, C99) | A narrow, deliberate exception — `static` linkage means each translation unit gets its own private copy, so no multiple-definition conflict arises. |

| Does *not* belong in a header (ordinarily) | Why |
|---|---|
| Ordinary function *definitions* (with a body) | If two `.c` files both `#include` a header containing a full function body, both translation units define that function — a **multiple-definition link error**, since (per `functions/01_functions_and_stack_frames.md` §3) there must be exactly one definition of anything with external linkage. |
| Ordinary external *variable definitions* (`int sp;`, not `extern int sp;`) | Same problem, for variables instead of functions. |

### Worked example — from Book B's own case study

`[Book-derived, B]` Splitting a small calculator program into `main.c`, `stack.c` (holding
`push`/`pop` and their shared state), and `getop.c`, the natural shared header `calc.h`
holds the pieces every file needs to agree on — the function prototypes for `push`, `pop`,
and `getop`, and any shared `#define`s — while each `.c` file keeps its own *private*
implementation details (like `stack.c`'s actual stack array and index, kept `static` so
they're invisible outside that file — see `functions/01_functions_and_stack_frames.md` §5)
to itself.

### 💡 Key insight — the header-granularity tradeoff
`[Book-derived, B]` There's a genuine, named tradeoff between minimizing what each file
can see (better isolation) and minimizing the number of header files to keep synchronized
(better maintainability). Book B's own conclusion for small-to-moderate programs: **one
shared header covering everything two or more files need in common** is usually the right
default; larger programs earn more fine-grained headers as the cost of accidental coupling
starts to outweigh the cost of managing more files.

---

## 3. Include guards — the short version

`[Book-derived, B]` Because `#include` is pure text-pasting, including the same header
**twice** in one translation unit (directly, or indirectly through two other headers that
each include it) re-pastes its entire content twice — which is a hard error for anything
that can't legally appear twice (a `struct` definition, for instance). The fix is a
conditional-compilation guard around the header's entire content — full mechanics belong to
`03_conditional_compilation.md`, but the pattern itself is universal enough to show here:

```c
#ifndef HDR_H
#define HDR_H
/* ... header contents ... */
#endif
```

The first inclusion in a translation unit defines `HDR_H` and includes the content;
every subsequent inclusion (in the same translation unit) finds `HDR_H` already defined and
skips straight to `#endif`, pasting nothing. This is what lets headers freely include other
headers they depend on, without the *user* of a header needing to manually track and avoid
duplicate inclusion themselves.

`[External]` Many modern compilers also support a non-standard but extremely widely
supported shortcut, `#pragma once`, placed once at the top of a header, which achieves the
same effect without a manually-chosen guard macro name (and sidesteps guard-name collisions
between unrelated headers). It is **not** part of the ISO C standard, so strictly portable
code still uses the `#ifndef`/`#define`/`#endif` pattern — but `#pragma once` is common
enough in real-world code that you should recognize it on sight.

---

## 4. ⚠️ Common mistakes

- **`#include`-ing a `.c` file.** Legal (it's just text-pasting), but almost always a design
  smell — it usually means code that should have been split into a header (declarations)
  plus separately-compiled `.c` files (definitions) instead got glued together textually.
- **Forgetting an include guard on a header that gets included from more than one place.**
  Fine as long as it only ever gets included once per translation unit in practice — a
  latent bug that surfaces the moment some other header starts including it too, producing
  a confusing "redefinition" error far from the actual root cause.
- **A header that depends on another header's declarations but doesn't `#include` it
  itself**, relying on every *user* of the header to include things in the right order
  first. Fragile — a **self-contained header** (one that includes everything it itself
  needs, so it can be `#include`d first, alone, in any file) is the far more robust design.

---

## 5. 🎯 Interview questions

**Easy**
- What's the practical difference between `#include "foo.h"` and `#include <foo.h>`? →
  Search order — local/relative-to-source-file first for quotes, implementation-defined
  system paths for angle brackets (§1).
- Why does a header usually contain function *prototypes* rather than full function
  *definitions*? → To avoid multiple-definition link errors when more than one file
  includes it (§2).

**Medium**
- What actually breaks if you `#include` the same (unguarded) header twice in one file, and
  why? → Its content — including anything that can't legally repeat, like a `struct`
  definition — gets pasted in twice, since `#include` is pure text substitution (§3).
- Why is `static inline` a safe exception to "no function bodies in headers"? → `static`
  linkage means each translation unit that includes it gets its own private copy, so no
  cross-file multiple-definition conflict arises (§2).

**Hard**
- A project has header `A.h` that uses a type from `B.h`, but `A.h` doesn't `#include B.h`
  itself — it just assumes callers will include `B.h` first. What's the design problem here,
  and what's the fix? → `A.h` isn't self-contained; any file that includes `A.h` without
  first (and correctly) including `B.h` breaks, and the fix — making every header
  responsible for its own dependencies — removes that fragile, order-dependent contract
  (§4).

**Systems-level**
- Explain precisely why changing one header can force recompilation of many `.c` files that
  didn't themselves change, and how this connects to the linking discussion in
  `fundamentals/01_program_structure.md`. → `#include` is textual pasting, so every `.c`
  file that (directly or transitively) includes the changed header effectively has
  different *preprocessed* content afterward, even though the `.c` file's own text is
  untouched — each such translation unit must be recompiled to a new object file before
  re-linking, which is exactly the dependency-tracking problem build systems like `make`
  automate.

---

## 6. External references

- [cppreference — `#include`](https://en.cppreference.com/w/c) `[External]` — precise,
  current search-path and nesting rules.
- [GCC documentation — header files](https://gcc.gnu.org/onlinedocs/) `[External]` — the
  `-I` flag and GCC's concrete search-path behavior, useful once you're configuring a real
  multi-directory project.

## 7. 🛠 Practice

1. `[Book-derived, B]` Split a small program (e.g. the `strindex`/`getline` pattern-search
   program referenced in `functions/01_functions_and_stack_frames.md`) into `main.c` plus
   one or two supporting `.c` files, with a shared `.h` holding only the prototypes/macros
   they need in common. Build it with separate `gcc -c` steps plus one link step
   (`fundamentals/01_program_structure.md` §8's practice exercise 3, revisited with real
   shared declarations this time).
2. Deliberately remove a header's include guard, arrange for it to be included twice in one
   translation unit (directly plus via a second header), and read the resulting compiler
   error closely — then restore the guard and confirm it's gone.
3. Write a header that isn't self-contained (uses a type from another header without
   including it), then a file that includes it *without* first including the dependency —
   observe the failure, then fix the header to include its own dependency.
