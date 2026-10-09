# 05 — Domains and Industry Knowledge

> How computer vision and pattern recognition are actually used, what each domain cares about, and the vocabulary that lets you hold a credible conversation.
> Grounding: Davies ch. 20–23 (inspection, grain sorting, surveillance, in-vehicle vision), ch. 25–27 (acquisition, hardware, trade-offs); Baggio (AR, plates, faces, SfM); DHS/Beyerer (decision theory, evaluation, rejection). Everything else is **[Modern]** general professional knowledge — **verify specifics (standards, regulations, products) before relying on them; they change.**

---

## 0. The one-page comparison

| Domain | What the customer pays for | Dominant constraint | Typical metric | Dominant method today **[Modern]** |
|---|---|---|---|---|
| Industrial inspection | Fewer escapes, less scrap, traceability | Speed, lighting, false-reject cost | Escape rate, false-reject rate, gauge R&R | Engineered lighting + classical measurement; CNNs for cosmetic defects |
| Robotics / automation | A robot that finds and grasps | Latency, accuracy of pose, safety | Pick success rate, cycle time, pose error (mm) | Geometry + depth sensors + learned detection/grasp |
| Automotive / ADAS / AV | Safety functions, autonomy | Real-time, safety integrity, weather | Recall at tiny false-alarm rate, mAP, time-to-collision error | Deep multi-sensor perception + classical tracking/geometry |
| Medical imaging | Faster/more accurate reads | Regulation, data, explainability | Sensitivity/specificity, AUROC, Dice | CNN/transformer segmentation/classification under regulatory control |
| Document / OCR | Automated data capture | Layout variety, language, accuracy on fields | Character/field accuracy, straight-through rate | Learned OCR + layout models |
| Retail / logistics | Inventory, throughput, loss prevention | Scale, cost per camera | Count accuracy, read rate (barcodes), dwell-time accuracy | Detection + tracking + classic barcode decoders |
| Surveillance / security / biometrics | Alerts, access control | False alarms, privacy law, bias | False accepts/rejects, false alarms per camera-day | Deep detection/re-ID/face embeddings |
| AR/VR/XR, mobile photography | Experiences, picture quality | Latency, power, thermal | Pose drift, FPS, latency (ms) | SLAM/VIO + learned depth/segmentation |
| Agriculture & food | Yield, grading, sorting | Natural variation, outdoor light | Grading accuracy, throughput (kg/h) | Colour/NIR imaging + classical features and CNNs |
| Remote sensing / geospatial | Maps, change detection | Scale (huge images), labels | IoU, per-class accuracy, change-detection F1 | Segmentation CNNs, multispectral indices |
| Infrastructure inspection (drones) | Safety, cost savings | Access, image quality, labels | Detection recall for faults | Detection/segmentation + photogrammetry |
| Sports / media | Analytics, broadcast graphics | Real time, multi-camera calibration | Tracking accuracy, calibration error | Pose/tracking nets + camera calibration |

---

## 1. Industrial machine vision and inspection  *(Davies ch. 20–21, 25–27)*

**Tasks:** presence/absence, dimensional gauging, surface defects, print/label verification (OCR/OCV), code reading (1-D/2-D), colour and sorting, assembly verification, robot guidance.
**Why it is its own world:** *you control the scene.* Lighting, fixture, trigger and background are designed so the vision problem becomes easy and **repeatable 24/7**.

**How a project really goes**
1. **Specification:** defect catalogue with photos, tolerances in mm, throughput (parts/s), allowed false rejects and escapes, environment (vibration, dust, temperature), integration (PLC, fieldbus, reject mechanism).
2. **Feasibility with the real part and real light** before any software: can a human reliably see the defect in the image? If not, change the lighting/optics, not the algorithm.
3. **Optics & lighting:** resolution (≥3–5 px across the smallest defect), field of view, working distance, lens (telecentric for gauging), lighting scheme (back/dark-field/diffuse/structured — file 03 §4.2), strobe for motion.
4. **Algorithm:** threshold → blob analysis → measurement vs tolerance; golden-template/reference comparison; classifier for ambiguous cosmetic defects; learned anomaly detection when defects are rare/unpredictable.
5. **Validation:** *Gauge R&R* (repeatability & reproducibility), capability studies (Cp/Cpk), tests with deliberately defective samples, and a **long-run false-reject trend**.
6. **Operations:** image archiving, recipe/changeover management, operator UI, periodic re-calibration, version control of the vision program.

