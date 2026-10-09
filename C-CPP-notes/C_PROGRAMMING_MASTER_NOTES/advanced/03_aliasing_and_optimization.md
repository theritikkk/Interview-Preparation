# Aliasing & the Strict Aliasing Rule

> **Phase 11 · Undefined Behavior, Portability & Modern C.** Depends on:
> `memory/02_pointers.md` §9 (aliasing, first introduced), `memory/07_alignment_padding_aliasing.md`
> §5 (the type-punning preview this file now resolves precisely), `fundamentals/02_types.md`
> §6 (`restrict`), `libraries/03_string.md` §3 (`memcpy`'s `restrict`-qualified prototype).
> Feeds into: `04_modern_c_vs_historical_c.md` (this phase).
> Sources: **entirely `[External]`.** The strict-aliasing rule's effective-type list is
> quoted from the CERT C Coding Standard's own citation of C standard §6.5p7. The
> `-O0`-vs-`-O2` divergence and the compiler warning below are **real, captured output**
> from this vault's own build environment.

---

## 1. Aliasing, precisely — and the one rule that governs it

`[Cross-reference]` `memory/02_pointers.md` §9 defined aliasing as two names referring to
the same memory — ordinarily harmless, even essential (it's how passing a pointer to
modify a caller's variable works at all). It becomes a hazard specifically when the
*types* involved differ, because the compiler relies on an object's declared type to
decide what else in the program might (or can't) be touching that same memory while
optimizing.

### The strict aliasing rule

`[External, quoted from C §6.5p7 via CERT C's EXP39-C]` An object may be accessed only
through an lvalue of one of these types:

- a type compatible with the object's **effective type**,
- a qualified version of such a type,
- the signed or unsigned type corresponding to the effective type,
- an aggregate or union type that *includes* one of the above among its members
  (including recursively, through a nested union), or
- a **character type** (`char`, `signed char`, `unsigned char`) — the universal exception,
  which is exactly why `memcpy`'s byte-by-byte implementation (`libraries/03_string.md`)
  is always legal regardless of what types it's copying between.

Accessing an object through any **other** type — the textbook case: writing a `float`,
then reading the same bytes back through an `int *` — is undefined behavior, **even if the
addresses genuinely coincide and the sizes match.**

---

## 2. A real violation, and a real, captured consequence

`[External, verified in this vault's own build environment]`

```c
int convert(float *f, int *i) {
    *i = 1;
    *f = 0.0f;
    return *i;
}

int main(void) {
    float f = 1.0f;
    int result = convert(&f, (int *)&f);   /* i and f point to the SAME object */
    printf("result = %d\n", result);
    return 0;
}
```

```
$ gcc -O0 alias.c -o alias_O0 && ./alias_O0
result = 0
$ gcc -O2 alias.c -o alias_O2 && ./alias_O2
result = 1
```

**Same source, two genuinely different outputs.** At `-O0`, the compiler re-reads `*i`
from memory after the `*f = 0.0f` write, and — because `i` and `f` really do alias the same
4 bytes, and `0.0f`'s bit pattern happens to be all-zero — it reads back `0`. At `-O2`,
`-fstrict-aliasing` is enabled by default (`[External]`), so the compiler **assumes** a
`float *` and an `int *` can never point to the same object; under that assumption,
`*f = 0.0f` cannot possibly change what `*i` holds, so `return *i;` doesn't need to re-read
memory at all — it just returns the `1` already known from `*i = 1;`. **Both answers are
"correct" from the compiler's point of view** — because the program that produced them is
undefined behavior, there is no "correct" answer for either build to match.

### The compiler already tried to warn you

`[External, real captured warning]`

```
alias.c:12:37: warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
```

`[Synthesis]` `-Wstrict-aliasing` (included in `-Wall`) catches *some* violations at
compile time — worth always having enabled, though CERT's own documentation notes it
doesn't catch every case, so a clean build with this warning enabled is evidence, not proof,
of aliasing correctness.

---

## 3. The union exception — real, and narrower than it looks

`[External]` C explicitly **permits** reading a union member other than the one most
recently written, **as long as the access goes through the union type itself** — this is
*not* a strict-aliasing violation in C (this is a real difference from C++, where the same
pattern is undefined). `[Cross-reference]` This sharpens what
`memory/06_struct_union_layout.md` §7 and `memory/07_alignment_padding_aliasing.md` §5 left
as "implementation-dependent": the union-member-reading pattern specifically is
standard-permitted type punning in C, not the same hazard as §2's pointer-cast version.

```c
union { float f; unsigned int u; } conv;
conv.f = 1.0f;
printf("0x%08x\n", conv.u);     /* permitted — accessed through the union type */
```

**Verified, consistent at both `-O0` and `-O2`:** `0x3f800000` either way — unlike §2's
pointer-cast version, this pattern doesn't diverge under optimization, because it's not
relying on anything the strict aliasing rule forbids.

### 🎯 Interview point
"Is type punning through a union legal in C?" — **Yes, specifically through the union
type** — this is a genuinely permitted, standard pattern, distinct from (and safer than)
reinterpreting an address via an incompatible pointer cast, which §2 showed is undefined.
Interviewers who ask this are often testing whether you know this exact C-vs-C++
distinction, not just "is type punning ever OK."

---

## 4. The fully portable alternative: `memcpy`

`[External, verified consistent at both `-O0` and `-O2`]`

```c
float f = 1.0f;
unsigned int u;
memcpy(&u, &f, sizeof u);    /* 0x3f800000, identical at every optimization level */
```

**Why this is always safe:** `memcpy` operates through `char`-type accesses internally
(§1's universal exception), so it never triggers the strict aliasing rule regardless of
the two types involved. `[Synthesis]` And it is **not actually slow** — modern compilers
recognize a fixed-size `memcpy` between same-sized types at compile time and replace it
with a direct register move, identical in the generated assembly to the union or
pointer-cast version, with none of the latter's UB risk. `memcpy`-based reinterpretation is
the modern, universally-recommended default for this exact problem — prefer it over both
the union pattern (C-only, and easy to port incorrectly to C++) and the pointer-cast
pattern (outright UB) whenever reinterpreting bytes between types is genuinely necessary.

---

## 5. `restrict`, formalized — what it actually buys the compiler

`[Cross-reference, formalizing `fundamentals/02_types.md` §6's introduction and
`libraries/03_string.md` §3's `memcpy` application]` `restrict` on a pointer parameter is
the programmer's explicit promise: *for this pointer's lifetime, no other pointer will be
used to access the same memory.* This directly licenses the optimization the strict
aliasing rule only partially provides:

```c
void scale(float *a, float *b, int n) {       /* NO restrict: a and b MIGHT alias */
    for (int i = 0; i < n; i++)
        a[i] = b[i] * 2.0f;
}

void scale_r(float *restrict a, float *restrict b, int n) {  /* a and b CANNOT alias */
    for (int i = 0; i < n; i++)
        a[i] = b[i] * 2.0f;
}
```

`[External]` Without `restrict`, the compiler must consider that writing `a[i]` *might*
change a value `b[j]` will read on a later iteration (if the arrays overlap) — this can
block optimizations like reordering, vectorizing, or caching `b[i]` across iterations, even
though `a` and `b` are both `float *` (so the strict aliasing rule alone doesn't help here —
same-type pointers are *always* allowed to alias as far as that rule is concerned).
`restrict` closes exactly this gap: with it, the compiler can safely assume *no* iteration's
write through `a` affects *any* read through `b`, unlocking optimizations (including
auto-vectorization, `[External]`) that aliasing uncertainty would otherwise forbid.

### ⚠️ `restrict` is a promise, not a check
`[Synthesis, echoing §2's lesson exactly]` Just like the strict aliasing rule itself,
violating a `restrict` promise (calling `scale_r(buf, buf, n)` with overlapping arguments)
is undefined behavior — the compiler trusts you, generates code on that assumption, and a
broken promise produces exactly the same class of "which answer is 'correct'" confusion as
§2's example, for exactly the same underlying reason.

---

## 6. ⚠️ Misconceptions to unlearn now

- **"If the addresses genuinely coincide and the sizes match, reinterpreting through a
  different pointer type is safe."** It's undefined regardless — the rule is about
  *types*, not actual runtime addresses (§1–2).
- **"Union-based type punning and pointer-cast type punning are the same risky pattern,
  just different syntax."** They are not — the union form, accessed through the union
  type, is explicitly permitted in C; the pointer-cast form is a genuine strict-aliasing
  violation (§2–3).
- **"`memcpy` for type punning is a slow workaround compared to a direct pointer cast."**
  Modern compilers optimize a fixed-size `memcpy` between same-sized types down to the
  identical generated code, with none of the UB risk (§4).

---

## 7. 🎯 Interview questions

**Easy**
- Is type punning through a union legal in C? → Yes, when accessed through the union
  type — a genuine, standard-permitted pattern (§3).
- What's the fully portable way to reinterpret a `float`'s bits as an `unsigned int`? →
  `memcpy` between the two, which compiles to the same code as a direct reinterpretation on
  modern compilers (§4).

**Medium**
- Why does writing a `float` and reading it back through an `int *` produce different
  results at `-O0` versus `-O2`? → At `-O0` the compiler re-reads memory and sees the
  aliased write; at `-O2`, `-fstrict-aliasing` lets it assume the two pointer types can't
  alias, so it skips the re-read and returns a stale cached value instead (§2).
- What does `restrict` let the compiler do that the strict aliasing rule alone doesn't,
  for two same-typed pointer parameters? → Assume no overlap between them at all, even
  though same-typed pointers are always permitted to alias under the aliasing rule itself
  — unlocking optimizations like safely caching a value across loop iterations (§5).

**Hard**
- Explain precisely why the union-member-reading pattern is exempt from the strict
  aliasing rule while an equivalent pointer-cast reinterpretation is not, even though both
  "look like" reading the same bytes as a different type. → The aliasing rule's effective-
  type list explicitly includes "an aggregate or union type that includes" the accessed
  type among its members — the union's own type legitimizes access to any of its member
  types; a pointer cast has no such standing, since the pointed-to object's actual
  effective type never changed to match the new pointer's type (§1, §3).
- Why is `restrict`'s violation the same *class* of danger as a strict-aliasing
  violation, even though they're nominally different rules? → Both are promises the
  programmer makes to the compiler, which the compiler is then licensed to optimize on
  faith in — breaking either promise means the generated code's assumptions no longer
  match reality, producing the same "which answer is correct" incoherence (§5).

**Systems-level**
- A numeric library's inner loop runs noticeably slower than a hand-vectorized equivalent,
  despite identical arithmetic, and profiling shows the compiler isn't auto-vectorizing it.
  What's a likely first thing to check? → Whether the loop's pointer parameters lack
  `restrict` — without it, the compiler must conservatively assume potential aliasing
  between the pointers, which can block the exact safety proof auto-vectorization needs
  (§5).
- A codebase has relied on `*(int *)&some_float` for years without visible problems, then
  a compiler upgrade (or a new `-O3` flag) causes silent wrong output in exactly one
  function. What's almost certainly the cause, and what's the fix? → A latent strict-
  aliasing violation that an earlier compiler version/optimization level happened not to
  exploit — newer, more aggressive alias analysis now does; the fix is replacing the
  pointer-cast pattern with `memcpy`-based reinterpretation (§4), which is immune to this
  class of regression entirely.

---

## 8. External references

- [CERT C — EXP39-C: Do not access a variable through a pointer of an incompatible type](https://wiki.sei.cmu.edu/confluence/display/c)
  `[External]` — the source for §1's effective-type list and §3's union exception,
  with further noncompliant/compliant code examples.
- [GCC documentation — `-fstrict-aliasing`](https://gcc.gnu.org/onlinedocs/) `[External]`
  — the compiler flag behind §2's `-O2` behavior and its default-enabled optimization
  levels.
- [cppreference — `restrict`](https://en.cppreference.com/w/c) `[External]` — the
  complete, current semantics beyond §5's practical summary.

## 9. 🛠 Practice

1. Build and run §2's `convert` example yourself at `-O0` and `-O2`, confirming the same
   `0` vs. `1` divergence — then fix it with `memcpy` and confirm the result is identical
   (and correct) at both optimization levels.
2. Write `scale`/`scale_r` from §5, compile both with `-O3 -fopt-info-vec` (or your
   compiler's equivalent vectorization-report flag, `[External]`), and compare whether the
   `restrict`-qualified version vectorizes where the unqualified one doesn't.
3. Deliberately call `scale_r(buf, buf, n)` (violating its own `restrict` promise) and
   compare the result against the equivalent call to the non-`restrict` `scale` — observe
   whether they diverge, making §5's "promise, not a check" warning concrete.
