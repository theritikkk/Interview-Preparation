# ION Interview — Final Execution Plan (28 Oct, 5:40 PM → 29 Oct, 10:30 AM)

**Commitment:** I follow the blocks in order, say every drill OUT LOUD, and tick every checkbox. If work steals time, I cut from the bottom of the cut-list — never from Blocks 1–5.

**Hard facts:** Interview slot **10:30 AM** · arrive college **9:30 AM** · leave home **8:45 AM** (adjust if your commute ≠ ~45 min) · sleep **1:30 AM – 6:00 AM (4.5 h)** · everything below points into `ION_Software_Developer_Interview_Master_Revision_Guide.md` (same folder).

---

## EVENING — 28 Oct

### 5:40–6:20 · Block 1 — OOP live-code (the #1 scored skill)
- [ ] 🔴 **FIRST (if code feels shaky): read `ION_Live_Coding_Crash_Course.md` Part 1 — the 13 micro-lessons** (~30 min). Every construct in the drill is explained there from zero.
- [ ] Type the §**B1** Vehicle/Car model from memory — **twice** — clean compile (Crash-Course Part 2 has the annotated version + predicted output).
- [ ] Do **K1.1** (the confirmed ION drill) + write your own 5-bullet narration.
- [ ] Recite until automatic: overload vs override · virtual destructor · **never** call the base destructor manually.

### 6:20–6:50 · Block 2 — OOP traps + live run drill
- [ ] 🔴 **Type the bank system (Crash-Course Part 3) once → compile → PREDICT the output aloud → run → match.** Then recite the 5 interviewer modifications from memory.
- [ ] **K1.2** predict-output (both variants) → check **L1.2**.
- [ ] **K1.3** bug hunt (5 bugs) → check **L1.3**.
- [ ] Skim §**B2–B4** (only deep-read the virtual-destructor part).

### 6:50–7:30 · Block 3 — DSA core (reported problems)
- [ ] Read §**C0** tables + §**C1, C2, C3**.
- [ ] Do **K2.1** (second-largest, all variants), **K2.2** (Kadane + indices), **K2.3** (max product of 3) — code + speak approach.

### 7:30–8:00 · Dinner (no screens — mentally run the hourglass puzzle)

### 8:00–8:45 · Block 4 — DSA round 2
- [ ] Read §**C4, C8, C11**.
- [ ] Do **K2.4** (string compression), **K2.6** (reverse LL), **K2.7** (cycle + entry), **K2.14** (trie phone directory).

### 8:45–9:35 · Block 5 — LLD: traffic light + dialogue-LLD drill (the flagship design question)
- [ ] Read §**D1** fully.
- [ ] **Speak the 60-second design aloud twice** (IDs-as-keys · conflict matrix · phases · amber+all-red · pedestrian queue · emergency safe-transition · fail-safe).
- [ ] Do **K3.1**; skim §**D2** (snake & ladder + vanishing snake).
- [ ] 🔴 **NEW [CAND intel]: Transcript Part 4 (M12–M14)** — the dialogue-LLD format his friend got: no coding, cross-questioning, approach only. Do the 25-min drill: 5 rules → printing-system 6-stage escalation aloud (queue → priority heap + aging → LRU dedup → failure/DLQ → ledger → N printers) → DS-cheat prompts → parking-lot transfer.

### 9:30–10:00 · WORK BUFFER #1 (spill-over block if any block overran)

### 10:00–10:30 · Block 6 — Puzzles (graded on approach)
- [ ] Read §**G1–G5**; **derive on paper**: hourglass (4/7→9) + bike (2p=5q → 65/7 h) + jars + coins + banana method.
- [ ] Run **K4.1–K4.5**; check **L4**.

