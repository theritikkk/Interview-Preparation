# Chapter 8 — Dimensionality Reduction

## 🎯 Why this chapter matters
Real datasets can have thousands of features — pixels in an image, words in a vocabulary. Too many features slow training, risk overfitting, and make visualization impossible. This chapter teaches you to **compress data intelligently**, keeping the signal and dropping the redundancy — with **PCA** as the star technique you'll use constantly.

---

## 🌌 The Curse of Dimensionality

Intuition trained on 2D/3D space badly misleads you in high dimensions:

- In high-dimensional space, most points end up **very far from each other**, and also **very close to the edges/corners** of the space. A random point in a 1,000,000-dimensional unit hypercube is, on average, breathtakingly far from any other random point — training instances are likely to be **far apart**, making predictions less reliable (more extrapolation than interpolation) and overfitting more likely, since there's so much empty space to fit noise into.
- **The number of training instances you'd need to keep data "dense" grows explosively with dimensions.** To keep the same sample density as a 100-instance training set in 1D, you'd need $100^{10}$ instances for a 10D dataset — more than atoms in the observable universe for even modestly high dimensions.

> In practice, this means: don't just throw every possible feature at a model. Reduce dimensionality first, both for speed and (often) for better generalization — though ⚠️ **it always loses some information**, so try training on the original data first; only reduce dimensionality if training is too slow or you specifically need it (e.g., for visualization).

---

## 🧭 Two Main Approaches

### 1. Projection
Real-world data usually doesn't uniformly fill the full dimensional space — features are often correlated, so all the training instances actually lie close to a much lower-dimensional **subspace**. Example: a 3D "Swiss roll" dataset — projecting it flat onto 2D squashes different layers of the roll on top of each other, which **destroys the structure**.

### 2. Manifold Learning
Many datasets (like the Swiss roll) twist and turn in high-dimensional space but are locally similar to a lower-dimensional **manifold** — imagine a rolled-up sheet of paper: it's a 2D surface, even though it's embedded (bent) into 3D space. Manifold learning algorithms try to "unroll" the manifold rather than flatten it via simple projection. ⚠️ **The "manifold assumption"** — that real high-dimensional data lies near a lower-dimensional manifold — often holds, but not always: an unrolled manifold isn't guaranteed to be *simpler* to model, and sometimes the decision boundary is actually easier to draw in the higher-dimensional space.

---

## 🎯 PCA — Principal Component Analysis (the workhorse)

**Idea:** find the axis (direction) along which the data varies the **most** — this is the direction that preserves the most information when you project onto it. Call this the **first principal component (PC1)**. Then find a second axis, **orthogonal** (perpendicular) to the first, capturing the most of the *remaining* variance — **PC2**. Keep going until you've found as many orthogonal axes as there are dimensions.

**Why maximize variance?** Because the axis that preserves the most variance is also (provably) the axis that **minimizes the mean squared distance** between the original points and their projections — i.e., the projection that loses the *least* information.

### Finding the principal components: SVD
PCA uses a standard matrix factorization called **Singular Value Decomposition (SVD)**, which decomposes the (centered) data matrix $\mathbf{X}$ into $\mathbf{U}\mathbf{\Sigma}\mathbf{V}^\top$, where the columns of $\mathbf{V}$ give you the principal component directions, ranked by importance.

```python
X_centered = X - X.mean(axis=0)     # PCA requires the data to be zero-centered first!
U, s, Vt = np.linalg.svd(X_centered)
c1 = Vt.T[:, 0]    # first principal component (a unit vector)
c2 = Vt.T[:, 1]    # second principal component
```

### Projecting down to d dimensions
Once you have the principal components, reduce dimensionality by projecting onto the hyperplane defined by the **first $d$** of them (the ones capturing the most variance):

$$
\mathbf{X}_{d\text{-proj}} = \mathbf{X}\mathbf{W}_d
$$

### Using Scikit-Learn (the way you'll actually do this in practice)
```python
from sklearn.decomposition import PCA

pca = PCA(n_components=2)
X2D = pca.fit_transform(X)          # centers the data for you automatically
```

### Explained Variance Ratio — how much information did you keep?
```python
pca.explained_variance_ratio_
# e.g. array([0.842, 0.146]) → PC1 keeps 84.2% of the variance, PC2 keeps 14.6%
# → together, 98.8% of the original variance survives in just 2 dimensions!
```

### Choosing the number of dimensions
Instead of picking `n_components` arbitrarily, pick the smallest number of dimensions that keeps a target amount of variance (e.g., 95%):

```python
pca = PCA(n_components=0.95)         # pass a float between 0 and 1 → keeps 95% of variance automatically
X_reduced = pca.fit_transform(X_train)
```
Or plot **cumulative explained variance vs. number of dimensions** and look for the "elbow" — the point of diminishing returns.

### PCA for compression (and decompression)
Since PCA is (mostly) reversible, you can compress data and later reconstruct an approximation:
```python
pca = PCA(n_components=154)          # e.g., MNIST: 784 pixels → 154 dimensions, 95% variance kept
X_reduced = pca.fit_transform(X_train)
X_recovered = pca.inverse_transform(X_reduced)   # approximate reconstruction — not perfect (lossy)
```
The average squared distance between the original and reconstructed data is the **reconstruction error**.

### Randomized & Incremental PCA — for speed and huge datasets
- **Randomized PCA** (`svd_solver="randomized"`): a stochastic algorithm that quickly finds an *approximation* of the first $d$ principal components — much faster than full SVD when $d$ is much smaller than the number of features.
- **Incremental PCA (IPCA)**: for datasets **too large to fit in memory**, feed the algorithm mini-batches at a time via `.partial_fit()` — lets you train PCA on data streamed from disk.

