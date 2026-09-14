# Chapter 4 — Training Models

## 🎯 Why this chapter matters
So far you've called `.fit()` and trusted the magic. This chapter **opens the black box**: how does a Linear Regression actually find its best-fit line? What is Gradient Descent really doing? Why does Logistic Regression use a weird S-shaped curve? Understanding this chapter means every future "why isn't my model training" debugging session gets *way* easier — because you'll know what's supposed to be happening under the hood.

---

## 🧠 Linear Regression — the math

A linear model predicts by taking a **weighted sum of the input features plus a bias (intercept) term**:

$$
\hat{y} = \theta_0 + \theta_1 x_1 + \theta_2 x_2 + \dots + \theta_n x_n
$$

In vectorized form: $\hat{y} = h_\theta(\mathbf{x}) = \boldsymbol{\theta} \cdot \mathbf{x}$, where $\boldsymbol{\theta}$ is the parameter vector (bias + weights) and $\mathbf{x}$ is the feature vector (with $x_0 = 1$ prepended so the dot product includes the bias).

**Training = finding the θ that minimizes error.** The standard cost function is **Mean Squared Error (MSE)**:

$$
\text{MSE}(\boldsymbol{\theta}) = \frac{1}{m}\sum_{i=1}^{m}\left(\boldsymbol{\theta}^\top \mathbf{x}^{(i)} - y^{(i)}\right)^2
$$

### Option A — The Normal Equation (solve it in one shot)
For Linear Regression, there's a **closed-form solution** — an actual formula that jumps straight to the answer, no iteration needed:

$$
\hat{\boldsymbol{\theta}} = (\mathbf{X}^\top \mathbf{X})^{-1}\mathbf{X}^\top \mathbf{y}
$$

```python
import numpy as np

X = 2 * np.random.rand(100, 1)
y = 4 + 3 * X + np.random.randn(100, 1)   # true relationship: y = 4 + 3x + noise

X_b = np.c_[np.ones((100, 1)), X]    # add x0 = 1 to every instance (bias trick)
theta_best = np.linalg.inv(X_b.T.dot(X_b)).dot(X_b.T).dot(y)
# theta_best ≈ [[4.21], [2.77]]  — close to the true [4, 3], off only because of noise
```

⚠️ **Downside:** computing $(\mathbf{X}^\top \mathbf{X})^{-1}$ is roughly $O(n^3)$ in the number of features $n$ — it gets very slow if you have tens of thousands of features (though it scales *fine* with the number of training instances $m$, and handles huge datasets well as long as they fit in memory).

### Option B — Gradient Descent (iterate toward the answer)
When there's no closed-form solution (as with Logistic Regression, or with neural nets), or when there are too many features for the Normal Equation to be practical, use **Gradient Descent**: a general iterative optimization technique.

**The core idea:** imagine you're on a foggy mountain and want to reach the valley (minimum error). You feel the slope under your feet (the **gradient**) and take a step in the steepest *downhill* direction. Repeat until you stop descending.

$$
\theta_j \leftarrow \theta_j - \eta \frac{\partial}{\partial \theta_j}\text{MSE}(\boldsymbol{\theta})
$$

Here $\eta$ (eta) is the **learning rate** — the step size.
- Too small → training takes forever.
- Too large → you can overshoot and diverge (bounce around, never converge, or even get worse).

⚠️ **Gotcha:** Gradient Descent needs features on a similar scale to converge quickly and evenly — if one feature ranges 0–1 and another ranges 0–100,000, the cost surface becomes a long narrow valley and descent zig-zags slowly. **Always scale your features (e.g., `StandardScaler`) before Gradient Descent.**

**Three flavors, based on how much data you use per step:**

