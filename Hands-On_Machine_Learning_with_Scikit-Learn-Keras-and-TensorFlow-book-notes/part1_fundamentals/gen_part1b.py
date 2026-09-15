import sys
sys.path.insert(0, "/home/claude")
from nb_helper import md, code, make_notebook

OUT = "/mnt/user-data/outputs/ML_Notes/code/part1_fundamentals"

# ============================================================ Chapter 3
cells = [
md("""# Chapter 3 — Classification
Companion notebook to `part1_fundamentals/03_classification.md`.
Uses MNIST via `fetch_openml` (downloads ~50MB the first time — needs internet)."""),

code("""import numpy as np
import matplotlib.pyplot as plt
from sklearn.datasets import fetch_openml
from sklearn.linear_model import SGDClassifier
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import cross_val_score, cross_val_predict
from sklearn.metrics import (confusion_matrix, precision_score, recall_score, f1_score,
                              precision_recall_curve, roc_curve, roc_auc_score)"""),

code("""mnist = fetch_openml('mnist_784', version=1, as_frame=False)
X, y = mnist["data"], mnist["target"]
X_train, X_test, y_train, y_test = X[:60000], X[60000:], y[:60000], y[60000:]

def plot_digit(data):
    plt.imshow(data.reshape(28, 28), cmap="binary")
    plt.axis("off")

plot_digit(X_train[0])
plt.title(f"Label: {y_train[0]}")
plt.show()"""),

md("## Binary classifier: is it a 5?"),
code("""y_train_5 = (y_train == '5')
y_test_5 = (y_test == '5')

sgd_clf = SGDClassifier(random_state=42)
sgd_clf.fit(X_train, y_train_5)
print("Predict digit 0 is a 5?", sgd_clf.predict([X_train[0]]))

print("Cross-val accuracy:", cross_val_score(sgd_clf, X_train, y_train_5, cv=3, scoring="accuracy"))"""),

md("## Confusion matrix, precision, recall, F1"),
code("""y_train_pred = cross_val_predict(sgd_clf, X_train, y_train_5, cv=3)
print(confusion_matrix(y_train_5, y_train_pred))
print("Precision:", precision_score(y_train_5, y_train_pred))
print("Recall:   ", recall_score(y_train_5, y_train_pred))
print("F1:       ", f1_score(y_train_5, y_train_pred))"""),

md("## Precision/Recall trade-off & ROC curve"),
code("""y_scores = cross_val_predict(sgd_clf, X_train, y_train_5, cv=3, method="decision_function")
precisions, recalls, thresholds = precision_recall_curve(y_train_5, y_scores)

plt.plot(thresholds, precisions[:-1], "b--", label="Precision")
plt.plot(thresholds, recalls[:-1], "g-", label="Recall")
plt.xlabel("Threshold"); plt.legend(); plt.title("Precision/Recall vs threshold")
plt.show()

print("ROC AUC:", roc_auc_score(y_train_5, y_scores))"""),

md("## Multiclass classification + error analysis"),
code("""forest_clf = RandomForestClassifier(random_state=42)
y_train_pred_multi = cross_val_predict(forest_clf, X_train, y_train, cv=3)
conf_mx = confusion_matrix(y_train, y_train_pred_multi)

plt.matshow(conf_mx, cmap="gray")
plt.title("Confusion matrix (10-class)")
plt.show()

print("Overall accuracy:", (y_train_pred_multi == y_train).mean())"""),

md("""## ✍️ Practice
1. Train a 3-detector. Find the threshold giving >=95% precision — what recall do you get?
2. Why should a spam filter prioritize precision while cancer screening prioritizes recall?
3. A model gets "99.1% accuracy" on 0.5%-fraud data — what should you ask for instead?
4. Which two digits get confused most in the confusion matrix above? Look at a few misclassified examples."""),
code("# Your code / notes here\n"),
]
make_notebook(cells, f"{OUT}/03_classification.ipynb")