```python
from sklearn.decomposition import IncrementalPCA
inc_pca = IncrementalPCA(n_components=154)
for X_batch in np.array_split(X_train, n_batches):
    inc_pca.partial_fit(X_batch)
X_reduced = inc_pca.transform(X_train)
```

---

## 🌀 Kernel PCA — PCA for nonlinear structure

Just like SVMs (Ch. 5), you can apply the **kernel trick** to PCA — implicitly mapping data to a high-dimensional space and performing PCA there, without ever computing the expanded features directly. This lets Kernel PCA preserve **nonlinear** structure (e.g., correctly "unroll" a twisted manifold) that plain linear PCA can't capture.

```python
from sklearn.decomposition import KernelPCA
rbf_pca = KernelPCA(n_components=2, kernel="rbf", gamma=0.04)
X_reduced = rbf_pca.fit_transform(X)
```
Selecting the kernel and its hyperparameters (like `gamma`) usually needs **grid search** — since dimensionality reduction is often a preprocessing step for a supervised task, you can wrap it in a `Pipeline` and search over both the kPCA hyperparameters and the downstream classifier's hyperparameters together, optimizing for final task performance.

---

## 🧵 LLE — Locally Linear Embedding

A different, purely **manifold-learning** approach (no projection involved): for each training instance, LLE measures how it relates *linearly* to its closest neighbors, then finds a lower-dimensional representation where those same local linear relationships are best preserved. It's especially good at unrolling twisted manifolds like the Swiss roll — though the result can end up somewhat stretched or squeezed in places, and it doesn't scale well to very large datasets.

```python
from sklearn.manifold import LocallyLinearEmbedding
lle = LocallyLinearEmbedding(n_components=2, n_neighbors=10)
X_reduced = lle.fit_transform(X)
```

## 🧭 Other Techniques (know they exist)
- **Random Projections**: project onto a random lower-dimensional subspace — surprisingly, this preserves distances reasonably well (per the Johnson–Lindenstrauss lemma) and is very fast.
- **Multidimensional Scaling (MDS)**: reduce dimensions while trying to preserve distances between instances.
- **Isomap**: build a graph connecting each instance to its nearest neighbors, then reduce dimensions while preserving *geodesic* (graph) distances.
- **t-SNE**: keeps similar instances close and dissimilar ones apart — mostly used for **visualization** of high-dimensional data.
- **Linear Discriminant Analysis (LDA)**: technically a *classification* algorithm, but it learns the most discriminative axes between classes, which is also excellent as a dimensionality-reduction step *before* another classifier (unlike PCA, LDA uses the class labels).

---

## ⚠️ Common Pitfalls

- Forgetting to **center** (or scale) data before manual PCA — Scikit-Learn's `PCA` handles centering for you, but if you implement it yourself or feed unusual data in, don't skip this.
- Assuming dimensionality reduction always speeds up *and improves* downstream models — it usually speeds up training, but can occasionally hurt performance since some information is discarded; always empirically verify with cross-validation.
- Choosing `n_components` arbitrarily instead of basing it on explained variance or downstream task performance.
- Using plain (linear) PCA on data with strong **nonlinear** structure and being confused when it doesn't separate classes well — reach for Kernel PCA or a manifold method instead.
- Forgetting that a PCA "reconstruction" (`inverse_transform`) is an **approximation**, not the original data.

---

## 🔑 Cheat Sheet

- **Curse of dimensionality**: more features → sparser data, more overfitting risk, exponentially more instances needed for the same density.
- **Projection** (flatten onto a subspace) vs. **Manifold learning** (unroll a curved lower-dimensional surface) — two different strategies for the same goal.
- **PCA**: find orthogonal axes of maximum variance (via SVD); project onto the top $d$ to reduce dimensions while keeping the most information.
- `explained_variance_ratio_` tells you how much information each PC keeps; `n_components=0.95` auto-selects dimensions to retain 95% variance.
- **Randomized PCA** = fast approximate PCA. **Incremental PCA** = PCA on data that doesn't fit in memory (`partial_fit`).
- **Kernel PCA** = PCA + kernel trick, for nonlinear structure.
- **LLE** = manifold learning via local linear relationships; great at unrolling twisted manifolds.
- LDA reduces dimensions *using* class labels (supervised); PCA does not (unsupervised).

---

## ✍️ Practice

1. **Hands-on:** Load MNIST. Run `PCA(n_components=0.95)`. How many dimensions does it keep (down from 784)? Train a `RandomForestClassifier` on both the full and reduced data — compare training time and accuracy.
2. **Conceptual:** Explain why PCA's "maximize variance" objective is equivalent to "minimize reconstruction error." (Hint: think Pythagorean theorem — total spread = spread along the axis + spread of the projection error, a fixed total.)
3. **Debug-the-scenario:** You apply plain `PCA` to a Swiss-roll-shaped dataset and the classes end up hopelessly tangled in 2D, even though they were cleanly separable in 3D. What went wrong, and what would you try instead?
   <details><summary>Answer</summary>Plain PCA is a linear projection — it flattens the roll, squashing different "layers" of the manifold onto each other. Try `LocallyLinearEmbedding` or `KernelPCA` to properly unroll the nonlinear manifold instead.</details>
4. **Hands-on:** Compare `PCA`, `KernelPCA(kernel="rbf")`, and `LocallyLinearEmbedding` on `sklearn.datasets.make_swiss_roll()`, visualizing each 2D result with a color gradient for the roll position. Which one "unrolls" it most cleanly?

Next: `09_unsupervised_learning.md` — clustering, anomaly detection, and Gaussian Mixture Models: making sense of data with no labels at all.