# LLD PATTERNS HANDBOOK — every design question that could come, fresher-level

**How to use:** skim Part A once (the method), then read the patterns. The five with runnable code (`04/05/06/07/08`) you type tonight; the rest you only **read** — you'll recognise the shape instantly because every pattern is the same 6 moves.

---

# PART A — The method (this IS the scored part)

## A1. How the round actually runs [CAND]

- Informal. The interviewer **states basic requirements himself, narrates them back**, and keeps cross-questioning: *"how are you supposed to do this? how would you add this? how would you handle this?"*
- **No DSA coding. No full code required.** You narrate the approach and end with a **skeleton** (classes + fields + one-line responsibilities).
- What's graded: your **structure**, the **data structures you name out loud**, and your composure under cross-questioning.

## A2. The universal 6-step answer (works for every pattern below)

1. **Clarify + assume out loud:** "One printer to start / fixed number of floors / scores can be negative? I'll assume X — correct?"
2. **Name the entities:** the nouns in the question become classes (Job, Slot, Player, Seat…).
3. **Pick the core data structure and SAY it:** queue / heap / map / trie / LRU — one reason each.
4. **Define the key methods:** what does each class do — one line per method.
5. **Handle the state machine + failure:** every entity that "does something" has states; failures never lose data silently.
6. **End with the skeleton** — classes + fields, no method bodies needed.

## A3. The escalation formula (for every "how would you add X?")

> **"New requirement → the change is (component / data structure) → cost = complexity → trade-off = what degrades."**

Say it in exactly that shape every time. Then stop and let him pick the next probe.

## A4. The 5 dialogue rules

1. **Mirror the requirement first** — restate it + label your assumption, get a nod.
2. **One step per answer, then stop** — no monologues; his questions drive the design.
3. **Name the DS explicitly** — that's the keyword he's listening for.
4. **Volunteer the limitation yourself** ("the simple version scans O(n); a free-slot queue makes it O(1)") — it reads senior.
5. **Skeleton, not code** — unless he asks to run it (then: file 01/02 style).

---

# PART B — THE PATTERNS

## B1. Traffic Light ⭐ (runnable: `04_traffic_light.cpp`)

**The ask:** "Design a traffic-light management system. Roads can merge. Handle conflicting greens, multiple lanes, pedestrians, emergency vehicles."

**Assumptions to state:** one intersection to start; fixed-time phases; conflict = crossing or opposing-turn flows; pedestrians have buttons; emergency preemption expected; on fault → fail-safe.

**Entities:** `Signal` (road id + state) · `Phase` (a SET of signals green together) · `Controller` (rotates phases, only writer of states).

**The 6 say-lines:** phases are conflict-free by construction · GREEN→AMBER(3s)→ALL-RED(2s)→next · IDs are names, never array indexes (a merge adds a new ID, breaks nothing) · pedestrians walk only PARALLEL to the green they cross against · emergency = never jump straight to green · fault → all-red / flashing red.

**Escalations:** *"merge mid-operation?"* → new id + rebuild phases; *"who writes Signal state?"* → only the Controller, one writer = no races; *"two emergencies?"* → priority queue of requests; *"why not a timer per light?"* → safety would live in every light; here it lives in one validated structure.

## B2. Parking Lot ⭐ (runnable: `06_parking_lot.cpp`)

**The ask:** "Design a parking lot — multiple floors, vehicle types, entry/exit, fees."

**Assumptions:** N slots, flat rate first; fee per completed hour; ticket at entry.

**Entities:** `Vehicle` (number, type) · `Slot` (id, occupied, vehicleNumber, entryHour) · `ParkingLot` (park/unpark/fee/status).

**Core DS to name:** a **queue of free slot ids** → park is O(1) instead of scanning (the simple version scans — say the upgrade).

**Escalations:** floors → vector per floor + one shared free-queue · bike/car/truck sizes → slot type must match-or-exceed · nearest slot → min-heap by distance · two cars same instant → mutex around park/unpark · fee slabs → table lookup · find-my-car → map<number, slotId>.

