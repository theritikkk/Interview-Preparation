# 07 — Book Map: what to read, where, and in what order

> Page numbers come from each book's own contents. **How to find a page in your PDF:**
> - **Davies:** PDF page ≈ printed page **+ 37** (checked on chapters 5, 13 and 24; the first chapters sit a little differently — always confirm with the running header).
> - **Beyerer:** I give **PDF page numbers** directly.
> - **Baggio:** I give **PDF page numbers** directly.
> - **DHS (Duda–Hart–Stork):** your PDF is a **scanned image without a text layer** (you cannot search it unless you OCR it). PDF page ≈ printed page **+ 19** near the start (Chapter 2 starts at printed p. 20 = PDF p. 39). Printed pages for chapters 2, 9 and 10 were confirmed in your scan; the others are the standard 2nd-edition contents — **verify with the book's own contents page.**
> - **The Beyerer EPUB** in your upload duplicates the Beyerer PDF; use whichever reads better.
> - **Baggio PDF** contains chapters 1–8 only (chapter 9, the Kinect "fluid wall", is not in the file).

---

## 1. Davies — *Computer and Machine Vision: Theory, Algorithms, Practicalities* (27 chapters)

| Ch. | Title | Printed p. | Read it for… | Matches |
|---|---|---|---|---|
| 1 | Vision, the Challenge | 1 | The framing: why vision is hard, inverse problems, design philosophy | file 01 |
| **Part 1 — Low-level vision** | | | | |
| 2 | Images and Imaging Operations | 17 | Image formation, sampling, basic operations | 03 §1.1 |
| 3 | Basic Image Filtering Operations | 38 | Smoothing, sharpening, median, rank filters | 03 §1.2 |
| 4 | Thresholding Techniques | 82 | Global/adaptive thresholds, Otsu-type methods | 03 §1.3 |
| 5 | Edge Detection | 111 | Gradient operators, Canny, sub-pixel location | 03 §1.4 |
| 6 | Corner and Interest Point Detection | 149 | Harris, FAST, SIFT-like features | 03 §1.5 |
| 7 | Mathematical Morphology | 185 | Erosion/dilation, opening/closing, reconstruction | 03 §1.6 |
| 8 | Texture | 209 | Texture descriptors | 03 §1.7 |
| **Part 2 — Intermediate-level vision** | | | | |
| 9 | Binary Shape Analysis | 229 | Connected components, measurements, skeletons | 03 §2.1 |
| 10 | Boundary Pattern Analysis | 266 | Chain codes, signatures, Fourier descriptors | 03 §2.2 |
| 11 | Line Detection | 284 | Hough for lines, line models | 03 §2.3 |
| 12 | Circle and Ellipse Detection | 303 | Hough for circles/ellipses, tricks for speed | 03 §2.3 |
| 13 | The Hough Transform and Its Nature | 333 | Why voting works; generalised Hough | 03 §2.3 |
| 14 | Pattern Matching Techniques | 358 | Template matching, robust matching | 03 §2.4 |
| **Part 3 — 3-D vision and motion** | | | | |
| 15 | The Three-Dimensional World | 389 | Imaging geometry, depth cues, stereo | 03 §3.1, 3.3 |
| 16 | Tackling the Perspective *n*-point Problem | 424 | Pose from points (PnP) | 03 §3.2 |
| 17 | Invariants and Perspective | 439 | Projective invariants, vanishing points | 03 §3.2 |
| 18 | Image Transformations and Camera Calibration | 478 | Homographies, calibration | 03 §3.1 |
| 19 | Motion | 505 | Optical flow, motion analysis | 03 §3.5 |
| **Part 4 — Toward real-time pattern recognition systems** | | | | |
| 20 | Automated Visual Inspection | 525 | Inspection tasks and system design | 03 §4.1, 05 §1 |
| 21 | Inspection of Cereal Grains | 553 | A complete worked inspection design | 05 §1, §9 |
| 22 | Surveillance | 578 | Background models, tracking, particle filters | 03 §3.5, 05 §7 |
| 23 | In-Vehicle Vision Systems | 636 | Road/sign/vehicle detection, constraints | 05 §3 |
| 24 | Statistical Pattern Recognition | 672 | Bayes, neural nets, ROC, features — vision-practitioner view | 02 |
| 25 | Image Acquisition | 718 | Cameras, **illumination**, optics | 03 §4.2 |
| 26 | Real-Time Hardware and Systems Design Considerations | 742 | Platforms, speed, system design | 03 §4.4 |
| 27 | Epilogue — Perspectives in Vision | 767 | Trade-offs and the big picture | 01, 03 §4.4 |

**Davies' reading style:** dense, mathematical where needed, strong on *practicalities* (speed, robustness, cost). Read the introduction and summary of each chapter first; do the derivations only for the methods you plan to use.

---

## 2. Duda, Hart & Stork — *Pattern Classification* (2nd ed.)

