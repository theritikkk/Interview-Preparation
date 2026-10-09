# Types, Sizes & Representation

> **Phase 1 · Fundamentals.** Depends on: `01_program_structure.md`.
> Feeds into: `03_expressions_and_operators.md` (conversions/promotions use these types),
> `memory/01_memory_model.md`, `advanced/04_modern_c_vs_historical_c.md`.
> Sources: **Book B** §2.2–2.3 & Appendix A.4 (primary — precise, standard-grounded),
> **Book A** ch.12 (practical framing), **Book C** `<limits.h>`/`<float.h>` chapters (exact
> representation limits), `[External]` for C99+ fixed-width types.

---

## 1. The four basic types, and what "basic" really means

`[Book-derived, A]` Standard C has four basic data types: `int`, `char`, `float`, `double`.
Everything else — `short`, `long`, `unsigned`, arrays, pointers, structs — is either a
*modifier* of one of these or a *derived type* built out of them.

`[Book-derived, B]` Here's the crucial thing neither a quick tutorial nor casual habit
tells you: **the C standard does not fix how many bits any of these types occupy.** It only
fixes *minimums* and *relative orderings*:

| Type | Guaranteed minimum width | Typical width today (a fact about common platforms, not a guarantee) |
|---|---|---|
| `char` | ≥ 8 bits (exactly `CHAR_BIT` bits, `CHAR_BIT` ≥ 8) | 8 bits |
| `short` | ≥ 16 bits | 16 bits |
| `int` | ≥ 16 bits | 32 bits on essentially all current desktop/server platforms |
| `long` | ≥ 32 bits | 32 bits (Windows) or 64 bits (Linux/macOS) — a real, live portability trap |
| `long long` | ≥ 64 bits (C99+) | 64 bits |

`[Book-derived, B]` The only hard ordering the standard guarantees is
`sizeof(short) <= sizeof(int) <= sizeof(long) <= sizeof(long long)`. It is not guaranteed
that `short` is smaller than `int`, or that `long` is bigger — a compiler is free to make
all four the same size, as long as it also honors the minimum-bits table above.

### 💡 Key insight — a cross-book tension worth noticing

Book A frames `int` as "typically the size of one machine word, which on most modern home
PCs is 32 bits" and gives exact ranges (−2147483648…2147483647) as if they were guaranteed.
Book B is deliberately vaguer: it states only the *minimums* and says each compiler is
"free to choose appropriate sizes for its own hardware." **Both are true, but they're
answering different questions.** Book A tells you what to *expect* on a typical modern
machine; Book B tells you what the *language actually promises* — and only the latter is
safe to build portable code on. If you write code that assumes `int` is exactly 32 bits,
it will work on your laptop and may silently misbehave on an embedded target where `int`
is 16 bits. `[Synthesis]`

### 🧠 Mental model: why sizes aren't fixed

`[Synthesis]` C was designed to compile efficiently down to whatever a machine's native
word size is, rather than force every machine to emulate one fixed size. An `int` is meant
to be "the size the CPU handles most naturally" — that's *why* it's not pinned to a number.
This is also *why* `<stdint.h>` (C99, `[External]`) exists: when you genuinely need an
exact width — a file format, a network protocol, a hardware register — you reach for
`int32_t`, `uint16_t`, etc. instead of `int`/`short`, precisely because `int`/`short` were
never meant to give you that guarantee. Neither Book A nor Book B (both pre-C99 in outlook)
mentions `<stdint.h>`; treat it as the modern default whenever exact width matters.

---

## 2. `char`, signedness, and a genuine trap

`[Book-derived, B]` A `char` is a small integer, one byte (`CHAR_BIT` bits) by definition —
`sizeof(char)` is always `1`, by the standard's own definition of "byte" in this context.
Because a `char` is just a small integer, it can be freely used in arithmetic, which is
*why* code like `s[i] - '0'` (converting a digit character to its numeric value) or
`c + 'a' - 'A'` (ASCII case conversion) works: character constants participate in ordinary
integer arithmetic.

### ⚠️ Common bug — plain `char` signedness is implementation-defined

`[Book-derived, B]` The language does **not** specify whether a plain `char` (no explicit
`signed`/`unsigned`) is signed or unsigned — this is left to the implementation. On some
machines/compilers plain `char` is signed (range roughly −128…127); on others it's
unsigned (0…255). Concretely: `char c = 0xFF;` may compare as negative on one platform and
positive on another. The standard does guarantee one thing: any character in the machine's
*standard printing character set* will never come out negative, so ordinary text processing
is safe — the danger is specifically when you store **arbitrary binary byte values** (not
printable text) in a plain `char`. If you need arithmetic on raw bytes to be portable,
declare `signed char` or `unsigned char` explicitly rather than plain `char`.