## B3. Snake & Ladder ⭐ (runnable: `07_snake_ladder.cpp`)

**The ask:** "Design snake and ladder for N players."

**Assumptions:** 100-square board; exact 100 to win; snakes/ladders fixed per game.

**Entities:** `Player` (name, pos) · `Dice` (roll) · `Board` (two maps: snake head→tail, ladder base→top) · `Game` (turn loop, win check).

**Core DS to name:** `map` for snake/ladder jumps — O(log n) lookup; players in a `vector` → N players free.

**Escalations:** exact-100 → already coded, point at it · 2 sixes → bonus roll loop · undo → stack of (player, oldPos) · save/resume → serialize board maps + positions · "who moves next if a player quits?" → remove from vector, turn order preserved.

## B4. Leaderboard ⭐ (runnable: `08_leaderboard.cpp`)

**The ask:** "Design a game leaderboard: addScore, top(k), reset."

**Entities:** `Leaderboard` (map<player, score>).

**Core DS to name:** simple = map + sort-copy on top(k). Frequent top(k) = **min-heap of size k** (O(n log k)) or a sorted set updated on every addScore. **Tie-break:** score desc, then name/timestamp.

**Escalations:** negative scores? fine, map handles · concurrent updates → mutex · millions of players → only top-k lives in memory, full ranking in a DB · live rank of ONE player → keep a multiset and count greater elements, or store rank on update.

## B5. Printing System ⭐ (runnable: `05_printing_system.cpp`) — the dialogue pattern