| Ch. | Title | Printed p. | Read it for… | Matches |
|---|---|---|---|---|
| 1 | Introduction | 1 | Pattern-recognition problem, design cycle, overfitting, the "fish-sorting" running example | 01, 02 |
| 2 | Bayesian Decision Theory | **20** | Minimum-error and minimum-risk rules, discriminant functions, Gaussian classes, reject option, error bounds, ROC | 02 §1 |
| 3 | Maximum-Likelihood and Bayesian Parameter Estimation | 84 | MLE, Bayesian estimation, EM, curse of dimensionality, Hidden Markov models | 02 §2–3 |
| 4 | Nonparametric Techniques | 161 | Parzen windows, k-NN, metrics, edited/condensed neighbours | 02 §4 |
| 5 | Linear Discriminant Functions | 215 | Perceptron, LMS, support vector machines, relaxation | 02 §5 |
| 6 | Multilayer Neural Networks | 282 | Backpropagation, practical tricks (§6.8), regularisation | 02 §6 |
| 7 | Stochastic Methods | 350 | Simulated annealing, Boltzmann learning, genetic methods | (college Unit 1 GA) |
| 8 | Nonmetric Methods | 394 | Decision trees (CART, ID3, C4.5), string recognition, rule-based | 02 §7 |
| 9 | Algorithm-Independent Machine Learning | **453** | No-free-lunch, ugly-duckling, bias–variance, resampling, bagging/boosting, classifier combination | 02 §8 |
| 10 | Unsupervised Learning and Clustering | **517** | Mixtures, k-means, hierarchical clustering, PCA, ICA, SOM | 02 §9 |
| A | Mathematical Foundations | ≈601 | Linear algebra, probability, information theory refreshers | — |

**Reading tip:** Chapters 2–3 are the heart; spend time there. Chapter 9 is short and has some of the best *conceptual* material in the book. Read the chapter-end summaries first. Because your copy is a scan, **OCR it** (a free OCR tool will give a searchable PDF) if you want to use it for reference.

---

## 3. Beyerer, Richter & Nagel — *Pattern Recognition: Introduction, Features, Classifiers and Principles* (2018)

| Ch. | Title | PDF p. | Read it for… | Matches |
|---|---|---|---|---|
| 1 | Fundamentals and definitions (1.1 goals; 1.2 structure of a PR system) | 24 | Vocabulary, system structure, design steps | 01, 02 §11 |
| 2 | Features (2.1 types; 2.3 transformations; 2.4 distances; 2.6 selection & construction) | 35 | Feature types, normalisation, PCA/LDA-type transforms, distance measures, selection | 02 §10 |
| 3 | Bayesian decision theory (3.2 MAP classifier) | 144 | Decision theory, risk, rejection | 02 §1 |
| 4 | Parameter estimation (4.1 ML, 4.2–4.3 Bayesian) | 175 | Estimation theory | 02 §2 |
| 5 | Parameter-free methods (5.1 Parzen window) | 202 | Parzen, k-NN | 02 §4 |
| 6 | General considerations | 225 | Dimensionality, overfitting, regularisation | 02 §3 |
| 7 | Special classifiers (7.1 linear discriminants; 7.2 perceptron; 7.3 linear regression; 7.4 ANNs; 7.6 **deep learning**) | 237 (7.6 at 250) | Linear models, neural networks, a short introduction to deep learning and CNNs | 02 §5–6 |
| 8 | Classification with nominal features (8.4 grammars) | 288 | Trees, string/structural recognition | 02 §7 |
| 9 | Classifier-independent concepts (9.1 learning theory; 9.2 **empirical evaluation**) | 306 (9.2 at 311) | VC theory, evaluation protocols, ROC, **rejection** | 02 §11 |

**Why read it alongside DHS:** it is newer (includes deep learning), treats **feature design and evaluation** very practically, and has worked exercises.

---

## 4. Baggio et al. — *Mastering OpenCV with Practical Computer Vision Projects* (2012)

| Ch. | Project | PDF p. | Core techniques | Matches |
|---|---|---|---|---|
| 1 | Cartoonifier and Skin Changer | 22 | Median/bilateral filtering, Laplacian edges, colour spaces, performance tuning | 04 A1 |
| 2 | Marker-based AR (iOS) | 55 | Contours → quads, perspective rectification, calibration, pose, OpenGL | 04 A2 |
| 3 | Marker-less AR | 95 | Features, matching, homography, tracking | 04 A3 |
| 4 | Exploring Structure from Motion | 123 | Epipolar geometry, triangulation, PnP, bundle adjustment, optical flow, point clouds | 04 A4, 03 §3.3–3.4 |
| 5 | Number Plate Recognition (SVM + NN) | 148 | Sobel/morphology localisation, SVM verification, character segmentation, ANN OCR | 04 A5 |
| 6 | Non-rigid Face Tracking | 172 | Point-distribution (shape) model, patch models, Procrustes | 04 A6 |
| 7 | 3-D Head Pose Estimation (AAM + POSIT) | 208 | Active appearance models, pose-from-points | 04 A7 |
| 8 | Face Recognition (Fisherfaces) | 231 | Detection, pre-processing, Eigenfaces/Fisherfaces/LBP, rejection of unknowns | 04 A8 |
| Index | | ≈263 | | |

