# DBMS → SQL I: Joins, Aggregation, GROUP BY / HAVING, NULLs

**GATE 2027 syllabus:** Databases — SQL.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Standard SQL semantics with three-valued logic (comparisons with NULL are UNKNOWN). Use the following tables for **all** questions. `mgr` is the `eid` of the employee's manager.

**Dept(dno, dname)**

| dno | dname |
|---|---|
| 10 | CS |
| 20 | EE |
| 30 | ME |
| 40 | CE |

**Emp(eid, name, dno, salary, mgr)**

| eid | name | dno | salary | mgr |
|---|---|---|---|---|
| 1 | Asha | 10 | 50 | NULL |
| 2 | Bala | 10 | 60 | 1 |
| 3 | Chitra | 20 | 60 | 1 |
| 4 | Dev | 20 | NULL | 3 |
| 5 | Esha | NULL | 40 | 3 |
| 6 | Farid | 30 | 50 | 2 |
| 7 | Gita | 10 | 70 | 2 |
| 8 | Hari | 20 | 50 | 3 |

---

## Questions

### Q1 · NAT · Fundamental
```sql
SELECT COUNT(salary) + COUNT(DISTINCT dno) FROM Emp;
```
The value returned is ______.

---

### Q2 · MCQ · Fundamental
The value returned by `SELECT AVG(salary) FROM Emp;` (rounded to two decimals) is

- (A) 47.50
- (B) 54.29
- (C) 50.00
- (D) 60.00

---

### Q3 · NAT · Standard
```sql
SELECT d.dname, e.name
FROM Dept d LEFT JOIN Emp e ON d.dno = e.dno;
```
The number of rows returned is ______.

---

### Q4 · NAT · Standard
```sql
SELECT dno, COUNT(*) FROM Emp GROUP BY dno;
```
The number of rows returned is ______.

---

### Q5 · MCQ · Standard
The value returned by
```sql
SELECT COUNT(*) FROM Emp WHERE salary <> 50 OR salary = 50;
```
is

- (A) 8
- (B) 7
- (C) 6
- (D) 5

---

### Q6 · NAT · Moderate
```sql
SELECT e.name
FROM Emp e JOIN Emp m ON e.mgr = m.eid
WHERE e.salary > m.salary;
```
The number of rows returned is ______.

---

### Q7 · NAT · Moderate
```sql
SELECT dno, AVG(salary)
FROM Emp
GROUP BY dno
HAVING AVG(salary) >= 55;
```
The number of rows returned is ______.

---

### Q8 · MSQ · Moderate
Which of the following queries return exactly one row containing the value 8?

- (A) `SELECT COUNT(*) FROM Emp;`
- (B) `SELECT COUNT(salary) FROM Emp;`
- (C) `SELECT COUNT(*) FROM Emp e LEFT JOIN Dept d ON e.dno = d.dno;`
- (D) `SELECT COUNT(*) FROM Emp e JOIN Dept d ON e.dno = d.dno;`

---

### Q9 · NAT · Hard
```sql
SELECT COUNT(*) FROM Emp
WHERE eid NOT IN (SELECT mgr FROM Emp);
```
The value returned is ______.

---

### Q10 · NAT · Killer
```sql
SELECT d.dno, COUNT(*)
FROM Dept d LEFT JOIN Emp e ON d.dno = e.dno
GROUP BY d.dno
HAVING COUNT(*) < 2;
```
The sum of the values in the second column over all returned rows is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 10 | COUNT(salary) = 7 (one NULL), COUNT(DISTINCT dno) = 3 |
| 2 | B | 380 / 7; NULL is ignored, so not 380 / 8 |
| 3 | 8 | CS 3 + EE 3 + ME 1 + CE 1 (no employees, padded with NULL) |
| 4 | 4 | Groups 10, 20, 30 and one group for NULL |
| 5 | B | The row with NULL salary gives UNKNOWN for both conditions |
| 6 | 3 | Bala, Chitra, Gita |
| 7 | 2 | dno 10 (60) and dno 20 (55, NULL ignored) |
| 8 | A, C | (B) 7; (D) 7 |
| 9 | 0 | The subquery returns a NULL (Asha's mgr), so NOT IN is never TRUE |
| 10 | 2 | ME gives 1 and CE gives 1: COUNT(*) counts the NULL-extended row |