**Vocabulary:** FOV, WD (working distance), depth of field, telecentric, strobe, trigger, encoder, line-scan vs area-scan camera, GigE Vision/USB3 Vision/CoaXPress, PLC, OPC UA, golden sample, escape/false reject, Cp/Cpk, gauge R&R, OEE, takt time, fixture, collimated/diffuse/dark-field/bright-field, *blob analysis*.

**Line-scan vs area-scan:** line-scan builds the image row by row as the object moves — ideal for webs, long continuous products and very high resolution, needs an **encoder** for synchronisation; area-scan captures a 2-D frame — simpler for discrete parts.

**Typical failure causes (check in this order):** lighting change/aging LEDs, focus drift, vibration, product variation, dirt on lens, trigger timing, mechanical changes, *then* the algorithm.

**Economics to keep in mind:** the cost of a false reject (scrap + re-inspection) vs an escape (customer return, recall). The threshold is a **business decision** (file 02 §1).

**Davies ch. 21** (cereal grains) is a compact worked example: grain contaminant detection through shape/size/colour analysis, throughput requirements and design trade-offs.

---

## 2. Robotics and automation

**Tasks:** bin picking, pick-and-place, palletising/depalletising, visual servoing, mobile-robot navigation, human–robot collaboration.
**Core vision stack:**
- **Calibration chain:** camera intrinsics → **hand-eye calibration** (camera pose relative to the robot) → tool centre point. Errors multiply: a 0.5° orientation error at 1 m is ~9 mm at the target.
- **Pose estimation:** model-based (CAD + point cloud with **ICP**, PnP from known features), or learned 6-D pose **[Modern]**.
- **Depth sensing:** stereo, structured light, time-of-flight, LiDAR. Know their failure modes: shiny/transparent/black objects, sunlight, multipath.
- **Grasp planning:** from point cloud/depth + gripper model; learned grasp scoring **[Modern]**.
- **SLAM / localisation:** visual-inertial odometry, map building, loop closure (feature matching + RANSAC + bundle/pose-graph optimisation — the same ingredients as Baggio ch. 4).
**Metrics:** pick success rate, cycle time, pose error (mm/deg), collisions, uptime.
**Pitfalls:** calibration drift after a collision, reflective parts, latency between image and motion (timestamps!), safety certification.

---

## 3. Automotive: ADAS and autonomous driving  *(Davies ch. 23; DHS decision theory)*

**Functions:** lane keeping, traffic-sign recognition, forward collision warning/automatic emergency braking, pedestrian/cyclist detection, free-space/drivable-area, parking, driver monitoring.
**Why it is hard:** worst-case, not average-case, matters; all weather and lighting; extreme **recall at a very low false-alarm rate** (phantom braking is dangerous and annoying); real-time on automotive hardware; long-tail rare events.
**Typical stack [Modern]:** multi-camera (+ radar/LiDAR) → deep detection/segmentation/depth → multi-object tracking (Kalman-type filters with association) → prediction → planning. The **geometry is classical**: calibration, ground-plane homography, bird's-eye-view transforms, time-to-collision from scale change.
**Validation:** scenario-based testing, simulation, closed-track and public-road data, *disengagement*/intervention statistics, **SOTIF** (safety of the intended functionality) and functional-safety thinking (ISO 26262; ISO 21448 for SOTIF) — **verify current standards and obligations before relying on them**.
**Metrics:** per-class recall at fixed false positives per hour/km, mAP, tracking IDF1, distance-estimation error vs range, latency.
**Interview-relevant insight:** a detector's average mAP says little; look at *performance by distance, occlusion level, lighting* and the **tail**.

---

## 4. Medical imaging

**Modalities:** X-ray, CT, MRI, ultrasound, endoscopy, fundus/OCT, pathology slides (gigapixel), dermoscopy.
**Tasks:** detection (nodules), classification (diagnosis support), **segmentation** (organs, tumours), registration (aligning scans), quantification (volumes, growth), triage/prioritisation.
**Specific realities**
- **Labels are expensive and variable** (inter-reader disagreement is itself a ceiling — the "Bayes error" idea made concrete). Use multiple readers/adjudication.
- **Class imbalance:** most scans are normal; use sensitivity/specificity, PR curves, and clinically chosen operating points.
- **Dataset shift:** scanners, protocols, hospitals and populations differ; external validation on other sites is essential.
- **Split by patient**, never by image/slice (leakage is the classic error).
- **Explainability and calibration** matter to clinicians; always show uncertainty and let the human decide (the **reject option** in practice).
- **Regulation:** software that influences diagnosis is regulated as a medical device in most jurisdictions (approval, clinical evidence, post-market surveillance, quality management, data protection such as HIPAA/GDPR) — **check the current rules for your market**.
**Metrics:** sensitivity/specificity, AUROC, Dice/IoU for segmentation, Hausdorff distance for boundary error, calibration error.

