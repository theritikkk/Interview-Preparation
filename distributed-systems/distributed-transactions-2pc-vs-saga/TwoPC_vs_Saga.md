# Distributed Transactions: Two-Phase Commit vs. the Saga Pattern

### Real services, real injected failure, both patterns run side by side — the exact same failure handled two fundamentally different ways

---

## The problem

The multi-hop RPC chain earlier in this series (`A → B → C`) propagated a deadline and a trace ID, but never asked: what if this call chain represents **one transaction** — debit an account, reserve inventory, ship an order — and one step fails? Does the whole thing roll back? Does partial work stay committed? This is the actual, common, hard problem distributed transactions solve, demonstrated here with three real services (Account, Inventory, Shipping) and two real orchestrators implementing the two standard answers.

## Two-Phase Commit (2PC): prevent the bad state from ever existing

**Phase 1 — PREPARE**: ask every participant "can you do this?" Each one *tentatively reserves* the resource — checks feasibility, locks it — but does **not** touch real state yet, and votes yes/no.
**Phase 2 — COMMIT or ABORT**: if every vote was yes, tell everyone to commit (now safe, since each already proved it could). If *any* vote was no, tell **everyone** — including yes-voters — to abort, releasing their holds.

**Real run — the failure case:**
```
--- PHASE 1: PREPARE ---
  coordinator -> account   PREPARE -> vote=yes
  coordinator -> inventory PREPARE -> vote=yes
  coordinator -> shipping  PREPARE -> vote=no
--- shipping VOTED NO -> PHASE 2: GLOBAL ABORT ---
  coordinator -> account   ABORT (even services that voted yes release their hold)
  coordinator -> inventory ABORT (even services that voted yes release their hold)
RESULT: transaction ABORTED everywhere -- real state untouched anywhere.

STATE before: balance=70 stock=3     STATE after: balance=70 stock=3   <- IDENTICAL
```
**Real state never moved**, anywhere, despite account and inventory both having said yes. This is 2PC's entire value: the bad outcome is *prevented*, not *corrected afterward*.

**The cost, equally real**: between phase 1 and phase 2, every yes-voter is blocked holding its reservation — funds are marked unavailable, stock is marked reserved — unable to serve any *other* transaction, until the coordinator decides. A slow or dead coordinator means every participant sits locked indefinitely. This blocking-until-the-slowest-one-decides cost is exactly why 2PC is used far less in large-scale systems than it once was.

## Saga: let it happen, undo it if needed

Each step **commits real state immediately**, one after another. No participant is ever blocked waiting on another. If a later step fails, the orchestrator runs **compensating actions** for every earlier step, in reverse order.

**Real run — the failure case:**
```
--- STEP 1: debit account (REAL, immediate) ---
  orchestrator -> account   debit -> {'ok': True}        <- balance ACTUALLY drops, right now
--- STEP 2: reserve inventory (REAL, immediate) ---
  orchestrator -> inventory reserve -> {'ok': True}       <- stock ACTUALLY drops, right now
--- STEP 3: ship order (REAL, immediate) ---
  orchestrator -> shipping  ship -> {'ok': False}
--- STEP 3 FAILED -> running compensations, in REVERSE order ---
  orchestrator -> inventory compensate_reserve -> {'ok': True}
  orchestrator -> account   compensate_debit -> {'ok': True}
RESULT: saga FAILED, but all prior real effects were undone.
```
Final state matches pre-transaction state — but **during the window between step 2 and the compensation**, the account genuinely was debited and the stock genuinely was reduced. Any concurrent read in that window sees a state that will later be reversed — something 2PC never allows to be observable.

## The actual trade-off

| | 2PC | Saga |
|---|---|---|
| Participants blocked mid-transaction? | Yes — every yes-voter holds until the coordinator decides | No — each step is independent and immediate |
| Bad state ever observable? | Never | Yes, briefly, until compensation runs |
| Needs compensating logic? | No | Yes — every step author must write an "undo" |
| Scales to many services/high latency? | Poorly — blocking cost grows with slowest participant | Well — this is why most large microservice systems use sagas |

## Where this shows up

Sagas are the standard pattern in real e-commerce/payments microservice architectures (exactly this account/inventory/shipping shape) — AWS Step Functions and Temporal/Cadence are built specifically to orchestrate saga-style compensating workflows reliably. 2PC survives inside single-database-engine transaction managers and some message brokers' XA support, but is rarely used *across independent services* at scale, for exactly the blocking reason above. **Interview framing**: "design an order checkout" is, underneath, almost always asking you to recognize this exact trade-off and justify picking one.

---

## Quick recap

- 2PC prevents bad state from ever existing, at the cost of blocking every participant until the slowest one is heard from.
- Saga allows real, temporary bad state, requiring explicit compensation — in exchange, no participant is ever blocked on another.
- Verified directly: the 2PC failure run left state **completely unchanged**; the Saga failure run left state unchanged **only after** a real debit and reservation had already happened and been explicitly reversed.
