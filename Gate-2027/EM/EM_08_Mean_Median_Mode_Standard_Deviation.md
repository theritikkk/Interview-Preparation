# Engineering Mathematics → Mean, Median, Mode & Standard Deviation

**GATE 2027 syllabus:** Engineering Mathematics — Probability and Statistics: Mean, median, mode and standard deviation.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** For a dataset of n values, mean = (sum)/n. Median is the middle value when sorted (average of the two middle values if n is even). Mode is the most frequently occurring value (a dataset can be multimodal). Variance here means the **population** variance = (1/n)Σ(xᵢ − mean)², and standard deviation is its square root, unless a question explicitly asks for the sample version (dividing by n−1 instead).

---

## Questions

### Q1 · MCQ · Fundamental
For the dataset {2, 4, 4, 4, 5, 5, 7, 9}, the mode is

- (A) 4
- (B) 4.5
- (C) 5
- (D) There is no mode (all values are distinct)

---

### Q2 · NAT · Fundamental
For the dataset {2, 4, 4, 4, 5, 5, 7, 9}, the mean is ______.

---

### Q3 · MSQ · Standard
Which of the following statements is/are TRUE?

- (A) The median of a dataset is unaffected by extreme outlier values (as long as they remain on the same side of the median), while the mean can be strongly affected.
- (B) A dataset can have more than one mode.
- (C) Adding a constant c to every value in a dataset increases the mean by c but leaves the standard deviation unchanged.
- (D) Multiplying every value in a dataset by a constant c multiplies the standard deviation by c² .

---

### Q4 · NAT · Standard
For the dataset {2, 4, 4, 4, 5, 5, 7, 9}, the median is ______  (give as a decimal, e.g. 4.5).

---

### Q5 · MCQ · Standard
For the dataset {2, 4, 4, 4, 5, 5, 7, 9}, the population standard deviation is

- (A) 1.5
- (B) 2.0
- (C) 4.0
- (D) 2.83

---

### Q6 · NAT · Moderate
A dataset has values 10, 20, 30, 40, 50 occurring with frequencies 2, 3, 5, 4, 1 respectively (15 data points in total). The mean of this grouped data, rounded to two decimal places, is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements is/are TRUE?

- (A) For a symmetric, unimodal distribution, the mean, median and mode all coincide at the same value.
- (B) For a right-skewed (positively skewed) distribution, typically mode < median < mean.
- (C) The sample variance (dividing the sum of squared deviations by n − 1 instead of n) is used specifically to give an unbiased estimate of the population variance when working from a sample.
- (D) Standard deviation is always measured in the same units as the original data, whereas variance is measured in squared units.

---

### Q8 · NAT · Moderate
The dataset {10, 20, 30, 40, 50} occurring with frequencies {2, 3, 5, 4, 1} (same as Q6, 15 data points) has population variance ______  (rounded to two decimal places).

---

### Q9 · MSQ · Hard
Which of the following statements about the effect of transforming a dataset is/are TRUE?

- (A) If every value xᵢ is transformed to yᵢ = a·xᵢ + b (a, b constants, a ≠ 0), then mean(y) = a·mean(x) + b.
- (B) Under the same transformation, Var(y) = a²·Var(x) (the additive constant b has no effect on variance).
- (C) Under the same transformation, median(y) = a·median(x) + b, provided a > 0 (so that the sorted order is preserved).
- (D) The relation median(y) = a·median(x) + b continues to hold even when a < 0, despite the transformation reversing the sorted order of the data.

---

### Q10 · NAT · Killer
A dataset {2, 4, 4, 4, 5, 5, 7, 9} (same as Q1–Q2, Q4–Q5; mean 5, population standard deviation 2.0) is transformed by yᵢ = 3xᵢ − 4 for every value. The new dataset's mean plus its population standard deviation (i.e., mean(y) + std(y)) is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | A | 4 appears three times, more than any other value |
| 2 | 5 | Sum = 40, n = 8 |
| 3 | A, B, C | (D) is false — scaling by c multiplies the standard deviation by \|c\|, not c² (that's the effect on variance) |
| 4 | 4.5 | Sorted: 2,4,4,4,5,5,7,9; average of the 4th and 5th values (4 and 5) |
| 5 | B | Population variance = 4.0, so std = 2.0 |
| 6 | 29.33 | (10×2+20×3+30×5+40×4+50×1)/15 = 440/15 |
| 7 | A, B, C, D | All four are standard, correct facts |
| 8 | 126.22 | Population variance of the same grouped data, computed from the mean 29.33 |
| 9 | A, B, C, D | All four are standard, correct facts about linear transformations of data (D describes the reversal correctly, since reversing order still preserves which original value sits at the middle rank) |
| 10 | 17 | mean(y) = 3(5) − 4 = 11; std(y) = \|3\| × 2.0 = 6; 11 + 6 |
