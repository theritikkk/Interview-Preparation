# ION Code Folder — type, compile, run, explain (fresher-level)

**What this folder is:** every drill as a separate runnable `.cpp` file, written at fresher level — no smart pointers, no `explicit`, no `move`, no fancy keywords. Each file starts with a comment header explaining WHAT it shows, HOW to compile, and the PREDICTED OUTPUT (predict aloud → run → match).

## The files

| File | What it drills | Compile & run | Output check |
|---|---|---|---|
| `01_vehicle_car.cpp` | THE OOP question: overload + override + virtual + virtual destructor | `g++ -std=c++17 01_vehicle_car.cpp -o v && ./v` | `50, 55, 45, 50 \| 75` |
| `02_bank_system.cpp` | "Write a banking system and run it" [CAND-reported] | `g++ -std=c++17 02_bank_system.cpp -o b && ./b` | refused withdrawal at 900000 |
| `03a_second_largest.cpp` | DSA: second largest, single pass + variants | `... 03a_second_largest.cpp -o s && ./s` | `35, 34` |
| `03b_kadane.cpp` | DSA: max subarray sum + indices + all-negative case | `... 03b_kadane.cpp -o k && ./k` | `6, index 3..6` |
| `03c_max_product3.cpp` | DSA: product of 3 with the negative trap | `... 03c_max_product3.cpp -o p && ./p` | `300` |
| `03d_string_compress.cpp` | DSA: `"aaabbcccd" → "a3b2c3d"` | `... 03d_string_compress.cpp -o c && ./c` | `a3b2c3d` |
| `03e_linked_list.cpp` | DSA: reverse + cycle detection (Floyd) | `... 03e_linked_list.cpp -o l && ./l` | `4 3 2 1, no, yes` |
| `04_traffic_light.cpp` | LLD flagship: phases, amber, all-red, emergency | `... 04_traffic_light.cpp -o t && ./t` | GREEN→AMBER→all-red ×2 + emergency |
| `05_printing_system.cpp` | LLD dialogue round: FIFO queue + LRU duplicate check | `... 05_printing_system.cpp -o pr && ./pr` | `duplicate rejected: job 1` |
| `06_parking_lot.cpp` | LLD: park/unpark/fee/status | `... 06_parking_lot.cpp -o pk && ./pk` | `fee Rs.60` |
| `07_snake_ladder.cpp` | LLD: game loop, snakes, ladders, exact-100 win | `... 07_snake_ladder.cpp -o g && ./g` | ends with `WINS!` (random) |
| `08_leaderboard.cpp` | LLD: addScore / top(k) / reset | `... 08_leaderboard.cpp -o lb && ./lb` | `Priya 120` on top |

**`LLD_PATTERNS_HANDBOOK.md`** — every LLD question pattern that could come, with the skeleton + the escalation path for each, in the same fresher style. Read this once after typing the code.

## Tonight's order (time-boxed)

1. Type `01` from memory → compile → **predict output aloud** → run → match. (20 min)
2. Type `02` → predict → run → match → recite the 5 modifications from memory. (20 min)
3. DSA: type `03a` + `03b` + `03e` minimum (others if time). (25 min)
4. LLD: type `04` once (it's short) — **speak the 6 numbered say-lines in its header while typing.** (15 min)
5. Type `05` → run → narrate the 6-stage escalation aloud from its header. (15 min)
6. Skim `06/07/08` (don't type if time is short — read the header comments). (10 min)
7. Read `LLD_PATTERNS_HANDBOOK.md` once. (30 min)

## The universal answer script (for ANY question tonight)

**Say before solving:** *"Let me state my assumptions: …"* — always.
**While solving:** one step per answer, then let them cross-question.
**For every "how would you add X?":** *"New requirement → the change is (component / data structure) → cost = complexity → trade-off = what degrades."*
**Name the data structure out loud** — that's what's being scored.
**When out of depth:** *"Not implemented, but here's how I'd do it: …"* — always a passing answer.

## The explanations

- Line-by-line explanation of `01` and `02`: → `../ION_Live_Coding_Crash_Course.md` (Parts 1–2 are the 10 micro-lessons; the decoder table at the end covers any advanced word they might use).
- Full mock answers for every question type: → `../ION_Mock_Interview_Transcript.md` (M2 = vehicle, M3 = destructor traps, M4 = DSA, M5 = traffic light, M13 = printing system).
- This folder = the **typing** material. The transcripts = the **speaking** material. Use both.
