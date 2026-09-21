# DBMS → ER Model

**GATE 2027 syllabus:** Databases — ER-model.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions (standard ER-to-relational mapping, used in every counting question):** each strong entity set → one table; a weak entity set → one table that includes the primary key of its owner; each multivalued attribute → its own table; each M:N or ternary relationship → its own table; a 1:N relationship → a foreign key on the N side (no extra table); a 1:1 relationship → a foreign key on either side (no extra table), except that a 1:1 relationship with **total participation on both sides** is merged into a single table; specialization → one table for the superclass and one table per subclass. "Minimum number of tables" always means under these rules.

---

## Questions

### Q1 · MCQ · Fundamental
A weak entity set is uniquely identified by

- (A) its own primary key
- (B) its partial key together with the primary key of its owner entity set
- (C) a foreign key alone
- (D) any candidate key of the owner entity set

---

### Q2 · NAT · Fundamental
An ER diagram has entity sets A, B, C, D and these relationships:

- A–B: many-to-many
- B–C: one-to-many (one B, many C), with total participation of C
- C–D: one-to-one, with total participation of both C and D

The minimum number of tables needed is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about the ER-to-relational mapping is/are TRUE?

- (A) In a 1:N relationship, the primary key of the "one" side is added as a foreign key to the table of the "many" side.
- (B) The table of a weak entity set includes the primary key of its owner entity set.
- (C) In first normal form a multivalued attribute can be stored as a single column of the entity's table.
- (D) A derived attribute must always be stored as a column.

---

### Q4 · NAT · Standard
E1 has 3 entities and E2 has 4 entities. R is a one-to-one relationship between E1 and E2 in which **every** entity of E1 participates (E2 participation is partial). The number of different relationship sets R that are possible is ______.

---

### Q5 · MCQ · Standard
R is a ternary relationship among entity sets A, B and C. For every pair (a, b) there is at most one c related to it, but a given a may be related to many b's, and a given pair (a, c) may occur with many b's. In the table for R, the primary key is

- (A) {a}
- (B) {a, b}
- (C) {a, b, c}
- (D) {b, c}

---

### Q6 · MCQ · Moderate
Vehicle(vid, …) is specialised **partially** into Car and Truck: some vehicles belong to neither subclass. Which of the following mappings cannot represent every vehicle?

- (A) One table for Vehicle and one table for each subclass (with the key)
- (B) One table per subclass only, each containing all inherited attributes, and no table for Vehicle
- (C) A single table with all attributes and a "type" column
- (D) A single table with all attributes and one Boolean flag per subclass

---

### Q7 · MSQ · Moderate
"Each employee works in exactly one department. A department may have any number of employees, possibly none." Which of the following statements about the relationship Works_in between Employee and Department is/are TRUE?

- (A) Works_in is many-to-one from Employee to Department.
- (B) Employee participates totally in Works_in.
- (C) Department participates totally in Works_in.
- (D) A separate table for Works_in is required in a minimal relational design.

---

### Q8 · NAT · Moderate
Entity sets: Student(sid, name, {phone}) with multivalued attribute phone; Course(cid, title); Department(did, dname); Dependent, a weak entity set owned by Student with partial key dname.

Relationships: Enrolls (Student–Course, many-to-many, attribute grade); BelongsTo (Student–Department, many-to-one, total on Student); Offers (Department–Course, one-to-many, total on Course); Has (Student–Dependent, one-to-many, identifying).

The minimum number of tables is ______.

---

### Q9 · NAT · Hard
Entity sets: Doctor(did, name, {phone}); Patient(pid, name); Hospital(hid, hname); Drug(drugid, dname); Ward, a weak entity set owned by Hospital (partial key wno); Room, a weak entity set owned by Ward (partial key rno).

Relationships: Treats (Doctor–Patient, many-to-many, attribute date); WorksAt (Doctor–Hospital, many-to-one); Prescribes (Doctor–Patient–Drug, ternary); AdmittedTo (Patient–Ward, many-to-one); Has_Ward and Has_Room are the identifying relationships of Ward and Room.

The minimum number of tables is ______.

---

### Q10 · MSQ · Killer
Which of the following constraints can be enforced in a relational schema using only primary-key, foreign-key and NOT NULL constraints (no triggers, no assertions)?

- (A) Every employee must belong to a department.
- (B) Every department must have at least one employee.
- (C) An employee belongs to at most one department.
- (D) Every student must be enrolled in at least one course (Student–Course is many-to-many).

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Partial key + owner's key |
| 2 | 4 | A, B, R(A–B), and one merged table for C and D |
| 3 | A, B | Multivalued attributes need their own table; derived attributes need not be stored |
| 4 | 24 | Injective maps from 3 entities to 4: 4·3·2 |
| 5 | B | The FD {a, b} → c holds in R |
| 6 | B | Vehicles in neither subclass would have no row anywhere |
| 7 | A, B | (D): a foreign key in Employee is enough |
| 8 | 6 | Student (+ dept FK), Student_phone, Course (+ dept FK), Department, Enrolls, Dependent |
| 9 | 9 | Doctor (+ hospital FK), Doctor_phone, Patient (+ ward FK), Hospital, Ward, Room, Drug, Treats, Prescribes |
| 10 | A, C | (A) NOT NULL foreign key; (C) single-valued foreign key column; (B) and (D) need at-least-one constraints |
