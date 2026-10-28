# ION Interview — Final Execution Plan v2 (folder-complete, start-here edition)

**Hard anchors:** interview **10:30 AM** · arrive **9:30** · leave home **8:45** · sleep **1:30 AM – 6:00 AM** · this plan is duration-based — start the steps whenever you actually begin, keep the anchors.

**Rule 1:** every drill is typed in a real editor and compiled. **Rule 2:** predict the output OUT LOUD before running. **Rule 3:** if a block overruns 10 min, tick it incomplete, move on, and use Buffer 2 later.

---

## WHAT'S IN THIS FOLDER (and where the plan uses it)

| File | What it is | Used in step |
|---|---|---|
| `code/README.md` | folder map + universal answer script | 0 (2 min) |
| `ION_Live_Coding_Crash_Course.md` | 10 micro-lessons + decoder table; the explanation for ALL code | 1, 2 |
| `code/01_vehicle_car.cpp` | THE OOP drill (simple, verified output) | 1 |
| `code/02_bank_system.cpp` | write-compile-RUN drill + 5 modifications | 2 |
| `ION_Mock_Interview_Transcript.md` | every question + full model answer (M0–M14) | 3, 4, 7, 8, 9, 12 |
| `code/03a…03e .cpp` | DSA: second-largest, Kadane, max-product, compress, linked list | 4 |
| `code/04_traffic_light.cpp` | LLD flagship + 6 say-lines in header | 5 |
| `code/05_printing_system.cpp` | dialogue-LLD drill + 6-stage ladder in header | 5 |
| `code/06,07,08 .cpp` | parking / snake&ladder / leaderboard (header escalations) | 5 (skim if short) |
| `code/LLD_PATTERNS_HANDBOOK.md` | 19 design patterns + DS cheat table + 8 stock answers + SOLID | 5, 10 |
| `ION_Software_Developer_Interview_Master_Revision_Guide.md` | the deep reference (2,600 lines) — NOT read cover-to-cover; only § refs below | on demand |

---

# EVENING SEQUENCE (do in this exact order)

## STEP 0 · 3 min — Orientation
- [ ] Open this folder. Read `code/README.md` once (file table + universal answer script).
- [ ] Say the universal script aloud once: *"Let me state my assumptions…"* · one step per answer · *"requirement → change → cost → trade-off"* · name the DS · skeleton ends it.

## STEP 1 · 45 min — OOP: understand, then type and run ⭐ highest priority
- [ ] Read Crash Course **Part 1** (10 micro-lessons). Fuzzy on any → re-read only that one.
- [ ] Type `code/01_vehicle_car.cpp` **from memory** → `g++ -std=c++17 01_vehicle_car.cpp -o v && ./v`.
- [ ] **Predict the 4 output lines aloud** (50 / 55 / 45 / `50 | 75`) → run → match.
- [ ] Say the 5-line narration while pointing at code (crash course Part 2 end).
- [ ] Memorise: overload vs override · virtual destructor why · **never call the base destructor manually** · the name-hiding note in Car.

## STEP 2 · 25 min — The live run drill: bank system
- [ ] Type `code/02_bank_system.cpp` once → compile.
- [ ] **Predict the full output aloud** (both statements + the REFUSED ₹9,00,000 line) → run → match.
- [ ] Recite the 5 modifications from memory: FixedDeposit / richest account / two-teller mutex / interest payout / save-to-file.
- [ ] One-liner ready: "a senior would use `unique_ptr` — RAII, no leaks."

## STEP 3 · 15 min — OOP traps (guide §K/L + transcript M3)
- [ ] **K1.2** predict-output → check **L1.2**. **K1.3** bug hunt → check **L1.3**.
- [ ] Recite transcript M3.1–M3.3: base-dtor trap · non-virtual dtor = leak · slicing ("store by pointer, slice by value").

## STEP 4 · 30 min — DSA (type 3, speak the modifications)
- [ ] Type `03a_second_largest`, `03b_kadane`, `03e_linked_list` → predict → run → match.
- [ ] Read transcript **M4** aloud once: the variants they'll throw (duplicates, streams, all-negative, negatives in product, k-groups).
- [ ] If time: type `03c` + `03d` too (headers carry the say-lines).

## STEP 5 · 30 min — LLD: the flagship designs
- [ ] Type `code/04_traffic_light.cpp` → while typing, **speak the 6 say-lines** in its header.
- [ ] Type `code/05_printing_system.cpp` → run → **narrate the 6-stage ladder aloud**: queue → heap+aging → LRU dedup → jam/DLQ → ledger → N printers.
- [ ] Skim `LLD_PATTERNS_HANDBOOK.md` **B1–B5** (you just typed 2 of them — it clicks now).

