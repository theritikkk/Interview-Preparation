# 06 — Job Skills and Interview Preparation

> Aim: turn the knowledge in files 01–05 into a **hireable profile** and into **crisp answers**. The labs in `code/` give you evidence to quote ("I measured…"). **[Modern]** marks content from general professional knowledge rather than your four books — job requirements and tools change quickly, so check current job postings for the specifics.

---

## 1. Role map

| Role (titles vary) | Day-to-day | Emphasis | Typical tools |
|---|---|---|---|
| **Computer-vision / perception engineer** | Build and ship detection, segmentation, tracking, 3-D | Deep learning + geometry + engineering | Python, C++, PyTorch, OpenCV, ONNX, Docker |
| **Machine-vision / application engineer** | Design inspection stations: lighting, optics, algorithm, integration | Lighting, optics, classical vision, PLC integration, customer work | Vision software (vendor/OpenCV/Halcon-type), GigE/USB3 cameras, PLC |
| **ML engineer (vision)** | Training pipelines, data engines, evaluation, deployment | Data + MLOps + model quality | PyTorch, experiment tracking, cloud, CI |
| **Research scientist / applied scientist** | New methods, papers, benchmarks | Maths, experimentation, writing | PyTorch/JAX, large-scale training |
| **Robotics perception engineer** | Calibration, SLAM, depth, grasping | Geometry, estimation, real-time C++ | ROS, Eigen, Ceres/GTSAM-type, PCL/Open3D |
| **Embedded/edge vision engineer** | Make it run on small hardware | Optimisation, quantisation, memory/latency | C/C++, TensorRT/TFLite/ONNX Runtime, SoC toolchains |
| **Medical-imaging ML engineer** | Segmentation/classification under regulation | Data governance, validation, DICOM | PyTorch/MONAI-type, PACS/DICOM |
| **Solutions / forward-deployed engineer** | Fit the technology to a customer's problem | Communication + breadth + debugging | Everything above, lightly |

**How to choose:** if you like *physics, hardware and customers* → machine vision. *Maths/geometry* → perception/robotics. *Data and scale* → ML engineer. *Reading papers* → research. *Making things fast* → edge.

---

## 2. Skill matrix (what to be able to *do*, and how to prove it)