This exact trap resurfaces constantly in real code: functions like `getchar()` and
`fgetc()` return an `int`, not a `char`, *specifically* so that the value `EOF` (typically
−1) can be distinguished from every possible valid byte value (0–255) — if you store the
result of `getchar()` into a plain `char` first and *then* compare against `EOF`, you can
get a false match/mismatch on platforms where `char` is unsigned. `[Synthesis, following
directly from the signedness rule above]`

---

## 3. `sizeof` — what it is, and what it isn't

`[Book-derived, A]` `sizeof` looks like a function call but is a **unary operator**,
evaluated (almost always) entirely at **compile time**. It has two forms:

```c
sizeof expression   /* size of the expression's type, in bytes, as size_t */
sizeof(type)        /* size of a named type, in bytes, as size_t */
```

The result type is `size_t` (an unsigned integer type defined in `<stddef.h>`).

| Question | Answer |
|---|---|
| What is it? | An operator that yields the size, in bytes, of an object or type. |
| Why does it exist? | Because C doesn't fix type sizes (§1), code that needs to know a size — for `malloc`, for loop bounds, for I/O buffer sizing — cannot hard-code a number and stay portable. |
| What happens at compile time? | For any type whose size is known at compile time (everything except variable-length arrays — a C99 feature), the compiler substitutes a plain integer constant; `sizeof` costs **zero** at run time. |
| What happens at runtime? | Nothing, in the common case — the "computation" already happened during compilation. (Exception: `sizeof` on a C99 variable-length array **is** evaluated at run time, since the array's size isn't known until then. `[External]`) |
| Common mistake | Applying `sizeof` to a pointer when you meant the array it points to — `sizeof(ptr)` gives the size of the pointer itself (4 or 8 bytes), not the size of what it points to. This is one of the most common real-world C bugs; the full array/pointer-decay explanation lives in `memory/04_arrays_and_strings.md` (Phase 4). |
| Interview point | "Why is `sizeof(char)` always 1?" — because the standard *defines* a byte, in C's terms, as the storage a `char` occupies; `sizeof` is defined relative to that unit, so this isn't a fact about hardware, it's true by definition. |

---

## 4. Floating-point types

`[Book-derived, A + B]` `float` and `double` both store approximate real numbers;
`long double` adds extended precision. As with integers, **exact sizes are implementation-
defined** — the standard doesn't require `float`, `double`, and `long double` to even be
different sizes, only that `<float.h>` (`[Book-derived, C]`, covered in depth in
`libraries/10_limits_float_stddef.md`, Phase 7) expose symbolic constants describing
whatever precision/range the implementation actually provides.

Two rules worth internalizing now:
- An unsuffixed floating literal like `3.14` is a `double`, **not** a `float` — suffix
  with `f`/`F` (`3.14f`) if you specifically want a `float` constant.
- In an expression mixing `float` and `int`, the `int` is converted up to match — see
  `03_expressions_and_operators.md` for the full conversion-rank ladder.

---

## 5. Integer representation in memory

`[Synthesis]` Modern machines represent negative integers in **two's complement**: to
negate a value, invert every bit and add 1. This isn't stated as a hard C requirement in
either book (it's near-universal in practice, and C23 has since made two's complement the
*only* standard-permitted representation — see the historical/modern box below), but it's
the representation you need in your head to reason about bit patterns.

```
int x = 5;              int y = -5;   (assuming 32-bit int, two's complement)

  bit:  31 ......................... 0            31 ......................... 0
        00000000 00000000 00000000 00000101       11111111 11111111 11111111 11111011
        └────────────── 5 ──────────────┘         └───── two's complement of 5 ─────┘
```

### 🔬 Under the hood — byte order (endianness)

`[External]` Multi-byte types like `int` occupy several consecutive bytes in memory, and
the *order* those bytes are stored in is another implementation detail C leaves open:

```
int n = 0x01020304;   (4 bytes: 01 02 03 04, most-significant byte first, conceptually)

Little-endian layout (x86, ARM in its default mode):
  address:   0x1000  0x1001  0x1002  0x1003
  byte:        04      03      02      01     ← least-significant byte at lowest address

Big-endian layout (network byte order; some embedded/legacy CPUs):
  address:   0x1000  0x1001  0x1002  0x1003
  byte:        01      02      03      04     ← most-significant byte at lowest address
```

