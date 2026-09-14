# Chapter 6 — Decision Trees

## 🎯 Why this chapter matters
Decision Trees are the most **interpretable** ML model you'll meet — you can literally trace a prediction with a flowchart of yes/no questions. They also require almost no data preparation (no scaling needed!) and are the **building block for Random Forests**, one of the most powerful and widely used algorithms in practice (next chapter).

---

## 🧠 Core Concepts

### How a Decision Tree predicts
A tree is a series of yes/no questions about your features, arranged so each answer routes you to the next question, until you hit a **leaf** with a final prediction.

```python
from sklearn.datasets import load_iris
from sklearn.tree import DecisionTreeClassifier

iris = load_iris()
X = iris.data[:, 2:]   # petal length & width
y = iris.target

tree_clf = DecisionTreeClassifier(max_depth=2)
tree_clf.fit(X, y)
```

Walking a trained tree: *"Is petal length ≤ 2.45 cm? → Yes → predict Setosa (done, that's a pure leaf). → No → Is petal width ≤ 1.75 cm? → Yes → predict Versicolor. → No → predict Virginica."*

- **`samples`**: how many training instances reached this node.
- **`value`**: the class breakdown of those instances (e.g., `[0, 1, 45]` = 0 setosa, 1 versicolor, 45 virginica).
- **`gini`**: how "impure" (mixed) this node's classes are.

> 💡 **Decision Trees are "white box" models** — you can explain exactly *why* they made a prediction. Compare to "black box" models (Random Forests, neural nets) that predict great but are hard to explain in simple terms.

⚠️ Decision Trees **need no feature scaling or centering** — they split on raw thresholds, so it doesn't matter if features have wildly different ranges.

### Gini Impurity — how "mixed" is a node?

$$
G_i = 1 - \sum_{k=1}^{n} p_{i,k}^2
$$

Where $p_{i,k}$ is the fraction of class-$k$ instances in node $i$. A node is **pure** ($G_i = 0$) when it contains only one class. Example: a node with class counts `[0, 49, 5]` out of 54 has $G = 1 - (0/54)^2 - (49/54)^2 - (5/54)^2 \approx 0.168$.

**Gini vs. Entropy** (`criterion="entropy"`): entropy is an alternative impurity measure borrowed from information theory ($H_i = -\sum p_{i,k}\log_2 p_{i,k}$), zero when a node is pure. In practice the two produce **very similar trees**; Gini is slightly faster to compute (good default), while entropy tends to produce marginally more balanced trees. Not usually worth agonizing over.

### The CART Training Algorithm — how a tree is actually "grown"
Scikit-Learn uses **CART** (Classification And Regression Tree). At each node, it searches over every feature $k$ and every threshold $t_k$ for the split that produces the **purest possible children**, weighted by size:

$$
J(k, t_k) = \frac{m_{\text{left}}}{m}G_{\text{left}} + \frac{m_{\text{right}}}{m}G_{\text{right}}
$$

It then **recurses** on each child, repeating the same search, until it hits `max_depth`, or can't find a split that reduces impurity, or another stopping condition kicks in.

⚠️ **CART is a *greedy* algorithm** — it picks the best split *right now* at each step, without looking ahead to see if a different (locally worse) split might lead to a better tree overall a few levels down. Finding the *globally optimal* tree is NP-complete (computationally intractable), so every practical Decision Tree algorithm is a "good enough" greedy approximation, not a provably optimal tree.

**Complexity:** predicting is fast — $O(\log_2 m)$, since you just walk down a roughly balanced tree, independent of the number of features. Training is $O(n \times m \log_2 m)$, since every feature is compared at every node.

---

## 🛡️ Regularization — Decision Trees overfit *easily*

