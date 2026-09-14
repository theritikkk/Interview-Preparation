# Chapter 9 — Unsupervised Learning Techniques

## 🎯 Why this chapter matters
Most real-world data has **no labels** — nobody manually tags millions of photos or log entries. This chapter closes out Part I by teaching you to find structure in unlabeled data: grouping similar things (**clustering**), spotting the weird ones (**anomaly detection**), and modeling how data is distributed (**density estimation**).

---

## 🧠 The Big Three Unsupervised Tasks
- **Clustering**: group similar instances together (e.g., segment blog visitors by browsing behavior).
- **Anomaly detection**: learn what "normal" looks like, then flag instances that deviate (e.g., fraud detection, defect detection).
- **Density estimation**: estimate the probability density function that generated the dataset — useful for anomaly detection, visualization, and data analysis.

**Real applications of clustering specifically**: customer segmentation, data analysis (examine each cluster separately), dimensionality reduction (represent each instance by its distance/affinity to clusters), anomaly detection, semi-supervised learning (propagate labels within a cluster), search engines/image organization (find similar items), and image segmentation (group pixels by color for object detection/tracing).

---

## 🎯 K-Means

**Goal:** partition data into $k$ clusters, each represented by a **centroid** (the cluster's mean position), such that each instance is assigned to its nearest centroid.

### The algorithm (beautifully simple)
1. Place $k$ centroids randomly (e.g., pick $k$ random instances as starting points).
2. **Assign** every instance to its nearest centroid.
3. **Update** each centroid to the mean position of the instances now assigned to it.
4. Repeat steps 2–3 until centroids stop moving.

```python
from sklearn.cluster import KMeans

kmeans = KMeans(n_clusters=5)
y_pred = kmeans.fit_predict(X)     # cluster label for every instance
kmeans.cluster_centers_             # the 5 centroid locations
kmeans.predict(X_new)               # assign new instances to the nearest existing centroid
```

- Guaranteed to **converge** (mean squared distance to centroids only decreases each step) — but **not guaranteed to find the globally best clustering**; it can get stuck in a bad local optimum depending on the random starting centroids.
- Computational complexity is generally roughly linear in instances, clusters, and dimensions in practice — K-Means is one of the **fastest** clustering algorithms.

### Fixing bad initialization: K-Means++ and `n_init`
- **`n_init`** (default 10): run the whole algorithm this many times with different random starts, and keep the best result — measured by **inertia** (the mean squared distance from each instance to its assigned centroid; lower is better).
- **K-Means++**: a smarter default initialization strategy — pick the first centroid randomly, then pick each subsequent centroid with probability proportional to its squared distance from the nearest already-chosen centroid (so new centroids tend to be spread far apart). This dramatically reduces the chance of landing in a bad local optimum, and is Scikit-Learn's default.

```python
kmeans.inertia_     # lower = tighter clusters
kmeans.score(X)      # negative inertia (Scikit-Learn convention: "greater is better")
```

### Finding the right $k$
K-Means requires you to **choose $k$ in advance** — but you rarely know the "true" number of clusters ahead of time.

⚠️ **Don't just pick the $k$ with lowest inertia** — inertia keeps shrinking as $k$ increases (more clusters = points closer to *some* centroid), so it would always favor $k$ = number of instances, which is useless.

**Method 1 — The "elbow" method:** plot inertia vs. $k$. Inertia drops fast at first, then flattens out — look for the "elbow" (bend) in the curve; that's a reasonable $k$. Coarse, but quick.

**Method 2 — Silhouette score (more precise, more expensive):** for each instance, the **silhouette coefficient** is:

$$
\frac{b - a}{\max(a, b)}
$$

where $a$ = mean distance to other instances in the *same* cluster, $b$ = mean distance to instances of the *nearest other* cluster. Ranges from **−1** (probably wrong cluster) to **+1** (well inside its own cluster, far from others).

```python
from sklearn.metrics import silhouette_score
silhouette_score(X, kmeans.labels_)   # average over all instances
```
Plotting the silhouette score across candidate $k$ values (and inspecting a full **silhouette diagram**, which shows every instance's coefficient sorted by cluster) gives a richer picture than inertia alone — e.g., it can reveal that a slightly-lower-scoring $k$ actually produces more evenly sized, more meaningful clusters.

### Limits of K-Means
- You must specify $k$ in advance.
- Assumes clusters are **roughly spherical, similar size, similar density** — struggles badly with elongated clusters or clusters of very different sizes/densities.
- Sensitive to feature scale — scale your features first.
- Only finds a local optimum → run with multiple initializations (`n_init`).

### Practical uses beyond "just clustering"
- **Image segmentation**: cluster pixels by color to simplify/segment an image (e.g., replace each pixel with its cluster's mean color).
- **Preprocessing for supervised learning**: replace each instance's raw features with its **distance to every centroid** — this can act as a useful dimensionality reduction / feature engineering step before a classifier.
- **Semi-supervised learning**: cluster your unlabeled data, label just the instance closest to each centroid (the **representative instance**), then **propagate** that label to every other instance in the same cluster — lets you get useful labels for an entire dataset from labeling just $k$ instances.

---

## 🌌 DBSCAN — clustering by density, no $k$ required

**Core idea:** clusters are **continuous regions of high density**, separated by regions of low density. No need to specify the number of clusters upfront!

Algorithm:
1. For every instance, count how many other instances fall within distance **ε (epsilon)** — this is its **ε-neighborhood**.
2. If an instance has at least `min_samples` instances in its ε-neighborhood (including itself), it's a **core instance** (it's in a dense region).
3. All instances within a core instance's neighborhood join the same cluster — chains of neighboring core instances merge into one cluster.
4. Any instance that's neither a core instance nor in a core instance's neighborhood is flagged as an **anomaly** (noise).

```python
from sklearn.cluster import DBSCAN
dbscan = DBSCAN(eps=0.2, min_samples=5)
dbscan.fit(X)
dbscan.labels_                    # -1 means "anomaly"
dbscan.core_sample_indices_       # which instances are core instances
```

- **Strengths**: finds clusters of **any shape**, doesn't need $k$, robust to outliers, just 2 hyperparameters.
- **Weakness**: struggles when clusters have very different densities (a single `eps` can't fit all of them well). Roughly $O(m\log m)$ time.
- ⚠️ **DBSCAN has no `.predict()` method** for new instances — only `.fit_predict()`. To classify *new* points into existing clusters, train a separate classifier (e.g., `KNeighborsClassifier`) on the core instances and their labels.

### Other clustering algorithms worth knowing exist
- **Agglomerative clustering**: repeatedly merges the closest pair of clusters, building a hierarchy (tree) — lets you pick any cut of the resulting hierarchy as your final clustering, and scales to large numbers of instances if you provide a connectivity constraint.
- **BIRCH**: designed specifically for very large datasets, fast if the number of features isn't too large.
- **Mean-Shift**: shifts a circle for each instance toward the region's density peak, similar in spirit to DBSCAN but doesn't scale well to large datasets.
- **Affinity Propagation**: instances "vote" for which instance should represent them; doesn't require $k$ upfront but doesn't scale well.
- **Spectral clustering**: uses the eigenvalues of a similarity matrix, then applies a simpler clustering algorithm in the reduced space; can capture complex cluster structures but doesn't scale to large datasets.

---

## 🔔 Gaussian Mixture Models (GMMs)

**Idea:** assume the data was generated by a mix of several **Gaussian (bell-curve) distributions**, each with its own mean, covariance (shape/orientation), and weight — we just don't know which points came from which Gaussian. GMM fits these parameters using the **Expectation-Maximization (EM)** algorithm, which alternates between:
- **E-step**: estimate, for every instance, the *probability* it belongs to each Gaussian (like K-Means's "assign to nearest centroid," but **soft** — probabilistic — instead of hard assignment).
- **M-step**: update each Gaussian's mean/covariance/weight using those probability-weighted instances (like K-Means's "recompute centroid," generalized).

```python
from sklearn.mixture import GaussianMixture

gm = GaussianMixture(n_components=3, n_init=10)
gm.fit(X)
gm.weights_        # estimated weight (proportion) of each Gaussian
gm.means_          # estimated mean of each Gaussian
gm.covariances_    # estimated covariance of each Gaussian

gm.predict(X)              # hard cluster assignment
gm.predict_proba(X)        # soft assignment: probability of belonging to each cluster
gm.sample(6)                # generate new instances from the learned distribution!
```
Because GMM models the actual data-generating distribution, it can also be used as a **generative model** — you can literally sample brand-new, plausible-looking instances from it.

`covariance_type` controls how flexible each cluster's shape can be: `"full"` (any ellipsoid, default, most parameters), `"tied"` (all clusters share the same shape), `"spherical"` (perfectly round clusters), `"diag"` (axis-aligned ellipsoids) — fewer parameters = faster/less overfitting-prone, but less flexible fits.

### Anomaly detection with GMMs
Any instance sitting in a **low-density region** is likely an anomaly:

```python
densities = gm.score_samples(X)
density_threshold = np.percentile(densities, 4)   # flag the lowest-density 4% as anomalies
anomalies = X[densities < density_threshold]
```
This is the same **precision/recall trade-off** from Chapter 3: raise the threshold to reduce false positives (good instances wrongly flagged), lower it to reduce false negatives (bad instances missed).

> **Anomaly detection vs. novelty detection**: anomaly detection assumes the training data *may* already contain some outliers; novelty detection assumes the training data is clean, and the goal is to flag *new* instances that look different from anything seen during training.

### Selecting the number of clusters for a GMM
Inertia and silhouette score don't work well here (they assume roughly spherical, similar-sized clusters). Instead, use an information criterion that balances fit quality against model complexity:

$$
\text{BIC} = \log(m)\,p - 2\log(\hat{L}) \qquad \text{AIC} = 2p - 2\log(\hat{L})
$$

where $m$ = number of instances, $p$ = number of parameters the model learned, $\hat{L}$ = the model's maximized **likelihood** (how well it fits the data). Both **penalize complexity** (more clusters = more parameters) while **rewarding good fit** — pick the $k$ that minimizes BIC/AIC.

```python
gm.bic(X)
gm.aic(X)
```

> 💡 **Probability vs. Likelihood** (a subtlety worth internalizing): *probability* describes how plausible a future outcome is, given fixed parameters; *likelihood* describes how plausible a set of parameters is, given an outcome you've already observed. Training a model = finding the parameters that **maximize the likelihood** of the data you actually saw (Maximum Likelihood Estimation, MLE).

### Bayesian Gaussian Mixture — let the algorithm pick $k$ for you
Instead of manually searching over $k$, `BayesianGaussianMixture` can be given an intentionally generous upper bound on the number of clusters, and it will automatically assign near-zero weight to the clusters it decides are unnecessary:

```python
from sklearn.mixture import BayesianGaussianMixture
bgm = BayesianGaussianMixture(n_components=10, n_init=10)
bgm.fit(X)
np.round(bgm.weights_, 2)   # e.g. [0.4, 0.21, 0.4, 0, 0, 0, 0, 0, 0, 0] → found 3 real clusters
```

### Other anomaly/novelty detection algorithms (know they exist)
- **Fast-MCD**: assumes normal instances are generated from one Gaussian, robustly estimating that Gaussian even in the presence of outliers — great for outlier detection / cleaning a dataset.
- **Isolation Forest**: builds random decision trees; anomalies tend to get isolated (separated into their own leaf) in *fewer* splits than normal instances — efficient for high-dimensional data.
- **Local Outlier Factor (LOF)**: compares an instance's local density to its neighbors' local density.
- **One-class SVM**: better suited for novelty detection; learns a boundary around the "normal" region, especially good in high-dimensional space.

---

## ⚠️ Common Pitfalls

- Using inertia to select $k$ for K-Means — it monotonically decreases, so it will always favor more clusters.
- Running K-Means on **unscaled** features, or on clusters of very different sizes/densities/shapes, and being confused by poor results.
- Expecting `DBSCAN.predict()` to exist — it doesn't; use `.fit_predict()` or train a separate classifier for new points.
- Using K-Means's inertia/silhouette to select the number of components for a GMM — use BIC/AIC instead.
- Setting an anomaly detection threshold once and never revisiting it — like precision/recall, it should be tuned to your actual false-positive/false-negative costs.
- Confusing **probability** and **likelihood** — remember: probability varies the *outcome* with parameters fixed; likelihood varies the *parameters* with the outcome fixed.

---

## 🔑 Cheat Sheet

- **K-Means**: hard-assigns instances to $k$ centroids; fast, needs $k$ specified, assumes spherical/similar clusters; use K-Means++ init (default) and `n_init` to avoid bad local optima.
- Pick $k$ via the **elbow method** (quick) or **silhouette score/diagram** (more precise) — never via raw inertia alone.
- **DBSCAN**: density-based, no $k$ needed, finds arbitrary shapes, flags noise as −1; no native `.predict()`.
- **GMM**: soft, probabilistic clustering assuming Gaussian-shaped clusters; also a generative model (can sample new data); use BIC/AIC (not inertia/silhouette) to choose the number of components.
- **Anomaly detection** = flag low-density instances (GMM `score_samples`, Isolation Forest, LOF, One-class SVM, Fast-MCD); tune the threshold like a precision/recall trade-off.
- **Novelty detection** assumes clean training data; anomaly detection does not.

---

## ✍️ Practice

1. **Hands-on:** Generate `sklearn.datasets.make_blobs()` with 4 true clusters. Fit `KMeans` with $k = 2, 3, 4, 5, 8$. Plot inertia vs. $k$ and identify the elbow. Then plot silhouette score vs. $k$ — do they agree?
2. **Conceptual:** Explain why K-Means struggles on two clusters shaped like long, thin, crossing ellipses, while DBSCAN handles them fine.
3. **Debug-the-scenario:** You run `GaussianMixture(n_components=5)` and get a great BIC score, but when you inspect `gm.weights_`, two components have weight ≈ 0.001. What does this suggest, and what would you try next?
   <details><summary>Answer</summary>The true number of clusters is probably fewer than 5 (maybe 3); those near-zero-weight components are essentially unused. Try a `BayesianGaussianMixture` with a generous `n_components` to let the model prune automatically, or just retry `GaussianMixture` with smaller `n_components` values and compare BIC.</details>
4. **Hands-on:** Use K-Means for **image compression**: load a photo, reshape pixels to an (N, 3) RGB array, cluster with `KMeans(n_clusters=8)`, then replace every pixel with its cluster's centroid color. Display the result — how does image quality degrade as you reduce `n_clusters`?

---

🎉 **You've completed Part I — The Fundamentals of Machine Learning!** You now understand the classic ML toolbox: regression, classification, SVMs, trees, ensembles, dimensionality reduction, and clustering — the same tools used in the vast majority of real-world "boring but valuable" ML applications (forecasting, scoring, segmentation, tabular prediction).

Next: `part2_deep_learning/10_ann_with_keras.md` — Part II begins. Time to build your first neural network.