**How to use it:** read the *design discussion* of each chapter (the "why"), then re-implement the pipeline in modern OpenCV/Python rather than copying the 2012 code (APIs changed; platform-specific parts such as Android/iOS may not apply).

---

## 5. Reading plans by goal

| Goal | Read (in order) | Do alongside |
|---|---|---|
| **Understand the whole field fast (1 week)** | DHS ch. 1–2 (skim 3–5) → Beyerer ch. 1, 9.2 → Davies ch. 1, 27 | File 01; labs 01, 02 |
| **Classical vision foundations (4–6 weeks)** | Davies ch. 2–7, 9, 11–14 | Labs 03, 04, 05; projects 1, 3, 5 |
| **Machine-vision / inspection career** | Davies ch. 20, 21, 25, 26, 27, then 3–5, 7, 9 | File 03 Part 4; project 2; lighting experiments with a phone camera |
| **3-D, AR, robotics** | Davies ch. 15–19 → Baggio ch. 2–4, 7 | Labs 06, 07; projects 4, 9 |
| **Pattern-recognition theory** | DHS ch. 2–6, 9 → Beyerer ch. 3, 4, 6, 9 | File 02; lab 01–02; rederive §13 of file 02 |
| **Faces and biometrics** | Baggio ch. 6–8 → DHS ch. 10 → Beyerer ch. 2 | Project 6; fairness evaluation (file 05 §7) |
| **Interview prep (2 weeks)** | File 06 → Davies ch. 5, 6, 13 (summaries) → DHS ch. 2, 9 summaries | `code/10` on a whiteboard |
| **Move to deep learning with good foundations [Modern]** | Beyerer §7.4–7.6 → DHS ch. 6 | Project 6(c) fine-tuning; then a modern DL course/book of your choice |

---

## 6. How to read a technical chapter (3 passes)

1. **Skim (10 min):** title, introduction, headings, figures, summary → write *one sentence* on what problem the chapter solves.
2. **Understand (40–90 min):** read for the *idea* of each method; ask "what assumption makes this work? when does it break?"; re-derive the key equation on paper.
3. **Apply (60+ min):** implement the smallest version, **predict** what the output will look like, run it, compare. Put the lesson (and one failure) in your notebook.

**Notebook template per method:** *Problem · Idea in one sentence · Key equation · Assumptions · Typical parameters · Failure modes · Where used · One experiment I ran.*

---

## 7. Where each college topic is explained most deeply

| College topic | Best source |
|---|---|
| Bayes decision theory, discriminants | DHS ch. 2; Beyerer ch. 3 |
| Naïve Bayes, MLE, Bayesian estimation | DHS ch. 3; Beyerer ch. 4 |
| k-NN, Parzen | DHS ch. 4; Beyerer ch. 5 |
| Linear discriminants, perceptron, SVM, Fisher | DHS ch. 5, §3.8; Beyerer ch. 7 |
| Neural networks / backprop | DHS ch. 6; Beyerer §7.4–7.6 |
| Decision trees | DHS ch. 8; Beyerer ch. 8 |
| Genetic algorithms | DHS ch. 7 |
| Curse of dimensionality, overfitting | DHS §3.7, ch. 9; Beyerer ch. 6 |
| Filtering, thresholding, edges | Davies ch. 3–5 |
| Corners / interest points | Davies ch. 6 |
| Morphology | Davies ch. 7 |
| Texture | Davies ch. 8 |
| Shape analysis, skeletons, moments, boundary descriptors | Davies ch. 9–10 |
| Hough, matching | Davies ch. 11–14 |
| Camera, calibration, homography | Davies ch. 15–18; Baggio ch. 2–4 |
| Optical flow, tracking, occlusion | Davies ch. 19, 22; Baggio ch. 4 |
| Active contours / shape models | Baggio ch. 6–7 (PDM, AAM); Davies ch. 10 (boundary methods) |

---

## 8. Gaps in these four books (so you know what to learn elsewhere) **[Modern]**
- **Deep learning for vision** (CNN architectures, detection, segmentation, transformers, self-supervised learning, generative models): only introduced in Beyerer §7.6.
- **Modern SLAM / learned geometry** (NeRF, learned depth/flow): beyond these books.
- **MLOps, data engines, deployment optimisation:** not covered.
- **Fairness, privacy, regulation:** only touched on indirectly.
- **Large-scale retrieval/embeddings, foundation models:** beyond scope.
**File `08_Deep_Learning_for_Vision_Bridge.md` covers the essentials of the first, fourth and part of the second gap from general knowledge (it is tagged [Modern] and should be checked against current sources).** Plan to complement these books with current course material, library documentation and recent papers once the foundations here feel solid.