---

## 5. Document AI and OCR  *(Baggio ch. 5 for the classical flavour)*

**Tasks:** text detection and recognition, layout analysis (columns, tables, forms), key-value extraction (invoices, IDs), handwriting, barcodes/QR codes, signatures, ID/passport reading (MRZ).
**Pipeline:** capture → **pre-processing** (deskew/dewarp via homography, illumination normalisation, binarisation) → text detection → recognition → layout/semantic parsing → validation (checksums, dictionaries, business rules).
**What differs from natural-image CV:** structure matters (reading order, tables); languages and fonts; extreme accuracy needed on specific fields (amounts, IDs) — so **validation rules and human review queues** are core to the product.
**Metrics:** character/word error rate, field-level accuracy, **straight-through processing rate** (share that needs no human), processing time.
**Pitfalls:** mobile-captured skew/blur/shadows, stamps/watermarks, low-resolution scans, domain-specific vocabulary, privacy of sensitive documents.

---

## 6. Retail, logistics, warehouses

**Tasks:** barcode/QR reading (including damaged or moving codes), parcel dimensioning (depth sensors), sorting-line recognition, shelf monitoring, planogram compliance, checkout-free stores, customer counting/dwell time, damage detection, pallet/forklift safety.
**Specifics:** huge numbers of cameras ⇒ **cost per camera** and edge processing; new products appear constantly ⇒ need recognition without retraining everything (embeddings + nearest neighbour **[Modern]**); occlusion in crowded shelves; privacy.
**Metrics:** read rate, count error %, inventory accuracy, false alerts per store-day.

---

## 7. Surveillance, security and biometrics  *(Davies ch. 22; Baggio ch. 8)*

**Tasks:** intrusion/loitering detection, people/vehicle counting, tracking and re-identification across cameras, licence-plate recognition, access control (face/fingerprint/iris), anomaly detection.
**Davies' picture of surveillance:** background modelling, shadow handling, tracking with Kalman/particle filters, crowd analysis — a **tracking and false-alarm management problem** more than a recognition problem.
**Biometrics concepts**
| Term | Meaning |
|---|---|
| Verification (1:1) | Is this person who they claim to be? |
| Identification (1:N) | Who is this among N enrolled people? (harder; errors grow with N) |
| Open-set | The person may not be in the database → needs a reject threshold |
| FAR / FRR | False accept rate / false reject rate; trade-off set by threshold (the ROC/DET curve) |
| EER | Operating point where FAR = FRR |
| Liveness / anti-spoofing | Detect photos, masks, replays |
**Responsible practice:** demographic performance differences (measure accuracy per group), consent and data retention, legal restrictions on biometric/surveillance use that differ by jurisdiction, and the real cost of false matches. **Build the evaluation to expose bias, not hide it.**

---

## 8. AR / VR / XR and mobile photography  *(Baggio ch. 2–4, 6–7)*

- **Tracking:** marker-based (PnP), markerless (feature matching + homography), full **SLAM/VIO** (visual-inertial odometry) for room-scale.
- **Core quantities:** pose accuracy and **drift**, end-to-end **latency** (motion-to-photon ≈ must stay low to avoid discomfort), power/thermal limits on devices.
- **Face/hand/body:** landmark tracking (Baggio ch. 6–7 classical; learned today), head pose for avatars.
- **Depth and segmentation** for occlusion handling (virtual object behind a real one) and portrait effects.
- **Mobile photography:** burst alignment (homography/optical flow) + fusion, HDR, denoise, super-resolution — largely image processing/computational photography.
**Pitfalls:** low texture/lighting, fast motion blur, scale drift (monocular), rolling shutter, device calibration variation.

---

## 9. Agriculture and food

**Tasks:** fruit/produce grading (size, colour, blemish), foreign-material detection, sorting, plant phenotyping, weed detection, yield estimation, livestock monitoring.
**Specifics:** natural variability (no two apples alike), **multispectral/NIR** imaging reveals what RGB cannot (moisture, ripeness, contamination), outdoor illumination variation (shade/sun), throughput of tonnes per hour (Davies ch. 21 sets the flavour), hygiene-driven mechanical design.
**Metrics:** grading accuracy against expert panels, giveaway/escape rates, kg/h, false-reject cost.

