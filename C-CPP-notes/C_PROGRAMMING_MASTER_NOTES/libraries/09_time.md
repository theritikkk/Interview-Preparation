# `<time.h>` — Dates, Clocks, and the Standard's "Weasel Words"

> **Phase 7 · The Standard Library, By Design.** Depends on: `memory/06_struct_union_layout.md`
> §1 (struct basics — `struct tm` is a plain data struct).
> Sources: **Book C** ch.15 (primary).

---

## 1. Where this header's sophistication actually comes from

`[Book-derived, C]` `<time.h>`'s relative richness (broken-down calendar time, time-zone
awareness, Daylight Saving Time handling) traces directly back to early UNIX: several of
its developers were amateur astronomers, genuinely fussy about representing time correctly
across decades, and defaulted to Greenwich Mean Time (now UTC) rather than "whatever the
local wall clock says." The standard library inherited that same attention to detail,
scoped down to whatever doesn't depend on UNIX-specific peculiarities.

---

## 2. ⚠️ The standard's own "weasel words" — a genuinely important caveat

`[Book-derived, C]` Plauger is candid, almost wry, about a real weakness: a conforming
implementation only has to provide its **"best approximation"** to the current time and
date. He gives a deliberately absurd but *technically conforming* example: an implementation
could always report a fixed, wrong date, and still satisfy the letter of the standard. **In
practice, this means: never treat a C program's notion of "the current time" as
guaranteed-accurate** — for anything where correctness of a timestamp actually matters
(logging, scheduling, cryptographic protocols), verify your specific implementation's
actual behavior rather than trusting the standard alone to guarantee it.

---

## 3. The core types and the calendar-time model

`[Book-derived, C]`

| Type/macro | Purpose |
|---|---|
| `time_t` | An arithmetic type capable of representing calendar time (commonly, seconds since a fixed epoch — though the standard doesn't mandate this representation). |
| `clock_t` | An arithmetic type for processor time consumed. |
| `CLOCKS_PER_SEC` | The number of `clock_t` units per second — divide `clock()`'s return by this to get seconds. |
| `struct tm` | **Broken-down time** — a human-readable decomposition (`tm_sec`, `tm_min`, `tm_hour`, `tm_mday`, `tm_mon` [0–11], `tm_year` [years *since 1900*], `tm_wday`, `tm_yday`, `tm_isdst`). |

### ⚠️ Two real gotchas in `struct tm`, worth memorizing

`[Book-derived, C]` `tm_year` is **years since 1900**, not the actual year — `tm_year == 124`
means 2024, a classic off-by-a-large-constant bug source if you forget the offset (this
exact miscounting caused real, famous "Year 2000"-adjacent bugs in software that
mishandled it). `tm_mon` is **0-indexed** (`0` = January), while `tm_mday` (day of the
month) is **1-indexed** — an easy pair to transpose.

### `clock()` measures *processor* time, not wall-clock time

`[Book-derived, C]` A common confusion worth heading off directly: `clock()` reports **CPU
time actually consumed by your program**, not elapsed real-world time. A program that
sleeps for 10 seconds while consuming almost no CPU will show `clock()` advancing by only a
tiny fraction of that — measuring wall-clock elapsed time requires `time()`
(coarser-grained, typically 1-second resolution) or a platform-specific higher-resolution
facility (`[External]`, outside ISO C's scope).

---

## 4. ⚠️ Misconceptions to unlearn now

- **"The current date/time C reports is guaranteed accurate."** Only a "best
  approximation" is required by the standard (§2) — verify your platform's actual behavior
  for anything correctness-sensitive.
- **"`tm_year` holds the actual calendar year."** It's years *since 1900* — add 1900 to get
  the real year (§3).
- **"`clock()` measures how much real time has passed."** It measures processor time
  consumed, which can differ enormously from wall-clock time for an I/O-bound or sleeping
  program (§3).

---

## 5. 🎯 Interview questions

**Easy**
- What does `tm_year` actually store? → Years since 1900, not the literal year (§3).
- What does `clock()` measure? → Processor (CPU) time consumed by the program, not
  wall-clock elapsed time (§3).

**Medium**
- Why shouldn't you assume C's reported current date/time is accurate? → The standard only
  requires a "best approximation" — a technically conforming implementation could report a
  fixed, wrong value (§2).
- How would you measure wall-clock elapsed time versus CPU time consumed in a C program? →
  `time()` (coarse, wall-clock) versus `clock()` (CPU time, divided by `CLOCKS_PER_SEC`) —
  the two answer genuinely different questions (§3).

**Hard**
- A program computes `1900 + tm_year` to display a date and shows the correct year for
  decades, then suddenly displays a wrong one after a system update. What's the most likely
  explanation? → Nothing about `tm_year`'s definition changed — but if the code instead
  used `tm_year` directly without the `+ 1900` offset (or made some other off-by-a-constant
  assumption), it would have been silently wrong all along; a more likely real-world
  culprit is a fixed-width or otherwise bounded year representation elsewhere in the
  system reaching its limit — the general lesson (§3's footnote-worthy caution) is that
  time/date code is a disproportionately common source of latent, delayed-onset bugs.

---

## 6. External references

- [cppreference — `<ctime>`/`<time.h>`](https://en.cppreference.com/w/c) `[External]` —
  current, complete reference for every function and `struct tm` field.
- [man7.org — `strftime(3)`](https://man7.org/linux/man-pages/) `[External]` — the
  standard formatted-time-output function, for producing human-readable date/time strings
  from a `struct tm`.

## 7. 🛠 Practice

1. Write a program that prints both `time()`'s value and `clock()`'s value (divided by
   `CLOCKS_PER_SEC`) before and after a deliberate `sleep`-based delay versus a
   CPU-intensive busy-loop of similar wall-clock duration — compare how differently the two
   measurements respond to each.
2. Convert a `time_t` to `struct tm` (via `localtime` or `gmtime`) and print each field
   explicitly, double-checking the `tm_year + 1900` and 0-indexed `tm_mon` conventions
   directly against a known real date.
