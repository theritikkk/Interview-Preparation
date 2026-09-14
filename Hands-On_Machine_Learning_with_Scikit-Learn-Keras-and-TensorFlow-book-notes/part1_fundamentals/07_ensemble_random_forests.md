# Chapter 7 — Ensemble Learning and Random Forests

## 🎯 Why this chapter matters
> **"Wisdom of the crowd"**: aggregate the answers of many mediocre predictors, and the *combined* answer is often better than the single best expert's answer.

This is the core idea behind **ensemble learning**, and Random Forests (an ensemble of Decision Trees) are one of the most powerful, popular, "just works" algorithms in all of ML — often a great choice even before you reach for deep learning.

---

## 🗳️ Voting Classifiers — the simplest ensemble

Train several *different* good-but-imperfect classifiers (e.g., Logistic Regression, SVM, Random Forest, kNN), then combine their predictions:

- **Hard voting**: each classifier votes for a class; go with the majority.
- **Soft voting**: average each classifier's predicted *class probabilities*; go with the highest average. Usually **outperforms hard voting**, because it gives more weight to confident votes — but requires every classifier to support `predict_proba()`.

```python
from sklearn.ensemble import VotingClassifier
from sklearn.linear_model import LogisticRegression
from sklearn.ensemble import RandomForestClassifier
from sklearn.svm import SVC

voting_clf = VotingClassifier(
    estimators=[('lr', LogisticRegression()), ('rf', RandomForestClassifier()), ('svc', SVC(probability=True))],
    voting='soft')
voting_clf.fit(X_train, y_train)
```

**Why this works — the math intuition:** if you flip a slightly-biased coin (51% heads) 1,000 times, the *law of large numbers* means you'll get close to 51% heads overall, even though any single flip is barely better than random. Similarly, if each classifier is only **slightly better than random guessing** (a "weak learner") but their errors are **independent** (uncorrelated), combining enough of them converges toward a "strong learner." ⚠️ **The key assumption is independence** — if all your classifiers make the *same* kinds of mistakes (e.g., all trained on the same data with similar algorithms), the ensemble won't help nearly as much. This is why **using very different algorithms** is a good way to build a voting ensemble.

---

## 🎒 Bagging and Pasting — diversity from data sampling

Instead of using different algorithms, use the **same algorithm** but train each copy on a **different random subset** of the training data.

- **Bagging** ("**b**ootstrap **agg**regat**ing**"): sample **with replacement** (an instance can be picked multiple times for the same predictor).
- **Pasting**: sample **without replacement**.

Once trained, the ensemble predicts by **aggregating**: majority vote (classification) or average (regression) — same statistical trick as before.

```python
from sklearn.ensemble import BaggingClassifier
from sklearn.tree import DecisionTreeClassifier

bag_clf = BaggingClassifier(
    DecisionTreeClassifier(), n_estimators=500,
    max_samples=100, bootstrap=True, n_jobs=-1)   # bootstrap=True → bagging; False → pasting
bag_clf.fit(X_train, y_train)
```
- `n_jobs=-1` uses all CPU cores — bagging/pasting **parallelizes beautifully**, since every predictor trains independently.
- Each individual tree has *higher* bias (it only sees a subset of data), but **aggregation reduces variance a lot while keeping bias about the same** → net win: similar bias, much lower variance than one big tree.
- Bagging (with-replacement) introduces a bit *more* diversity than pasting → slightly higher bias, but lower variance overall → **bagging is generally preferred** in practice, though it's worth cross-validating both if you have the time.

### Out-of-Bag (OOB) Evaluation — a free validation set
With bagging, each predictor only sees about **63%** of the training instances on average (as the dataset grows large, the with-replacement sampling asymptotically leaves out $1-e^{-1}\approx 36.8\%$ of instances). The unseen 37% — different for every predictor — are called **out-of-bag** instances, and since the predictor never trained on them, you can use them as a **free validation set**:

```python
bag_clf = BaggingClassifier(DecisionTreeClassifier(), n_estimators=500,
                             bootstrap=True, n_jobs=-1, oob_score=True)
bag_clf.fit(X_train, y_train)
bag_clf.oob_score_   # e.g. 0.901 — estimated test accuracy, no separate val set needed!
```

### Random Patches & Random Subspaces
`BaggingClassifier` can also sample **features**, not just instances (via `max_features`/`bootstrap_features`):
- Sampling both instances *and* features → **Random Patches** (great for high-dimensional data like images).
- Keeping all instances but sampling features only → **Random Subspaces**.

