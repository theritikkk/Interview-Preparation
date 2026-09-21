# DBMS → SQL II: Subqueries, Set Operations, Views

**GATE 2027 syllabus:** Databases — SQL.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Standard SQL semantics with three-valued logic. In set operations (UNION, INTERSECT, EXCEPT) two NULLs are treated as equal. Use the following tables for Q1–Q10.

**Student(sid, sname, dept)**

| sid | sname | dept |
|---|---|---|
| 1 | Ann | CS |
| 2 | Bob | CS |
| 3 | Cat | EE |
| 4 | Dan | EE |
| 5 | Eve | ME |

**Course(cid, cname, credits)**

| cid | cname | credits |
|---|---|---|
| 101 | DB | 4 |
| 102 | OS | 4 |
| 103 | CN | 3 |
| 104 | AI | 3 |

**Enroll(sid, cid, grade)** — primary key (sid, cid)

| sid | cid | grade |
|---|---|---|
| 1 | 101 | A |
| 1 | 102 | B |
| 1 | 103 | A |
| 1 | 104 | C |
| 2 | 101 | B |
| 2 | 102 | B |
| 3 | 101 | A |
| 3 | 103 | B |
| 4 | 103 | NULL |

---

## Questions

### Q1 · NAT · Fundamental
Let X = `SELECT dept FROM Student UNION ALL SELECT dept FROM Student` and Y = `SELECT dept FROM Student UNION SELECT dept FROM Student`. (Number of rows of X) − (number of rows of Y) = ______.

---

### Q2 · MCQ · Fundamental
The value returned by
```sql
SELECT COUNT(*) FROM Student WHERE sid IN (SELECT sid FROM Enroll);
```
is

- (A) 3
- (B) 4
- (C) 5
- (D) 9

---

### Q3 · NAT · Standard
```sql
SELECT COUNT(*) FROM Student s
WHERE EXISTS (SELECT 1 FROM Enroll e WHERE e.sid = s.sid AND e.grade = 'A');
```
The value returned is ______.

---

### Q4 · NAT · Standard
```sql
SELECT sname FROM Student s
WHERE NOT EXISTS (SELECT 1 FROM Course c
                  WHERE NOT EXISTS (SELECT 1 FROM Enroll e
                                    WHERE e.sid = s.sid AND e.cid = c.cid));
```
The number of rows returned is ______.

---

### Q5 · MCQ · Standard
The value returned by
```sql
SELECT COUNT(*) FROM Student
WHERE sid NOT IN (SELECT sid FROM Enroll WHERE grade <> 'A');
```
is

- (A) 1
- (B) 2
- (C) 3
- (D) 5

---

### Q6 · NAT · Moderate
```sql
SELECT cname FROM Course
WHERE credits >= ALL (SELECT credits FROM Course);
```
The number of rows returned is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about views is/are TRUE?

- (A) A view defined by a join of two tables is always updatable.
- (B) A view is a virtual table defined by a stored query.
- (C) `WITH CHECK OPTION` rejects inserts and updates through the view that produce rows not satisfying the view's WHERE condition.
- (D) A materialised view stores its result physically.

---

### Q8 · NAT · Moderate
```sql
SELECT s.dept, COUNT(*)
FROM Student s JOIN Enroll e ON s.sid = e.sid
GROUP BY s.dept
HAVING COUNT(*) > (SELECT AVG(cnt)
                   FROM (SELECT COUNT(*) AS cnt FROM Enroll GROUP BY sid) t);
```
The sum of the values in the second column over all returned rows is ______.

---

### Q9 · NAT · Hard
```sql
SELECT grade FROM Enroll
EXCEPT
SELECT grade FROM Enroll WHERE sid = 1;
```
The number of rows returned is ______.

---

### Q10 · NAT · Killer
Let Q(k) be the value returned by
```sql
SELECT COUNT(*) FROM Student s
WHERE NOT EXISTS (SELECT 1 FROM Course c
                  WHERE c.credits = k
                    AND NOT EXISTS (SELECT 1 FROM Enroll e
                                    WHERE e.sid = s.sid AND e.cid = c.cid
                                      AND e.grade IS NOT NULL));
```
Then Q(4) + Q(9) = ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 7 | 10 rows − 3 distinct depts |
| 2 | B | Ann, Bob, Cat, Dan; Eve has no enrolment |
| 3 | 2 | Ann and Cat |
| 4 | 1 | Ann is enrolled in all four courses |
| 5 | B | Dan (only a NULL grade) and Eve (no rows) never appear in the subquery |
| 6 | 2 | DB and OS (maximum credits = 4) |
| 7 | B, C, D | (A) false in general |
| 8 | 9 | CS 6 and EE 3; average enrolments per enrolled student = 2.25; ME has no rows |
| 9 | 1 | Only the NULL grade remains; NULLs are equal in EXCEPT |
| 10 | 7 | Q(4) = 2 (Ann, Bob); Q(9) = 5 (no course has 9 credits, so the outer NOT EXISTS is vacuously true) |
