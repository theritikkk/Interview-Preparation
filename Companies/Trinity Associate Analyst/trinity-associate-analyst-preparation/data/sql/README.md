# SQL practice database — setup notes

This folder supports **Module 23: SQL for Business and Survey Analytics**.

> All data is **fictional**. It was generated for interview practice. It is not Trinity data, not real company data, and not indicative of any real market.

## What is here

| File | What it is |
|---|---|
| `trinity_sql_practice.db` | Ready-to-open SQLite database |
| `trinity_sql_practice.sql` | SQLite dump (schema + data); rebuild the database from this |
| `csv/` | One CSV per table, for inspection or import into another database |
| `README.md` | This file |

## Open the database

### Option A — DB Browser for SQLite (recommended if you prefer a GUI)

1. Install DB Browser for SQLite.
2. Open `trinity_sql_practice.db`.
3. Go to the **Execute SQL** tab.
4. Run `SELECT name FROM sqlite_master WHERE type='table';` to see the tables.

### Option B — SQLite command line

```bash
sqlite3 trinity_sql_practice.db
```

Useful shell commands:

```text
.tables
.schema sales
.headers on
.mode column
SELECT * FROM regions;
.quit
```

### Option C — Rebuild from the dump

```bash
sqlite3 /tmp/trinity_sql_practice.db < trinity_sql_practice.sql
```

## Tables and row counts

| Table | Rows | Grain |
|---|---:|---|
| `regions` | 4 | one row per region |
| `products` | 3 | one row per product |
| `reps` | 12 | one row per sales representative |
| `sales` | 72 | month × region × product |
| `physicians` | 42 | one row per physician |
| `survey` | 42 | one row per survey response (40 unique physicians; 2 duplicate responses) |
| `survey_items` | 126 | response × Likert item (42 × 3) |
| `calls` | 120 | one row per rep call |
| `prescriptions` | 276 | physician × product × month where TRx > 0 |
| `quotas` | 5 | one row per awareness stage |

## Dialect

The database is SQLite. The module teaches standard SQL, with SQLite syntax for the runnable examples and a dialect map for PostgreSQL, MySQL, and SQL Server. The CSV folder lets you load the same data into another engine.

## Important design notes

- `survey` contains two exact duplicate responses (`P-007` and `P-023`) and one out-of-window response (`P-030`). These are deliberate data-quality exercises.
- `survey_items` contains one invalid Likert code (`7`) and one `N/A` value. These are deliberate.
- `sales` has two blank targets and three blank call counts. These are deliberate.
- `prescriptions` is consistent with the June survey snapshot: June TRx = 344. It also includes two non-respondent physicians (`P-041`, `P-042`) who stop prescribing after April, so retention queries are meaningful.
- Revenue and TRx figures are in fictional units; do not use them as market estimates.