# ============================================================ Chapter 4
cells = [
md("""# Chapter 4 — Training Models
Companion notebook to `part1_fundamentals/04_training_models.md`. Pure numpy/sklearn, no downloads needed."""),

code("""import numpy as np
import matplotlib.pyplot as plt
from sklearn.linear_model import LinearRegression, SGDRegressor, Ridge, Lasso, ElasticNet, LogisticRegression
from sklearn.preprocessing import PolynomialFeatures
from sklearn.pipeline import Pipeline
from sklearn.model_selection import learning_curve
from sklearn import datasets"""),

md("## The Normal Equation, by hand, vs scikit-learn"),
code("""np.random.seed(42)
X = 2 * np.random.rand(100, 1)
y = 4 + 3 * X + np.random.randn(100, 1)

X_b = np.c_[np.ones((100, 1)), X]
theta_best = np.linalg.inv(X_b.T.dot(X_b)).dot(X_b.T).dot(y)
print("Normal Equation theta:", theta_best.ravel())

lin_reg = LinearRegression()
lin_reg.fit(X, y)
print("sklearn theta:        ", [lin_reg.intercept_[0], lin_reg.coef_[0][0]])"""),

md("## Gradient Descent (SGDRegressor)"),
code("""sgd_reg = SGDRegressor(max_iter=1000, tol=1e-3, penalty=None, eta0=0.1, random_state=42)
sgd_reg.fit(X, y.ravel())
print("SGD theta:", [sgd_reg.intercept_[0], sgd_reg.coef_[0]])"""),

md("## Polynomial Regression + Learning Curves (diagnose under/overfitting)"),
code("""np.random.seed(42)
m = 100
X_q = 6 * np.random.rand(m, 1) - 3
y_q = 0.5 * X_q ** 2 + X_q + 2 + np.random.randn(m, 1)

def plot_learning_curves(model, X, y, degree_label=""):
    train_sizes, train_scores, val_scores = learning_curve(
        model, X, y, cv=5, scoring="neg_root_mean_squared_error",
        train_sizes=np.linspace(0.1, 1.0, 10))
    plt.plot(train_sizes, -train_scores.mean(axis=1), "r-+", label="train")
    plt.plot(train_sizes, -val_scores.mean(axis=1), "b-", label="validation")
    plt.legend(); plt.xlabel("Training set size"); plt.ylabel("RMSE")
    plt.title(f"Learning curves {degree_label}")
    plt.show()

for degree in [1, 2, 30]:
    poly_model = Pipeline([
        ("poly", PolynomialFeatures(degree=degree, include_bias=False)),
        ("lin_reg", LinearRegression()),
    ])
    plot_learning_curves(poly_model, X_q, y_q, degree_label=f"(degree={degree})")"""),

md("## Regularization: Ridge, Lasso, Elastic Net"),
code("""for name, model in [("Ridge", Ridge(alpha=1)), ("Lasso", Lasso(alpha=0.1)),
                     ("ElasticNet", ElasticNet(alpha=0.1, l1_ratio=0.5))]:
    model.fit(X, y)
    print(f"{name:12s} coef={model.coef_}")"""),

md("## Logistic & Softmax Regression on iris"),
code("""iris = datasets.load_iris()
X_iris = iris["data"][:, 3:]                  # petal width only
y_iris = (iris["target"] == 2).astype(int)     # 1 = Iris virginica

log_reg = LogisticRegression()
log_reg.fit(X_iris, y_iris)
print(log_reg.predict_proba([[1.7], [1.5]]))
print(log_reg.predict([[1.7], [1.5]]))

# Softmax (multinomial) on all 4 features, all 3 classes
X_all, y_all = iris["data"], iris["target"]
softmax_reg = LogisticRegression(multi_class="multinomial", solver="lbfgs", C=10)
softmax_reg.fit(X_all, y_all)
print("Softmax accuracy:", softmax_reg.score(X_all, y_all))"""),

md("""## ✍️ Practice
1. By hand: theta0=4, theta1=3, x1=2 -> predict y_hat.
2. Compare degree-1/2/300 polynomial fits + learning curves (done above) — describe what you see.
3. Why does Lasso zero out weights exactly, while Ridge only shrinks them close to zero?
4. Debug: SGD training loss oscillates wildly — name two likely causes + fixes.
5. Train softmax regression on all 4 iris features (done above) — inspect a few misclassified rows."""),
code("# Your code / notes here\n"),
]
make_notebook(cells, f"{OUT}/04_training_models.ipynb")