Sampling features trades a bit more bias for even lower variance (more diversity between predictors).

---

## 🌲 Random Forests — bagged trees, with an extra trick

A **Random Forest** is (typically) a bagging ensemble of Decision Trees:

```python
from sklearn.ensemble import RandomForestClassifier
rnd_clf = RandomForestClassifier(n_estimators=500, max_leaf_nodes=16, n_jobs=-1)
rnd_clf.fit(X_train, y_train)
```
*(Roughly equivalent to a `BaggingClassifier` wrapping `DecisionTreeClassifier`s, but more optimized and convenient.)*

**The extra trick — extra randomness:** instead of searching for the best split among *all* features at each node (like a plain Decision Tree), a Random Forest searches only among a **random subset of features** at each split. This **trades a bit more bias for lower variance**, generally producing an even better overall model — more tree diversity = more benefit from averaging.

### Extra-Trees ("Extremely Randomized Trees")
Push randomness even further: instead of searching for the *optimal* threshold for each candidate feature (like normal trees do), use **random thresholds** too, and just pick the best of those random options.
```python
from sklearn.ensemble import ExtraTreesClassifier
```
This trades yet more bias for lower variance, and it's **much faster to train** (no need to search for optimal thresholds — the single most expensive part of growing a tree). Whether Extra-Trees beats plain Random Forest depends on the dataset — the only way to know is to try both (typically via cross-validation).

### Feature Importance — a free bonus of tree-based models
Random Forests make it easy to measure how much each feature actually contributes to reducing impurity, averaged across all trees in the forest:
```python
for name, score in zip(iris["feature_names"], rnd_clf.feature_importances_):
    print(name, score)
# e.g. petal length: 0.44, petal width: 0.42, sepal length: 0.11, sepal width: 0.02
```
This is an easy, fast way to do **feature selection** or just understand your data better — one of the most underrated benefits of tree ensembles.

---

## 🚀 Boosting — predictors that correct each other's mistakes

Unlike bagging (predictors trained *independently*, in parallel), **boosting** trains predictors **sequentially**, each one trying to fix its predecessor's errors.

### AdaBoost (Adaptive Boosting)
1. Train a base classifier (often a shallow tree, a.k.a. **Decision Stump**: `max_depth=1`).
2. Find which instances it got wrong, and **increase their weight**.
3. Train a second classifier on the *reweighted* data — it's now forced to pay more attention to the hard cases the first one missed.
4. Repeat. Each new predictor gets its own **weight $\alpha_j$** based on how accurate it was — more accurate predictors get louder votes.
5. Final prediction: weighted vote across all predictors.

$$
\alpha_j = \eta \log\frac{1 - r_j}{r_j}
$$

where $r_j$ is predictor $j$'s weighted error rate and $\eta$ is the learning rate. A perfectly accurate predictor gets a huge weight; a predictor at chance level ($r_j = 0.5$) gets weight ≈ 0; a predictor *worse* than random even gets a **negative** weight (its votes are effectively flipped!).

```python
from sklearn.ensemble import AdaBoostClassifier
ada_clf = AdaBoostClassifier(
    DecisionTreeClassifier(max_depth=1), n_estimators=200,
    algorithm="SAMME.R", learning_rate=0.5)
ada_clf.fit(X_train, y_train)
```
⚠️ **Boosting can't be parallelized** (each predictor depends on the previous one's results) → slower to train than bagging at the same ensemble size. If your AdaBoost ensemble overfits, reduce `n_estimators` or regularize the base estimator more strongly.

### Gradient Boosting
Also sequential, but instead of reweighting misclassified *instances*, each new predictor is trained to predict the **residual errors** (the leftover mistakes) of the previous ensemble:

