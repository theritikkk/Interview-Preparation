# 23 — Interactive & I/O

Interactive problems and batch I/O are two different games. Interactivity is about **protocol** — write, flush, read, repeat. Batch I/O is about **throughput** — one `fread` reader and `'\n'` everywhere. Mixing the two toolkits up is exactly how interactive solutions die.

---

## The interactive protocol

The judge talks back to you. The standard shape of the entire program:

```cpp
int main() {
    int n; cin >> n;               // the statement gives you the parameters first
    // loop: ask a question, read the answer, adapt
    cout << "? " << mid << endl;   // endl == '\n' + flush — see below
    string r; cin >> r;
    // ...
    cout << "! " << answer << endl;
    return 0;                      // print nothing after the final answer
}
```

The judge is effectively a second program piped to your stdin/stdout. It **cannot see your query until you flush** — without it, your program waits for an answer that will never come while the judge waits for a query that never arrives.

**Use the exact query format the statement gives you.** `? 1 2` vs `?1 2` vs `1 2` — copy it from the problem, don't improvise.

---

## Flushing — the #1 cause of failure

```cpp
cout << "? " << x << endl;              // endl writes '\n' AND flushes ✅
cout << "? " << x << '\n';              // ❌ '\n' alone does NOT flush
cout << "? " << x << '\n' << flush;     // ✅ same as endl
cout << "? " << x << '\n'; cout.flush();// ✅ same
printf("? %d\n", x); fflush(stdout);    // ✅ C-style equivalent
```

- Missing flush on Codeforces → **"Idleness limit exceeded"** (sometimes reported as TLE elsewhere).
- Flush after **every** query — you always read after you write, so there is never a query you can batch. A few hundred flushes cost nothing.
- With `ios::sync_with_stdio(false)` everything above still works; just remember `'\n'` is not a flush.
- **Never mix `printf` and `cout`** with sync off — the orders interleave wrongly; pick one.
- Debug prints go to **`cerr`**, never `cout` — stderr is not part of the protocol and won't corrupt it.

---

## Query budget — spend it in the worst case

You get a fixed number of queries; the statement says so ("at most 30 queries"). Count your **worst case**, not your average.

| Queries you have | Range you can binary-search |
|---|---|
| 10 | n ≤ 1e3 |
| 17 | n ≤ 1e5 |
| 20 | n ≤ 1e6 |
| 30 | n ≤ 1e9 |
| 40 | n ≤ 1e12 |
| 60 | n ≤ 1e18 (⌈log₂ n⌉ ≤ 60, always) |

**Binary search costs ⌈log₂ n⌉ queries.** [1, 1e18] fits in 60 — you will see this number constantly. Ternary search on a unimodal function: ~⌈log₁.₅ n⌉. Golden-section: same idea, one retained point.

### The guess-the-number skeleton (memorize cold)

```cpp
int lo = 1, hi = n;
while (lo < hi) {
    int mid = lo + (hi - lo) / 2;
    cout << "? " << mid << endl;
    string r; cin >> r;
    if (r == "<") hi = mid - 1;      // hidden value < mid
    else          lo = mid + 1;      // hidden value > mid   ("=" → answer is mid)
}
cout << "! " << lo << endl;
```

The binary search is the easy part — the flush and the exact formats are where people lose.

---

## Adaptive judges

An **adaptive** judge is an adversary: after each query it may choose any answer that is still consistent with *some* remaining candidate. It must answer consistently, but not from a fixed hidden value.

Consequences:

- Your strategy must be correct for **every** consistent answer sequence. "It worked on the sample" proves nothing.
- Ties and boundaries are where you get burned — the adversary picks the branch that costs you most. Prefer strict/consistent window updates (`hi = mid - 1` above) over sloppy `lo = mid` loops.
- If the problem allows **lies** (at most k false answers), plain binary search dies. The standard fix is error-correcting search — ask, keep a candidate set, eliminate a fraction with each query; costs ~⌈log₂ n⌉ + k·⌈log₂ log₂ n⌉. Implement it only if a problem actually demands it.
- **Randomization** is your friend against adaptive judges: shuffling the candidate order or probing random positions makes adversarial worst cases unlikely. `mt19937_64` seeded from `chrono` (`24_FLOATING_POINT_AND_RANDOM.md`), never bare `rand()`.

---

## Custom fast reader (batch problems only)

For non-interactive problems with 1e6+ numbers, this is several times faster than even `cin` with sync off:

```cpp
static char buf[1 << 16];
int bufLen = 0, bufPos = 0;
inline int gc() {
    if (bufPos == bufLen) {
        bufLen = (int)fread(buf, 1, sizeof buf, stdin);
        bufPos = 0;
        if (bufLen == 0) return 0;          // EOF
    }
    return buf[bufPos++];
}
inline int readInt() {
    int c = gc();
    while (c > 0 && c <= ' ') c = gc();     // c > 0 guards EOF
    int neg = 0;
    if (c == '-') { neg = 1; c = gc(); }
    int x = 0;
    while (c >= '0' && c <= '9') { x = x * 10 + (c - '0'); c = gc(); }
    return neg ? -x : x;
}
// ll version: same body with ll x and x = x * 10 + (c - '0')
```

**Never use this in an interactive problem.** `fread` waits for a full buffer (or EOF); the judge waits for your query; you deadlock. Interactive = `cin` + flush, always. Speed is the wrong thing to optimize there.

**Output side:** build one big `string` (or use `'\n'` instead of `endl`) when printing 1e5+ lines — `endl` flushes every line and gets slow.

---

## Output precision

```cpp
cout << fixed << setprecision(10);     // 10 digits AFTER the decimal point
cout << setprecision(15);              // without fixed — 15 SIGNIFICANT digits
```

- `fixed` + `setprecision(9..12)` is the safe default; judges typically accept 1e-6 absolute/relative error, so 10 decimals is comfortable.
- `long double` → `setprecision(15)` or more; plain `double` gives you ~15–16 significant digits, period.
- Sharpen constants with `acos(-1.0L)` etc. — see `24_FLOATING_POINT_AND_RANDOM.md` for EPS choice.
- Printing `__int128` needs a custom printer — `16_MICRO_TEMPLATES.md`.
- Negative zero (`-0.000000000`) is accepted by every sane checker; don't burn time on it.

---

## Traps

```
[ ] wrote a query but never flushed → Idleness limit / TLE
[ ] used '\n' instead of endl/cout.flush() before reading the answer
[ ] wrong query format — "?1 2" when the statement says "? 1 2"
[ ] off-by-one in the search window (should the loop be lo <= hi or lo < hi?)
[ ] exceeded the query budget in the WORST case (ties, boundaries)
[ ] assumed a fixed hidden value when the judge is adaptive
[ ] used the fread reader in an interactive problem → deadlock
[ ] debugging output sent to stdout (it IS the protocol — use cerr)
[ ] printf/cout mixed with sync off → garbled interleaving
[ ] printed anything after the final "! x" answer
[ ] forgot that x = 0 / empty / n = 1 are valid hidden states
```

## Memory hooks

- *Write, flush, read. No flush, no reply.*
- *`endl` = `'\n'` + flush; in interactive the flush is the entire point.*
- *Binary search is log₂(n) questions — [1, 1e18] is exactly 60.*
- *Adaptive judge = adversary that must stay consistent; worst case is the only case.*
- *stderr is free; stdout is the game channel.*
- *fread reader for batch, `cin` + flush for interactive — never the other way round.*
