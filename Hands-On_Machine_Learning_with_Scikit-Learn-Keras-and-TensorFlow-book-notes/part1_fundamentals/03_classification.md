# Chapter 3 — Classification

## 🎯 Why this chapter matters
Regression predicts a *number*; classification predicts a *category*. This chapter uses the classic **MNIST** dataset (70,000 images of handwritten digits 0–9) to teach you the metrics that actually matter for classifiers — because **accuracy alone is dangerously misleading**, especially when classes are imbalanced (e.g., fraud detection, disease screening).

---

## 🧠 Core Concepts

### The dataset: MNIST
70,000 small (28×28 pixel) grayscale images of handwritten digits, each labeled 0–9. It's the "hello world" of classification — small enough to experiment quickly, rich enough to teach real lessons.

```python
from sklearn.datasets import fetch_openml
mnist = fetch_openml('mnist_784', version=1, as_frame=False)
X, y = mnist["data"], mnist["target"]
X_train, X_test, y_train, y_test = X[:60000], X[60000:], y[:60000], y[60000:]
```
*(MNIST already comes pre-shuffled and split at index 60,000 — convenient, since shuffling matters for algorithms sensitive to instance ordering, like SGD.)*

### Start simple: a binary classifier
Before tackling all 10 digits, simplify: build a "5-detector" — is this image a 5, or not?

```python
y_train_5 = (y_train == '5')   # True for all 5s, False for all other digits
y_test_5 = (y_test == '5')

from sklearn.linear_model import SGDClassifier
sgd_clf = SGDClassifier(random_state=42)   # Stochastic Gradient Descent classifier
sgd_clf.fit(X_train, y_train_5)
sgd_clf.predict([X_train[0]])
```
`SGDClassifier` is a good default first choice for large datasets: it handles instances one at a time (or in mini-batches), which also makes it naturally suited to **online learning**.

---

## ⚖️ Why accuracy is a trap

Imagine a "detector" that just always predicts **"not a 5."** Since only ~10% of digits are 5s, this dumb classifier is already ~90% accurate — despite being *completely useless*. This is the **imbalanced classes** problem, and it's why you should almost never trust accuracy alone for classification, especially skewed datasets.

### Measuring accuracy properly: cross-validation
```python
from sklearn.model_selection import cross_val_score
cross_val_score(sgd_clf, X_train, y_train_5, cv=3, scoring="accuracy")
# e.g. array([0.95035, 0.96035, 0.9604])
```
Looks great — but per the paragraph above, don't be fooled. We need better tools.

---

## 🧮 The Confusion Matrix

The foundation for every classification metric that matters. Count, for every pair (actual class, predicted class), how many instances fell into that cell.

```python
from sklearn.model_selection import cross_val_predict
from sklearn.metrics import confusion_matrix

# cross_val_predict returns *out-of-fold* predictions — each prediction was made
# by a model that never saw that instance during training, just like a real test.
y_train_pred = cross_val_predict(sgd_clf, X_train, y_train_5, cv=3)
confusion_matrix(y_train_5, y_train_pred)
# array([[53057,  1522],     <- actual "not 5":  53057 correct, 1522 wrongly flagged as 5
#        [ 1325,  4096]])    <- actual "5":       1325 missed,  4096 correctly caught
```

- **Rows** = actual class. **Columns** = predicted class.
- **True Negative (TN)** = correctly said "not 5". **False Positive (FP)** = wrongly said "5".
- **False Negative (FN)** = wrongly said "not 5" (missed it). **True Positive (TP)** = correctly said "5".
- A *perfect* classifier has nonzero values only on the diagonal (TN, TP) — zero everywhere else.

### Precision & Recall

$$
\text{precision} = \frac{TP}{TP + FP} \qquad\qquad \text{recall} = \frac{TP}{TP + FN}
$$

