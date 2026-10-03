# PRCV (ML-411T) — Complete Revision Pack

Pattern Recognition & Computer Vision · Units 1–4 notes, diagrams and solved practice problems.

## What is inside

| Folder / file | What it is |
|---|---|
| `1_Unit1_Unit2_Notes/PRCV_Unit1_Unit2_Notes.md` | **Unit 1 + Unit 2 notes** (induction algorithms, Naïve Bayes, SVM, GA, statistical pattern recognition, Fisher, feed-forward nets) with 28 diagrams in `images/` |
| `2_Unit3_Unit4_Notes/PRCV_Unit3_Unit4_Notes.md` | **Unit 3 + Unit 4 notes** (filtering, thresholding, edges, corners, morphology, texture, binary shape analysis, snakes, chain codes, Fourier descriptors, moments) with 41 diagrams in `images/` |
| `3_Practice_Problems/PRCV_Unit1_Unit2_Practice_Problems.md` | 13 **solved numericals** for Units 1–2 (entropy/gain, Naïve Bayes + Laplace, Bayes, k-NN, SVM, GA, backprop, least squares, Fisher, MLE, Kalman …) |
| `5_Unit3_Unit4_Practice_Problems/PRCV_Unit3_Unit4_Practice_Problems.md` | 17 **solved numericals** for Units 3–4 (histogram equalisation, filters, Otsu, Sobel, Harris, morphology, GLCM, LBP, labelling, chamfer distance transform, chain code, Zhang–Suen, moments, Fourier descriptors, snake energy, occlusion) |
| `4_Cheat_Sheet/PRCV_Unit1_Unit2_CheatSheet.md` | **One-glance cheat sheet** for Units 1–2 + **48 rapid-fire Q&A** + a list of marks-losing traps — for the night before the exam |
| `standalone_single_file_versions/` | The same two notes files with every image **embedded** — each works on its own, no `images/` folder needed (larger files) |

## How to open

- **Keep each notes `.md` next to its `images/` folder** (the layout above already does this).
- Best viewers (render both the LaTeX formulas and the images): **Typora**, **Obsidian**, **VS Code** (Ctrl+Shift+V = Markdown Preview), **Jupyter**, **GitHub**.
- A plain text editor (Notepad etc.) will show raw LaTeX — use one of the viewers above.

## Marking conventions used in the notes

- **[+]** = added from standard textbooks (Duda–Hart–Stork, Bishop, Davies, Gonzalez & Woods).
- **⚠ Fix** = an error in the original class PDFs; do not copy it into an exam answer.

## Important corrections found in the original PDFs

1. **Naïve Bayes golf example (Unit 1):** the PDF mixes two versions of the dataset. From the printed 14-row table: P(Yes) ≈ 0.822. With the standard textbook table: 0.67 / 0.33. Prediction is "Yes" either way — count from the table you are given.
2. **Factory decision tree:** "Net Not Expand" is 3.2 M, not 3 M.
3. **m-connectivity** is the *mixed* 4/8 rule, not "more than 8 directions".
4. **Labelling example** shows 2 objects, not 3; **size-filtering example** object 2 has 5 pixels, not 4.
5. **4-connected chain code** has only 4 axis directions; the raw chain code is not rotation-invariant (its first difference is).

## Suggested revision order (Units 1 & 2 exam)

1. Read the Unit 1–2 notes once, using the *Table of contents* and the *Likely exam questions* section.
2. Redo the numericals in `3_Practice_Problems` on paper (problems 1, 2, 3, 9, 10 first).
3. Finish with the *Formula sheet* and *Last-minute checklist* at the end of the notes.
4. If you also need Units 3–4: read the Unit 3–4 notes, then redo `5_Unit3_Unit4_Practice_Problems` (problems 1, 3, 4, 6, 9, 10, 11, 13 first).
5. Night before (Units 1–2): `4_Cheat_Sheet` — cover the answers in Part B and quiz yourself; read Part C (traps) last.
