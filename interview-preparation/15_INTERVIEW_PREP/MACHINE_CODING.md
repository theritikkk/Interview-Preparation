# Machine Coding

The format: 45-90 minutes, build a working, reasonably complete mini-system from stated requirements — evaluated on correctness, OOP/SOLID design (`03_CS_FUNDAMENTALS/OOP.md`), extensibility, and (sometimes) tests. The interviewer will often add a requirement mid-way to see how cleanly your design accommodates change — a sign your initial design was appropriately extensible, or wasn't.

**General approach:** clarify requirements (2 min) → identify core entities/classes (3 min) → design interfaces before implementations → build incrementally, keeping it runnable at each step → leave time to demo and discuss tradeoffs.

---

## Parking Lot

**Requirements:** multiple levels, multiple spot sizes (motorcycle/car/bus), assign a spot on entry, compute fee on exit, track availability.

**Core entities:** `ParkingLot` (has levels), `Level` (has spots), `ParkingSpot` (has a size, occupied state), `Vehicle` (has a size), `Ticket` (entry time, spot, vehicle), `FeeCalculator` (interface — strategy pattern, since fee logic is exactly the kind of thing an interviewer adds a twist to later: "now make it free for the first hour").

**Interview Questions:**
- **L2:** "How do you handle a vehicle that doesn't fit any spot?" (a fallback: allow a smaller vehicle into a larger spot if nothing else is available — implies your spot-assignment logic needs a size-compatibility check, not just an exact match).
- **L3:** "The interviewer now asks for multiple entry/exit gates working concurrently — how does your design change?" — spot assignment becomes a concurrency concern (two gates assigning the same spot to two vehicles simultaneously) — this is where `03_CS_FUNDAMENTALS/OS.md`'s synchronization primitives (or an atomic claim on the spot, echoing `07_DISTRIBUTED_SYSTEMS/CONCURRENCY.md`'s optimistic-concurrency pattern at a smaller scale) becomes directly relevant, live, in an interview.

---

## Splitwise (Expense Sharing)

**Requirements:** add an expense split among a group (equal, exact amounts, or percentage splits), compute each person's net balance, simplify debts (minimize the number of transactions needed to settle up).

**Core entities:** `User`, `Expense` (payer, amount, split strategy), `Split` (interface — `EqualSplit`, `ExactSplit`, `PercentSplit` implementations — another natural strategy-pattern fit), `Ledger`/`BalanceSheet` (tracks pairwise or net balances).

**Interview Questions:**
- **L2:** "How do you validate an ExactSplit adds up to the total expense amount?" — a real, easy-to-miss validation requirement.
- **L3:** "How would you implement debt simplification?" — a graph problem in disguise: model balances as a graph (who owes whom how much), and greedily settle the largest creditor against the largest debtor repeatedly — a nice, concrete tie-back to `02_DSA/PATTERNS.md`'s greedy cluster, live in a design-round context.

---

## Rate Limiter (Implementation-Level)

System-design-level treatment is in `11_SYSTEM_DESIGN/COMMON_DESIGNS.md` Design 2 — machine coding wants the actual working code for one algorithm (commonly token bucket), not just the architecture.

**Core entities:** `RateLimiter` (interface), `TokenBucketRateLimiter` (implementation — tracks tokens + last-refill-timestamp per client, refills lazily on each check rather than needing a background thread).

**Interview Questions:**
- **L2:** "Implement `allowRequest(clientId)` for a token-bucket limiter." — the lazy-refill trick (compute tokens-to-add based on elapsed time since last check, rather than running a background timer) is the elegant, expected implementation.

---

## Other Common Prompts (Lighter Treatment)

- **Elevator system:** entities — `Elevator`, `ElevatorController` (scheduling algorithm: nearest-elevator-first is the simple baseline), `Request` (floor + direction). The interesting design question is the scheduling strategy, not the basic state machine.
- **Tic-Tac-Toe / Chess:** classic OOP exercises — `Board`, `Piece`/`Player`, `Move` validation, win-condition checking. Chess specifically tests whether you can cleanly model very different piece-movement rules without a giant if/else (polymorphic `Piece.getValidMoves()` per piece type is the expected design).
- **Notification service:** `NotificationService` (interface) with `EmailNotifier`/`SMSNotifier`/`PushNotifier` implementations (strategy pattern again) — a good chance to explicitly narrate that this mirrors your own Hermes/EventBridge fan-out pattern, at a much smaller, single-process scale.
- **Inventory system:** `Product`, `Warehouse`/`Stock`, `Order` — the interesting design question is usually concurrency-safe stock deduction (don't oversell inventory under concurrent orders) — directly echoes the claiming-pattern discussion in `01_PROGRAMMING/SQL.md`.

### Quick Revision
- Strategy pattern shows up constantly (fee calculation, split types, notification channels) — recognize it early and design for it from the start, not as an afterthought.
- Interviewers often add a requirement mid-session specifically to test extensibility — clean interfaces upfront pay off directly.
- Several of these problems (concurrent spot assignment, concurrent stock deduction) are small-scale versions of concurrency patterns you've already implemented for real (Hermes, Task Scheduler) — say so, it's a strong signal.