- **Precision**: "Of everything I flagged as positive, how much was actually positive?" (Trustworthiness of a positive prediction.)
- **Recall** (a.k.a. *sensitivity*, *true positive rate*): "Of everything that was actually positive, how much did I catch?"

```python
from sklearn.metrics import precision_score, recall_score
precision_score(y_train_5, y_train_pred)   # 0.729 → correct 72.9% of the time it says "5"
recall_score(y_train_5, y_train_pred)      # 0.756 → catches 75.6% of all actual 5s
```

### The trade-off is unavoidable
**You cannot maximize both simultaneously** — pushing one up typically pushes the other down. Which one matters more is a **business decision**, not a math decision:
- **Kid-safe video filter**: prioritize **precision** — better to reject some good videos than let one bad one through.
- **Shoplifter/fraud/disease detector**: prioritize **recall** — a few false alarms are fine; missing a real case is costly.

### F1 score — one number, when you need it
$$
F_1 = \frac{2}{\frac{1}{\text{precision}} + \frac{1}{\text{recall}}} = 2 \times \frac{\text{precision} \times \text{recall}}{\text{precision} + \text{recall}}
$$

This is the **harmonic mean** of precision and recall — it punishes imbalance between the two much more than a plain average would, so a classifier only scores well on F1 if *both* precision and recall are decent.

```python
from sklearn.metrics import f1_score
f1_score(y_train_5, y_train_pred)   # 0.742
```

### Controlling the trade-off: decision threshold
Under the hood, classifiers compute a **score** for each instance and compare it to a **threshold** — above it → positive class. `SGDClassifier` lets you inspect and override this:

```python
y_scores = sgd_clf.decision_function([some_digit])
threshold = 0
y_some_digit_pred = (y_scores > threshold)

# Raise the threshold → precision goes up, recall goes down
```

Plot precision and recall as functions of the threshold to pick the sweet spot for your use case:

```python
from sklearn.metrics import precision_recall_curve

y_scores = cross_val_predict(sgd_clf, X_train, y_train_5, cv=3, method="decision_function")
precisions, recalls, thresholds = precision_recall_curve(y_train_5, y_scores)
```

### The ROC Curve
The **Receiver Operating Characteristic** curve plots the **True Positive Rate** (recall) against the **False Positive Rate** (FPR = FP / (FP+TN)) across all thresholds — another lens on the same trade-off. The **Area Under the Curve (AUC)** summarizes it as one number: 1.0 = perfect classifier, 0.5 = random guessing.

```python
from sklearn.metrics import roc_auc_score
roc_auc_score(y_train_5, y_scores)
```
**Rule of thumb:** prefer the **PR curve** when the positive class is rare (imbalanced data, like our 5-detector) or when false positives matter more than false negatives; prefer the **ROC curve** otherwise.

---

## 🔢 Multiclass Classification

Some algorithms (Logistic Regression, SVM) are natively binary; others (Random Forest, Naive Bayes) natively handle multiple classes. Scikit-Learn automatically uses one of two strategies to extend binary classifiers:

- **One-vs-Rest (OvR)**: train one binary classifier per class (e.g., "is it a 0?", "is it a 1?"...), and pick whichever gives the highest confidence score. Needs only *N* classifiers for *N* classes.
- **One-vs-One (OvO)**: train a binary classifier for **every pair** of classes (e.g., "0 vs 1", "0 vs 2"...) — N×(N-1)/2 classifiers — and let them vote. Each classifier only needs to be trained on the subset of data for its two classes, which is faster for algorithms that scale poorly with dataset size (like SVMs), even though there are more classifiers overall.

```python
sgd_clf.fit(X_train, y_train)   # scikit-learn auto-detects multiclass target and uses OvR/OvO
sgd_clf.predict([some_digit])
```

### Error Analysis
Once you have a working multiclass model, look at *where* it's wrong — this drives your next improvements far more effectively than blindly tuning hyperparameters:

