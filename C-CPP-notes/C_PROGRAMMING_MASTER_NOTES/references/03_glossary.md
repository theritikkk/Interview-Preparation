# Glossary

> Every term gets a one-line definition and a pointer to its full treatment. The 20 terms
> the original task brief named explicitly get the fuller "deeper meaning" treatment it
> asked for; everything else gets a precise one-liner — this file is a map back into the
> other 65, not a replacement for them.

---

## The brief's 20 core terms, in full

**Object** — a region of storage that can hold a value; the standard's own word for "a
variable, really." *Deeper meaning:* every value in C lives in some object, and an
object's **type** governs how its bytes are interpreted — the root concept behind the
strict aliasing rule. *Related:* lvalue, storage duration. *See:* `memory/01_memory_model.md`.

**Lvalue** — an expression referring to an object (something that *could*, syntactically,
appear on the left of `=`, though not every lvalue is actually assignable — an array name
is an lvalue but not assignable). *Deeper meaning:* the standard's precise term for "has an
identifiable location," used throughout the strict aliasing rule's own wording. *Related:*
object, rvalue. *See:* `advanced/03_aliasing_and_optimization.md` §1.

**Rvalue** — an expression that produces a value but doesn't necessarily refer to a
persisting object (`2 + 2`, a function's return value). *Related:* lvalue. *See:*
`fundamentals/03_expressions_and_operators.md`.

**Translation unit** — one `.c` file *after* preprocessing: every `#include` pasted in,
every macro expanded. The actual unit the compiler proper operates on. *Deeper meaning:*
this is why `static` (internal linkage) confines a name to "this translation unit," not
"this source file as originally typed" — they're the same thing only because preprocessing
already happened. *Related:* linkage, compilation. *See:* `fundamentals/01_program_structure.md`
§4.

**Linkage** — whether a name is visible outside the translation unit that declares it.
**External** linkage (the default for functions/globals): visible everywhere, resolved by
the linker. **Internal** linkage (`static`): confined to one translation unit. **No**
linkage: ordinary local variables. *Deeper meaning:* `nm`'s uppercase-vs-lowercase symbol
letters are linkage made visible — real, captured evidence of this exact distinction.
*Related:* scope, storage duration, symbol table. *See:* `functions/01_functions_and_stack_frames.md`
§5, `debugging/03_static_and_binary_inspection.md` §1.

**Scope** — the region of program text where a name is visible. Block scope (a variable
declared inside `{ }`), file scope (declared outside any function), function-prototype
scope. *Deeper meaning:* scope is a compile-time, textual concept; it's easy (and wrong) to
conflate with **lifetime**, a runtime concept — a `static` local has block *scope* but
program-*duration* lifetime, the two genuinely differ. *Related:* linkage, storage
duration. *See:* `functions/01_functions_and_stack_frames.md` §4.

**Storage duration** — how long an object's storage persists: **automatic** (one function
call), **static** (the whole program run), **allocated** (from `malloc` to matching
`free`), **thread** (C11, one thread's lifetime). *Related:* scope, linkage. *See:*
`memory/01_memory_model.md` §2.

**Alignment** — the restriction that an object's address must be a multiple of some value
(often its size), for correct/efficient hardware access. *Deeper meaning:* this is *why*
padding exists at all — the compiler inserts it to satisfy each member's own alignment
requirement. *Related:* padding, ABI. *See:* `memory/07_alignment_padding_aliasing.md` §1.

**Padding** — compiler-inserted, unnamed bytes between (or after) a struct's members,
added so each member satisfies its own alignment requirement. *Deeper meaning:* this is why
`sizeof(struct)` is never safely assumed to equal the sum of its members' sizes — verified,
not just stated, in this vault via real `offsetof` output. *Related:* alignment. *See:*
`memory/07_alignment_padding_aliasing.md` §2.

**ABI (Application Binary Interface)** — the compiled-code-level contract a platform
enforces: calling conventions (which registers hold arguments/return values), struct
layout rules, symbol naming — the binary analog of the C language's own (source-level)
rules. *Deeper meaning:* two compilers can both be fully ISO-C-conformant and still produce
mutually incompatible binaries if they don't share an ABI — conformance is a *source*-level
guarantee, not a binary one. *Related:* calling convention, linking. *See:*
`functions/01_functions_and_stack_frames.md` §6, `systems/01_compilation_toolchain.md`.

**Pointer** — a variable holding an address: a number naming a byte, tagged by its
declared type with how many bytes to treat as one unit. *Related:* dereference, array
decay. *See:* `memory/02_pointers.md` (this vault's single most emphasized topic).

**Dereference** — following a pointer to access the object it points to (`*p`).
*Deeper meaning:* dereferencing is only well-defined when the pointer is valid (not
`NULL`, dangling, or wild) **and** the access goes through a type-compatible lvalue (the
strict aliasing rule). *Related:* pointer, undefined behavior. *See:*
`memory/02_pointers.md` §1, `advanced/03_aliasing_and_optimization.md`.

**Undefined behavior** — behavior for which the standard imposes no requirement
whatsoever; the most dangerous of four classification categories (alongside unspecified,
implementation-defined, locale-specific). *Deeper meaning:* UB's defining danger isn't that
it crashes — it's that it's permitted to "work" silently, and that an optimizer is free to
assume it never happens, which can delete reachable code. *Related:* implementation-defined,
constraint violation. *See:* `advanced/01_undefined_behavior.md` (this vault's full catalogue).

**Implementation-defined** — unspecified behavior the implementation is *required to
document* its choice for (plain `char`'s signedness is the vault's running example).
*Deeper meaning:* safe to rely on for a known, documented target; a portability bug
waiting to happen the moment that target changes without re-checking. *Related:* undefined
behavior, portability. *See:* `advanced/02_implementation_defined_and_portability.md`.

**File descriptor** — a small, non-negative integer the kernel uses to track an open
file/pipe/socket; the foundational abstraction of POSIX I/O. *Deeper meaning:* a `FILE*`
stream is a buffering layer built *on top of* one — this vault showed the exact line
(`_fillbuf`'s `read` call) where the two layers meet, in real, working code. *Related:*
stream, system call. *See:* `systems/03_file_descriptors_and_unix_io.md`.

**Stream** — ISO C's `FILE*` abstraction over a file descriptor, adding buffering
(`<stdio.h>`). *Related:* file descriptor, buffer. *See:* `libraries/01_stdio.md`.

**System call** — a request from a program to the kernel (`read`, `write`, `fork`, …) —
POSIX, not ISO C. *Deeper meaning:* this vault's central "C language vs. C library vs.
POSIX vs. kernel syscalls" four-layer distinction exists precisely to keep this term from
being conflated with "any library function." *Related:* POSIX, file descriptor. *See:*
`systems/04_system_calls_vs_libc_vs_posix.md`.

**Buffer** — a block of memory holding data temporarily, usually to batch up
otherwise-expensive operations (I/O, most prominently). *Deeper meaning:* this vault
demonstrated the buffering payoff directly and empirically — an unbuffered `getchar` makes
one system call per character; a buffered one, one per `BUFSIZ` characters. *Related:*
stream, system call. *See:* `systems/03_file_descriptors_and_unix_io.md` §4,
`libraries/01_stdio.md` §3.

**Allocator** — the subsystem (part of the C library, not the OS kernel) managing the
heap: `malloc`/`calloc`/`realloc`/`free`, implemented as a free-list of blocks, requesting
more memory from the OS (via `sbrk`/`brk`/`mmap`) only when its own free list can't satisfy
a request. *Related:* heap, fragmentation. *See:* `memory/05_dynamic_memory.md`.

**Process** — a running program: its own virtual address space, file descriptors, and
execution state, created by `fork` and (optionally) given a new program image by `exec`.
*Deeper meaning:* `fork` returns *twice* — once in each resulting process — the single
fact most responsible for process-model confusion when first learned. *Related:* file
descriptor, signal. *See:* `systems/05_processes_fork_exec_wait.md`.

**Signal** — an asynchronous (or synchronous) notification delivered to a process
(`SIGINT`, `SIGSEGV`, `SIGCHLD`, …), handled via a default action, ignored, or caught with
a handler function. *Deeper meaning:* ISO C's `<signal.h>` and POSIX's `sigaction`/`kill`
are two different views of the same mechanism, at two very different levels of
capability and safety guarantee. *Related:* process, async-signal-safety. *See:*
`libraries/07_setjmp_and_signal.md` §3, `systems/06_pipes_and_signals.md` §4.

---

## Everything else, by area

### Types & representation
**Integer promotion** — automatic widening of `char`/`short` to `int` in most
expressions. *See:* `fundamentals/03_expressions_and_operators.md` §3.
**Usual arithmetic conversions** — the full ladder governing mixed-type expressions.
*See:* same file, §3.
**Two's complement** — the near-universal (C23-mandated) negative-integer representation.
*See:* `fundamentals/02_types.md` §5, `advanced/04_modern_c_vs_historical_c.md` §4.
**Endianness** — the byte order of a multi-byte value in memory; portable code never
assumes one. *See:* `fundamentals/02_types.md` §5, `systems/07_sockets_and_networking.md` §3.

### Pointers, arrays & memory
**Array decay** — an array-typed expression's automatic conversion to a pointer to its
first element, in most contexts. *See:* `memory/02_pointers.md` §3.
**Dangling pointer** — a pointer to an object whose lifetime has ended. *See:*
`memory/02_pointers.md` §8.
**Aliasing** — two different expressions referring to the same memory. *See:*
`advanced/03_aliasing_and_optimization.md`.
**`restrict`** — a programmer's promise of non-aliasing, licensing optimizations. *See:*
`advanced/03_aliasing_and_optimization.md` §5.
**Fragmentation** — wasted memory from an allocator's free space being scattered
(external) or over-allocated (internal). *See:* `memory/05_dynamic_memory.md` §9.

### Functions & compilation
**Stack frame** — one function call's region of the call stack: return address, saved
registers, parameters, locals. *See:* `functions/01_functions_and_stack_frames.md` §6.
**Calling convention** — the ABI rules for how arguments/return values move between
caller and callee. *See:* same file.
**Object file / relocation / symbol table** — the compiled-but-not-yet-linked artifact,
and its two key internal structures. *See:* `systems/01_compilation_toolchain.md` §3,
`systems/02_linking_and_loading.md` §1–3.
**Static vs. shared library** — linked fully at build time vs. resolved at load/run time.
*See:* `systems/01_compilation_toolchain.md` §4–5.

### Library & I/O
**Variadic function** — a function accepting a variable-length argument list (`printf`,
most famously). *See:* `libraries/08_stdarg.md`.
**`errno`** — the global signaling a failed library call's specific error, never cleared
on success. *See:* `libraries/06_assert_and_errno.md` §2.
**Zombie / orphan process** — an exited-but-unreaped child; a running child whose
original parent has exited. *See:* `systems/05_processes_fork_exec_wait.md` §6.

### Debugging & tooling
**Sanitizer** — a compile-time-instrumented bug detector (ASan, UBSan, LSan). *See:*
`debugging/02_sanitizers_and_valgrind.md`.
**Core dump** — a crashed process's captured memory, for post-mortem debugging. *See:*
`debugging/01_toolchain_gdb.md` §3.

### Standards & process
**Constraint violation** — a rule the standard requires a compiler to diagnose (unlike
UB, which requires no diagnostic at all). *See:* `advanced/01_undefined_behavior.md` §4.
**Freestanding vs. hosted implementation** — a conforming C compiler needing only the
language plus four headers, vs. the full standard library. *See:*
`libraries/10_limits_float_stddef.md` §1.