| Area | Foundation (can explain & use) | Working (can build alone) | Advanced (can design & debug in production) | Proof |
|---|---|---|---|---|
| **Maths** | Linear algebra (vectors, matrices, eigen/SVD), probability (Bayes, Gaussians), calculus (gradients) | Optimisation (least squares, gradient descent), estimation (MLE, Kalman) | Robust & non-linear optimisation, bundle adjustment | `code/10` implementations |
| **Python & scientific stack** | NumPy, plotting | Vectorised code, profiling, packaging | Performance engineering, testing | Repo with tests |
| **C++** | Read OpenCV C++ | Write efficient CV code | Real-time, memory-aware systems | A C++ module with benchmarks |
| **Image processing** | Filters, thresholds, morphology | Pipelines for segmentation/measurement | Metrology-grade measurement | Gauge project (file 04 #2) |
| **Classical CV** | Edges, corners, matching | Calibration, homography, RANSAC, tracking | SfM/SLAM components, stereo | AR/pose + stitcher projects |
| **Pattern recognition / ML** | Bayes, k-NN, SVM, trees, CV | Proper evaluation, feature design | Calibration, imbalance, cost-sensitive design | Classifier study (file 04 #6) |
| **Deep learning for vision [Modern]** | CNN basics, transfer learning | Train/fine-tune detectors & segmenters | Architecture choices, training stability, distillation | Fine-tuned model with error analysis |
| **Data & labelling** | Splits, leakage | Label guidelines, active learning | Data engines, quality metrics | Dataset README |
| **Deployment [Modern]** | Export a model | Optimise/quantise, benchmark on target | Monitoring, drift, rollbacks | Edge-benchmark project |
| **Optics & lighting** | Pixel size, FOV, exposure/blur | Choose lighting & lens | Telecentric/structured-light design | Lighting study with photos |
| **Communication** | Clear READMEs | Design docs, trade-off write-ups | Customer/stakeholder alignment | Writing samples |

---

## 3. Math you should be able to write from memory

1. Bayes' rule, likelihood-ratio test with costs.
2. Gaussian density; Mahalanobis distance; MLE of mean/covariance.
3. Least squares: $\hat w=(X^TX)^{-1}X^Ty$; ridge: $(X^TX+\lambda I)^{-1}X^Ty$.
4. PCA via SVD / eigen-decomposition of covariance; Fisher's direction $S_W^{-1}(m_2-m_1)$.
5. Pinhole projection $x\sim K[R|t]X$; homography DLT; epipolar constraint $x'^TFx=0$; $E=K'^TFK$.
6. RANSAC iteration count $N=\ln(1-p)/\ln(1-w^s)$.
7. Convolution; separable filters; Gaussian; gradient magnitude/direction.
8. Harris matrix and response; Lucas–Kanade normal equations.
9. Kalman predict/update.
10. IoU, precision, recall, AP; softmax and cross-entropy; backprop for one layer.
All of 1–9 appear (with checks) in your college notes and in `code/10_snippets_from_scratch.py`.

---

## 4. The CV system-design recipe (use it in interviews and in real life)

**Say these steps out loud:**
1. **Clarify the goal & constraints** — what decision does the output drive? Latency, throughput, accuracy in real units, hardware, budget, privacy, regulation.
2. **Define errors and metrics** — cost of each error type; imbalance; the metric that matches the business (recall at fixed false-alarm rate, mm error, count error).
3. **Understand the scene and data** — can we control lighting/viewpoint? What variation exists (shifts, seasons, products)? How will data be collected, labelled, split (by group/time)? What is the long tail?
4. **Baseline** — simplest thing that could work; measure it.
5. **Model** — choose among classical geometry, features + classifier, or a pretrained deep model fine-tuned; justify with data size and variability (file 01 Figure "method chooser").
6. **Evaluate properly** — locked test set, confidence intervals, per-condition breakdown, error analysis, calibration.
7. **Serve** — latency budget per stage; edge vs cloud; batching; hardware; failure/fallback path (reject → human).
8. **Monitor & iterate** — drift detection, sampling audits, feedback loop to retrain.
9. **Risks & ethics** — bias, privacy, safety, adversarial/spoofing.

### Worked example A — "Detect defects on a bottle-cap line, 600 caps/min"
- **Budget:** 600/min = 10/s ⇒ ≤100 ms per cap end-to-end; with 2 ms exposure strobe and one camera per station.
- **Resolution:** smallest defect 0.3 mm; need ≥3 px ⇒ ≤0.1 mm/px; cap FOV 40 mm ⇒ ≥ 400 px across (use ≥ 800 px for margin).
- **Lighting:** diffuse dome for glossy plastic; dark-field to highlight scratches.
- **Classical first:** threshold/blob for missing liner and gross damage; golden-template comparison for print; CNN/anomaly model for cosmetic scratches.
- **Metrics:** **escape rate** (defects passed) ≤ target; **false-reject rate** ≤ target; both with confidence intervals from ≥ N defective samples (the standard error of a rate $p$ is $\sqrt{p(1-p)/n}$ — to pin a 1 % rate down to ±0.2 % at 95 % confidence you need about 9,500 samples).
- **Production:** save all rejected images + a random 1 % of accepted ones; weekly audit; re-calibrate lighting; alarm if the average brightness or focus score drifts.

### Worked example B — "Count people entering a store, 500 stores × 8 cameras"
- **Bandwidth reasoning:** a 1080p H.264 stream is on the order of **4 Mbps**; 4,000 cameras ⇒ ~16 Gbps if streamed to the cloud. **Process at the edge**, send only counts/metadata (a few kb/s).
- **Pipeline:** detector (person) → tracker → virtual line crossing → counts; calibrate once per camera (homography to floor plane for accuracy).
- **Accuracy protocol:** manually count sampled hours per store type; report error % by lighting/crowding/camera height; track **ID switches** (the main double-counting source).
- **Privacy:** no raw video leaves the store; blur/discard faces at the edge; retention limits; document consent where required.
- **Ops:** fleet management (remote model updates, health checks), per-store drift monitoring.

---

## 5. Interview questions with model answers

*(Answers are deliberately short — expand with an example or a number from your own labs. Cover the answer, say yours aloud, then compare.)*

### A. Fundamentals of ML / pattern recognition
1. **Bias–variance trade-off?** Error = bias² + variance + noise. Simple models underfit (high bias); flexible models fit noise (high variance). Pick complexity using validation data; reduce variance with more data/regularisation/ensembles; reduce bias with better features/richer models.
2. **How do you detect and fight overfitting?** Large gap between training and validation error. Remedies: more/augmented data, regularisation (L2/L1/dropout/early stopping), simpler model, fewer features, cross-validation for model choice.
3. **L1 vs L2 regularisation?** L1 gives sparse weights (diamond constraint has corners on axes); L2 shrinks all weights smoothly. L1 = feature selection; L2 handles correlated features better.
4. **Why is accuracy a bad metric for imbalanced data?** A majority-class rule scores high (95 % when 5 % positives) yet finds none. Use precision/recall, PR curves, recall at fixed precision, or cost-weighted error. *My lab: the accuracy-optimal rule caught 31 % of defects; the cost-optimal threshold caught 84 %.*
5. **ROC vs precision–recall?** ROC (TPR vs FPR) is insensitive to prevalence; with very rare positives PR curves reveal poor precision that ROC hides.
6. **What is calibration and why care?** Predicted probability matches observed frequency. Needed for threshold setting, risk-based decisions and human trust. Check with reliability diagrams; fix with Platt scaling/isotonic regression/temperature scaling.
7. **Generative vs discriminative?** Generative models $p(x\mid y)p(y)$ (Naïve Bayes, LDA, GMM) → can sample, handle missing data, need correct density assumptions; discriminative models $p(y\mid x)$ (logistic regression, SVM, nets) → usually more accurate with enough data.
8. **State Bayes' decision rule with unequal costs.** Choose the action minimising expected cost: for two classes decide $\omega_1$ if $\frac{p(x|\omega_1)}{p(x|\omega_2)}>\frac{(\lambda_{12}-\lambda_{22})P(\omega_2)}{(\lambda_{21}-\lambda_{11})P(\omega_1)}$.
9. **Curse of dimensionality?** Volume grows exponentially → sparse data, distances concentrate, more parameters than data. Mitigate with feature selection/reduction, regularisation, priors/structure. *Lab: 400 noise features dropped k-NN from 97.7 to 61.5 % but the SVM only to 91.8 %.*
10. **PCA vs LDA?** PCA: unsupervised, max variance. LDA: supervised, max between/within class scatter, at most C−1 dimensions. PCA may keep lighting variance that hurts recognition; LDA targets class separation (why Fisherfaces beat Eigenfaces under varying light).
11. **Why standardise features?** Distance/gradient-based methods are scale-sensitive; fit the scaler on training data only to avoid leakage.
12. **When would you pick k-NN? SVM? Random forest?** k-NN: tiny data/good embeddings, simple baseline; SVM: moderate data, high-dim features, strong margin-based baseline; RF/boosting: tabular data, little tuning. *Lab: k-NN won with 20 samples; the SVM overtook it by 300.*
13. **What is data leakage? Examples.** Information from the test set influencing training: scaling on all data, near-duplicate frames split across train/test, same patient in both sets, tuning on the test set. Prevent by splitting by group/time and locking a final test set.
14. **How would you estimate how many test samples you need?** Standard error of accuracy ≈ $\sqrt{p(1-p)/n}$; a 95 % interval is about ±1.96 standard errors, so for 99 % ± 0.5 % you need about 1,500 test samples.
15. **Explain the reject option.** Abstain when the max posterior is below a threshold; trades coverage for accuracy. *Lab: rejecting the 30 % least-confident samples raised accuracy on the rest from 95.9 % to 99.4 %.*

### B. Classical computer vision
16. **Convolution vs correlation?** Convolution flips the kernel; correlation doesn't. Identical for symmetric kernels. (Libraries often compute correlation and call it convolution.)
17. **Why is a Gaussian blur fast?** Separable: a 2-D $k\times k$ kernel = two 1-D passes → $O(k)$ per pixel instead of $O(k^2)$. *(Check: 13×13: 169 vs 26 multiplications.)*
18. **Median vs Gaussian filtering?** Median is non-linear, removes salt-and-pepper noise and preserves edges; Gaussian is linear, best for Gaussian-like noise and as scale-space smoothing.
19. **Explain Canny.** Gaussian smoothing → gradients → non-maximum suppression → double threshold → hysteresis edge tracking.
20. **What is the aperture problem?** Locally an edge constrains motion/position only perpendicular to itself; corners resolve both directions. Hence corners are good features.
21. **Explain Harris.** Structure matrix of gradient products in a window; $R=\det M-k(\mathrm{tr}M)^2$; both eigenvalues large → corner; one large → edge. (Lucas–Kanade uses the same matrix.)
22. **SIFT vs ORB?** SIFT: scale-space DoG keypoints + 128-D gradient histogram, scale/rotation invariant, accurate but slower. ORB: oriented FAST + binary BRIEF descriptors, matched with Hamming distance, very fast, a bit less accurate. *Lab: SIFT 0.8 px vs ORB 3–10.7 px homography error under degradation.*
23. **Why Lowe's ratio test?** Compare best to second-best match distance; ambiguous matches have similar distances and are rejected (typical threshold 0.7–0.8).
24. **Explain RANSAC and iteration count.** Repeatedly fit a model to minimal random samples, keep the largest consensus set, refit. $N=\ln(1-p)/\ln(1-w^s)$; e.g. homography ($s=4$, $w=0.5$, $p=0.99$) ≈ 72 iterations.
25. **How does the Hough transform work and why is it robust?** Each edge point votes for all parameter values of shapes passing through it; peaks = shapes. Missing or noisy parts simply fail to vote. *Lab: circles still found 95 % of the time with half the circle hidden.*
26. **Otsu's method?** Choose the threshold maximising between-class variance $\sigma_B^2=P_1P_2(\mu_1-\mu_2)^2$. Assumes roughly bimodal histogram and balanced classes.
27. **Global vs adaptive thresholding?** Global: one threshold, needs uniform lighting. Adaptive: per-neighbourhood threshold, handles shading; sensitive to block size/offset.
28. **What does morphological opening do? closing?** Opening (erode→dilate) removes small objects/specks; closing (dilate→erode) fills small holes/gaps. Top-hat removes slowly varying background.
29. **Integral image?** Summed-area table allowing any rectangular sum in 4 lookups → constant-time box filters, Haar features.
30. **Distance transform and uses?** Distance to nearest background for each object pixel; skeletons (ridge), watershed seeds, shape thickness, chamfer matching.
31. **Connected components?** Two-pass labelling with union–find (or flood fill); choose 4/8 connectivity (dual pairing for background).
32. **Template matching and its limits?** NCC is invariant to brightness/contrast but not rotation/scale/perspective; use pyramids, feature-based matching or learned detectors for those.
33. **Histogram equalisation?** Map intensities through the CDF to spread the histogram and boost contrast; global (can over-amplify noise) → CLAHE for local contrast with a clip limit.
34. **Image pyramids?** Repeated blur+downsample; coarse-to-fine search/matching, multi-scale detection, large-motion optical flow.
35. **Optical flow vs tracking by detection?** Flow estimates dense/sparse pixel motion assuming brightness constancy (Lucas–Kanade: constant motion in a window; pyramids for large motion). Tracking-by-detection detects every frame and associates; combine with Kalman prediction for robustness.
36. **Background subtraction failure modes?** Lighting change, shadows, camera shake, dynamic backgrounds, stopped objects.
37. **How do you measure something precisely with a camera?** Calibrate (intrinsics, distortion), fix pose relative to the measured plane, use telecentric optics if possible, sub-pixel edge localisation, controlled lighting, validate against a physical standard, report repeatability and uncertainty.

### C. Geometry and 3-D
38. **Pinhole model & parameters?** $x\sim K[R|t]X$; intrinsics $K$ (focal lengths, principal point, skew); extrinsics $[R|t]$; plus lens distortion.
39. **How does calibration work?** Capture a planar pattern in many poses; detect corners; solve for intrinsics/distortion/extrinsics by minimising reprojection error (Zhang's method). Need pose diversity; validate independently. *Lab: near-parallel views gave 72 px focal error vs 7 px at 30° tilts, though the RMS reprojection error was the same.*
40. **Homography vs fundamental matrix?** $H$: maps points between two views of a *plane* (or pure rotation); $F$: epipolar constraint for general 3-D scenes — a point maps to a *line*.
41. **Fundamental vs essential?** $E=K'^TFK$ for calibrated cameras; $E$ has 5 DoF and decomposes into $R$ and $t$ (up to scale, four candidate solutions, resolved by positive depth).
42. **How many points to estimate H, F, E?** 4, 8 (7 minimal), 5 (minimal) — used inside RANSAC.
43. **How does triangulation work?** Intersect the rays from two (or more) calibrated views; solved by DLT/SVD or optimisation; accuracy depends on baseline and viewing angle.
44. **Why is monocular SfM scale-ambiguous?** Multiplying all 3-D points and translations by a constant leaves images unchanged; fix scale with a known length, IMU, stereo baseline or depth sensor.
45. **What is bundle adjustment?** Joint non-linear least-squares refinement of all camera poses and 3-D points minimising reprojection error; sparse structure makes it feasible; prevents drift.
46. **PnP?** Pose from $n$ 3-D–2-D correspondences; P3P minimal (up to 4 solutions), EPnP/iterative for more; RANSAC for outliers. Basis of marker AR.
47. **Stereo depth and its error?** $Z=fB/d$; $\Delta Z\approx Z^2\Delta d/(fB)$: error grows with the *square* of distance; larger baseline/focal length helps. *Lab: marker depth error 0.8 mm at 0.5 m vs 56 mm at 3 m.*
48. **Why is planar pose from a marker unreliable face-on?** Two nearly equal pose solutions (planar ambiguity); *lab:* rotation error 1.8–9° face-on vs 0.4° at 45–80° tilt.
49. **Rolling vs global shutter?** Rolling exposes rows at different times → skew/wobble on fast motion and bad for geometry; global exposes all at once (needed for fast inspection and precise 3-D).
50. **How do you handle lens distortion?** Model radial/tangential coefficients via calibration; undistort before measurement or include distortion in the projection model.

### D. Deep learning for vision [Modern]
51. **Why CNNs for images?** Local connectivity + weight sharing give translation equivariance with far fewer parameters; depth builds hierarchical features; pooling gives local invariance.
52. **Receptive field?** The input region influencing a unit; grows with depth, kernel size, stride and dilation; must cover the object scale.
53. **Transfer learning/fine-tuning?** Start from a model pretrained on large data; replace head; train head first then (carefully) the whole network with a small learning rate. Works well with thousands of images or fewer.
54. **Data augmentation?** Random crops/flips/rotations/colour jitter etc. teach invariances and reduce overfitting; must preserve labels (don't flip text; keep it physically plausible).
55. **Batch normalisation / residual connections?** BN stabilises and speeds training by normalising activations; skip connections ease gradient flow, enabling very deep networks.
56. **One-stage vs two-stage detectors?** Two-stage: region proposals then classification/refinement (accurate, slower); one-stage: dense predictions directly (fast, simpler). Transformer-based detectors remove hand-crafted components like NMS in some designs.
57. **IoU, NMS, mAP?** IoU = intersection/union of boxes; NMS removes duplicate boxes by suppressing lower-score overlaps; AP = area under precision–recall curve per class at an IoU threshold; mAP averages over classes (and often IoU thresholds). *(Implemented and checked in `code/10`.)*
58. **Semantic vs instance vs panoptic segmentation?** Per-pixel class; per-object masks; both together.
59. **CNN vs Vision Transformer?** CNN: strong built-in locality bias, data-efficient; ViT: global attention, less inductive bias, needs more data or pretraining but scales well.
60. **Self-supervised/contrastive learning?** Learn embeddings without labels by making augmentations of the same image close and others far (or by masked reconstruction); used for pretraining when labels are scarce.
61. **Class imbalance in detection/segmentation?** Re-weighting, focal loss, hard-negative mining, oversampling rare classes, appropriate metrics.
62. **Domain shift?** Train/test distribution mismatch (camera, season, site). Detect via input statistics/performance audits; mitigate with diverse data, augmentation, fine-tuning on target data, domain adaptation.
63. **How do you shrink a model for the edge?** Lower input resolution, smaller backbone, pruning, quantisation (e.g. INT8, with calibration/QAT), distillation, operator fusion, export to an optimised runtime; always re-measure accuracy and *end-to-end* latency.
64. **Embeddings & metric learning?** Train a network so same-class items are close and different-class far (triplet/contrastive/margin losses); enables retrieval, few-shot recognition, face verification.

### E. Systems and practical judgement
65. **Production accuracy dropped — what do you do?** Check inputs first (camera moved? lighting/lens changed? exposure/focus?), compare input statistics with training, sample and inspect failures, check pipeline/version changes, verify labels/ground truth, then retrain with new data. Add monitoring so you see it earlier.
66. **How do you label efficiently?** Clear guidelines with examples, pre-label with a model and correct, active learning for uncertain samples, measure inter-annotator agreement, audit regularly.
67. **How do you choose the operating threshold?** From the cost ratio and the ROC/PR curve on held-out data, validate on a fresh set, review periodically as prevalence changes.
68. **How do you explain a vision model's decision?** Saliency/attribution maps (with caution), nearest training examples, error analysis by condition, simpler surrogate models, and — in measurement tasks — traceable geometric computations.
69. **Real-time constraints?** Budget each stage; reduce resolution/ROI; run detection every $N$ frames with tracking between; batch on GPU; avoid copies/format conversions; profile before optimising.
70. **Build vs buy (library/vendor/model)?** Consider accuracy on *your* data, latency/cost, licence, support, ability to debug/customise, data privacy; prototype two options on the same test set.

### F. Behavioural / portfolio
71. **Describe a project where you learned from failure.** (Structure: situation → what failed → how you diagnosed → measurable fix → what you changed in your process. Example: calibration — "RMS reprojection looked great but my focal length was off; I added pose diversity and an independent ruler check.")
72. **How do you decide when a result is good enough?** Tie to the requirement (error budget in physical/business units), show confidence intervals, and describe remaining risks.
73. **How do you keep up with the field?** A fixed weekly routine (a paper/code reproduction, a dataset or tool trial), notes, small reproducible experiments — like the labs in this handbook.

---

## 6. Whiteboard / live-coding list (practise until fluent)
| Task | Where it is checked |
|---|---|
| 2-D convolution/correlation, separable Gaussian | `code/10` §1–2 |
| Integral image box sum | `code/10` §3 |
| Otsu threshold | `code/10` §4 |
| Harris corner response | `code/10` §5 |
| IoU and non-maximum suppression | `code/10` §6 |
| Average precision | `code/10` §7 |
| RANSAC line fit | `code/10` §8 |
| Homography by DLT (SVD) | `code/10` §9 |
| k-means (Lloyd) | `code/10` §10 |
| PCA via SVD; Fisher/LDA direction | `code/10` §11 |
| Triangulation from two views | `code/10` §12 |
**Also practise:** histogram equalisation, connected components (two-pass), NCC template matching, non-max suppression for Canny, Kalman filter update, softmax/cross-entropy, a simple CNN forward pass **[Modern]**.

---

## 7. Numbers worth remembering (from this handbook's labs and books)
- Separable Gaussian: 169 → 26 multiplications per pixel (13×13).
- RANSAC iterations at $w=0.5$, $p=0.99$: line 16, homography 72, fundamental 1,177.
- Depth error ∝ distance²: marker depth error 0.8 mm @0.5 m → 56 mm @3 m.
- Calibration: near-parallel views → 72 px focal error; 30° tilts → 7 px; 20 views → 3.8 px; reprojection RMS stayed at the noise level.
- Hough circles: 100 % (0–25 % occluded) → 95 % (50 %) → 80 % (65 %) → 40 % (80 %).
- Digits: HOG misconfigured 90.9 % vs well-configured 99.3 %; raw RBF-SVM 98.1 %.
- 400 noise features: k-NN 97.7 → 61.5 %; SVM 98.2 → 91.8 %.
- Rare-defect threshold: recall 31 % (accuracy-optimal) vs 84 % (cost-optimal).
- Reject 30 % → accuracy 95.9 → 99.4 %.
- Occlusion: Kalman 8.0 px vs hold-last 28.6 px at a 25-frame gap.
(Remember these as *patterns* from small experiments, not universal constants.)

---

## 8. Learning plans

### 3-month "foundation + first portfolio piece"
- **Weeks 1–3:** image basics, filters, thresholds, morphology → projects 1 and 3 (file 04).
- **Weeks 4–6:** edges, corners, features, RANSAC, homography → project 5.
- **Weeks 7–9:** pattern-recognition basics, evaluation → project 6.
- **Weeks 10–12:** polish: README with metrics, failure gallery, write-up; practise questions A–B.

### 6-month "employable"
Add: calibration & pose (projects 2 and 4), a fine-tuned deep model with proper evaluation **[Modern]**, a small deployment (ONNX export + latency benchmark), C++ basics; practise sections C–E and whiteboard list.

### 12-month "strong candidate"
Add: tracking (project 8) or 3-D (project 9) or edge deployment (project 11); one domain deep-dive (file 05); a public write-up or talk; contribute to an open-source vision library (docs/tests/small fixes); mock interviews.

**Daily habit (30–60 min):** read one section of a book → write three bullet points in your own words → run or modify one experiment → add one failure case to your notebook.

---

## 9. Résumé and portfolio guidance
- Lead with **outcomes and numbers**: "Reduced false rejects from 4.1 % to 1.3 % at 99 % recall on 12,000 images by adding dark-field lighting and a CNN second stage" (only claim what you actually measured).
- Show **evaluation discipline**: splits, confidence intervals, error analysis.
- Show **system thinking**: latency, deployment, monitoring.
- One strong repo beats ten shallow ones: README (problem, metric, results table, failure gallery, how to run), tests, and a demo.
- Be ready to explain **every design choice and every number** on your résumé.

---

## 10. Study checklist before an interview
- [ ] I can state the Bayes decision rule with costs and explain how thresholds move with priors/costs.
- [ ] I can explain bias–variance, regularisation, cross-validation, leakage.
- [ ] I can explain Canny, Harris, SIFT/ORB, RANSAC, Hough and when each fails.
- [ ] I can derive/explain pinhole projection, homography, $F$/$E$, triangulation, PnP, and the sources of error.
- [ ] I can explain a CNN, transfer learning, detection metrics, domain shift. **[Modern]**
- [ ] I can design an inspection or counting system end to end with numbers.
- [ ] I have 3 projects I can discuss in depth, each with a metric, a failure and a lesson.
- [ ] I can write IoU/NMS, convolution, Otsu, RANSAC, k-means on a whiteboard.