```python
y_train_pred = cross_val_predict(sgd_clf, X_train_scaled, y_train, cv=3)
conf_mx = confusion_matrix(y_train, y_train_pred)
# Normalize by row (i.e. by actual class support) and visualize as a heatmap —
# bright off-diagonal cells reveal exactly which digits get confused with which.
```
E.g., you might discover 8s and 3s are confused a lot → maybe engineer features that count closed loops, or gather more training images of that specific confusion pair.

### Multilabel & Multioutput Classification
- **Multilabel classification**: each instance can belong to **multiple classes at once** (e.g., a face-recognition system flagging "[Alice: yes, Bob: no, Charlie: yes]" for one photo containing two people).
```python
from sklearn.neighbors import KNeighborsClassifier
y_multilabel = np.c_[(y_train.astype(int) >= 7), (y_train.astype(int) % 2 == 0)]
knn_clf = KNeighborsClassifier()
knn_clf.fit(X_train, y_multilabel)
```
- **Multioutput classification** (a generalization): each label itself can take on more than two values — e.g., a denoising system that, for every pixel, predicts its correct intensity (0–255) — that's multiple outputs, each with many possible classes.

---

## ⚠️ Common Pitfalls

- Judging a classifier by **accuracy alone**, especially on imbalanced classes.
- Not realizing precision and recall trade off — chasing "high accuracy" without asking *which kind of error is worse for my application*.
- Using `cross_val_score`/`cross_val_predict` on data that hasn't been shuffled when the underlying algorithm is order-sensitive (SGD).
- Forgetting that raising the classification threshold **always** trades recall for precision (and vice versa) — there's no threshold that improves both simultaneously on the same model.
- Comparing ROC-AUC across wildly imbalanced problems without also checking the PR curve — ROC can look deceptively good even when precision is poor.

---

## 🔑 Cheat Sheet

- **Confusion matrix**: rows = actual, columns = predicted. TN / FP / FN / TP.
- **Precision** = TP/(TP+FP) — "how trustworthy are my positive calls?"
- **Recall** = TP/(TP+FN) — "how many real positives did I catch?"
- **F1** = harmonic mean of precision & recall — use when you need one balanced number.
- Precision ↑ ⇒ Recall ↓ (and vice versa) — controlled by the **decision threshold**.
- **PR curve** → best for rare positive class. **ROC curve/AUC** → best for roughly balanced classes.
- **OvR**: N classifiers, each "class vs. everyone else." **OvO**: N(N−1)/2 classifiers, each "class A vs class B", used natively by algorithms that scale badly with data size.
- **Multilabel**: several yes/no tags per instance. **Multioutput**: several *non-binary* predictions per instance.

---

## ✍️ Practice

1. **Hands-on:** Using MNIST, train an `SGDClassifier` 3-detector (is it a "3" or not?). Report precision, recall, and F1. Then plot the precision/recall curve and pick a threshold that guarantees ≥95% precision — what recall do you get at that threshold?
2. **Conceptual:** Explain why a spam filter should probably prioritize precision, while a cancer-screening test should probably prioritize recall. Can you think of a real system where you'd want to *balance* both equally (i.e., optimize F1 directly)?
3. **Debug-the-scenario:** A colleague reports "99.1% accuracy!" on a fraud-detection model where only 0.5% of transactions are fraudulent. What's your first question, and what two additional metrics would you ask for?
   <details><summary>Answer</summary>Ask for the confusion matrix / precision & recall on the fraud class specifically — with such a skewed base rate, a model that predicts "not fraud" for everything would already score ~99.5% accuracy while catching zero fraud.</details>
4. **Hands-on (harder):** Build a multiclass classifier for all 10 MNIST digits with `RandomForestClassifier`. Plot the normalized confusion matrix. Which two digits get confused most often, and can you guess why by looking at a few misclassified example images?

Next: `04_training_models.md` — what's actually happening inside `.fit()`: the math of Linear/Polynomial/Logistic Regression and Gradient Descent.