### 10:30–11:10 · Block 7 — Projects (this is where you're strongest — own it)
- [ ] §**E1**: 30s + 2min scripts; the 8-step data flow; quote the verified numbers (8,562 peak · 74 msg/s · ~1,600/s burst · 7m46s · p95 160→76).
- [ ] §**E2**: the race-condition story (dropped = confirm-channel; duplicated = requeue→republish+ack) + the idempotency-key follow-up answer.
- [ ] §**E3**: outbox honest version; migration ADR story; pipeline-defect mechanism.
- [ ] Say each 30-second intro **out loud twice**.
- [ ] §**E7**: recite the 3 strongest why-answers (RabbitMQ≠Kafka · DynamoDB≠Postgres · no sticky sessions) + the 3-part formula.

### 11:10–11:40 · WORK BUFFER #2

### 11:40 PM–12:10 AM · Block 8 — Case study (separate round)
- [ ] Memorize §**H1** (7 steps) + §**H2** formulas (LTV:CAC≥3, break-even, contribution margin).
- [ ] **K5.1** with a 20-minute timer → self-grade with the rubric.
- [ ] Skim §**H6** (AI-tool discipline: ask the rules, verify one number, challenge one AI claim, own the recommendation).

### 12:10–12:30 AM · Block 9 — HR
- [ ] §**I.1** 60-second intro — **aloud 3×**.
- [ ] §**K6.3**: your own weakness / pressure / teamwork / hire-me one-liners (each with one real story beat).

### 12:30–12:50 AM · Block 10 — Rapid review
- [ ] Read §**J** cheat sheets (J1–J7); mark anything shaky for the morning.

### 12:50–1:10 AM · Setup (do not skip)
- [ ] Laptop charged · editor open on empty `.cpp` · `g++ -std=c++17 x.cpp && ./a.out` verified.
- [ ] **AI tool installed + login working** (case round requirement [EMAIL]).
- [ ] Bag packed · water · resume copies · college ID.

### 1:10–1:30 AM · Lights-out ritual (recite, then phone away)
- [ ] Overload vs override (one line each) · virtual destructor why · never call base dtor · hourglass steps.
- [ ] **1:30 AM — SLEEP. Non-negotiable.**

---

## MORNING — 29 Oct

### 6:00 AM — wake (4.5 h sleep). Shower + food. **No new material.**

### 6:20–6:50 · Final checklist (§**A.4** compressed)
- [ ] 60-second intro aloud once.
- [ ] **K7 quiz** as recall test (≤20 s each) → check **L7** only on misses.
- [ ] Traffic-light 60s design — once, aloud.
- [ ] Printing-system escalation (Transcript M13) — once, aloud: queue → heap+aging → LRU → failure → ledger → scale.
- [ ] Hourglass steps — once, aloud.
- [ ] Case 7-step framework — once, aloud.

### 6:50–7:10 · Killer follow-ups (skim only)
- [ ] The ⚠️ honesty boxes in §**E1 / E2 / E3** (one per project, with your prepared "I'd do differently" answers).

### 7:10–8:30 · Get ready + buffer (light recall only if anxious — no new topics)

### 8:30–8:45 · Final setup check → **leave home 8:45 AM** (adjust for actual commute)

### 9:30–10:25 · Wait-time protocol at college
- No new topics. Recite the 60s intro once · hourglass once.
- Two questions ready to ask (§**I.4**).
- Slow, deep breaths; shoulders down; water.

### **10:30 — TECHNICAL INTERVIEW.** Speak your approach before coding; state assumptions; it's fine to say "not implemented — here's how I'd do it."

---

## IF WORK EATS TIME — cut in this order (protect 1–5 always)
1. ~~Block 10 (§J)~~ → read J in the morning queue instead
2. ~~K5.1 timed case~~ → framework + formulas only (15 min)
3. ~~K2.14 trie~~ → know the *why* (prefix search) without coding it
4. ~~Block 7 project follow-ups~~ → 30s intros only
5. ~~K1.2/K1.3~~ → keep Block 1 (K1.1) at all costs

## DO-NOT-DO list
- No new DSA topics · no §F deep read (the K7 quiz covers it) · no full E follow-up lists · no inventing NetSight facts · no all-nighter — a rested brain beats one more problem.