## 🔶 BUFFER 1 · as needed — displaced work / overrun spillover

## STEP 6 · 20 min — Puzzles (grade = approach)
- [ ] Guide **§G**: derive on paper + speak: hourglass (9 min) · bike (65/7 h) · jars · coins · camel (533, cost/km = 2·loads−1).
- [ ] Universal opener aloud: *"Let me state my assumptions first…"*

## STEP 7 · 30 min — Project defense (your strongest weapon)
- [ ] Transcript **M7**: speak each 30-second script (scheduler / WebChat / Hermes) **twice**.
- [ ] Quote the verified numbers cold: 10,000 tasks · peak 8,562 · 73–74 msg/s · ~1,600/s burst · 7m46s drain · p95 160→76 · zero loss.
- [ ] Recite the three honest "I'd do differently" answers (outbox · idempotency key · TransactWriteItems) — as **your own audit**.
- [ ] §**E7**: the 3 strongest whys — RabbitMQ≠Kafka · DynamoDB≠Postgres · no sticky sessions.

## 🔶 BUFFER 2 · as needed

## STEP 8 · 30 min — Case study (separate round)
- [ ] Transcript **Part 2 (M9)**: read the worked FreshBite case WITH the numbers (contribution margin, break-even, LTV:CAC, payback).
- [ ] Recite the 7-step framework + formulas aloud once.
- [ ] AI-tool rules line ready: *"May I confirm what use is permitted? I'd use it to organise data, verify one number by hand, and own the recommendation."*

## STEP 9 · 20 min — HR & stakeholder
- [ ] 60-second intro ×3 aloud (transcript M0).
- [ ] Why-ION · why-you · AI-and-jobs · weakness · the **$1M stakeholder pitch** (M10) — once each, aloud.
- [ ] Two questions to ask ready (M11 end).

## STEP 10 · 15 min — Pattern tables (the recall layer)
- [ ] Recite handbook **Part C** DS table: read left column, answer with the DS + cost.
- [ ] Recite **Part D** 8 stock answers once.
- [ ] SOLID in one breath each (Part E).

## STEP 11 · 10 min — Setup (non-negotiable)
- [ ] Empty `.cpp` compiles: `g++ -std=c++17 t.cpp -o t && ./t`.
- [ ] **AI tool installed + logged in** (case round requirement [EMAIL]).
- [ ] Bag: laptop charged · resume copies · college ID · water.

## STEP 12 · 10 min — Lights-out ritual
- [ ] Recite: overload/override one-liners · never call base dtor · hourglass steps · printing 6-stage ladder.
- [ ] Skim transcript **M2 + M13** trap boxes only.
- [ ] **1:30 AM — SLEEP. Non-negotiable.**

---

# MORNING — 29 Oct

## 6:00 AM — wake. Shower + food. No new material.

## 6:20–6:55 · Final recall (once each, aloud)
- [ ] 60-second intro.
- [ ] Re-run `01` in your head: write the 4 output lines on paper BEFORE checking.
- [ ] Traffic-light 6 say-lines · printing 6-stage ladder — once each.
- [ ] Hourglass + bike — once each.
- [ ] Case 7 steps + formulas — once.
- [ ] **K7 quiz** (guide §K) as recall test; check **L7** only on misses.

## 6:55–7:10 · Killer follow-ups (skim)
- [ ] Transcript M7 honesty boxes (scheduler gaps · WebChat no-idempotency · Hermes outbox) — you own these as "my audit".

## 7:10–8:30 · Get ready + buffer (light recall only — no new topics)

## 8:30–8:45 · Final check → **leave 8:45 AM**

## 9:30–10:25 · Wait-time protocol
- No new topics. Intro once · hourglass once · printing ladder once. Breathe. Water.

## **10:30 — TECHNICAL INTERVIEW.** Approach before code; state assumptions; "not implemented — here's how I'd do it" is a pass.

---

## IF TIME IS LOST — cut in this order (protect Steps 1–5)
1. ~~Step 10 tables~~ → read in morning queue
2. ~~Step 8 timed case~~ → framework + formulas only (10 min)
3. ~~Step 4: 03c/03d~~ → headers carry the say-lines
4. ~~Step 7 follow-ups~~ → 30s intros only
5. ~~Step 3 K1.2/K1.3~~ → keep Step 1 at all costs

## DO-NOT-DO
- No cover-to-cover reading of the Master Guide tonight (it's the on-demand reference — § refs are in every step).
- No new DSA topics · no §F deep read · no inventing NetSight facts · no all-nighter.
- A rested brain that *talks through* problems beats a tired brain that knows more.