Neither Book A, B, nor C names "endianness" explicitly in the pages sampled for this file,
but it's implied directly by their shared point that representation is implementation-
defined, and it matters the moment you read/write raw bytes (file formats, network
protocols, casting a pointer between types) — see `advanced/03_aliasing_and_optimization.md`
(Phase 11) for why reading an `int`'s bytes through a differently-typed pointer is a UB
minefield, not just an endianness question.

---

## 6. Qualifiers: `const`, `volatile`, and `restrict`

`[Book-derived, B, Appendix A]` `const` and `volatile` were both introduced by the ANSI
standard (they don't exist in pre-ANSI/"Classic" C). A `const`-qualified object may be
initialized but not assigned to afterward — the compiler is required to *diagnose* an
attempted assignment, but having done so, it's then permitted to ignore the qualifier
(i.e., `const` is a compile-time contract, not a hardware write-protection mechanism, unless
the object also happens to live in genuinely read-only memory). Its main purposes: telling
the *reader* a value won't change, and giving the *optimizer* license to assume it won't.

`volatile` does the opposite job: it tells the compiler *not* to assume anything about how
or when the object's value can change, and to suppress optimizations that would otherwise
be valid — Book B's own example is a pointer to a memory-mapped hardware device register,
where a value can change due to hardware activity the compiler has no way to see, so
"redundant"-looking reads through the pointer must not be optimized away.

`[External]` `restrict` is a C99 addition — neither Book A nor Book B (both effectively
ANSI-C89-era in this material) covers it. It's a promise from the programmer to the
compiler that, for the pointer's lifetime, no other pointer will be used to access the same
object — enabling optimizations the compiler couldn't otherwise safely make. It comes up
most often in standard-library function prototypes (e.g. `memcpy`'s parameters are
`restrict`-qualified, which is *why* `memcpy` — unlike `memmove` — is allowed to misbehave
on overlapping buffers). Full treatment belongs in `advanced/03_aliasing_and_optimization.md`.

---

## 7. Constants — the short version

`[Book-derived, B §2.3]`
- An integer constant like `1234` is `int`; append `L`/`l` for `long`, `U`/`u` for
  `unsigned`, `UL` for both; a constant too large for `int` is silently promoted to `long`.
- A leading `0` means **octal** (`037` is decimal 31); a leading `0x`/`0X` means
  **hexadecimal** (`0x1F` is also decimal 31). *(`[External]` C23 additionally allows a
  `0b`/`0B` binary-literal prefix — a genuine language change worth knowing if you're
  reading modern code; neither source book covers it since both predate it.)*
- A character constant like `'x'` **is an `int`**, not a `char` — its value is the numeric
  code of that character in the execution character set. `'x'` and `"x"` are *not* the
  same thing: the former is an integer, the latter is a two-element `char` array (the
  letter plus a trailing `'\0'`). This distinction trips up beginners constantly.
- Escape sequences: `\n` `\t` `\b` `\r` `\f` `\v` `\a` `\\` `\'` `\"`, plus `\ooo` (1–3
  octal digits) and `\xhh` (hex digits) for an arbitrary byte value.
- `enum` gives a list of named integer constants; the first name is 0 by default and each
  subsequent name increments by 1 unless given an explicit value. Enum names occupy
  ordinary identifier scope like any other name — they aren't automatically namespaced to
  their enum.

---

## 8. Historical C vs. modern C — types

`[Synthesis]` Both source books predate these; treat this whole section as `[External]`
context layered on top of book-derived material above.

| | K&R2 / Book A's world (≈ C89) | Modern C (C99 → C23) |
|---|---|---|
| Exact-width integers | Not available — `int`/`long` sizes vary by platform | `<stdint.h>`: `int32_t`, `uint16_t`, `int64_t`, etc. — use these when a format/protocol needs an exact width |
| Boolean type | None — `0`/nonzero convention only | C99 `_Bool` + `<stdbool.h>`'s `bool`/`true`/`false` (still just sugar over an integer type) |
| Negative-number representation | Implementation-defined (sign-magnitude, one's complement, or two's complement all permitted) | C23 mandates two's complement as the *only* permitted representation — a genuine simplification of the standard `[External, verified against cppreference's C23 changelog; see `advanced/04_modern_c_vs_historical_c.md` §4 — including the crucial caveat that this fixes *representation* only: signed overflow remains undefined behavior]` |
| Binary integer literals | Not available | C23 permits `0b101010` |

**What you should actually use today:** `<stdint.h>` types for anything where exact width
matters (file formats, network code, bit-manipulation that assumes a specific width),
`<stdbool.h>`'s `bool` for boolean-intent variables (purely for readability — it compiles
to an integer either way), and plain `int`/`long`/etc. everywhere else, exactly as both
source books teach.

---

## 9. ⚠️ Misconceptions to unlearn now

- **"`sizeof(int)` is always 4."** It's *usually* 4 on today's mainstream platforms — it is
  not guaranteed by the language (§1). Never hard-code it; always write `sizeof(int)`, not
  the number 4, anywhere a size matters.
- **"Signed integer overflow just wraps around, like unsigned does."** It does not — this
  is **undefined behavior**, not a well-defined wraparound, even though it "happens to"
  wrap on most hardware most of the time. `unsigned` overflow, by contrast, **is** well-
  defined: it wraps modulo `2ⁿ`. This distinction gets its own worked treatment in
  `advanced/01_undefined_behavior.md` (Phase 11) — it's one of the most consequential UB
  traps in the language because optimizers are legally allowed to assume signed overflow
  never happens, which can delete safety checks you wrote.
- **"`char` is always signed" / "`char` is always unsigned."** Neither — it's
  implementation-defined (§2); `signed char` and `unsigned char` are the only two forms
  with a guaranteed signedness.

---

## 10. 🎯 Interview questions

**Easy**
- Is `sizeof(int)` guaranteed to be 4? → No; only that it's ≥ 16 bits and
  `sizeof(short) ≤ sizeof(int) ≤ sizeof(long)`.
- What's the difference between `'x'` and `"x"`? → `int` holding a character code vs. a
  2-byte `char[]` (the character plus `'\0'`).

**Medium**
- Why do `getchar()`/`fgetc()` return `int` instead of `char`? → So the sentinel value
  `EOF` can be represented outside the range of every valid byte value; storing the result
  into a `char` first can break the `== EOF` comparison depending on `char`'s signedness.
- What does `const` actually guarantee, and what does it not guarantee? → Guarantees the
  compiler diagnoses direct reassignment through that name; does not guarantee the memory
  is hardware-write-protected, and a `const` object can still be modified through a
  non-`const` alias, which is itself undefined behavior if the underlying object was
  originally declared `const`.

**Hard**
- Explain why `unsigned int` overflow is well-defined but `signed int` overflow is
  undefined behavior, and what a compiler is legally permitted to do differently because
  of that distinction. → Unsigned arithmetic is defined by the standard to be arithmetic
  modulo `2ⁿ`; signed overflow has no defined result at all, so the compiler may assume it
  never happens and optimize on that assumption (e.g., eliminating an overflow check that
  can only trigger via UB) — see `advanced/01_undefined_behavior.md`.

**Systems-level**
- You receive a 4-byte value over a network socket and need to interpret it as an `int` on
  both a little-endian x86 machine and a big-endian embedded target. What goes wrong if you
  just `memcpy` the bytes into an `int`, and what's the standard-library fix? → Byte order
  differs by platform (§5), so the raw bytes decode to different numeric values on each
  machine; the fix is to use defined-order conversion functions (`ntohl`/`htonl` in
  networking code, `[External]`, covered in `systems/07_sockets_and_networking.md`) rather
  than relying on the host's native byte order.

---

## 11. External references

- [cppreference — Arithmetic types](https://en.cppreference.com/w/c) `[External]` — exact,
  version-annotated (C89/C99/C11/C17/C23) size and behavior guarantees for every type.
- [GNU libc manual](https://www.gnu.org/software/libc/manual/) `[External]` — glibc's
  concrete choices (e.g. `long` being 64-bit on Linux x86-64) where the standard leaves
  things open.

## 12. 🛠 Practice

1. `[Book-derived, B, Exercise 2-1]` Write a program that prints `sizeof` for every basic
   type (signed and unsigned variants) on your machine, then compare against the symbolic
   limits in `<limits.h>`/`<float.h>` — do they agree?
2. Force a signed integer overflow deliberately (`INT_MAX + 1`) and observe what your
   compiler does at `-O0` versus a higher optimization level — do the results match? (Save
   this exercise's deeper "why" for `advanced/01_undefined_behavior.md`.)
3. Write a 4-byte union of a `uint32_t` and a `unsigned char[4]`, set the integer to
   `0x01020304`, and print the bytes to determine your machine's endianness directly rather
   than looking it up.
