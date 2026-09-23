# Understanding Papy: A Systems & C++ Deep Dive

*A guided tour through `noahpop77/Papy` — a C++ JSON load-tester that went from 100 to 20,000 requests/sec — built to teach you the underlying Linux systems, networking, and C++ engineering concepts, not just what the code does.*

---

## How to read this

This document is organized in three layers, and you can read it in any order depending on what you want:

1. **The map** — what Papy is, how it's built, and how a request actually flows through it (sections 1–3).
2. **The story** — the 100 → 20,000 req/s optimization journey, but re-organized by *concept* rather than by timestamp, so each idea gets a real explanation instead of a one-liner (sections 4–7).
3. **The toolbox and the exercises** — the debugging/profiling tools involved, three real, verified things in the current code you can go fix yourself, a glossary, and further reading (sections 8–12).

Every code snippet below is taken verbatim from the repository you uploaded (`Papy-main.zip`), not paraphrased or reconstructed from the blog posts. Where the blog posts, the video transcript, and the README's own changelog disagree slightly on a number, I used the README's changelog as the source of truth, since it's the developer's own running log inside the actual repo, and it's more granular than the blog retelling.

---

## 1. What Papy actually is

Papy (short for *Papyrus*) is a command-line load-testing / fuzzing tool written in C++. You point it at a URL, tell it how many threads to run, and it fires HTTP or HTTPS requests at that target as fast as it can — either empty GETs, a static JSON file you give it, or one of two built-in **semi-random payload generators**: a League-of-Legends match generator (`--payload lol`) and a simpler "ocean" generator (`--payload ocean`). It was built by Brian Sawa originally to stress-test a personal project called M-Track (a match-history tracker), and its explicit design goal — stated directly in the README — was to be simple to build, dependency-light, portable, and capable of things a lot of load-testing tools aren't: HTTP *and* HTTPS, GET *and* POST, and payload generation *and* raw delivery, all in one small binary.

What makes it worth studying isn't the League of Legends payloads — it's that in ~1,300 lines of application code (excluding vendored dependencies), it touches almost every fundamental of systems programming you'll eventually need:

- **Concurrency**: OS threads, shared mutable state, atomics, mutexes, `thread_local`, and a real, reproducible race condition.
- **Networking**: HTTP vs HTTPS clients, TLS, headers, timeouts, connection reuse (or the lack of it — more on that later).
- **Memory & data structures**: `std::vector` growth and shifting costs, heap allocation avoidance, move vs copy.
- **The C++ build model**: translation units, the One Definition Rule, static initialization, `constexpr`.
- **Systems diagnosis**: profiling with flame graphs, debugging with GDB, memory-checking with Valgrind, and reading `htop` correctly.
- **I/O**: why writing to a terminal is a surprisingly expensive syscall, and why that dominates when everything else gets fast enough.

The rest of this document walks through all of that, using Papy's own code and its own documented journey as the case study.

---

## 2. The architecture

### 2.1 Directory layout (as it actually exists in the zip)

```
Papy-main/
├── makefile
├── README.md
├── docs/
│   ├── developerDocumentation.md
│   ├── userDocumentation.md
│   └── documentationImages/, excalidraw/   (diagrams, screenshots)
├── src/
│   ├── main.cpp                # entry point, thread orchestration
│   ├── cliHelper.{hpp,cpp}     # argv parsing, --help, banner
│   ├── threadWorks.{hpp,cpp}   # the worker-thread loop, metrics, gzip
│   ├── apiClient.{hpp,cpp}     # HTTP/HTTPS client wrapper (cpp-httplib)
│   ├── matchBuilder.{hpp,cpp}  # League-of-Legends payload generator
│   ├── oceanBuilder.{hpp,cpp}  # simpler payload generator
│   ├── configPayload.{hpp,cpp} # loads payload.json + fills {NUMBER-n}/{STRING-n} placeholders
│   ├── mapping.{hpp,cpp}       # all the static JSON "pools" random values are drawn from
│   ├── myRandom.{hpp,cpp}      # the entire randomness engine (this is where most of the story lives)
│   ├── millisecondClock.{hpp,cpp} # a tiny chrono wrapper for elapsed time
│   └── dependencies/           # vendored third-party code (see below)
└── tests/
    ├── testMain.cpp
    ├── timer.hpp                # a scope-based instrumentation profiler (by "The Cherno")
    └── unit/                    # a couple of small, hand-rolled unit tests
```

### 2.2 Vendored dependencies

Papy doesn't use a package manager — it vendors (copies the source directly into the repo) three libraries under `src/dependencies/`:

| Library | What it is | Why it's here |
|---|---|---|
| [`nlohmann/json`](https://github.com/nlohmann/json) (`json.hpp`) | A single-header C++ JSON library | Parses, builds, and serializes every payload |
| [`cpp-httplib`](https://github.com/yhirose/cpp-httplib) (`httplib.h`) | A single-header, *synchronous/blocking* HTTP/HTTPS client-and-server library | Sends the actual GET/POST requests |
| OpenSSL (headers + prebuilt `.so`/`.dylib`) | TLS implementation | Backs `httplib::SSLClient` so Papy can hit `https://` targets |
| zlib (`zlib.h` + `libz.so`) | The reference DEFLATE/gzip compression library | Backs `threadWorks::gzip_compress` |

Vendoring header-only libraries like `json.hpp` and `httplib.h` is a deliberate portability tradeoff: you don't need the user to have the right version of a package manager, you just need a C++ compiler. The cost is a bigger repo and no automatic security updates — you have to manually re-vendor a new version when one ships.

### 2.3 The build system

Papy uses a hand-written `makefile` rather than CMake or another meta-build system, which is appropriate for a project this size. A few things worth understanding from it:

```makefile
CXXFLAGS_COMMON = -I$(OPENSSL_DIR)/include -I$(ZLIB_DIR)/include -MMD -MP -O3
CXXFLAGS = -std=c++23 $(CXXFLAGS_COMMON)      # macOS uses -std=c++2b instead
LDFLAGS  = -L$(OPENSSL_DIR)/lib -L$(ZLIB_DIR)/lib -lssl -lcrypto -lz
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
```

- **`-O3`** tells GCC to apply its most aggressive optimizations: function inlining, loop unrolling, auto-vectorization, dead-code elimination. This matters a lot for what follows — several of the hand-written optimizations later in this document (like `constexpr` arrays and `static` distributions) work *with* `-O3`, by removing obstacles (heap allocations, non-constant sizes, repeated construction) that would otherwise stop the optimizer from doing its job.
- **`-MMD -MP`** auto-generates per-file dependency (`.d`) files so that `make` only recompiles a `.cpp` when a header it actually includes has changed — standard incremental-build hygiene.
- **`Darwin` (macOS) vs Linux branching**: OpenSSL and zlib are linked differently per platform (Homebrew paths on macOS, the vendored directory on Linux), and the C++ standard flag changes from `-std=c++23` to `-std=c++2b` because, at the time this was written, Apple's Clang hadn't caught up to GCC's naming for the same (then-still-being-finalized) C++23 standard.
- Every `.cpp` in `src/` is compiled independently into its own `.o` object file, and the linker (`g++ $(OBJ_FILES) -o $(TARGET) $(LDFLAGS)`) stitches them together at the end. This "one object file per source file" model is exactly what makes the linker bug in section 6.4 possible — keep it in mind.

### 2.4 Module dependency shape

The repo ships its own architecture sketch (`docs/documentationImages/papyArchitecture.png`), which is slightly aspirational — it references a `common.hpp` and `utilities.cpp` that don't exist in the current tree — but the underlying idea it's expressing still holds: **every `.cpp` file owns exactly one `.hpp` file with the same name**, dependencies (`json.hpp`, `httplib.h`, OpenSSL) are included only where actually needed, and `main.cpp` is the only file that has to know about thread orchestration. Reconstructed against the actual current source tree, the real dependency shape looks like this:

```
main.cpp
 ├── cliHelper.hpp        (parse argv, print banner/help)
 └── threadWorks.hpp      (spawn + run N worker threads)
        │
        ├── apiClient.hpp ──── httplib.h ──── OpenSSL (HTTPS only)
        │
        ├── matchBuilder.hpp ─┬─ mapping.hpp     (static JSON data pools)
        │                     └─ myRandom.hpp ─── json.hpp
        │
        ├── oceanBuilder.hpp ─── mapping.hpp, myRandom.hpp
        │
        ├── configPayload.hpp ── mapping.hpp, myRandom.hpp
        │
        ├── millisecondClock.hpp
        │
        └── gzip_compress()  ── zlib.h
```

Notice `matchBuilder`, `oceanBuilder`, and `configPayload` are siblings that all depend on `mapping` and `myRandom` but don't know about each other — that's a clean separation, and it's also exactly why (as you'll see in section 11) an optimization applied to one of them doesn't automatically apply to the others.

---

## 3. Program lifecycle: from `main()` to a request on the wire

### 3.1 Startup and thread orchestration

```cpp
// main.cpp
cliHelper::parseArguments(argc, argv, numThreads, payloadCount, rateLimit, ramp, spike,
                           target, endpoint, verbose, payload, parameter, bearer, authorization);
cliHelper::printBanner(target, endpoint, numThreads, rateLimit);

signal(SIGINT, threadWorks::signalHandler);

std::vector<std::thread> threads;
threads.reserve(numThreads);
for (int i = 0; i < numThreads; ++i) {
    threads.emplace_back(&threadWorks::runWorkerThread, target, endpoint, verbose,
                          payloadCount, rateLimit, ramp, spike, payload, parameter,
                          bearer, authorization);
}
for (auto& thread : threads) {
    if (thread.joinable()) thread.join();
}
```

A few things worth naming explicitly:

- **`--threads N` maps 1:1 to real OS (kernel) threads.** `std::thread` on Linux is a thin wrapper around `pthread_create`, which is a genuine kernel thread — not a green thread, a fiber, or a coroutine. Every one of those threads is independently scheduled by the Linux kernel's scheduler (CFS, the Completely Fair Scheduler, in mainline Linux) across whatever CPU cores are available. This is why the README explicitly warns that assigning more threads than your CPU physically has doesn't help throughput once you're CPU-bound: the kernel just time-slices more threads across the same cores, adding context-switch overhead instead of parallelism. (It *does* help once you're I/O-bound instead — see section 6.4.)
- **`threads.reserve(numThreads)`** pre-allocates the vector's backing storage so it never has to reallocate while you're pushing thread objects into it. `std::thread` is move-only (it owns a kernel thread handle that can't be copied), so a reallocation mid-loop would have to *move* every already-constructed `std::thread` object into new storage — safe, but wasted work `reserve` avoids for free. This is a small instance of a pattern you'll see repeatedly in this codebase: know your final size up front, reserve it once, never pay for growth.
- **`signal(SIGINT, threadWorks::signalHandler)`** installs a handler for Ctrl+C. The handler itself does almost nothing dangerous — it locks a mutex, prints a message, and flips an `std::atomic<bool>` (`isProgramActive`) to `false`. That's deliberate: POSIX signal handlers run in a very restricted context (they can interrupt *any* code at *any* point, including in the middle of a `malloc` call), so the list of functions safe to call from one is short. Setting an atomic flag and letting the worker threads notice it on their own time (`while (isProgramActive) { ... }`) is the standard, safe pattern for "stop cooperatively" — the alternative (doing real cleanup work *inside* the handler) is a common source of subtle, hard-to-reproduce crashes in signal-heavy programs.

### 3.2 The worker-thread loop

```cpp
// threadWorks.cpp
void threadWorks::runWorkerThread(const std::string& targetURL, const std::string& endpoint, ...) {
    millisecondClock clock;
    apiClient client(targetURL);      // ← ONE apiClient per thread, built once
    client.setEndpoint(endpoint);
    client.setParameter(parameter);

    clock.start();
    while (isProgramActive) {
        sendRequest(client, verbose, payload, clock, bearer, authorization);
        if (payloadCount > 0 && --payloadCount == 0) break;
        if (rateLimit) std::this_thread::sleep_for(std::chrono::milliseconds(rateLimit));
        if (ramp > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(ramp));
            ramp = std::max(ramp / 2, 1);
        } else if (spike > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(myRandom::generateRandomInt(0, spike)));
        }
    }
}
```

Each thread constructs its **own** `apiClient` (its own connection object) once, outside the loop, and reuses it for every request that thread sends. Nothing is shared between threads at the connection level — each thread has an entirely independent path to the network. This is a good instinct for a load generator: sharing one connection across threads would force serialization (or heavy locking) right at your hottest point.

`--ramp` and `--spike` are two different traffic *shapes*, and it's worth being precise about the difference since they model two real-world load patterns:
- **Ramp**: start with a `ramp`-millisecond delay between requests, then halve it after every request (`ramp = max(ramp/2, 1)`) — an exponentially *accelerating* send rate, simulating load that gradually ramps up until it saturates your hardware.
- **Spike**: sleep a *uniformly random* amount of time between `0` and `spike` milliseconds before every request — bursty, unpredictable traffic, closer to how real user traffic actually looks than a perfectly steady rate would be.

### 3.3 Sending a request and updating metrics

```cpp
// threadWorks.cpp — sendRequest(), abbreviated
if (payload.empty()) {
    response = client.sendGETRequest(bearer, authorization);
} else if (payload == "lol") {
    nlohmann::json lolPayload = matchBuilder::randomMatch();
    std::string compressedLolPayload = gzip_compress(lolPayload.dump());
    client.setPayload(compressedLolPayload);
    response = client.sendPOSTRequest(bearer, authorization);
} else if (payload == "ocean") {
    client.setPayload(oceanBuilder::randomOcean().dump());
    response = client.sendPOSTRequest(bearer, authorization);
}
// ... "payload.json" and arbitrary-file-path branches follow the same POST pattern
```

and then, for the non-verbose path, the metrics/printing block:

```cpp
std::lock_guard<std::mutex> guard(consoleMutex);

int elapsedSec = clock.elapsedMilliseconds() / 1000;
packetsPerSecond = (elapsedSec > 0) ? (totalPayloadsSent / elapsedSec) : 0;
if (response == "200") totalPayloadsSuccessful++;
totalPayloadsSent++;

static auto last_update_time = std::chrono::steady_clock::now();
auto now = std::chrono::steady_clock::now();
auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update_time);
constexpr unsigned int DELAY_MS = 75;
if (elapsed.count() < DELAY_MS) return;   // skip the print — see section 7
last_update_time = now;

std::cout << "\rTotal Sent: " << totalPayloadsSent << " | Successful: " << totalPayloadsSuccessful
          << " | Failed: " << (totalPayloadsSent - totalPayloadsSuccessful)
          << " | Packets/s: " << packetsPerSecond
          << " | Elapsed Time: " << clock.elapsedMilliseconds() << "     " << std::flush;
```

Notice that `totalPayloadsSent`, `totalPayloadsSuccessful`, and `packetsPerSecond` are already declared `std::atomic<int>` — so why is this whole block *also* wrapped in a `std::mutex` (`consoleMutex`)? Because atomicity per-variable isn't the same as atomicity across a *sequence* of related operations. This block reads two atomics to compute `packetsPerSecond`, conditionally increments a third, and then prints a coherent snapshot of all of them together — without the mutex, one thread could print a `packetsPerSecond` computed from a `totalPayloadsSent` that a different thread is simultaneously incrementing mid-read, or two threads' `std::cout <<` chains could interleave their output character-by-character (`operator<<` chains are not atomic as a whole, even though each individual write into the stream buffer is thread-safe at the OS level for small writes). The mutex here is buying **consistency of a composite snapshot** and **serialized, non-interleaved terminal output** — two things individual atomics can't give you on their own.

---

## 4. The optimization journey — the numbers

Before getting into *why* each change worked, here's the full, reconciled timeline, taken from the README's own changelog (the most granular primary source in the repo):

| # | Result (req/s) | Change | Concept cluster |
|---|---|---|---|
| 1 | ~100 | Starting point | — |
| 2 | 170 | gzip-compress the `lol` payload body before sending | Bandwidth vs. CPU tradeoff |
| 3 | 360 | Stop recreating the random device/generator on every `generateRandomInt()` call | Randomness engineering |
| 4 | 400 | Pull 70 items in **one** JSON parse instead of 70 separate parses | JSON/data-structure batching |
| 5 | 830 | Apply the same batching to champions, summoners, keystones, secondary runes | JSON/data-structure batching |
| 6 | 870–905 | Store the mapping objects as native `nlohmann::json` instead of `std::string` that gets re-parsed every call | JSON/data-structure batching |
| 7 | 2,400 | Remove the random device from `generateRandomNumberString`, `generateRandomString`, `getRandomBool` too | Randomness engineering |
| 8 | 3,000 | Make the character pool a `constexpr char[]`, its length a `constexpr size_t` | Compile-time optimization |
| 9 | 3,300 | `reserve()` the output string up front; make the distribution `static` | Compile-time / allocation optimization |
| 10 | ~6,000 | Consolidate the randomization code (incl. `getRandomVectorFromJSON`) into one class sharing one random engine | Randomness engineering |
| 11 | 8,000 | Remove a redundant vector copy + duplicate `reserve` — pass the destination vector by reference | Allocation / move semantics |
| 12 | 7,600 | **Segfault under load** → fix: `thread_local` random engine per thread instead of one shared global | Concurrency correctness |
| 13 | 12,000 (real DB-backed endpoint) / **20,000** (mock endpoint, no DB) | Throttle terminal redraws; crank thread count 16 → 120 | I/O-bound scheduling |

Notice something important in the shape of this table: **steps 12 and 13 are not "more speed," they're "the code stops lying to you."** Step 12 traded raw throughput *down* (8,000 → 7,600) in exchange for correctness — an important value judgment we'll come back to. Step 13 isn't a code optimization in the usual sense at all; it's the discovery that a completely different part of the system (the terminal) had been the real bottleneck the entire time, hiding underneath what looked like a CPU problem.

The next four sections group these thirteen steps by underlying concept, so you actually learn the transferable idea, not just "and then it got faster."

---

## 5. Concept cluster 1: Randomness engineering

This is the single biggest recurring theme in Papy's optimization story (steps 3, 7, 10, 12), and it's worth understanding properly because "don't recreate a random number generator on every call" generalizes to "don't recreate *any* expensive object inside a hot loop."

### 5.1 Why constructing a `std::random_device` repeatedly is slow

```cpp
// The ORIGINAL, slow pattern (from the blog post, no longer in the current code):
int myRandom::generateRandomInt(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(min, max);
    return distrib(gen);
}
```

`std::random_device` is meant to be a source of **non-deterministic entropy** — true randomness pulled from the operating system, as opposed to a deterministic, seedable algorithm. On Linux with GCC's standard library (which is what this project is built with — `g++`), that typically means the implementation opens a handle to the kernel's entropy source (historically `/dev/urandom`, or the `getrandom()` syscall on newer kernels) *every time you construct one*, and closes it when the object is destroyed. Opening and closing a file/syscall handle means crossing from user space into kernel space — a context switch — which is orders of magnitude more expensive than ordinary user-space arithmetic. Doing that thousands of times a second, just to seed a generator you're about to throw away, is the single most expensive thing the original code was doing per call.

`std::mt19937` is the Mersenne Twister algorithm — a deterministic pseudo-random number generator (PRNG) with roughly 2.5 KB of internal state (624 32-bit words). Seeding it (`mt19937 gen(rd())`) requires initializing that entire internal state array from the seed. That's real, non-trivial work too, though smaller than the entropy-source cost above — and it's completely unnecessary to repeat on every call, because a single seeded generator can produce an effectively endless, statistically-uniform stream of numbers on its own.

The fix, applied incrementally across steps 3, 7, and 10, is simply: **construct the entropy source and the generator exactly once**, and keep reusing the same generator object for every subsequent call:

```cpp
// myRandom.cpp — as it exists today
thread_local std::random_device myRandom::rd;
thread_local std::mt19937 myRandom::gen(myRandom::rd());

int myRandom::generateRandomInt(int min, int max) {
    static std::uniform_int_distribution<> distrib(min, max);
    return distrib(gen);
}
```

Note that even `std::uniform_int_distribution` — a genuinely lightweight object — is hoisted out with `static` so it's constructed exactly once per process (well, per thread, given the enclosing function is called from many threads — more on that below) rather than once per call. This is the general lesson: **profile to find what's actually expensive, then look at everything in the hot path with the question "does this need to happen every single time, or just once?"**

### 5.2 Global vs. `thread_local`, and the segfault it caused

The intermediate version of this code (used between roughly steps 10 and 11, at ~6,000–8,000 req/s) made `rd` and `gen` **plain global/static class members**, shared by every thread:

```cpp
// The version that caused segfaults (not thread_local yet):
std::random_device myRandom::rd;
std::mt19937 myRandom::gen(myRandom::rd());
```

This is where a real, reproducible concurrency bug entered the picture. `std::mt19937::operator()` (invoked every time `distrib(gen)` runs) doesn't just *read* the generator's internal state — it *mutates* it, advancing the sequence for next time. If two threads call `distrib(gen)` at the same instant on the *same shared* `gen` object, you get two threads concurrently reading-and-writing the same non-atomic memory with no synchronization between them. In the C++ memory model, that's a **data race**, and a data race is formally **undefined behavior** — the compiler and hardware are allowed to do anything, including corrupting the object's internal state such that a subsequent read walks off the end of an internal array. That's exactly what manifested as an intermittent segmentation fault.

Two things made this bug genuinely hard to find, and both are worth internalizing as general debugging lessons:

1. **It was load-dependent.** At 100 req/s, the window during which two threads' calls could actually overlap in time was vanishingly small, so the race essentially never fired. At 8,000 req/s, that same tiny window is hit constantly. This is a classic "Heisenbug": the probability of hitting a race scales with throughput and thread count, so a program can look perfectly correct for months and then start crashing the moment you make it faster — which is exactly backwards from most people's intuition that "fast" and "broken" are unrelated.
2. **Standard memory tools didn't catch it.** Valgrind's Memcheck tool (see section 9) is designed to catch invalid reads/writes, use-after-free, and leaks by instrumenting every memory access — but it does *not* reliably detect data races between correctly-in-bounds memory accesses from different threads, because from Memcheck's point of view, both threads are doing "legal" reads and writes to memory they own; the problem is purely about the *lack of ordering* between them. (A tool built specifically for this — ThreadSanitizer — would have caught it far faster; see section 9.)

The fix was to give every thread its own private copy of the generator, using the `thread_local` storage-duration specifier:

```cpp
thread_local std::random_device myRandom::rd;
thread_local std::mt19937 myRandom::gen(myRandom::rd());
```

`thread_local` means: instead of one instance of this object existing for the whole process, **each OS thread gets its own separate instance**, lazily constructed the first time that thread touches it. There is no longer anything to race over, because no two threads ever touch the same `gen` object. This is a *lock-free* solution to the concurrency problem — the alternative would have been to wrap every call in a `std::mutex`, which would work but would force every thread to serialize on every single random draw, which for a program whose entire job is "generate lots of randomness, fast" would have been a much worse tradeoff. `thread_local` sidesteps contention entirely at the cost of a small amount of extra memory (one generator per thread instead of one total) and losing a single shared random sequence (irrelevant here, since nothing about Papy's correctness depends on a single global stream of numbers).

The throughput cost of this fix — 8,000 down to 7,600 req/s — most plausibly comes from `thread_local` variables typically requiring one extra layer of indirection to access (through a thread-control-block pointer) compared to a plain global, which sits at a single fixed address. That per-access cost, paid millions of times, adds up to a measurable but small regression — a completely reasonable price for turning undefined behavior into defined, correct behavior. As the developer put it in the blog post: *"there's no point in going fast if you're just going to drive off the side of the road the second there's a turn."*

### 5.3 A bonus: the linker error that pointed at the real bug

While chasing this, a build attempt threw this at the developer:

```
/usr/bin/ld: obj/myRandom.o:(.bss+0x0): multiple definition of `myRandom::rd';
             obj/matchBuilder.o:(.bss+0x0): first defined here
/usr/bin/ld: obj/matchBuilder.o: in function `matchBuilder::getRandomVectorFromJSON(...)':
             matchBuilder.cpp:(.text+0x1aeb): undefined reference to `matchBuilder::gen'
```

This is worth understanding on its own, because it's a textbook illustration of the **One Definition Rule (ODR)** and the C++ compilation model. In C++, a `.cpp` file is compiled independently into its own object file (a *translation unit*); a `static` (or namespace-scope) variable can be *declared* (with `extern`) in as many headers as you like, but it must be *defined* — given actual storage — in exactly **one** translation unit. "Multiple definition" means two different `.o` files each tried to allocate storage for the same symbol (`myRandom::rd`), so the linker can't decide which one is real. "Undefined reference" is the complementary error: somewhere, code tries to *use* a symbol (`matchBuilder::gen`) that was never actually defined anywhere the linker could find. In this case, the getRandomVectorFromJSON logic had at one point lived partly in `matchBuilder` with its own copy of the random engine, colliding with `myRandom`'s copy — a direct symptom of the code being mid-refactor as the randomization logic was consolidated into one class (step 10 in the table). It's a good reminder that **linker errors are almost always more informative than they look** — this one was pointing, correctly, straight at the exact class boundary that was about to matter for the segfault investigation.

---

## 6. Concept cluster 2: JSON and data-structure performance

### 6.1 From string-that-gets-parsed to native, pre-built JSON

Early Papy stored things like the champion list as a `const std::string` containing raw JSON text, and called `nlohmann::json::parse()` on it every time a value was needed. `mapping.cpp` today instead declares these as **native `nlohmann::json` objects**, built once:

```cpp
// mapping.cpp
namespace mapping {
   const nlohmann::json KEYSTONES_JSON = {
        {"8369", "First Strike"}, {"8124", "Predator"}, /* ... */
   };
   const nlohmann::json ITEMS_JSON = { /* ~200 entries */ };
   const nlohmann::json CHAMPIONS_JSON = { /* ~170 entries */ };
   const nlohmann::json MATCH_TEMPLATE_JSON = { /* the full match skeleton */ };
}
```

Because these are namespace-scope objects with *static storage duration*, the C++ runtime constructs them exactly once, during static initialization, **before `main()` even runs** — not on the first request, and never again after that. Every subsequent access is just reading an already-built in-memory tree; no parsing, no allocation, no tokenizing ever happens again for the lifetime of the process. Parsing JSON text into a DOM (document object tree) is genuinely real work — the parser has to tokenize the text, validate syntax, and allocate a node (and, for `nlohmann::json`'s default backing container, typically a `std::map`-like or `std::vector`-like structure per object/array) for every key and value. Doing that once at startup instead of thousands of times a second is a straightforward, large win — this is the same idea as section 5's "construct expensive things once," just applied to a parser instead of a random engine.

### 6.2 Batching: turning N parses into 1

The core insight behind steps 4–5 is best explained with the exact numbers involved. A single simulated match needs **70** random items (10 players × 7 items each). The unoptimized approach called a "get one random value" function 70 separate times:

```cpp
// The slow shape (conceptually — this exact function no longer exists,
// but the pattern is what test_getRandomFromJson.cpp still demonstrates):
std::string getRandomFromJson(const nlohmann::json& jsonObject) {
    std::vector<std::string> keys;
    for (auto it = jsonObject.begin(); it != jsonObject.end(); ++it) keys.push_back(it.key());
    // ... pick one random key and return it
}
// called 70 times ⇒ the whole ITEMS_JSON key list gets rebuilt 70 times
```

If the items pool has *n* entries, building the key list once costs O(n). Doing that 70 times costs O(70·n) — the same fixed-size work repeated 70 times over, for no reason, since the pool doesn't change between calls. The current implementation collects the key list **once**, then draws 70 indices from it:

```cpp
// myRandom.cpp — getRandomVectorFromJSON, current implementation
bool myRandom::getRandomVectorFromJSON(std::vector<std::string>& participantData,
                                        const nlohmann::json& jsonObject, size_t count) {
    participantData.reserve(count);
    std::vector<std::string> keys;
    if (!getKeysFromJsonObject(keys, jsonObject)) return false;   // O(n), done ONCE

    std::uniform_int_distribution<> distrib(0, keys.size() - 1);
    for (int i = 0; i < count; i++) {
        participantData.push_back(keys[distrib(gen)]);            // O(1) each
    }
    return true;
}
```

This changes the total cost from **O(count · n)** to **O(n + count)** — for `count = 70` and `n` in the hundreds, that's roughly a 70x reduction in the dominant term. It's worth being precise here: the blog post describes this as going from "O(n)" to "O(1)," which is a bit loose — what actually collapsed from *linear-in-count* to *constant-in-count* was the **number of times the key list gets rebuilt** (70 rebuilds → 1 rebuild), not the total work of the function, which is still proportional to the pool size for that one rebuild. Getting comfortable with this level of precision — *what specifically* is O(1) now, versus what's simply been paid once instead of *count* times — is a genuinely useful habit to build; "it went from O(n) to O(1)" is often a slightly informal shorthand for "we moved a cost from inside the loop to outside it," and it's worth being able to say exactly which cost, and where it moved.

### 6.3 `pop_back()` vs. `erase(begin())`: how `std::vector` is actually laid out

Once you have a batch of 70 random items in a `std::vector<std::string>`, you need to hand one out per player without reusing any. The first version pulled from the *front*:

```cpp
participant["championName"] = participantChamp[0];
participantChamp.erase(participantChamp.begin());
```

`std::vector` stores its elements in one **contiguous** block of memory (that's what makes indexing `O(1)` and cache-friendly in the first place). Removing the first element means every remaining element has to be physically moved one slot to the left to close the gap — a `memmove`-style shift over *n − 1* elements. That's an **O(n)** operation, and doing it once per player, 70 times per match, for a vector that starts at length 70 and shrinks by one each time, costs roughly `70 + 69 + 68 + ... + 1` element-moves in total — quadratic, **O(n²)**, growth in the batch size.

The fix — used throughout the current `matchBuilder.cpp` — pulls from the *back* instead:

```cpp
participant["championName"] = participantChamp.back();
participantChamp.pop_back();
```

`.back()` just reads the last element (no shifting, O(1)), and `.pop_back()` just decrements the vector's internal `size` counter (again O(1), no data movement at all — the memory isn't even zeroed, just marked unused). Doing that 70 times costs a flat **O(n)**, not O(n²). Since all of this data is randomized in the first place, there's zero behavioral difference between "hand out the first item" and "hand out the last item" — this is a case where you get a real algorithmic-complexity improvement for literally no downside, purely by picking the cheap end of the data structure to work from. Visually:

```
erase(begin())  — every remaining element shifts left one slot, every single time:
[A][B][C][D][E]  →  erase A  →  [B][C][D][E][ ]   (4 elements moved)

pop_back()      — nothing moves, just the length shrinks:
[A][B][C][D][E]  →  pop_back  →  [A][B][C][D][ ]   (0 elements moved)
```

### 6.4 `reserve()`, growth factors, and pass-by-reference

`std::vector` and `std::string` are both *dynamic* containers: when you push past their current capacity, they allocate a bigger buffer (typically by *doubling* capacity, in GCC's standard library — an amortized-growth strategy that keeps the *average* cost of `push_back` at O(1) even though any individual reallocation is O(n), since it has to copy/move every existing element into the new buffer). `.reserve(n)` lets you skip all of that by allocating the final capacity you need up front, in one shot — used throughout this codebase, e.g. inside `generateRandomString`:

```cpp
std::string randomStr;
randomStr.reserve(length);       // one allocation, sized exactly right, instead of several doublings
for (size_t i = 0; i < length; ++i) randomStr += chars[distrib(gen)];
```

Step 11 (6,000 → 8,000 req/s) applied the same idea to a subtler case. The old shape of `getRandomVectorFromJSON` looked like this:

```cpp
// OLD shape (paraphrased from the README's own note on this change):
std::vector<std::string> participantItems;
participantItems.reserve(1400);                                        // reserve #1 — wasted
participantItems = getRandomVectorFromJSON(mapping::ITEMS_JSON, 70);    // returns a NEW vector...
```

...where `getRandomVectorFromJSON` itself built and returned its *own* freshly-reserved vector by value. The caller's `reserve(1400)` was completely wasted — the moment the assignment (`participantItems = ...`) runs, the caller's carefully pre-sized buffer gets replaced wholesale by the function's return value, deallocating the caller's buffer and taking ownership of a brand new one. The fix (the version you saw in section 6.2) takes the destination vector **by reference** and fills it in place, so there's exactly one `reserve` call and zero extra allocation or copying:

```cpp
bool myRandom::getRandomVectorFromJSON(std::vector<std::string>& participantData, ...) {
    participantData.reserve(count);   // the ONLY reserve — fills the caller's own buffer directly
    ...
}
```

This is worth flagging because it cuts slightly against a common piece of modern-C++ advice: "just return by value, the compiler's move semantics and Return Value Optimization (RVO/NRVO) will make it free." That's often true — but RVO only helps when the *caller* doesn't already own a buffer it wants filled in place. Here, the caller already had one, pre-sized; "output parameter by reference" beat "return by value" precisely because it let the function write directly into memory the caller already controlled, instead of building a separate object and handing it over. In a hot path called thousands of times a second, that's a meaningful, measurable difference — which is exactly what the jump from 6,000 to 8,000 req/s shows.

---

## 7. Concept cluster 3: compile-time and "pay once" optimizations

Steps 8–9 (3,000 → 3,300 req/s) are smaller in magnitude than the others but illustrate a different, complementary idea: **let the compiler do work at compile time instead of runtime, wherever the value is actually known at compile time.**

```cpp
std::string myRandom::generateRandomString(size_t length) {
    constexpr char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    constexpr size_t chars_len = sizeof(chars) - 1;

    static std::uniform_int_distribution<> distrib(0, chars_len - 1);
    std::string randomStr;
    randomStr.reserve(length);
    for (size_t i = 0; i < length; ++i) randomStr += chars[distrib(gen)];
    return randomStr;
}
```

- `constexpr char chars[]` — a fixed character pool that never changes — is baked directly into the binary's read-only data section at compile time. Compare that to a `std::string` literal, which (even accounting for small-string optimization) still runs actual constructor logic on every entry into the function; a `constexpr` C-array pays that cost exactly zero times, ever, at runtime.
- `constexpr size_t chars_len = sizeof(chars) - 1` computes the pool's length *at compile time* — the compiler literally writes the number into the binary — instead of calling something like `.size()` or `strlen()` at runtime on every single call.
- `static std::uniform_int_distribution<> distrib(...)` is a function-local static: C++11 guarantees its initialization is both **lazy** (it only runs the first time this function is ever called, across any thread) and **thread-safe** (the compiler inserts a hidden guard check around it — small, but non-zero, overhead on every call after the first, in exchange for never re-constructing the distribution object again).

None of these individually is dramatic, but together they remove essentially everything from the hot loop except the one thing that actually has to happen every iteration: drawing a random index and appending a character. This is the same principle as section 5 and section 6, at a finer grain — and it's also exactly the kind of code `-O3` (section 2.3) is best at further optimizing once you've gotten it out of its way.

---

## 8. Concept cluster 4: the terminal I/O bottleneck and thread scheduling

This is the most conceptually surprising part of the story, and the README calls it out explicitly as "The Great Realization." Two odd observations kicked it off, both quoted from the README:

1. Running with 16 threads on a 16-thread-capable CPU, `htop` showed only **60% CPU usage** — if 16 threads were genuinely saturating 16 hardware threads with real work, usage should have been near 100%.
2. Adding `print` statements for debugging made throughput **drop**, every time — which is backwards if the bottleneck were pure computation, since a `std::cout` call is comparatively tiny computationally.

### 8.1 Why writing to a terminal is expensive

`std::cout << ... << std::flush` ultimately becomes a `write()` syscall targeting a TTY (terminal) device. Writing to a terminal is meaningfully more expensive than writing the same bytes to a regular file or a pipe, because the terminal driver has to actually *render* the update — interpret the carriage return (`\r`), redraw the line, handle any escape sequences — rather than just appending bytes to a buffer. In `threadWorks::sendRequest`, this happens on *every single completed request*, and it's additionally serialized behind `consoleMutex`, so at high throughput, most threads spend most of their time either blocked waiting for the mutex, or blocked inside the `write()` syscall itself, rather than doing useful work.

Here's the key conceptual point: **a thread that's blocked — on a mutex, on a syscall, on I/O of any kind — is not "using" the CPU from the scheduler's point of view.** The Linux scheduler takes a blocked thread off the CPU entirely and gives that core to something else runnable. So `htop` showing 60% usage across "16 busy threads" wasn't lying — it was accurately reporting that, at any given instant, a good chunk of those 16 threads genuinely weren't running; they were queued up waiting for their turn to write to the same terminal. **Low CPU utilization under load is itself a diagnostic signal**, and it points *away* from "the computation is too slow" and *toward* "something is serializing/blocking your otherwise-parallel work." That's a general debugging heuristic worth keeping: if your program is slow *and* your CPU usage is well below 100% across all cores, you almost certainly have a contention or I/O bottleneck, not a compute bottleneck, and profiling for "hot functions" (which is what a flame graph shows you — see section 9) won't find it, because the code isn't spending time *executing*, it's spending time *waiting*.

### 8.2 The two-part fix

**Part one — throttle the redraw**, exactly the same "pay once, not every time" idea from sections 5–7, just applied to I/O instead of computation:

```cpp
static auto last_update_time = std::chrono::steady_clock::now();
auto now = std::chrono::steady_clock::now();
auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update_time);
constexpr unsigned int DELAY_MS = 75;      // ~13 redraws/sec — comfortably above what a human eye needs
if (elapsed.count() < DELAY_MS) return;    // most calls hit this early return and skip printing entirely
last_update_time = now;
// ... only NOW does the actual std::cout << ... happen
```

This is the exact same shape as rate-limiting logs or metrics emission in any production system: the *data* (a completed request) happens thousands of times a second, but the *display* of that data only needs to update as fast as a human can perceive — roughly 10–20 times a second is already smoother than most people can consciously track. Decoupling "how often does the underlying event happen" from "how often do we render/report it" is directly transferable to logging, progress bars, dashboards, and telemetry pipelines in any language.

**Part two — once the terminal stopped being the bottleneck, the CPU limit reappeared, and the response was to massively increase the thread count**, from 16 up to 120, on a 16-hardware-thread machine. This looks like it contradicts the advice from section 3.1 ("more threads than cores doesn't help") — but it doesn't, because the *nature* of the workload changed. Once the terminal contention was removed, each thread's request loop spends most of its wall-clock time not computing, but *blocked waiting on the network* (inside `cpp-httplib`'s `Post()`/`Get()` call, waiting for a TCP connection and a response). A thread blocked on network I/O, like a thread blocked on a mutex, isn't consuming its core — so the kernel can happily interleave far more of these mostly-idle threads than it has physical cores, because at any given instant, most of them are simply waiting, not running. The right thread count is a function of *what you're bottlenecked on*, not a fixed multiple of your core count: for CPU-bound work, threads-beyond-cores mostly adds scheduling overhead; for I/O-bound work (this one), it lets you have many requests "in flight" simultaneously, which is exactly what a load tester wants.

### 8.3 The honest caveat about the final numbers

The README is explicit that the headline 20,000 req/s figure was measured against a **mock Go endpoint on the same machine**, doing nothing but parsing the JSON body and returning `200` — no database, no business logic. Against a **real** Go REST API backed by PostgreSQL, still on the same machine, throughput was ~12,000 req/s instead. Two things are worth learning from that gap, not just the number itself:

- **Client and server sharing one box contend for the same CPU, memory bandwidth, and (loopback) network stack.** Papy's own worker threads were pinned near 100% CPU on that machine, which means the PostgreSQL-backed service was fighting Papy for the same cores it was trying to measure. Any number you get from a "load generator and system-under-test on the same box" setup should be read as a rough sanity check, not a true capacity number — a real capacity test runs the generator on separate hardware from the target, and watches the *target's* resource usage (not just the generator's throughput counter) to find the actual ceiling.
- **This entire benchmark is over loopback (`127.0.0.1`/`localhost`), not a real network.** The blog post and README both note this explicitly ("net code still needs improvement," "these numbers are testing locally"). Loopback traffic skips real network hardware, has effectively zero propagation delay, and never experiences packet loss or congestion — so what's actually being measured here is almost entirely userspace CPU + syscall + threading overhead, not networking performance in the sense of "how would this behave over a real WAN." A genuinely remote target would add, at minimum, a full TCP three-way handshake's round-trip time before any HTTP data can even be sent, plus a TLS handshake's worth of additional round-trips for `https://` targets, plus normal network variability (queuing, congestion control ramping up slowly at the start of a connection, occasional retransmission on packet loss) — none of which loopback testing exercises at all.

That second point leads directly into the most significant *unaddressed* opportunity in the current code — covered next.

---

## 9. Case study: reading the networking layer critically

`apiClient.cpp` wraps `cpp-httplib`'s `Client`/`SSLClient` classes, choosing between them based on the target URL's scheme:

```cpp
apiClient::apiClient(const std::string& serverAddress) {
    if (serverAddress.find("https://") == 0) {
        sslClient = std::make_unique<httplib::SSLClient>(serverAddress.substr(8));
    } else if (serverAddress.find("http://") == 0) {
        client = std::make_unique<httplib::Client>(serverAddress.substr(7));
    } else { /* ... error ... */ }
}
```

Both are held as `std::unique_ptr` — RAII (Resource Acquisition Is Initialization) in action: the connection object's lifetime is tied exactly to the `apiClient`'s lifetime, and it's automatically cleaned up (socket closed, TLS context torn down) whenever the owning `apiClient` goes out of scope, with no manual `delete` or leak risk. `sendPOSTRequest` sets a 7-second read/write timeout on the SSL client — a sensible defensive choice for a load-testing tool specifically, since you never want one hung connection to permanently stall a worker thread that's supposed to be firing thousands of requests.

**Here's the part worth reading carefully rather than taking for granted.** Because `runWorkerThread` constructs exactly one `apiClient` per thread and reuses it across the entire request loop (section 3.2), it's tempting to assume that means each thread reuses one underlying TCP (and, for HTTPS, one TLS) connection across all of its requests — i.e., that HTTP **keep-alive** is in effect. Checking `httplib.h` directly (it's vendored right there in the repo) shows that's *not* actually what happens:

```cpp
// httplib.h — inside the ClientImpl class definition:
bool keep_alive_ = false;                    // ← the default

// httplib.h — inside the request-sending logic:
auto close_connection = !keep_alive_;
// ...
if (socket_should_be_closed_when_request_is_done_ || close_connection || !ret) {
    shutdown_ssl(socket_, true);
    shutdown_socket(socket_);
    close_socket(socket_);               // ← the socket gets torn down after EVERY request
}
```

`keep_alive_` defaults to `false`, and it's only ever flipped by explicitly calling `set_keep_alive(true)` on the client — which `apiClient.cpp` never does, for either `client` or `sslClient`. That means: **despite reusing one `apiClient` object per thread for the whole run, every single request still opens a brand-new TCP connection from scratch — and, for `https://` targets, redoes the entire TLS handshake from scratch — every time.** At the request rates this project reaches (thousands to tens of thousands per second), that's a substantial, entirely un-addressed cost sitting directly in the hottest path of the whole program, and it fully explains why the README's own "next steps" section says outright: *"Make the sending part of the net code better."*

This is a great, concrete illustration of a broader lesson: **library defaults are not something to assume — they're something to go read**, especially for anything performance-sensitive. "I constructed the client once outside the loop" *feels* like it should imply connection reuse, but the two are actually independent: object reuse (in your code) and connection reuse (inside the library) are two different things, and only checking the library's actual behavior — not just its API shape — tells you which one you're getting. Section 12 turns this into a concrete exercise you can try yourself.

---

## 10. The measurement toolbox

The developer's own benchmarking commands are preserved in the README, and they're worth understanding individually — this is a genuinely solid, representative Linux performance-engineering toolkit:

| Tool | What it actually does | What it's good for | What it *won't* catch |
|---|---|---|---|
| **`perf record`** | Samples the CPU at a fixed frequency (`-F 99` = ~99 times/sec) and records the call stack at each sample | "Where is time actually being spent?" — turns into raw data for a flame graph | I/O waits and blocking time don't show up the same way as active CPU time |
| **FlameGraph** ([brendangregg/FlameGraph](https://github.com/brendangregg/FlameGraph), [brendangregg.com/flamegraphs.html](https://www.brendangregg.com/flamegraphs.html)) | Turns `perf`'s sampled stacks into an SVG where box width = relative time spent in that function/its callees | Visually spotting the one function eating 40% of total CPU time at a glance — this is exactly what found the randomization bottleneck in section 5 | Doesn't explain *why* something is slow, just *where* |
| **Valgrind (Memcheck)** | Runs your program under a full CPU emulator, checking every memory access | Invalid reads/writes, use-after-free, memory leaks | Data races between *valid* memory accesses from different threads — this is why it came back clean on the segfault bug in section 5.2 |
| **GDB** | An interactive debugger — attach to or launch a process, inspect its state, set breakpoints, walk stack traces | `info threads` (list all threads), `thread N` (switch context), `bt` (backtrace — print the call stack) — exactly the three commands the README lists, and exactly what's needed to localize *which* thread was doing *what* at the moment of a crash | Won't tell you *why* a race happened, only *where* the crash landed |
| **ThreadSanitizer** (`-fsanitize=thread`, a GCC/Clang compiler flag, not used in this project but directly relevant) | Instruments every memory access and synchronization operation to detect data races *directly*, without needing to get unlucky | Would likely have caught the section 5.2 race in a single short run, with an exact report of both racing accesses | Meaningful runtime/memory overhead — normally used in testing, not in the production/shipped binary |
| **`htop`** | Live, per-core CPU/memory usage | The "why is usage only 60%" observation in section 8 that cracked open the terminal-I/O bottleneck | Doesn't show you *why* a thread isn't running — just that it isn't |

The methodology that ties all of these together, and that's worth extracting as the real takeaway from this entire project, is: **profile before you optimize, and pick the tool that matches the *kind* of question you're asking.** "Where is CPU time going?" → flame graph. "Is memory being misused?" → Valgrind. "Why did this crash, and on which thread?" → GDB. "Is my CPU actually the bottleneck at all?" → `htop`, read carefully. Every step in this project's optimization story started with a measurement, not a guess.

---

## 11. Three real things in the code you can go verify and fix yourself

These aren't hypothetical exercises — they're genuine observations from reading the actual uploaded source, listed roughly in order of how much they'd likely matter for throughput.

**1. No HTTP keep-alive, anywhere.** As shown in section 9, `apiClient` never calls `set_keep_alive(true)` on either the plain or SSL client, so every request pays a fresh TCP handshake (and, over HTTPS, a fresh TLS handshake) instead of reusing the connection already open from the previous request on that thread. *Try it yourself*: add `client->set_keep_alive(true);` / `sslClient->set_keep_alive(true);` right after construction in `apiClient::apiClient`, and re-run the same `--payload ocean` benchmark against a local mock endpoint before and after. Given how central connection setup cost is to network performance, this is plausibly the single biggest remaining lever in the entire codebase — a genuinely good first real contribution if you wanted to send a pull request upstream.

**2. `getRandomVectorFromJSON` still rebuilds its key list on every call.** Look again at section 6.2's current implementation: `getKeysFromJsonObject` walks the *entire* underlying JSON object (all ~200 entries of `ITEMS_JSON`, all ~170 of `CHAMPIONS_JSON`, etc.) and rebuilds a fresh `keys` vector, **every single time `getRandomVectorFromJSON` is called** — and `matchBuilder::randomMatch()` calls it five times per match (items, champions, summoners, keystones, secondary runes), even though none of those underlying JSON pools ever change between calls, or ever change at all after program startup. This is exactly the same "recompute a constant every call" anti-pattern that sections 5 and 6 spent multiple optimization passes eliminating elsewhere — just not yet applied here. *Try it yourself*: what would it take to compute each mapping object's key list exactly once (e.g., a `static` local cache inside the function, keyed by which JSON object was passed in, or a set of pre-computed key vectors sitting alongside the `mapping::*_JSON` globals) instead of on every call? What's the actual expected speedup, given what you now know about the O(n + count) cost from section 6.2?

**3. The gzip `Content-Encoding` header is sent unconditionally, but the body isn't always actually compressed.** `apiClient::sendPOSTRequest` always sets `{"Content-Encoding", "gzip"}` in its headers — but looking back at `threadWorks::sendRequest` (section 3.3), only the `"lol"` branch actually calls `gzip_compress()` on the payload before sending; the `"ocean"`, `"payload.json"`, and arbitrary-file-path branches send their raw, uncompressed `.dump()` output while `sendPOSTRequest` still claims (via the header) that the body is gzip-encoded. A server that honors `Content-Encoding` and tries to `gunzip` those bodies would fail to decode them. *Try it yourself*: is the right fix to compress unconditionally inside `sendPOSTRequest` itself (so the header is always true), or to make the header conditional on whether the caller actually compressed the payload? What are the tradeoffs of each?

---

## 12. Glossary

- **RAII** (Resource Acquisition Is Initialization) — tying a resource's lifetime to an object's scope, so it's automatically released when the object is destroyed (e.g., `apiClient`'s `std::unique_ptr<httplib::Client>`).
- **ODR (One Definition Rule)** — a C++ rule that a given variable/function/class must have exactly one definition across the whole program; violating it produces linker errors like "multiple definition."
- **Data race** — two or more threads accessing the same non-atomic memory concurrently, where at least one access is a write, with no synchronization between them. Undefined behavior in C++, regardless of whether it "seems to work."
- **`thread_local`** — a storage duration giving each OS thread its own independent instance of a variable, eliminating races on it without locking.
- **Amortized complexity** — describing the *average* cost per operation over a sequence, even when individual operations vary (e.g., `std::vector::push_back` is amortized O(1) even though any single call that triggers reallocation is O(n)).
- **Syscall** — a request from a user-space program into the kernel (e.g., opening a file, writing to a terminal, sending network data); crossing this boundary is inherently more expensive than pure user-space computation.
- **gzip / DEFLATE** — gzip is a container format wrapping the DEFLATE compression algorithm with a header and CRC32 checksum; `zlib`'s `deflateInit2(..., 15 + 16, ...)` is exactly how you request the gzip wrapper specifically, as opposed to raw DEFLATE or zlib's own format.
- **Flame graph** — a visualization of sampled call stacks where box *width* represents relative time spent, making the "hottest" code paths visually obvious at a glance.
- **CSPRNG vs. PRNG vs. entropy source** — `std::random_device` is meant to be a true entropy source (non-deterministic, ideally from the OS); `std::mt19937` is a fast, deterministic PRNG, typically *seeded* from an entropy source once and then reused.
- **RVO / NRVO** (Return Value / Named Return Value Optimization) — a compiler optimization that can construct a function's return value directly in the caller's storage, avoiding a copy; doesn't help when the caller already owns separate storage it wants filled in place (section 6.4).
- **Keep-alive** — reusing one already-open TCP (and, for HTTPS, one already-negotiated TLS session) connection across multiple HTTP requests, instead of opening a fresh connection per request.
- **CPU-bound vs. I/O-bound** — CPU-bound work is limited by computation and benefits little from more threads than cores; I/O-bound work spends most of its time blocked (network, disk, terminal, locks) and can genuinely benefit from far more threads than cores, since blocked threads yield the CPU.

---

## 13. A suggested path if you want to go deeper from here

1. **Build it yourself.** Clone `noahpop77/Papy`, run `make rebuild`, and get `./bin/papy --help` working locally. Nothing here will fully click until you've compiled and run it once.
2. **Reproduce one benchmark.** Stand up the simplest possible mock endpoint (any language, just return `200`), run Papy against it with increasing `--threads`, and watch `htop` while you do it — try to actually see the CPU-bound-vs-I/O-bound transition described in section 8 with your own eyes.
3. **Get a flame graph of your own run**, using the exact `perf record` / `stackcollapse-perf.pl` / `flamegraph.pl` pipeline preserved in the README. Comparing what *you* see against what the blog post described is far more instructive than reading about it.
4. **Deliberately reintroduce the race condition** from section 5.2 (revert `thread_local` back to a shared global, just locally) and try to catch it with `-fsanitize=thread` instead of waiting for a crash. Seeing a sanitizer point at the exact two racing lines is a genuinely different experience than reading about data races in the abstract.
5. **Pick one of the three exercises in section 11** and actually implement and benchmark it. Section 11's keep-alive fix in particular is a small, contained, and realistic first contribution if you want to eventually open a pull request.
6. **Read up on what the loopback benchmark hides** (section 8.3): look into TCP's three-way handshake, TLS 1.3's handshake round-trips specifically, and what "connection pooling" means in an HTTP client — this is the exact gap between "fast on `localhost`" and "fast on the real internet" that the project's own README flags as unfinished work.

---

## 14. References

**Primary sources (this repository and its author's own writing, all provided in this conversation):**
- Repository: `github.com/noahpop77/Papy` (README, developer docs, and full source, as uploaded)
- Brian Sawa, *"Exploring Your Limits! Limit Testing Your APIs With Papy"* (Nov 27, 2024)
- Brian Sawa, *"Exploring Your Limits! Limit Testing Your API's With Papy — Part II"* (Dec 10, 2024)
- Brian Sawa, *"200x Papy Performance Increase: Road To 20,000 Requests/sec"* (Jan 23, 2025), and the accompanying video walkthrough

**External references for the underlying concepts (verify against current docs, since some of these evolve):**
- [cppreference: `std::random_device`](https://en.cppreference.com/w/cpp/numeric/random/random_device), [`std::mt19937`](https://en.cppreference.com/w/cpp/numeric/random/mersenne_twister_engine), [`std::thread`](https://en.cppreference.com/w/cpp/thread/thread), [`thread_local`](https://en.cppreference.com/w/cpp/language/storage_duration)
- [Brendan Gregg — Flame Graphs](https://www.brendangregg.com/flamegraphs.html) and the [FlameGraph toolset](https://github.com/brendangregg/FlameGraph)
- [Valgrind — Memcheck manual](https://valgrind.org/docs/manual/mc-manual.html)
- [nlohmann/json](https://github.com/nlohmann/json) and [cpp-httplib](https://github.com/yhirose/cpp-httplib) — the two vendored libraries this project is built on
- [zlib manual](https://zlib.net/manual.html) for the exact meaning of the `deflateInit2` parameters used in `gzip_compress`
- GCC's [`-fsanitize=thread` (ThreadSanitizer)](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html) documentation