```python
from sklearn.tree import DecisionTreeRegressor

tree_reg1 = DecisionTreeRegressor(max_depth=2)
tree_reg1.fit(X, y)

y2 = y - tree_reg1.predict(X)          # residual errors of predictor 1
tree_reg2 = DecisionTreeRegressor(max_depth=2)
tree_reg2.fit(X, y2)                    # predictor 2 learns to predict predictor 1's mistakes

y3 = y2 - tree_reg2.predict(X)          # residual errors of predictors 1+2 combined
tree_reg3 = DecisionTreeRegressor(max_depth=2)
tree_reg3.fit(X, y3)

# Final prediction: sum every tree's contribution
y_pred = sum(tree.predict(X_new) for tree in (tree_reg1, tree_reg2, tree_reg3))
```
In practice, use the built-in class (it also supports **`learning_rate`**, which shrinks each tree's contribution — lower rates need more trees but tend to generalize better, a technique called **shrinkage**):
```python
from sklearn.ensemble import GradientBoostingRegressor
gbrt = GradientBoostingRegressor(max_depth=2, n_estimators=3, learning_rate=1.0)
gbrt.fit(X, y)
```
**Finding the right `n_estimators`:** use **early stopping** — track validation error as trees are added (`staged_predict()`), and stop once it stops improving, instead of guessing a fixed number upfront.

> 💡 Modern, highly optimized Gradient Boosting implementations — **XGBoost**, **LightGBM**, **CatBoost** — are extremely popular in industry and ML competitions (not covered in this book, but built on exactly this idea, with major speed/regularization improvements).

### Stacking (Stacked Generalization)
Instead of combining predictions with a simple vote/average, **train a model to learn the best way to combine them**:
- Split training data; train several base predictors ("layer 0") on one part.
- Have each base predictor make predictions on a held-out part.
- Train a final **blender/meta-learner** ("layer 1") whose *inputs are the base predictors' predictions* and whose target is the original label — it learns the optimal combination automatically, rather than a plain average/vote. (Scikit-Learn added native `StackingClassifier`/`StackingRegressor` support after this book's original writing — worth checking the current docs.)

---

## ⚠️ Common Pitfalls

- Building a voting ensemble from classifiers that are **all trained the same way on the same data** — their errors are correlated, so the "wisdom of crowds" benefit shrinks.
- Forgetting soft voting requires every base estimator to support `predict_proba()`.
- Treating boosting ensembles as parallelizable like bagging — they're inherently sequential.
- Setting a very high `learning_rate` with Gradient Boosting and too few estimators (underfitting), or a very low rate with too many estimators without early stopping (overfitting + wasted compute).
- Assuming Random Forest feature importances are perfectly reliable for **highly correlated features** — importance can get "split" between correlated features, understating both.

---

## 🔑 Cheat Sheet

- **Voting classifier**: combine different algorithms; soft voting (probability-averaging) usually beats hard voting (majority vote).
- **Bagging** (sample with replacement) / **Pasting** (without replacement): same algorithm, different data subsets, trained in parallel; reduces variance.
- **OOB evaluation**: free validation using the ~37% of instances each bagged predictor never saw.
- **Random Forest** = bagged Decision Trees + random feature subset at each split → more diversity, usually better than plain bagged trees.
- **Extra-Trees** = Random Forest + random split thresholds too → faster, more bias/less variance.
- **Feature importance**: free output of tree ensembles, great for quick feature selection/insight.
- **Boosting (AdaBoost, Gradient Boosting)**: sequential, each predictor fixes the previous one's mistakes; can't parallelize; watch for overfitting via `n_estimators`/`learning_rate`/early stopping.
- **Stacking**: train a meta-model to combine base predictors' outputs, instead of a fixed vote/average.

---

## ✍️ Practice

1. **Hands-on:** On the moons dataset, compare cross-validation accuracy of a single `DecisionTreeClassifier`, a `BaggingClassifier` of 500 trees, and a `RandomForestClassifier` of 500 trees. How much does each ensemble step improve accuracy?
2. **Conceptual:** Why does averaging many *correlated* weak classifiers help much less than averaging many *independent* ones? Connect this to the "wisdom of the crowd" coin-flip analogy.
3. **Debug-the-scenario:** Your `GradientBoostingRegressor` with `n_estimators=1000, learning_rate=0.01` trains for a very long time and the validation error is still falling by estimator 1000. What would you try?
   <details><summary>Answer</summary>Increase `n_estimators` further and use early stopping (via `staged_predict()` or `n_iter_no_change`) to automatically find the optimal count instead of guessing, and/or slightly raise `learning_rate` to converge faster (watching for overfitting either way).</details>
4. **Hands-on:** Train a `RandomForestClassifier` on the full iris dataset (all 4 features) and print `.feature_importances_`. Which feature matters least? Try dropping it and re-evaluating — does accuracy change meaningfully?

Next: `08_dimensionality_reduction.md` — when you have *too many* features, and need to compress without losing the signal.