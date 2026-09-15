import sys
sys.path.insert(0, "/home/claude")
from nb_helper import md, code, make_notebook

OUT = "/mnt/user-data/outputs/ML_Notes/code/part1_fundamentals"

# ============================================================ Chapter 7
cells = [
md("""# Chapter 7 — Ensemble Learning and Random Forests
Companion notebook to `part1_fundamentals/07_ensemble_random_forests.md`."""),

code("""import numpy as np
import matplotlib.pyplot as plt
from sklearn.datasets import make_moons
from sklearn.model_selection import train_test_split, cross_val_score
from sklearn.linear_model import LogisticRegression
from sklearn.svm import SVC
from sklearn.tree import DecisionTreeClassifier, DecisionTreeRegressor
from sklearn.ensemble import (VotingClassifier, BaggingClassifier, RandomForestClassifier,
                               ExtraTreesClassifier, AdaBoostClassifier, GradientBoostingRegressor)"""),

code("""X, y = make_moons(n_samples=500, noise=0.3, random_state=42)
X_train, X_test, y_train, y_test = train_test_split(X, y, random_state=42)"""),

md("## Voting Classifier (soft voting, combining different algorithms)"),
code("""log_clf = LogisticRegression()
svm_clf = SVC(probability=True)
rf_clf = RandomForestClassifier(n_estimators=100, random_state=42)

voting_clf = VotingClassifier(
    estimators=[("lr", log_clf), ("rf", rf_clf), ("svc", svm_clf)], voting="soft")
voting_clf.fit(X_train, y_train)

for name, clf in [("LogReg", log_clf), ("RandomForest", rf_clf), ("SVC", svm_clf), ("Voting", voting_clf)]:
    clf.fit(X_train, y_train)
    print(f"{name:14s} test accuracy = {clf.score(X_test, y_test):.3f}")"""),

md("## Single tree vs Bagging vs Random Forest"),
code("""tree_clf = DecisionTreeClassifier(random_state=42)
bag_clf = BaggingClassifier(DecisionTreeClassifier(), n_estimators=500,
                             max_samples=100, bootstrap=True, n_jobs=-1, random_state=42,
                             oob_score=True)
rnd_clf = RandomForestClassifier(n_estimators=500, max_leaf_nodes=16, n_jobs=-1, random_state=42)

for name, clf in [("Single Tree", tree_clf), ("Bagging", bag_clf), ("Random Forest", rnd_clf)]:
    scores = cross_val_score(clf, X, y, cv=10)
    print(f"{name:14s} CV accuracy = {scores.mean():.3f} (+/- {scores.std():.3f})")

bag_clf.fit(X_train, y_train)
print("Bagging OOB score (free validation estimate):", bag_clf.oob_score_)"""),

md("## Feature importance (free from tree ensembles)"),
code("""from sklearn.datasets import load_iris
iris = load_iris()
rnd_clf_iris = RandomForestClassifier(n_estimators=500, random_state=42)
rnd_clf_iris.fit(iris["data"], iris["target"])
for name, score in sorted(zip(iris["feature_names"], rnd_clf_iris.feature_importances_),
                           key=lambda t: -t[1]):
    print(f"{name:20s} {score:.3f}")"""),

md("## AdaBoost & Gradient Boosting with early stopping"),
code("""ada_clf = AdaBoostClassifier(DecisionTreeClassifier(max_depth=1), n_estimators=200,
                              algorithm="SAMME.R", learning_rate=0.5, random_state=42)
ada_clf.fit(X_train, y_train)
print("AdaBoost test accuracy:", ada_clf.score(X_test, y_test))"""),
code("""np.random.seed(42)
X_gb = np.random.rand(200, 1) - 0.5
y_gb = 3 * X_gb[:, 0] ** 2 + 0.05 * np.random.randn(200)
X_gb_train, X_gb_val, y_gb_train, y_gb_val = train_test_split(X_gb, y_gb, random_state=42)

gbrt = GradientBoostingRegressor(max_depth=2, n_estimators=120, learning_rate=0.1,
                                  random_state=42)
gbrt.fit(X_gb_train, y_gb_train)

errors = [np.mean((y_gb_val - y_pred) ** 2)
          for y_pred in gbrt.staged_predict(X_gb_val)]
best_n_estimators = np.argmin(errors) + 1
print("Best number of trees (early stopping via staged_predict):", best_n_estimators)

plt.plot(range(1, len(errors) + 1), errors)
plt.axvline(best_n_estimators, color="r", linestyle="--")
plt.xlabel("Number of trees"); plt.ylabel("Validation MSE")
plt.title("Finding the optimal n_estimators")
plt.show()"""),

md("""## ✍️ Practice
1. Compare single tree vs Bagging(500) vs RandomForest(500) CV accuracy above — how much does each step help?
2. Why does averaging correlated weak classifiers help less than averaging independent ones?
3. Debug: GradientBoostingRegressor(n_estimators=1000, lr=0.01) still improving at 1000 — what would you try?
4. Print rnd_clf_iris feature_importances_ (done above) — which feature matters least? Try dropping it."""),
code("# Your code / notes here\n"),
]
make_notebook(cells, f"{OUT}/07_ensemble_random_forests.ipynb")

