# Chapter 5 — Support Vector Machines

## 🎯 Why this chapter matters
SVMs offer a completely different philosophy for drawing a decision boundary than Logistic Regression: instead of just separating the classes, **find the boundary that stays as far away from every class as possible.** This one idea — plus a clever mathematical trick — makes SVMs powerful for small-to-medium, complex datasets, and it's the algorithm that introduces the **kernel trick**, a concept that echoes throughout ML.

---

## 🧠 Core Concepts

### Large margin classification
Picture two classes of points that can be separated by a straight line. There are *infinitely many* lines that separate them perfectly. An SVM doesn't just pick *any* separating line — it finds the one that fits the **widest possible "street"** between the two classes.

> Think of it as parking the fattest possible road between two neighborhoods, then drawing your boundary line right down the middle of that road.

The training instances sitting exactly on the edges of this street are called **support vectors** — they're the only points that actually determine where the boundary goes. Remarkably, adding more training points *outside* the street doesn't move the boundary at all.

⚠️ **SVMs are sensitive to feature scale.** If one feature has a much larger range than another, the "widest street" gets skewed toward that feature. **Always scale your features** (`StandardScaler`) before training an SVM.

### Hard margin vs. soft margin
- **Hard margin**: require *every* instance to be strictly off the street, on the correct side. Problems: (1) only works if data is perfectly linearly separable, (2) extremely sensitive to outliers — one bad point can make a valid margin impossible, or force a terrible boundary.
- **Soft margin**: allow some instances to violate the margin (be inside the street or even on the wrong side), in exchange for a wider, more generalizable street. This is what's used in practice.

The trade-off is controlled by the hyperparameter **C**:
- **Low C** → wider street, more margin violations tolerated → more regularization (good if overfitting).
- **High C** → narrower street, fewer violations tolerated → less regularization (risk of overfitting to outliers).

```python
import numpy as np
from sklearn import datasets
from sklearn.pipeline import Pipeline
from sklearn.preprocessing import StandardScaler
from sklearn.svm import LinearSVC

iris = datasets.load_iris()
X = iris["data"][:, (2, 3)]                          # petal length, petal width
y = (iris["target"] == 2).astype(np.float64)          # Iris virginica or not

svm_clf = Pipeline([
    ("scaler", StandardScaler()),
    ("linear_svc", LinearSVC(C=1, loss="hinge")),
])
svm_clf.fit(X, y)
svm_clf.predict([[5.5, 1.7]])   # array([1.])
```
⚠️ Unlike Logistic Regression, **SVMs don't natively output class probabilities** — just a class prediction (though `SVC` can approximate probabilities via `probability=True`, at extra computational cost).

---

## 🌀 Nonlinear SVM Classification — the Kernel Trick

Real data is rarely linearly separable. Two ideas, both leading to the same powerful trick:

### Idea 1: Add polynomial features
Just like Ch. 4's Polynomial Regression, transform features (`x1, x2` → `x1, x2, x1², x1x2, x2²`, ...) so a straight line in the *new* space corresponds to a curve in the original space. Works, but at high degree, you get a **combinatorial explosion** of features → slow and memory-heavy.

### Idea 2: The kernel trick
For SVMs, there's a mathematical shortcut: you can get the *exact same result* as if you'd expanded the data into a huge (even infinite-dimensional!) feature space, **without ever actually computing those extra features.** This is the **kernel trick**, and it's the single most important idea in this chapter.

```python
from sklearn.svm import SVC

poly_kernel_svm_clf = Pipeline([
    ("scaler", StandardScaler()),
    ("svm_clf", SVC(kernel="poly", degree=3, coef0=1, C=5)),
])
poly_kernel_svm_clf.fit(X, y)
```
- `degree`: the polynomial degree — increase if underfitting, decrease if overfitting.
- `coef0`: controls how much high-degree terms influence the model relative to low-degree terms.

### Similarity features & the Gaussian RBF kernel
Another nonlinear trick: pick **landmarks** (reference points), and for every instance, compute how *similar* it is to each landmark using a similarity function like the **Gaussian Radial Basis Function (RBF)**:

$$
\phi_\gamma(\mathbf{x}, \boldsymbol{\ell}) = \exp\left(-\gamma \lVert \mathbf{x} - \boldsymbol{\ell} \rVert^2\right)
$$

This bell-shaped function outputs 1 when $\mathbf{x}$ is exactly at the landmark, decaying toward 0 as it moves away. Replacing each instance's raw features with its "similarity to every landmark" can turn a non-linearly-separable dataset into a linearly-separable one — but computing this for every instance is expensive... unless, again, you use the kernel trick:

```python
rbf_kernel_svm_clf = Pipeline([
    ("scaler", StandardScaler()),
    ("svm_clf", SVC(kernel="rbf", gamma=5, C=0.001)),
])
rbf_kernel_svm_clf.fit(X, y)
```

- **`gamma` (γ)** controls how narrow/wide the bell curve is → acts like a **regularization** knob:
  - High γ → narrow bells → each instance only influences its close neighborhood → wiggly, overfit-prone boundary. **Reduce γ if overfitting.**
  - Low γ → wide bells → smoother, simpler boundary. **Increase γ if underfitting.**
- `C` still works the same as before.