Left unconstrained, a Decision Tree will keep splitting until every leaf is pure — which usually means it has **memorized the training data** (severe overfitting; this is a **nonparametric model**: it has no fixed number of parameters set in advance, so its structure can grow to match the training data's every quirk). This is exactly what happened with the `DecisionTreeRegressor` in Chapter 2's project that hit zero training error.

Control tree growth with hyperparameters (all of which restrict the tree's freedom, i.e., **regularize** it):

| Hyperparameter | What it limits |
|---|---|
| `max_depth` | Maximum number of levels in the tree |
| `min_samples_split` | Minimum samples a node must have before it's allowed to split |
| `min_samples_leaf` | Minimum samples a leaf must have |
| `min_weight_fraction_leaf` | Same as above, but as a fraction of total weighted instances |
| `max_leaf_nodes` | Maximum number of leaf nodes total |
| `max_features` | Maximum number of features considered when looking for the best split at each node |

**Rule of thumb:** increasing the `min_*` hyperparameters or decreasing the `max_*` hyperparameters **regularizes** (simplifies) the model.

```python
tree_clf_regularized = DecisionTreeClassifier(max_depth=3, min_samples_leaf=5)
```

---

## 📈 Regression with Decision Trees

Same tree structure, but each leaf predicts the **average target value** of the training instances that land there, instead of a class:

```python
from sklearn.tree import DecisionTreeRegressor
tree_reg = DecisionTreeRegressor(max_depth=2)
tree_reg.fit(X, y)
```

CART's cost function changes accordingly — instead of minimizing weighted Gini impurity, it minimizes weighted **MSE** of the children. Regression trees are equally prone to overfitting without regularization (`max_depth`, `min_samples_leaf`, etc. all still apply).

---

## ⚠️ Instability — Decision Trees' Achilles' heel

Two real limitations to know about, both worth remembering before you reach for a plain Decision Tree in production:

1. **Orthogonal decision boundaries only.** CART splits are always perpendicular to an axis (e.g., "petal length ≤ 2.45"), so trees are sensitive to training set **rotation** — a diagonal boundary that a linear model captures with one line might take a Decision Tree many staircase-like splits to approximate. (One mitigation: PCA, Ch. 8, can rotate the data into a better-aligned orientation first.)
2. **High variance to small changes in the data.** Because splits are chosen greedily, removing/adding just one training point (especially one that changes what the "best split" happens to be) can produce a **completely different tree**. Scikit-Learn's tree-growing algorithm is also stochastic by default (it randomly selects the set of features to evaluate at each node unless you fix `random_state`), so even retraining on the *same* data can produce different trees.

> 🔑 This instability is exactly the motivation for **Random Forests** (next chapter): average many different (unstable, high-variance) trees together, and the ensemble becomes far more stable and accurate than any single tree.

---

## ⚠️ Common Pitfalls

- Training a Decision Tree with no depth/leaf constraints and being surprised it overfits — this is the *default* behavior, not a bug.
- Interpreting a single Decision Tree's near-perfect training accuracy as a good sign — check cross-validation error instead.
- Assuming feature scaling helps Decision Trees — it doesn't (though it never hurts, either — it's just unnecessary work here).
- Trusting a single tree's structure as "the one true explanation" of the data — a slightly different sample could produce a very different tree (see Instability above).
- Forgetting that CART only produces **binary** splits (yes/no) — this is a Scikit-Learn/CART-specific property, not universal to all Decision Tree algorithms (e.g., ID3 allows multi-way splits).

---

## 🔑 Cheat Sheet

- Tree = sequence of single-feature threshold questions, ending in a leaf prediction.
- **Gini impurity**: $1 - \sum p_k^2$; 0 = pure node. Entropy is a close alternative; both work similarly in practice.
- **CART**: greedy, recursive algorithm; not guaranteed globally optimal (NP-complete to find the true optimum).
- No feature scaling needed. Predictions are fast ($O(\log m)$); training is $O(nm\log m)$.
- Regularize via `max_depth`, `min_samples_leaf`, `min_samples_split`, `max_leaf_nodes`, `max_features` — unconstrained trees overfit almost by default.
- Regression trees predict the average target value per leaf; classification trees predict the majority class (or class probabilities via `value`).
- Trees are unstable: sensitive to axis orientation and to small changes in training data → motivates Random Forests.

---

## ✍️ Practice

1. **Hands-on:** Train `DecisionTreeClassifier(max_depth=None)` (unconstrained) vs. `max_depth=3` on the iris dataset. Compare cross-validation accuracy. Which generalizes better?
2. **By hand:** A node has 40 samples: 30 of class A, 10 of class B. Compute its Gini impurity.
   <details><summary>Answer</summary>$G = 1 - (30/40)^2 - (10/40)^2 = 1 - 0.5625 - 0.0625 = 0.375$</details>
3. **Conceptual:** Why does rotating a 2D dataset 45° potentially make it much harder for a Decision Tree to model, even though the *underlying* class separation hasn't changed at all?
4. **Debug-the-scenario:** Two data scientists train a Decision Tree on the same dataset (no `random_state` set) and get visibly different trees. Is this a bug? What's happening?
   <details><summary>Answer</summary>Not a bug — CART's default feature-selection-at-each-split has a random component, and trees are highly sensitive to small differences during greedy splitting. Set `random_state` for reproducibility, but know this instability is inherent to single trees.</details>
5. **Hands-on:** Use `export_graphviz` (or `sklearn.tree.plot_tree`) to visualize a trained tree on the iris dataset. Manually trace the path for a flower with petal length 4.5 cm, petal width 1.5 cm — what class does the tree predict?

Next: `07_ensemble_random_forests.md` — turn many "weak," unstable trees into one very strong model.