---

## 10. Remote sensing and geospatial

**Imagery:** satellite (optical, multispectral, SAR), aerial, drone. **Tasks:** land-cover classification, building/road extraction, change detection, crop monitoring, disaster assessment, object detection (vehicles, ships).
**Concepts:** ground sampling distance (GSD, metres/pixel), spectral bands and **indices** (e.g. NDVI from red and near-infrared), orthorectification (geometric correction using camera model + terrain), georeferencing, tiled processing of gigapixel images, label scarcity, seasonal/atmospheric variation.
**Pitfalls:** domain shift across sensors/seasons/regions, class definitions that vary, spatial autocorrelation (neighbouring pixels are not independent — **split train/test by region, not by random pixel**).

---

## 11. Infrastructure and energy inspection (drones/robots)

**Tasks:** crack/corrosion detection, power-line and insulator inspection, wind-turbine blade inspection, solar-panel defects (thermal imaging), pipelines, construction progress.
**Stack:** drone imaging → photogrammetry/SfM for 3-D context (Baggio ch. 4 ideas) → detection/segmentation → geo-referenced defect map → report. **Pitfalls:** image blur from vibration/wind, glare, scale and distance variation, rare defects (extreme imbalance), safety and regulation of flights.

---

## 12. Sports and media

**Tasks:** player/ball tracking, pose analysis, camera calibration from field lines (homography), automatic camera control, highlight detection, graphics overlay.
**Core:** homography from known field geometry (RANSAC on line intersections), multi-view calibration, multi-object tracking with identity switches being the main error mode, real-time constraints.

---

## 13. Cross-domain themes you should be able to discuss

### 13.1 Data
- **Collect the long tail on purpose** (rare defects, bad weather). Most failures live there.
- **Label quality:** define the labelling protocol, measure inter-annotator agreement, audit samples; noisy labels cap achievable accuracy.
- **Splits:** by group (patient, site, camera, part, session) and by time; keep a locked final test set.
- **Active learning / human-in-the-loop:** use model uncertainty to choose what to label next; route low-confidence cases to humans (the reject option) and feed corrections back.
- **Synthetic data and augmentation:** useful for rare cases and for teaching invariance; validate on real data.

### 13.2 Metrics and operating points
- Define **business-aligned metrics** (cost per error type), not just accuracy.
- For imbalanced problems: PR curves, recall at fixed precision (or at fixed false alarms per hour).
- Report **confidence intervals**; examine **per-condition** performance (lighting, distance, camera).
- **Calibration:** do predicted probabilities match frequencies? Needed for thresholding and for human trust.

### 13.3 Deployment and operations (MLOps for vision) **[Modern]**
- **Edge vs cloud:** latency, bandwidth, privacy, cost; edge devices (embedded GPUs/NPUs/DSPs), model compression (quantisation, pruning, distillation), formats (ONNX and vendor runtimes).
- **Monitoring:** input statistics (brightness, blur/focus score), prediction distribution, confidence, human-override rate, accuracy audits on sampled data; **alert on drift**.
- **Versioning:** data, code, models, configurations; reproducible training; rollback.
- **Feedback loop:** capture failures → label → retrain → regression-test → release.
- **Security/robustness:** adversarial examples and spoofing (esp. biometrics), sensor tampering.

### 13.4 Ethics, privacy, regulation **[Modern — verify locally]**
- Privacy-by-design (collect/store the minimum, blur or discard faces/plates where possible, retention limits).
- Fairness testing across demographic and environmental groups.
- Transparency about limits and *intended use*.
- Domain regulations: medical devices, automotive safety, biometrics/surveillance law, data-protection law (e.g. GDPR-style). The rules differ by country and change — involve legal/regulatory expertise early.

---

## 14. Quick "talk like an insider" cheat sheet

- **"What's the cost of a miss vs a false alarm?"** — the first question in any domain.
- **"Can we change the lighting?"** — second question for any controlled setting.
- **"How was it split?"** — third; probing for leakage.
- **"What does it do on the tail?"** — fourth; average metrics hide danger.
- **"What's the latency budget end-to-end?"** — fifth; real systems are timing systems.
- **"How do you know when it drifts?"** — sixth; monitoring.
- **"What happens when it's unsure?"** — seventh; reject/human-in-the-loop.