| Variant | Data used per update | Pros | Cons |
|---|---|---|---|
| **Batch GD** | *entire* training set, every step | Converges smoothly to the exact minimum | Slow / expensive on huge datasets — recomputes the full gradient every step |
| **Stochastic GD (SGD)** | 1 random instance at a time | Fast, works on huge data, naturally escapes small local minima (bounces around) | Never fully "settles" — the cost keeps bouncing near the minimum; needs a decreasing learning rate (**simulated annealing**) to actually converge |
| **Mini-batch GD** | a small random batch (e.g., 32–256 instances) | A practical middle ground; can use hardware-optimized matrix ops (GPUs) | Slightly less stable than Batch GD, less "escape-y" than pure SGD |

```python
from sklearn.linear_model import SGDRegressor
sgd_reg = SGDRegressor(max_iter=1000, tol=1e-3, penalty=None, eta0=0.1)
sgd_reg.fit(X, y.ravel())
```

---

## 📈 Polynomial Regression — fitting curves with a linear model

Trick: add **powers of each feature as new features**, then run plain Linear Regression on the expanded feature set. The model is still "linear" in its parameters — it's just linear in a richer feature space.

```python
from sklearn.preprocessing import PolynomialFeatures
from sklearn.linear_model import LinearRegression

poly_features = PolynomialFeatures(degree=2, include_bias=False)
X_poly = poly_features.fit_transform(X)   # now contains [x, x^2]

lin_reg = LinearRegression()
lin_reg.fit(X_poly, y)
```

⚠️ With `degree=d` and multiple original features, `PolynomialFeatures` also adds **all cross-terms** (e.g., $a \times b$) up to degree $d$ — the feature count explodes combinatorially, so keep `degree` modest.

### Learning Curves — diagnosing under/overfitting *without* looking at the test set
Plot training error and validation error as functions of **training set size**:

```python
from sklearn.model_selection import learning_curve
```

- **Both curves plateau at a high error, close together** → **underfitting**. Adding more data won't help; you need a more powerful model.
- **Training error is low, but there's a big gap to a higher validation error** → **overfitting**. More training data tends to close this gap; so does reducing model complexity or adding regularization.

This is one of the most useful diagnostic tools in ML — before reaching for "more data" or "a bigger model," plot the learning curves and let them tell you which problem you actually have.

> 🧭 **Bias/Variance trade-off, precisely:** A model's generalization error can be decomposed into **bias** (wrong assumptions — e.g., assuming linear when the data is quadratic → underfitting), **variance** (excessive sensitivity to small variations in training data — typically from a too-flexible model → overfitting), and **irreducible error** (noise inherent to the data itself, only fixable by cleaning the data). Increasing model complexity typically increases variance and reduces bias, and vice versa — hence "trade-off."

---

## 🛡️ Regularized Linear Models — fighting overfitting by constraining weights

The fewer degrees of freedom a model has, the harder it is for it to overfit. For linear models, we constrain the **weights**.

### Ridge Regression (ℓ2 penalty)
Adds a penalty proportional to the **sum of squared weights**:

$$
J(\boldsymbol{\theta}) = \text{MSE}(\boldsymbol{\theta}) + \alpha \frac{1}{2}\sum_{i=1}^{n}\theta_i^2
$$

- $\alpha = 0$ → plain Linear Regression. $\alpha$ very large → all weights shrink toward 0 (a nearly flat line through the mean).
- The bias term $\theta_0$ is **never** regularized (sum starts at $i=1$).
- Shrinks weights smoothly toward (but not exactly to) zero.
- ⚠️ **Always scale your data first** — Ridge (like most regularized models) is sensitive to feature scale.

```python
from sklearn.linear_model import Ridge
ridge_reg = Ridge(alpha=1, solver="cholesky")
ridge_reg.fit(X, y)
```

### Lasso Regression (ℓ1 penalty)
Adds a penalty proportional to the **sum of absolute weights**:

$$
J(\boldsymbol{\theta}) = \text{MSE}(\boldsymbol{\theta}) + \alpha\sum_{i=1}^{n}|\theta_i|
$$