**The ask (your friend's real round):** requirements narrated informally; escalation queue → LRU → more DS; approach only.

**The 6-stage escalation (memorise this exact ladder):**
1. FIFO → **queue** (enqueue/dequeue O(1); decouples slow printer from fast users)
2. VIP job jumps → **priority queue (heap)** ordered by (priority, arrivalTime); starvation → **aging**
3. double-click / crash-resubmit → **LRU of recent job ids** (O(1) get/put; eviction = bounded memory)
4. printer jams → status machine QUEUED→PRINTING→DONE/FAILED + retry + **dead-letter list** + notify
5. usage per user → **ledger** map<user, pages>
6. 100 printers → **per-printer queues + dispatcher** routes by printerId; shard by id

**Killer line:** "the queue is the buffer — that's why a jammed printer loses nothing; jobs hold."

## B6. Phone Directory / Contacts (Trie — reported asked)

**The ask:** "Which data structure for a phone directory? Explain it."

**Answer:** "**Trie** — prefix search in O(|prefix|) no matter how many contacts; a hash map can't do prefix search; sorting + binary search costs O(log n) + a range scan."

**Skeleton:**
```cpp
struct TrieNode {
    TrieNode* child[26];
    bool isEnd;                                  // a full name ends here
    TrieNode() { isEnd = false; for (int i = 0; i < 26; i++) child[i] = NULL; }
};
// insert(name): walk/create child per character, mark isEnd at the end
// search(name): walk; return true only if the last node exists AND isEnd
// suggest(prefix): walk to the prefix node, then DFS all branches collecting names
```

**Escalations:** phone numbers attached → store number at isEnd nodes · ranked suggestions → keep counts per node (that's how Google-style autocomplete works) · delete → unmark isEnd, prune empty branches · memory worry → children as a map<char, node> only for used letters.

## B7. Library Management

**The ask:** "Design a library system — issue, return, fine, search."

**Entities:** `Book` (id, title, totalCopies, issuedCopies) · `Member` (id, issued list) · `Transaction` (book, member, issueDate, returnDate) · `Library` (issue/return/search).

**Core DS:** `map<bookId, Book>` for O(log n) lookup; issued copies as a counter per book; member's loans in a `vector`.

**The one race to volunteer:** "issuedCopies must never go negative — check availability, then increment, in one protected step."

**Escalations:** fine → days late × rate, computed at return · reservations → **queue** per book; first member in queue gets the next return · search by title → map<title, vector<bookId>> built at setup (index) · max-loan rule → check member's issued count at issue · lost book → fine = replacement cost, mark copy lost.

## B8. Elevator System

**The ask:** "Design an elevator for a building."

**Entities:** `Elevator` (currentFloor, direction, doorOpen, requests) · `Request` (floor, direction) · `Controller`.

**Core DS:** request queue — simple FCFS first; better = **two heaps** (min-heap of up-requests above, max-heap of down-requests below) → the elevator sweeps up, then down (SCAN algorithm — name it, it impresses).

**State machine:** IDLE → UP → DOORS_OPEN → IDLE/DOWN. Never reverse mid-sweep while requests exist in the current direction.

**Escalations:** multiple lifts → controller assigns the nearest idle/heading lift · door safety → sensor, reopen on obstruction · emergency → all cars to ground floor, doors open · capacity → weight sensor refuses entry, next car called.

## B9. ATM

**The ask:** "Design an ATM: withdraw with notes."

**Entities:** `Account` (balance — from file 02!) · `Card` · `ATM` (cash inventory, session states).

**State machine (say it):** IDLE → CARD_INSERTED → PIN_VALIDATED → SESSION → EJECT. Three wrong PINs → retain card.

**Core DS:** cash inventory as `map<denomination, count>`; withdraw = greedy from biggest note, check feasibility, then dispense.

**Escalations:** can't make the amount with available notes → refuse with "try another amount" (say why greedy can fail: notes = {1×100}, amount 150) · concurrent ATM + online debit → the account update must be atomic — one lock per account · network down → offline decline, never partially dispense · receipt/audit → transaction log append.

## B10. Vending Machine

**The ask:** "Design a vending machine."

**Entities:** `Item` (code, price, count) · `CoinSlot` · `Machine` (state + inventory).

**State machine (this is THE pattern for it):** IDLE → (insert coin) → HAS_MONEY → (select) → DISPENSED → (change + refund) → IDLE. Cancel at any money state → full refund.

**Core DS:** `map<code, Item>` for slots; coin inventory `map<denom, count>` for change.

**Escalations:** exact change unavailable → refuse selection before taking money, or return coins · refill → admin mode restocks · item stuck → sensor retry, refund on failure · popular item empty → show OUT_OF_STOCK but keep accepting other selections.

## B11. Movie Ticket Booking

**The ask:** "Design a movie ticket booking system."

**Entities:** `Movie`, `Show` (movie, time, screen), `Seat` (row, number, status), `Booking` (user, seats, payment, status), `Theatre`.

**The ONE question he's really asking:** *"two users click the same seat"* → **book = check seat is FREE → mark HELD (temporary) → collect payment → mark BOOKED; timeout releases HELD seats back** (that's why seats "hold" for 5 minutes on BookMyShow — say that real-world example).

**Core DS:** seats as a 2-D grid per show; held-seat expiry via timestamps.

**Escalations:** payment fails → held seats released, booking FAILED · group booking partially unavailable → all-or-nothing · seat map per screen reused across shows → separate status per (show, seat) · popular show queue → waitlist queue when house-full.

## B12. Cab Booking (Ola/Uber basic)

**The ask:** "Design a cab booking flow."

**Entities:** `Rider`, `Driver` (location, available), `Trip` (states), `Dispatcher`.

**Trip state machine:** REQUESTED → DRIVER_ASSIGNED → IN_PROGRESS → COMPLETED / CANCELLED.

**Core DS:** available drivers in a **map or grid bucket by area**; matching = nearest driver (distance formula) — scanning is fine to start; say "a geo-grid bucket makes it O(neighbours)".

**Escalations:** driver cancels mid-assignment → trip back to REQUESTED, re-dispatch; rider cancels → fee rule after N minutes; surge pricing → multiplier by demand/supply ratio; fare estimate → distance × rate + time factor; trip history → per-rider list (ledger).

## B13. Chat / WhatsApp basic

**The ask:** "Design a chat application." (You can bridge to YOUR WebChat here — instant credibility.)

**Entities:** `User`, `Message` (sender, text, status), `Conversation`, `ConnectionManager`.

**Message status machine:** SENT → DELIVERED → READ (the two grey ticks and blue ticks — everyone knows this, use it).

**Core DS to name:** per-user connection map; undelivered messages **stored and forwarded** when the user comes online (that's why offline people get messages later); ordering by timestamp per conversation.

**Escalations:** group chat → one message fans out to N members (like your WebChat Redis adapter — say it) · typing indicator → transient event, never stored · delivery across servers → pub/sub between servers (name Redis pub/sub) · deletion for everyone → tombstone flag on the message.

## B14. URL Shortener

**The ask:** "Design bit.ly."

**Entities:** `UrlStore` (short → long, and reverse), `Counter`.

**Core DS:** `map<shortCode, longUrl>` + a counter: encode counter in **base62** (a–z, A–Z, 0–9) → 62^7 covers billions in 7 chars.

**Escalations:** collision → counter-based codes never collide (say why vs random-hash) · custom alias → check availability in the same map · expiry → store createdAt; reject dead links at redirect · analytics (clicks) → increment a per-code counter · same URL twice → reverse map long→short returns the existing code.

## B15. Calculator with extensible operations ⭐ (REPORTED — Open/Closed test)

**The ask:** "Design a calculator with add and subtract. Now add multiply and divide **without modifying existing code**."

**This is the trap:** if you wrote a `switch(op)`, you must EDIT it for every new operation — that's what he wants you to notice.

**Skeleton:**
```cpp
class Operation {                       // the interface: every operation obeys this
public:
    virtual double apply(double a, double b) { return 0; }
    virtual ~Operation() { }
};
class Add : public Operation {
public: double apply(double a, double b) { return a + b; }      // OVERRIDING
};
class Multiply : public Operation {
public: double apply(double a, double b) { return a * b; }      // NEW CLASS, zero edits above
};
class Calculator {
    map<char, Operation*> ops;          // '+': Add, 'x': Multiply ...
public:
    void registerOp(char symbol, Operation* o) { ops[symbol] = o; }
    double calc(char symbol, double a, double b) { return ops[symbol]->apply(a, b); }
};
```
**Killer line:** "adding Multiply means **writing a new subclass and registering it — no existing line changes.** That's the Open/Closed principle: open for extension, closed for modification." (This is the same Vehicle/Car story from file 01 — say that connection out loud.)

## B16. Employee / HR Management (the "QA-system" style ask)

**The ask:** "Design an employee management system — departments, salaries, reporting."

**Entities:** `Employee` (id, name, baseSalary) → `Manager : Employee` (team list, bonus) — inheritance again · `Department` (map of employees) · `Payroll` (computes payouts).

**Core DS:** `map<empId, Employee*>`; department → vector of ids.

**Escalations:** org tree → each Employee has a managerId; walk up for reporting chain · payroll export → Payroll writes a report file (RAII file wrapper) · two HR edits at once → lock per employee record · search by department → prebuilt map<dept, vector<id>> · salary hike policy → virtual method `annualIncrement()` overridden by role.

## B17. Tic-Tac-Toe

**The ask:** "Design tic-tac-toe. Make it work for N×N."

**Entities:** `Board` (grid, place, checkWin), `Player` (symbol), `Game` (turn loop).

**Core DS:** 2-D array; checkWin = scan rows, columns, both diagonals — **generalize with loops over N, not hard-coded 3** (that's the "N×N" follow-up answered by design).

**Escalations:** draw detection → moves count == N×N and no win · undo → stack of moves · AI opponent → "score every possible move and pick the best (minimax)" — say the name, not the code · board sizes → Board constructor takes N; nothing else changes.

## B18. Queue-from-stacks / Stack-from-queues (the DS-pattern trap)

**The ask:** "Implement a queue using two stacks."

**Idea (say, then code if allowed):** stack 1 = inbox, stack 2 = outbox. push → inbox.push. pop → if outbox empty, pour ALL of inbox into outbox (reverses order), then pop. Each element poured once → amortized O(1).

**Escalations:** stack via two queues → keep one queue, push then rotate size−1 elements (or two queues alternating) · min-stack → auxiliary stack storing the min at each depth · "why amortized?" → the expensive pour happens only when outbox is empty; averaged over pushes it's constant.

## B19. Task Scheduler mini-design (bridge to YOUR project)

**The ask:** "Design a job/task scheduler." — **This is literally your project. You are the strongest candidate in the room for this one.**

Say it in LLD language: "I built this. Tasks go into a **queue with a per-message delay** (I used RabbitMQ TTL); a **worker pool** consumes; a **reconciler** sweeps for anything delayed too long; state lives in a database with row locks; I load-tested 10,000 tasks — peak backlog 8,562, zero lost."

Then design fresh if he insists: `TaskStore` (map + status), `DueQueue` (heap by dueTime), `WorkerPool` (fixed threads), `Reconciler` (periodic scan), `Executor` (timeout per task).

**Escalations:** recurring tasks → after success, re-insert with next due time (cron math) · worker crash → task returns to pending (visibility timeout idea) · priorities → heap key (priority, dueTime) · duplicates → idempotency key.

---

# PART C — The DS-choice cheat table (name these OUT LOUD)

| When he says… | You name… | Cost |
|---|---|---|
| "in arrival order / fair / first-come" | **Queue** | O(1) both ends |
| "VIP / urgent / priority / jump" | **Heap (priority queue)** + arrival-time tie-break | O(log n) |
| "same priority but still fair" | tie-break by **timestamp** | — |
| "old jobs starving" | **aging** (priority += k × wait) | — |
| "fast lookup by id/name" | **map / unordered_map** | O(log n) / O(1) avg |
| "prefix search / autocomplete / directory" | **Trie** | O(\|prefix\|) |
| "duplicate / double-submit / recent" | **LRU** (map + list) | O(1) |
| "undo / last move / history" | **Stack** | O(1) |
| "top k / richest / ranking" | **min-heap of size k** or sorted set | O(n log k) |
| "unique items / no repeats" | **set** | O(log n) |
| "free slots / available resources" | **queue/stack of free ids** | O(1) handout |
| "it failed / hung / crashed" | **status machine + retry + dead-letter + notify** | — |
| "usage / billing / quota" | **ledger** (map + append-only log) | O(1) |
| "two users at once" | **mutex / one-writer rule** | — |

# PART D — The 8 stock escalation answers (memorise the shapes)

1. **Concurrency:** "shared check-then-update needs one writer — a mutex around park/seat/score updates."
2. **Persistence:** "an appendix-only log or a store class; on restart, rebuild state from it."
3. **Failure:** "status machine → retry N times → dead-letter list → notify the user; never silent loss."
4. **Fairness:** "aging for priority systems; round-robin for equal groups."
5. **Scale:** "partition by id — per-printer / per-lift / per-area queues; a dispatcher routes."
6. **Search speed:** "prebuild an index map at setup; lookups become O(1)."
7. **Extensibility:** "new behaviour = new subclass registered in a map — no edits (Open/Closed)."
8. **Testing:** "state is private behind methods — each class is testable alone."

# PART E — SOLID in one breath each (they may name-drop it)

- **S — Single responsibility:** "Account does banking, not printing. One class, one reason to change."
- **O — Open/Closed:** the calculator (B15): "extend by new subclasses, not by editing tested code."
- **L — Liskov substitution:** "any Car must work anywhere a Vehicle* is expected — that's why overrides keep the contract (file 01)."
- **I — Interface segregation:** "small, focused virtual hooks (type/interest/withdrawLimit) instead of one fat base class."
- **D — Dependency inversion:** "Calculator depends on the Operation interface, not on Add directly — swap implementations freely."

# PART F — Tonight, with the clock

- **Type:** 01, 02, 04, 05 (must) → 06, 08 (should) → 03a/03b/03e (DSA minimum).
- **Read:** this handbook once — by B19 you'll see every pattern is the same 6 moves.
- **Recite aloud once each:** the 6-stage printing ladder (B5), the parking escalations (B2), the movie-seat HELD trick (B11), the calculator Open/Closed line (B15).
- If only 10 minutes remain: read B5 + B15 + the DS table. Those three cover the highest-probability asks.