> 🔑 **Rule of thumb for choosing a kernel:** Always try the **linear kernel first** (`LinearSVC`, especially fast on large/high-feature datasets). If the dataset isn't too large, try the **RBF kernel** next — it works well in most cases. Explore other/specialized kernels (e.g., string kernels for text/DNA) only if you have time and a good reason.

---

## ⚙️ Computational Complexity Cheat Sheet

| Class | Time complexity | Out-of-core (online)? | Needs scaling? | Kernel trick? |
|---|---|---|---|---|
| `LinearSVC` (liblinear) | $O(m \times n)$ | No | Yes | No |
| `SGDClassifier(loss="hinge")` | $O(m \times n)$ | **Yes** | Yes | No |
| `SVC` (libsvm) | $O(m^2 n)$ to $O(m^3 n)$ | No | Yes | **Yes** |

`SVC` gets painfully slow past roughly a hundred thousand instances — it's built for **complex but small-to-medium datasets**. For huge datasets, use `LinearSVC` or an `SGDClassifier` instead (trading away the kernel trick).

---

## 📉 SVM Regression

Flip the objective: instead of fitting the widest street *between* two classes, fit the widest street that contains **as many instances as possible**, with the street's width controlled by hyperparameter `ε` (epsilon) — points *inside* the street don't affect the prediction at all (the model is said to be **ε-insensitive**).

```python
from sklearn.svm import LinearSVR, SVR

svm_reg = LinearSVR(epsilon=1.5)         # linear SVM regression
svm_reg.fit(X, y)

svm_poly_reg = SVR(kernel="poly", degree=2, C=100, epsilon=0.1)   # nonlinear, via kernel trick
svm_poly_reg.fit(X, y)
```

---

## 🔬 Under the Hood (brief intuition, for the curious)

- SVM training is formulated as a **constrained optimization problem**: minimize $\frac{1}{2}\lVert\mathbf{w}\rVert^2$ (which maximizes the margin — a smaller weight vector norm corresponds to a wider street) subject to every instance being correctly classified (hard margin) or correctly classified up to some slack (soft margin, via **slack variables** $\zeta^{(i)}$, one per instance).
- This is a **Quadratic Programming (QP)** problem, solvable directly, or via its **dual problem** — a reformulation that (crucially) makes the kernel trick mathematically possible, since the dual only ever needs **dot products between instances**, which the kernel function can compute implicitly without expanding to the higher-dimensional space.
- **Online SVMs**: for streaming/huge data, train incrementally using Gradient Descent to minimize the **hinge loss** function ($\max(0, 1-t)$) plus a regularization term — this is exactly what `SGDClassifier(loss="hinge")` does.

*(The full QP/dual-problem derivation lives in Appendix C of the book if you want to go deeper — treat it as optional for a first pass.)*

---

## ⚠️ Common Pitfalls

- Forgetting to scale features before training an SVM (skews the "widest street" search).
- Using `SVC` on a very large dataset and wondering why training seems to hang — switch to `LinearSVC` or `SGDClassifier`.
- Confusing which direction to move `C` or `gamma` when overfitting vs. underfitting — remember both act like *inverse* regularization strength (high value = less regularization = more overfitting risk).
- Expecting `.predict_proba()` to work by default on `SVC` — it requires `probability=True` at construction (and is expensive; uses internal cross-validation).
- Forgetting `loss="hinge"` when using `LinearSVC` (it's not the default) if you want the standard SVM formulation.

---

## 🔑 Cheat Sheet

- SVM = maximize the margin ("widest street") between classes; only **support vectors** determine the boundary.
- **Hard margin**: no violations allowed, fragile to outliers. **Soft margin**: allow some violations, controlled by `C` (low C = wider street/more tolerant = more regularized).
- **Kernel trick**: get the effect of high-dimensional feature expansion (polynomial or RBF similarity features) without ever computing those features explicitly.
- `gamma` (RBF kernel) also acts as a regularizer: high γ → overfit-prone/wiggly; low γ → smoother.
- Try linear kernel first, then RBF; specialized kernels only when you have a specific reason.
- `LinearSVC`/`SGDClassifier` scale to huge data; `SVC` is for smaller, more complex datasets.
- SVM Regression flips the objective: fit as many points *inside* an ε-wide street as possible.

---

## ✍️ Practice

1. **Hands-on:** Load the "moons" dataset (`sklearn.datasets.make_moons(noise=0.15)`). Train a `LinearSVC`, then an `SVC(kernel="rbf")`. Plot both decision boundaries. Which one handles the curved shape correctly, and why?
2. **Conceptual:** Explain in your own words why the kernel trick avoids the "combinatorial explosion" of adding high-degree polynomial features directly.
3. **Debug-the-scenario:** An `SVC(kernel="rbf", gamma=50, C=1000)` gets 100% training accuracy but poor test accuracy. Name the likely cause and two hyperparameter changes that would help.
   <details><summary>Answer</summary>Overfitting — both `gamma` and `C` are set very high (low regularization). Reduce `gamma` (smoother boundary) and/or reduce `C` (wider margin, more tolerant of violations).</details>
4. **Design question:** You have a dataset with 2 million rows and only 20 features, and you need a fast linear-ish classifier. Would you reach for `SVC`, `LinearSVC`, or `SGDClassifier`? Why?

Next: `06_decision_trees.md` — a model you can literally draw as a flowchart, and the foundation for Random Forests.