- The key difference from Ridge: Lasso tends to push the *least important* feature weights all the way to **exactly zero** — it performs **automatic feature selection**, producing a **sparse model**.

```python
from sklearn.linear_model import Lasso
lasso_reg = Lasso(alpha=0.1)
lasso_reg.fit(X, y)
```

### Elastic Net — the best of both
A weighted mix of Ridge and Lasso penalties, controlled by a mix ratio $r$ (`l1_ratio` in scikit-learn): $r=0$ → pure Ridge, $r=1$ → pure Lasso.

```python
from sklearn.linear_model import ElasticNet
elastic_net = ElasticNet(alpha=0.1, l1_ratio=0.5)
```

**Which to pick?**
- Almost always use **some** regularization — plain unregularized Linear Regression is rarely the best choice.
- **Ridge** is a good default.
- If you suspect only a few features actually matter → **Lasso** or **Elastic Net** (they zero out the rest).
- **Prefer Elastic Net over pure Lasso** in general — Lasso can behave erratically when features outnumber instances, or when features are strongly correlated.

### Early Stopping — regularizing *iterative* algorithms for free
For Gradient-Descent-trained models, just **stop training as soon as validation error starts increasing** (even while training error keeps dropping — a sure sign of overfitting starting). Geoffrey Hinton reportedly called this a "beautiful free lunch."

```python
from sklearn.base import clone
best_val_loss = float("inf")
for epoch in range(1000):
    sgd_reg.partial_fit(X_train, y_train)          # continue training from where it left off
    y_val_predict = sgd_reg.predict(X_val)
    val_loss = mean_squared_error(y_val, y_val_predict)
    if val_loss < best_val_loss:
        best_val_loss = val_loss
        best_model = clone(sgd_reg)                 # snapshot the best-so-far model
```

---

## 🎯 Logistic Regression — regression for classification

Same linear "weighted sum" core, but the output is squeezed through the **sigmoid (logistic) function** to produce a **probability between 0 and 1**:

$$
p = h_\theta(\mathbf{x}) = \sigma(\boldsymbol{\theta}^\top \mathbf{x}), \qquad \sigma(t) = \frac{1}{1 + e^{-t}}
$$

Prediction rule: predict class **1** if $p \geq 0.5$ (equivalently, if the raw score $\boldsymbol{\theta}^\top\mathbf{x} \geq 0$), else class **0**.

**Training via log loss.** There's no Normal-Equation-style closed form for Logistic Regression, but its cost function is **convex**, so Gradient Descent is guaranteed to find the global minimum (given a reasonable learning rate and enough time):

$$
J(\boldsymbol{\theta}) = -\frac{1}{m}\sum_{i=1}^{m}\Big[y^{(i)}\log(p^{(i)}) + (1-y^{(i)})\log(1-p^{(i)})\Big]
$$

**Intuition:** if the true label is 1 but the model predicted a probability near 0, $-\log(p)$ blows up → huge penalty. If the prediction was confidently correct, the penalty is near 0. Symmetric logic for label 0.

```python
from sklearn.linear_model import LogisticRegression
from sklearn import datasets

iris = datasets.load_iris()
X = iris["data"][:, 3:]              # petal width only
y = (iris["target"] == 2).astype(int)  # 1 = Iris virginica, else 0

log_reg = LogisticRegression()
log_reg.fit(X, y)

log_reg.predict_proba([[1.7], [1.5]])   # probability of each class
log_reg.predict([[1.7], [1.5]])         # array([1, 0])
```

⚠️ Scikit-Learn's `LogisticRegression` applies **ℓ2 regularization by default**, controlled by `C` (note: this is the **inverse** of `alpha` — *higher* `C` means *less* regularization, unlike every other linear model in this chapter).

