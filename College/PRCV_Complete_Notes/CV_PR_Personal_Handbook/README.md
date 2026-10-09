# Computer Vision & Pattern Recognition — Personal Handbook

> **Purpose:** *my own* long-term understanding, project building, domain knowledge and job readiness.
> **Not** an exam pack. It is **separate from the college Unit 1–4 notes** (different folder, different zip, different style) — those are syllabus-shaped; this is understanding-shaped.

**Built from the four books you uploaded** (plus a clearly tagged **[Modern]** bridge file for deep learning)

| Short name | Book | What it gives you |
|---|---|---|
| **Davies** | E. R. Davies, *Computer and Machine Vision: Theory, Algorithms, Practicalities* (27 chapters) | Classical vision, end to end — from pixels to **real inspection systems** (lighting, optics, speed) |
| **DHS** | Duda, Hart & Stork, *Pattern Classification*, 2nd ed. | The **theory** of recognition: Bayes decision theory, estimation, discriminants, networks, trees, clustering |
| **Beyerer** | Beyerer, Richter & Nagel, *Pattern Recognition: Introduction, Features, Classifiers and Principles* (2018) | A modern, compact treatment: features, evaluation, rejection, learning theory, **a chapter on deep learning** |
| **Baggio** | D. L. Baggio et al., *Mastering OpenCV with Practical Computer Vision Projects* (2012) | **Eight complete projects** (AR, SfM, plate reading, face tracking, face recognition …) with code |

Where a statement comes from general professional knowledge rather than these books (mostly the deep-learning era and industry practice) it is tagged **[Modern]**. Tools and library versions move fast — treat those parts as a map, and verify details before relying on them.

---

## Contents of this folder

| File | What is in it | Read it when you want to… |
|---|---|---|
| `01_Big_Picture_and_Mental_Models.md` | The 10 ideas that organise the whole field; classical vs deep learning; glossary | …*understand* the field instead of memorising it |
| `02_Pattern_Recognition_Deep_Understanding.md` | Bayes decision theory → estimation → curse of dimensionality → k-NN, SVM, nets, trees → evaluation, rejection, clustering, with **my own measured experiments** | …choose and debug classifiers, or explain them in an interview |
| `03_Classical_Computer_Vision_Deep_Understanding.md` | Filtering, thresholds, edges, corners, morphology → shape, Hough, RANSAC → camera, calibration, stereo, motion → real systems; **measured labs** | …build vision systems that work outside the demo |
| `04_Projects_Playbook.md` | Baggio's 8 projects distilled + 12 portfolio blueprints with metrics and pitfalls | …plan projects that impress and teach |
| `05_Domains_and_Industry_Knowledge.md` | Inspection, surveillance, automotive, medical, documents, biometrics, AR/robotics, agriculture… | …speak the language of a domain |
| `06_Job_Skills_and_Interview_Prep.md` | Role map, skill matrix, system-design recipe, ~70 interview questions with answers, whiteboard list | …prepare for jobs and interviews |
| `08_Deep_Learning_for_Vision_Bridge.md` | **[Modern] bridge notes** for what the books lack: CNN arithmetic, a gradient-checked from-scratch CNN, training/debugging, detection metrics, calibration, deployment, hybrid classical+deep patterns | …connect the classical foundations to today's practice |
| `07_Book_Map.md` | Chapter-by-chapter reading guide with page numbers; what to read for which goal; reading plans | …know exactly where to look in your books |
| `projects/01_gauge_starter/` | A **ready-to-run, tested starter project** (calibrated measurement gauge): synthetic ground-truth scene, calibration + held-out validation, mm measurement, CLI for real photos, 8 exercises | …start building immediately (blueprint #2 of file 04) |
| `code/` | 13 tested Python scripts (every number in the notes comes from them) + `run_all.sh` | …re-run, modify and learn by experiment |
| `images/` | 18 figures (diagrams + results of the experiments) | …revise visually |

## How this relates to your college notes

| College unit | Where the *deep* version lives here |
|---|---|
| Unit 1 (induction, Naïve Bayes, SVM, GA, instance-based) | `02` §1–7 |
| Unit 2 (statistical PR, features, curse, Bayes, Fisher, NN) | `02` §1–5, §8–10 |
| Unit 3 (filtering, thresholds, edges, corners, morphology, texture) | `03` Part 1 |
| Unit 4 (shape, connectedness, skeletons, snakes, chain codes, moments) | `03` Part 2 |
| *(not in the syllabus)* 3-D vision, calibration, motion, real systems, deployment, jobs | `03` Parts 3–4, `04`–`06` |

## A suggested way to use it

1. **Skim `01` once** (30 min) — the mental models make everything else easier.
2. **Pick a project in `04`** and let it pull you through `02`/`03` as needed — projects teach faster than chapters.
3. **Run the labs in `code/`** before reading the matching section; *predict* the result first, then compare with what is written. Wrong predictions are where learning happens.
4. **Before interviews**: `06`, plus `code/10_snippets_from_scratch.py` (be able to write these on a whiteboard).
5. **Keep `07` open** while reading the books.

## Honest limitations

- The books are **classical** (Davies/Baggio 2012–2018, DHS 2001). Deep learning is covered only briefly (Beyerer ch. 7); file `08` and the **[Modern]** sections bridge the gap from general knowledge. *No deep-learning framework was available when I built this, so file 08's experiments use a from-scratch NumPy CNN and calculators, not real modern networks.*
- Your **Baggio PDF contains chapters 1–8** (chapter 9, the Kinect "fluid wall", is not in the file). The **DHS PDF is a scan** without a text layer (use the PDF's page images, or OCR it, to search). The Beyerer **EPUB duplicates the PDF**.
- My experiments are **small, synthetic or bundled-dataset demos** designed to show *effects*, not to benchmark methods. Their absolute numbers will differ on your data; the **patterns** are what to remember.