# ============================================================ Chapter 8
cells = [
md("""# Chapter 8 — Dimensionality Reduction
Companion notebook to `part1_fundamentals/08_dimensionality_reduction.md`.
Uses MNIST (downloads the first time) and a synthetic Swiss Roll (no download needed)."""),

code("""import numpy as np
import matplotlib.pyplot as plt
from sklearn.datasets import fetch_openml, make_swiss_roll
from sklearn.decomposition import PCA, KernelPCA
from sklearn.manifold import LocallyLinearEmbedding
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split
import time"""),

md("## PCA on MNIST: how many dimensions to keep 95% variance?"),
code("""mnist = fetch_openml('mnist_784', version=1, as_frame=False)
X, y = mnist["data"][:20000], mnist["target"][:20000]   # subsample for speed
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

pca = PCA(n_components=0.95)
X_train_reduced = pca.fit_transform(X_train)
print("Original dims:", X_train.shape[1], "-> Reduced dims:", X_train_reduced.shape[1])
print("Explained variance kept:", pca.explained_variance_ratio_.sum())"""),

code("""# Compare training time / accuracy: full data vs PCA-reduced data
X_test_reduced = pca.transform(X_test)

for name, Xtr, Xte in [("Full (784-d)", X_train, X_test), ("PCA (95%)", X_train_reduced, X_test_reduced)]:
    clf = RandomForestClassifier(n_estimators=50, random_state=42)
    t0 = time.time()
    clf.fit(Xtr, y_train)
    train_time = time.time() - t0
    acc = clf.score(Xte, y_test)
    print(f"{name:15s} train_time={train_time:.1f}s  test_accuracy={acc:.3f}")"""),

md("## PCA compression / reconstruction"),
code("""pca_154 = PCA(n_components=154)
X_reduced_154 = pca_154.fit_transform(X_train)
X_recovered = pca_154.inverse_transform(X_reduced_154)

fig, axes = plt.subplots(1, 2, figsize=(6, 3))
axes[0].imshow(X_train[0].reshape(28, 28), cmap="binary"); axes[0].set_title("Original"); axes[0].axis("off")
axes[1].imshow(X_recovered[0].reshape(28, 28), cmap="binary"); axes[1].set_title("Reconstructed (154-d)"); axes[1].axis("off")
plt.show()"""),

md("## Unrolling a nonlinear manifold: PCA vs Kernel PCA vs LLE"),
code("""X_swiss, t_swiss = make_swiss_roll(n_samples=1500, noise=0.05, random_state=42)

fig = plt.figure(figsize=(6, 5))
ax = fig.add_subplot(111, projection="3d")
ax.scatter(X_swiss[:, 0], X_swiss[:, 1], X_swiss[:, 2], c=t_swiss, cmap="jet")
ax.set_title("Original Swiss Roll (3D)")
plt.show()"""),
code("""fig, axes = plt.subplots(1, 3, figsize=(15, 4))

pca_2d = PCA(n_components=2).fit_transform(X_swiss)
axes[0].scatter(pca_2d[:, 0], pca_2d[:, 1], c=t_swiss, cmap="jet"); axes[0].set_title("Plain PCA (flattens, tangles layers)")

kpca_2d = KernelPCA(n_components=2, kernel="rbf", gamma=0.04).fit_transform(X_swiss)
axes[1].scatter(kpca_2d[:, 0], kpca_2d[:, 1], c=t_swiss, cmap="jet"); axes[1].set_title("Kernel PCA (rbf)")

lle_2d = LocallyLinearEmbedding(n_components=2, n_neighbors=10).fit_transform(X_swiss)
axes[2].scatter(lle_2d[:, 0], lle_2d[:, 1], c=t_swiss, cmap="jet"); axes[2].set_title("LLE (unrolls cleanly)")

plt.show()"""),

md("""## ✍️ Practice
1. Above: compare training time/accuracy full vs PCA-reduced — was the trade-off worth it here?
2. Explain why maximizing variance = minimizing reconstruction error (Pythagorean theorem intuition).
3. Above: which method unrolled the Swiss Roll most cleanly — plain PCA, Kernel PCA, or LLE? Why?
4. Try KernelPCA with a few different gamma values on the Swiss Roll — how sensitive is the result?"""),
code("# Your code / notes here\n"),
]
make_notebook(cells, f"{OUT}/08_dimensionality_reduction.ipynb")