### Softmax Regression — Logistic Regression for *more than 2* classes at once
Instead of training multiple binary classifiers (Ch. 3's One-vs-Rest), Softmax Regression natively handles $K$ classes. Compute a score for each class, then convert all scores to probabilities that sum to 1 via the **softmax function**:

$$
p_k = \frac{\exp(s_k(\mathbf{x}))}{\sum_{j=1}^{K}\exp(s_j(\mathbf{x}))}, \qquad s_k(\mathbf{x}) = \boldsymbol{\theta}^{(k)\top}\mathbf{x}
$$

```python
softmax_reg = LogisticRegression(multi_class="multinomial", solver="lbfgs", C=10)
softmax_reg.fit(X, y)   # y now has 3 classes, e.g. iris species
```

⚠️ Softmax Regression predicts **only one class at a time** (multiclass, not multilabel) — it's not the right tool if an instance can belong to several classes simultaneously (recall Ch. 3's multilabel classification).

---

## ⚠️ Common Pitfalls

- Running Gradient Descent on **unscaled features** → slow, zig-zagging convergence.
- Choosing a learning rate that's too high (diverges) or too low (crawls) without ever *plotting the cost over iterations* to check.
- Forgetting that **SGD's cost never fully settles** — a fixed low learning rate keeps it bouncing near (not at) the optimum; use a learning schedule.
- Using plain Linear Regression with zero regularization "because it's simpler" — a little regularization almost always helps generalization.
- Confusing Ridge's `alpha` with Logistic Regression's `C` — they're inversely related.
- Interpreting Logistic Regression's `predict_proba` output as a guaranteed-calibrated real-world probability — it's an estimate, not a certainty.

---

## 🔑 Cheat Sheet

- Linear Regression prediction: $\hat{y} = \boldsymbol{\theta}\cdot\mathbf{x}$. Cost: MSE. Closed-form: Normal Equation, $O(n^3)$ in features.
- Gradient Descent: iterative, needs scaled features and a good learning rate $\eta$. Batch (stable, slow) / Stochastic (fast, noisy) / Mini-batch (practical default).
- Polynomial Regression = Linear Regression + engineered polynomial features.
- Learning curves diagnose underfitting (both curves high & close) vs. overfitting (big gap between them).
- Ridge (ℓ2, shrinks smoothly) / Lasso (ℓ1, zeroes out features) / Elastic Net (mix, generally the safer default over pure Lasso).
- Early stopping = free regularization for iterative algorithms.
- Logistic Regression: sigmoid + log loss, for binary classification. `C` (inverse of α) controls regularization strength.
- Softmax Regression: sigmoid → softmax, for native multiclass (not multilabel) classification.

---

## ✍️ Practice

1. **By hand:** Given $\theta_0=4, \theta_1=3$ and a new instance $x_1 = 2$, compute $\hat{y}$.
2. **Hands-on:** Generate noisy quadratic data (`y = 0.5*x**2 + x + 2 + noise`). Fit degree-1, degree-2, and degree-300 `PolynomialFeatures` + `LinearRegression`. Plot each fit. Which underfits? Which overfits? Now plot learning curves for the degree-300 model — describe what you see.
3. **Conceptual:** Why does Lasso tend to produce *exact* zeros for unimportant features while Ridge only shrinks them close to (but not exactly) zero? (Hint: think about the shape of the ℓ1 vs. ℓ2 penalty near zero.)
4. **Debug-the-scenario:** Your SGD-trained model's training loss oscillates wildly and never goes down. Name two likely causes and their fixes.
   <details><summary>Answer</summary>Learning rate too high → reduce it or use a learning-rate schedule. Unscaled features → apply `StandardScaler` before training.</details>
5. **Hands-on:** On the iris dataset, train a `LogisticRegression(multi_class="multinomial")` on all 4 features to classify all 3 species. Report accuracy and inspect a few misclassified rows.

Next: `05_support_vector_machines.md` — a completely different way to draw a decision boundary: maximize the margin.