# ============================================================ Chapter 5
cells = [
md("""# Chapter 5 — Support Vector Machines
Companion notebook to `part1_fundamentals/05_support_vector_machines.md`."""),

code("""import numpy as np
import matplotlib.pyplot as plt
from sklearn import datasets
from sklearn.pipeline import Pipeline
from sklearn.preprocessing import StandardScaler
from sklearn.svm import LinearSVC, SVC
from sklearn.datasets import make_moons"""),

md("## Linear SVM on iris (soft margin)"),
code("""iris = datasets.load_iris()
X = iris["data"][:, (2, 3)]
y = (iris["target"] == 2).astype(np.float64)

svm_clf = Pipeline([
    ("scaler", StandardScaler()),
    ("linear_svc", LinearSVC(C=1, loss="hinge", max_iter=10000)),
])
svm_clf.fit(X, y)
print(svm_clf.predict([[5.5, 1.7]]))"""),

md("## Nonlinear SVM: the moons dataset"),
code("""def plot_decision_boundary(clf, X, y, title=""):
    x0s = np.linspace(X[:, 0].min() - 0.5, X[:, 0].max() + 0.5, 200)
    x1s = np.linspace(X[:, 1].min() - 0.5, X[:, 1].max() + 0.5, 200)
    x0, x1 = np.meshgrid(x0s, x1s)
    X_grid = np.c_[x0.ravel(), x1.ravel()]
    y_pred = clf.predict(X_grid).reshape(x0.shape)
    plt.contourf(x0, x1, y_pred, alpha=0.3, cmap="RdBu")
    plt.scatter(X[:, 0], X[:, 1], c=y, cmap="RdBu", edgecolors="k")
    plt.title(title)
    plt.show()

X_moons, y_moons = make_moons(n_samples=200, noise=0.15, random_state=42)

linear_svc = Pipeline([("scaler", StandardScaler()), ("svc", LinearSVC(C=10, max_iter=10000))])
linear_svc.fit(X_moons, y_moons)
plot_decision_boundary(linear_svc, X_moons, y_moons, "LinearSVC on moons (underfits the curve)")"""),

code("""poly_kernel_svm_clf = Pipeline([
    ("scaler", StandardScaler()),
    ("svm_clf", SVC(kernel="poly", degree=3, coef0=1, C=5)),
])
poly_kernel_svm_clf.fit(X_moons, y_moons)
plot_decision_boundary(poly_kernel_svm_clf, X_moons, y_moons, "Polynomial kernel SVC")"""),

code("""rbf_kernel_svm_clf = Pipeline([
    ("scaler", StandardScaler()),
    ("svm_clf", SVC(kernel="rbf", gamma=5, C=0.1)),
])
rbf_kernel_svm_clf.fit(X_moons, y_moons)
plot_decision_boundary(rbf_kernel_svm_clf, X_moons, y_moons, "RBF kernel SVC")"""),

md("""## ✍️ Practice
1. Compare LinearSVC vs SVC(rbf) decision boundaries above — which handles the moons shape correctly?
2. Explain why the kernel trick avoids a combinatorial explosion of polynomial features.
3. Debug: SVC(gamma=50, C=1000) gets 100% train accuracy, poor test accuracy — name the cause + 2 fixes.
4. Design: 2M rows, 20 features, need speed — SVC, LinearSVC, or SGDClassifier?"""),
code("# Your code / notes here\n"),
]
make_notebook(cells, f"{OUT}/05_support_vector_machines.ipynb")

# ============================================================ Chapter 6
cells = [
md("""# Chapter 6 — Decision Trees
Companion notebook to `part1_fundamentals/06_decision_trees.md`."""),

code("""import numpy as np
import matplotlib.pyplot as plt
from sklearn.datasets import load_iris
from sklearn.tree import DecisionTreeClassifier, DecisionTreeRegressor, plot_tree
from sklearn.model_selection import cross_val_score"""),

md("## Classification tree on iris + visualization"),
code("""iris = load_iris()
X, y = iris.data[:, 2:], iris.target   # petal length & width

tree_clf = DecisionTreeClassifier(max_depth=3, random_state=42)
tree_clf.fit(X, y)

plt.figure(figsize=(12, 8))
plot_tree(tree_clf, feature_names=["petal length", "petal width"],
          class_names=iris.target_names, filled=True)
plt.show()"""),

md("## Regularization: unconstrained vs max_depth=3"),
code("""unconstrained = DecisionTreeClassifier(random_state=42)
constrained = DecisionTreeClassifier(max_depth=3, min_samples_leaf=5, random_state=42)

for name, clf in [("Unconstrained", unconstrained), ("Regularized", constrained)]:
    scores = cross_val_score(clf, X, y, cv=10)
    print(f"{name:14s} CV accuracy = {scores.mean():.3f} (+/- {scores.std():.3f})")"""),

md("## Gini impurity, by hand"),
code("""def gini(counts):
    counts = np.array(counts, dtype=float)
    p = counts / counts.sum()
    return 1 - np.sum(p ** 2)

print("Gini([30, 10]):", gini([30, 10]))"""),

md("## Regression tree on noisy quadratic data"),
code("""np.random.seed(42)
X_reg = np.sort(6 * np.random.rand(200, 1) - 3, axis=0)
y_reg = 0.5 * X_reg[:, 0] ** 2 + X_reg[:, 0] + np.random.randn(200) * 0.3

tree_reg = DecisionTreeRegressor(max_depth=3)
tree_reg.fit(X_reg, y_reg)

X_test = np.linspace(-3, 3, 500).reshape(-1, 1)
plt.scatter(X_reg, y_reg, s=10)
plt.plot(X_test, tree_reg.predict(X_test), "r-", linewidth=2)
plt.title("Decision Tree Regressor (max_depth=3)")
plt.show()"""),

md("""## ✍️ Practice
1. Compare unconstrained vs max_depth=3 CV accuracy above — which generalizes better?
2. By hand: node has 40 samples, 30 class A / 10 class B — compute Gini impurity.
3. Why does rotating a 2D dataset 45 degrees make it harder for a tree to model?
4. Train two trees on the same data without random_state — are they identical? Why/why not?
5. Trace the visualized tree above for petal length=4.5, petal width=1.5 — what class is predicted?"""),
code("# Your code / notes here\n"),
]
make_notebook(cells, f"{OUT}/06_decision_trees.ipynb")

print("Chapters 3-6 done")