# ============================================================ Chapter 9
cells = [
md("""# Chapter 9 — Unsupervised Learning Techniques
Companion notebook to `part1_fundamentals/09_unsupervised_learning.md`. No downloads needed."""),

code("""import numpy as np
import matplotlib.pyplot as plt
from sklearn.datasets import make_blobs, make_moons
from sklearn.cluster import KMeans, DBSCAN
from sklearn.mixture import GaussianMixture, BayesianGaussianMixture
from sklearn.metrics import silhouette_score"""),

md("## K-Means: choosing k via the elbow method and silhouette score"),
code("""X, y_true = make_blobs(n_samples=1000, centers=4, cluster_std=0.7, random_state=42)

inertias, silhouettes = [], []
k_range = range(2, 9)
for k in k_range:
    km = KMeans(n_clusters=k, n_init=10, random_state=42)
    km.fit(X)
    inertias.append(km.inertia_)
    silhouettes.append(silhouette_score(X, km.labels_))

fig, axes = plt.subplots(1, 2, figsize=(11, 4))
axes[0].plot(list(k_range), inertias, "bo-"); axes[0].set_xlabel("k"); axes[0].set_ylabel("Inertia"); axes[0].set_title("Elbow method")
axes[1].plot(list(k_range), silhouettes, "go-"); axes[1].set_xlabel("k"); axes[1].set_ylabel("Silhouette score"); axes[1].set_title("Silhouette score")
plt.show()"""),

code("""best_k = list(k_range)[int(np.argmax(silhouettes))]
kmeans = KMeans(n_clusters=best_k, n_init=10, random_state=42)
y_pred = kmeans.fit_predict(X)

plt.scatter(X[:, 0], X[:, 1], c=y_pred, cmap="tab10", s=10)
plt.scatter(kmeans.cluster_centers_[:, 0], kmeans.cluster_centers_[:, 1],
            marker="X", s=200, c="black")
plt.title(f"K-Means with k={best_k}")
plt.show()"""),

md("## DBSCAN: density-based clustering (no k needed, handles arbitrary shapes)"),
code("""X_moons, _ = make_moons(n_samples=1000, noise=0.05, random_state=42)

dbscan = DBSCAN(eps=0.2, min_samples=5)
labels = dbscan.fit_predict(X_moons)

plt.scatter(X_moons[:, 0], X_moons[:, 1], c=labels, cmap="tab10", s=10)
plt.title(f"DBSCAN on moons ({len(set(labels)) - (1 if -1 in labels else 0)} clusters found, -1 = noise)")
plt.show()"""),

md("## Gaussian Mixture Models: soft clustering + generation + BIC/AIC model selection"),
code("""bics, aics = [], []
n_range = range(1, 8)
for n in n_range:
    gm = GaussianMixture(n_components=n, n_init=10, random_state=42)
    gm.fit(X)
    bics.append(gm.bic(X))
    aics.append(gm.aic(X))

plt.plot(list(n_range), bics, "bo-", label="BIC")
plt.plot(list(n_range), aics, "go-", label="AIC")
plt.xlabel("Number of components"); plt.legend()
plt.title("Choosing the number of GMM components")
plt.show()

best_n = list(n_range)[int(np.argmin(bics))]
print("Best n_components by BIC:", best_n)"""),

code("""gm = GaussianMixture(n_components=best_n, n_init=10, random_state=42)
gm.fit(X)

# Generate brand-new synthetic instances!
new_samples, new_labels = gm.sample(10)
print("Sampled 10 new points from the learned distribution:\\n", new_samples[:3])

# Anomaly detection via low density
densities = gm.score_samples(X)
density_threshold = np.percentile(densities, 4)
anomalies = X[densities < density_threshold]
plt.scatter(X[:, 0], X[:, 1], s=10, label="normal")
plt.scatter(anomalies[:, 0], anomalies[:, 1], color="red", marker="x", s=60, label="flagged anomaly")
plt.legend(); plt.title("GMM-based anomaly detection (lowest-density 4%)")
plt.show()"""),

md("""## ✍️ Practice
1. Above: does the elbow method agree with the silhouette score on the best k?
2. Why does K-Means struggle on two long, thin, crossing ellipse clusters while DBSCAN handles them fine?
3. Debug: GMM with n_components=5 has 2 components with weight ~0.001 — what does this suggest? Try BayesianGaussianMixture(n_components=10) instead.
4. Bonus: use KMeans(n_clusters=8) to compress an image's colors (load any small photo as an (N,3) RGB array)."""),
code("# Your code / notes here\n"),
]
make_notebook(cells, f"{OUT}/09_unsupervised_learning.ipynb")

print("Chapters 7-9 done")