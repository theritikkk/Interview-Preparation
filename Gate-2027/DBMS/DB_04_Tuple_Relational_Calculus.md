# DBMS → Tuple Relational Calculus

**GATE 2027 syllabus:** Databases — tuple calculus.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Relations are sets. A tuple variable ranges over the tuples of the relation named by its range predicate, e.g. Student(s). Quantifiers are written ∃x(…) / ∀x(…). Use the following instance for Q3, Q4, Q5, Q6, Q8 and Q10.

**Student(sid, name, age):** (1, Ann, 20), (2, Bob, 22), (3, Cat, 21), (4, Dan, 23), (5, Eve, 20), (6, Fay, 22)

**Course(cid, title, dept):** (101, DB, CS), (102, OS, CS), (103, VLSI, EE), (104, Heat, ME)

**Enrolled(sid, cid, grade):** (1,101,A), (1,102,A), (1,103,B), (1,104,A), (2,101,A), (2,102,F), (3,103,A), (3,104,F), (4,101,B), (4,102,B), (4,103,F), (4,104,B), (5,101,B), (5,104,A)

---

## Questions

### Q1 · MCQ · Fundamental
The tuple calculus expression { t | Student(t) ∧ t.age > 20 } is equivalent to

- (A) σ_{age>20}(Student)
- (B) π_{age>20}(Student)
- (C) Student − σ_{age>20}(Student)
- (D) σ_{age>20}(Student) × Student

---

### Q2 · MCQ · Fundamental
Which one of the following tuple calculus expressions is **unsafe** (it can define an infinite relation)?

- (A) { t | Student(t) ∧ t.age > 20 }
- (B) { t | ¬Student(t) }
- (C) { t | ∃s (Student(s) ∧ t.name = s.name) }
- (D) { t | Student(t) ∧ ∃e (Enrolled(e) ∧ e.sid = t.sid) }

---

### Q3 · NAT · Standard
The number of tuples returned by { t | ∃e (Enrolled(e) ∧ e.grade = 'A' ∧ t.sid = e.sid) } is ______.

---

### Q4 · MSQ · Standard
Which of the following expressions define "students who have no F grade in any course"?

- (A) { s | Student(s) ∧ ¬∃e (Enrolled(e) ∧ e.sid = s.sid ∧ e.grade = 'F') }
- (B) { s | Student(s) ∧ ∀e ((Enrolled(e) ∧ e.sid = s.sid) → e.grade ≠ 'F') }
- (C) { s | Student(s) ∧ ∃e (Enrolled(e) ∧ e.sid = s.sid ∧ e.grade ≠ 'F') }
- (D) { s | Student(s) ∧ ∀e (Enrolled(e) ∧ e.sid = s.sid ∧ e.grade ≠ 'F') }

---

### Q5 · NAT · Standard
The number of tuples returned by

{ s | Student(s) ∧ ∃e ∃c (Enrolled(e) ∧ Course(c) ∧ e.sid = s.sid ∧ e.cid = c.cid ∧ c.dept = 'CS') ∧ ¬∃f (Enrolled(f) ∧ f.sid = s.sid ∧ f.grade = 'F') }

is ______.

---

### Q6 · MCQ · Moderate
Which one of the following expressions is equivalent to π_name(σ_{age>21}(Student ⋈ Enrolled))?

- (A) { t | ∃s ∃e (Student(s) ∧ Enrolled(e) ∧ s.sid = e.sid ∧ s.age > 21 ∧ t.name = s.name) }
- (B) { t | ∃s (Student(s) ∧ s.age > 21 ∧ t.name = s.name) }
- (C) { t | ∃s ∀e (Student(s) ∧ Enrolled(e) ∧ s.sid = e.sid ∧ s.age > 21 ∧ t.name = s.name) }
- (D) { t | ∃s ∃e (Student(s) ∧ Enrolled(e) ∧ s.age > 21 ∧ t.name = s.name) }

---

### Q7 · MSQ · Moderate
Which of the following statements is/are TRUE?

- (A) Safe tuple relational calculus and relational algebra have the same expressive power.
- (B) Every tuple calculus expression defines a finite relation.
- (C) The transitive closure of a binary relation can be expressed in relational algebra.
- (D) Division can be expressed in tuple calculus using a universal quantifier.

---

### Q8 · NAT · Moderate
The number of tuples returned by

{ s | Student(s) ∧ ∀c (Course(c) → ∃e (Enrolled(e) ∧ e.sid = s.sid ∧ e.cid = c.cid)) }

is ______.

---

### Q9 · MSQ · Hard
R(A, B) and S(A, B) are relations. Which of the following expressions define π_A(R) − π_A(S)?

- (A) { t | ∃r (R(r) ∧ t.A = r.A) ∧ ¬∃s (S(s) ∧ s.A = t.A) }
- (B) { t | ∃r (R(r) ∧ t.A = r.A ∧ ¬∃s (S(s) ∧ s.A = r.A)) }
- (C) { t | ∃r (R(r) ∧ t.A = r.A ∧ ∃s (S(s) ∧ s.A ≠ r.A)) }
- (D) { t | ∃r ∃s (R(r) ∧ S(s) ∧ r.A ≠ s.A ∧ t.A = r.A) }

---

### Q10 · NAT · Killer
The number of tuples returned by

{ s | Student(s) ∧ ∀c ∀e ((Course(c) ∧ Enrolled(e) ∧ e.sid = s.sid ∧ e.cid = c.cid) → c.dept = 'CS') }

is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | A | Selection |
| 2 | B | Negation alone ranges over all possible tuples |
| 3 | 4 | sid 1, 2, 3, 5 |
| 4 | A, B | Ann, Eve, Fay; (C) also returns Bob, Cat, Dan and drops Fay; (D) is empty |
| 5 | 2 | Ann and Eve (Bob and Dan have an F) |
| 6 | A | (B) and (D) also return Fay; (C) is wrong |
| 7 | A, D | (B) is false for unsafe expressions; (C) neither RA nor TRC can express transitive closure |
| 8 | 2 | Ann and Dan are enrolled in all four courses |
| 9 | A, B | (C) and (D) test "some other A value exists" instead of "no matching A value" |
| 10 | 2 | Bob (only CS courses) and Fay (no enrolments, vacuously true) |
