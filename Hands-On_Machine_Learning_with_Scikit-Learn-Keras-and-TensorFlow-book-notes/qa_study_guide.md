# ❓ Q&A Study Guide — Hands-On Machine Learning

**How to use this:** for each question, try to answer it out loud or on paper *before* expanding the answer. That's the actual memory-building step — reading the answer first defeats the purpose. This covers all 19 chapters; if a question stumps you, that's a signal to reopen that chapter's notes.

---

## Part I — The Fundamentals of Machine Learning

### Chapter 1 — The Machine Learning Landscape

**Q1.** What are the four main types of ML, categorized by how much supervision they use during training?
<details><summary>Answer</summary>
Supervised (trains on labeled data — classification/regression), Unsupervised (no labels — clustering/anomaly detection/dimensionality reduction), Semisupervised (a little labeled data + a lot of unlabeled), and Reinforcement Learning (an agent learns a policy from rewards/penalties in an environment).
</details>

**Q2.** What's the difference between batch and online learning, and what hyperparameter controls how fast an online system adapts?
<details><summary>Answer</summary>
Batch learning trains on the entire dataset at once and is redeployed to update; online learning trains incrementally on data as it arrives. The **learning rate** controls adaptation speed in online learning — high means fast adaptation but more sensitivity to noise, low means slower but more stable.
</details>

**Q3.** Define overfitting and underfitting, and name one fix for each.
<details><summary>Answer</summary>
Overfitting: the model is too complex for the data (or the data is too noisy/limited), so it fits training data very well but generalizes poorly — fix with a simpler model, more/cleaner data, or regularization. Underfitting: the model is too simple to capture the real pattern — fix with a more powerful model or better features.
</details>

**Q4.** What's the purpose of a validation set, separate from the test set?
<details><summary>Answer</summary>
The validation set is used to compare candidate models and tune hyperparameters. The test set is touched only once, at the very end, to estimate real-world generalization error — using it for tuning would overfit to the test set itself.
</details>

**Q5.** State the No Free Lunch theorem in your own words.
<details><summary>Answer</summary>
No single algorithm is guaranteed to perform best across every possible problem — the only way to know which model works best for your specific data is to reasonably evaluate a handful of candidates.
</details>

**Q6.** What's the difference between sampling noise and sampling bias?
<details><summary>Answer</summary>
Sampling noise is nonrepresentative data due to random chance in a small sample. Sampling bias is nonrepresentative data because the *sampling method itself* is flawed — it persists even with a large sample (e.g., the 1936 Literary Digest poll oversampled wealthier respondents).
</details>

**Q7.** When would you prefer instance-based learning over model-based learning?
<details><summary>Answer</summary>
Instance-based learning (like k-Nearest Neighbors) can be a good fit when the relationship between features and target is too irregular/local for a simple parametric formula to capture, or when you want predictions to be driven directly by similarity to known examples rather than a fitted global function.
</details>

---

### Chapter 2 — End-to-End Machine Learning Project

**Q1.** Why should you create a test set *before* exploring the data, not after?
<details><summary>Answer</summary>
To avoid data snooping bias — if you (or your model selection) is influenced by patterns you noticed in what becomes your test set, your generalization estimate becomes unreliably optimistic.
</details>

**Q2.** What is stratified sampling, and when would you use it over plain random sampling?
<details><summary>Answer</summary>
Stratified sampling splits the population into homogeneous groups (strata) and samples proportionally from each, ensuring the sample matches the population's distribution on an important feature. Use it when a feature (like income) strongly affects the outcome and a plain random split risks skewing that feature's distribution between train/test.
</details>

**Q3.** What's the difference between RMSE and MAE, and when would you prefer one over the other?
<details><summary>Answer</summary>
RMSE (ℓ2 norm) squares errors before averaging, so it penalizes large errors heavily; MAE (ℓ1 norm) averages absolute errors and is more robust to outliers. Prefer MAE when your data has significant outliers you don't want to dominate the metric.
</details>

**Q4.** Why should you fit a `Scaler`/`Imputer` only on the training data, never on the full dataset?
<details><summary>Answer</summary>
Fitting on the full dataset (including test data) leaks information about the test set's distribution into preprocessing, which is a subtle form of data snooping — it makes your generalization estimate optimistic and unrealistic.
</details>

**Q5.** What does `cross_val_score` give you that a single train/validation split doesn't?
<details><summary>Answer</summary>
It gives you both a mean performance estimate *and* its standard deviation across multiple folds — telling you not just how good a model is on average, but how stable/sensitive that performance is to which data it happens to see.
</details>

**Q6.** What's the difference between `GridSearchCV` and `RandomizedSearchCV`?
<details><summary>Answer</summary>
GridSearchCV exhaustively tries every combination of specified hyperparameter values. RandomizedSearchCV samples a fixed number of random combinations instead — much more efficient when the search space is large.
</details>

**Q7.** Why should you only evaluate on the test set once, at the very end?
<details><summary>Answer</summary>
Any tweaking based on test set performance starts to overfit your choices to that specific test set, defeating its purpose as an unbiased estimate of real-world generalization.
</details>

---

### Chapter 3 — Classification

**Q1.** Why is accuracy a misleading metric when classes are imbalanced?
<details><summary>Answer</summary>
A trivial classifier that always predicts the majority class can score very high accuracy while being completely useless — e.g., predicting "not fraud" for everything scores ~99.5% accuracy when only 0.5% of transactions are fraudulent.
</details>

**Q2.** Define precision and recall in plain language.
<details><summary>Answer</summary>
Precision: "of everything I flagged as positive, how much was actually positive?" (trustworthiness of positive predictions). Recall: "of everything that was actually positive, how much did I catch?" (completeness of detection).
</details>

**Q3.** What does the F1 score capture that precision or recall alone doesn't?
<details><summary>Answer</summary>
F1 is the harmonic mean of precision and recall — it only scores well if *both* are reasonably high, punishing a large imbalance between the two more harshly than a plain average would.
</details>

**Q4.** When would you use a Precision/Recall curve instead of a ROC curve?
<details><summary>Answer</summary>
When the positive class is rare (imbalanced data) or when false positives matter more than false negatives — ROC-AUC can look deceptively good on imbalanced data even when precision is actually poor.
</details>

**Q5.** What's the difference between One-vs-Rest (OvR) and One-vs-One (OvO) multiclass strategies?
<details><summary>Answer</summary>
OvR trains one binary classifier per class ("this class vs. everyone else") — N classifiers for N classes. OvO trains one classifier per *pair* of classes — N(N-1)/2 classifiers — which is often faster overall for algorithms (like SVM) that scale poorly with dataset size, since each classifier only sees two classes' worth of data.
</details>

**Q6.** What's the difference between multilabel and multioutput classification?
<details><summary>Answer</summary>
Multilabel: each instance can belong to multiple classes at once, each still binary (e.g., "contains Alice: yes, contains Bob: no"). Multioutput: a generalization where each output can take on more than two possible values (e.g., predicting a pixel's intensity 0-255 for every pixel).
</details>

**Q7.** How does raising a classifier's decision threshold affect precision and recall?
<details><summary>Answer</summary>
Raising the threshold always increases precision and decreases recall (fewer, more confident positive predictions); lowering it does the opposite. There's no threshold that improves both simultaneously on a fixed model.
</details>

---

### Chapter 4 — Training Models

**Q1.** What does the Normal Equation compute, and what's its main computational downside?
<details><summary>Answer</summary>
It computes the exact closed-form optimal parameters for Linear Regression in one step: θ = (XᵀX)⁻¹Xᵀy. Its downside is that computing the matrix inverse is roughly O(n³) in the number of features, making it impractical for datasets with very many features.
</details>

**Q2.** Compare Batch, Stochastic, and Mini-batch Gradient Descent.
<details><summary>Answer</summary>
Batch GD uses the full training set per step (stable, slow on big data). Stochastic GD uses one random instance per step (fast, noisy, never fully settles — needs a decreasing learning rate). Mini-batch GD is the practical middle ground, using small random batches.
</details>

**Q3.** Why must you scale features before running Gradient Descent?
<details><summary>Answer</summary>
Unscaled features (very different ranges) create a long, narrow cost surface, causing Gradient Descent to zig-zag slowly toward the minimum instead of converging directly and evenly.
</details>

**Q4.** What's the key difference between Ridge and Lasso regularization?
<details><summary>Answer</summary>
Ridge (ℓ2) shrinks weights smoothly toward zero but rarely to exactly zero. Lasso (ℓ1) tends to push unimportant feature weights to exactly zero, performing automatic feature selection and producing a sparse model.
</details>

**Q5.** What is early stopping, and why is it sometimes called a "free lunch"?
<details><summary>Answer</summary>
Early stopping halts training as soon as validation error starts increasing, even while training error keeps falling — a sign of overfitting starting. It's a "free" regularizer because it costs nothing extra to implement and doesn't change the model's capacity.
</details>

**Q6.** What does Logistic Regression's cost function (log loss) penalize?
<details><summary>Answer</summary>
It heavily penalizes confident-but-wrong predictions (e.g., predicting a probability near 0 for an instance that's actually labeled 1) via a -log(p) term that blows up as the predicted probability for the true class approaches 0.
</details>

**Q7.** How does Softmax Regression differ from training multiple One-vs-Rest Logistic Regression classifiers?
<details><summary>Answer</summary>
Softmax Regression natively handles all classes at once, producing probabilities across K classes that sum to 1 in a single model, and predicts only one class per instance (not multilabel) — it's a direct multiclass generalization rather than K separate binary models combined afterward.
</details>

---

### Chapter 5 — Support Vector Machines

**Q1.** What is the "margin" an SVM tries to maximize?
<details><summary>Answer</summary>
The width of the "street" (gap) between the decision boundary and the closest training instances of each class — the support vectors are the instances sitting exactly on the edges of this street.
</details>

**Q2.** What's the difference between hard margin and soft margin classification?
<details><summary>Answer</summary>
Hard margin requires every instance to be strictly correctly classified outside the margin — fragile, and only works on perfectly separable data. Soft margin allows some margin violations in exchange for a wider, more generalizable street, controlled by the hyperparameter C.
</details>

**Q3.** What is the kernel trick, and why is it powerful?
<details><summary>Answer</summary>
It lets you get the mathematical effect of mapping data into a much higher-dimensional (even infinite-dimensional) feature space — making nonlinear patterns linearly separable — without ever actually computing those expanded features, avoiding a combinatorial explosion in computation/memory.
</details>

**Q4.** How does the `gamma` hyperparameter in an RBF kernel act as a regularizer?
<details><summary>Answer</summary>
High gamma makes each instance's influence very narrow/local, producing a wiggly, overfit-prone boundary; low gamma makes influence broader, producing a smoother boundary. Reduce gamma if overfitting, increase it if underfitting.
</details>

**Q5.** Why don't SVMs (the kernelized `SVC`) scale well to very large datasets, and what's the alternative?
<details><summary>Answer</summary>
`SVC`'s training time scales roughly O(m²n) to O(m³n) in the number of instances, becoming impractical past roughly a hundred thousand instances. For large datasets, use `LinearSVC` or `SGDClassifier(loss="hinge")` instead (trading away the kernel trick).
</details>

**Q6.** What is a support vector?
<details><summary>Answer</summary>
A training instance sitting exactly on (or, in the soft-margin case, violating) the edge of the margin — these are the only instances that actually determine where the decision boundary goes; adding more instances outside the street doesn't move it.
</details>

---

### Chapter 6 — Decision Trees

**Q1.** What is Gini impurity, and what does a value of 0 mean?
<details><summary>Answer</summary>
Gini impurity = 1 − Σ(pₖ)², measuring how mixed a node's classes are. A value of 0 means the node is "pure" — it contains only one class.
</details>

**Q2.** Why doesn't CART guarantee finding the globally optimal tree?
<details><summary>Answer</summary>
CART is a greedy algorithm — at each node, it picks the locally best split right now without considering whether a different split might lead to a better tree several levels down. Finding the true global optimum is NP-complete, so greedy approximation is the practical necessity.
</details>

**Q3.** List three hyperparameters that regularize a Decision Tree.
<details><summary>Answer</summary>
Any three of: `max_depth`, `min_samples_split`, `min_samples_leaf`, `min_weight_fraction_leaf`, `max_leaf_nodes`, `max_features`.
</details>

**Q4.** Why don't Decision Trees need feature scaling?
<details><summary>Answer</summary>
Trees split on raw threshold comparisons per feature (e.g., "petal length ≤ 2.45"), so the relative scale of different features doesn't affect where good split points are found.
</details>

**Q5.** Why are Decision Trees considered unstable models?
<details><summary>Answer</summary>
They're highly sensitive to small changes in training data (removing/adding even one instance can change the "best" split and cascade into a very different tree), and their splits are always axis-aligned, making them sensitive to how the data happens to be rotated/oriented.
</details>

**Q6.** How does a classification tree's leaf prediction differ from a regression tree's?
<details><summary>Answer</summary>
A classification leaf predicts the majority class (or class probabilities) of the training instances that landed there. A regression leaf predicts the *average target value* of those instances.
</details>

---

### Chapter 7 — Ensemble Learning and Random Forests

**Q1.** What's the difference between bagging and pasting?
<details><summary>Answer</summary>
Both train the same algorithm on different random subsets of the training data, in parallel. Bagging samples *with* replacement (bootstrap); pasting samples *without* replacement.
</details>

**Q2.** What is out-of-bag (OOB) evaluation, and why is it useful?
<details><summary>Answer</summary>
Each bagged predictor only trains on ~63% of instances on average; the remaining ~37% "out-of-bag" instances it never saw can be used as a free validation set — no separate holdout needed.
</details>

**Q3.** How does a Random Forest differ from a plain bagging ensemble of Decision Trees?
<details><summary>Answer</summary>
A Random Forest adds extra randomness: at each split, it only considers a random *subset* of features rather than all of them — trading a bit more bias for lower variance, usually producing a better overall ensemble than plain bagging.
</details>

**Q4.** What's the key difference between AdaBoost and Gradient Boosting?
<details><summary>Answer</summary>
AdaBoost reweights misclassified *instances* so the next predictor focuses on them, and weights each predictor's final vote by its accuracy. Gradient Boosting instead trains each new predictor to fit the *residual errors* of the combined ensemble so far.
</details>

**Q5.** Why can't boosting be parallelized the way bagging can?
<details><summary>Answer</summary>
Boosting is inherently sequential — each new predictor depends on the errors/residuals produced by the previous ones, so predictors can't be trained independently/simultaneously.
</details>

**Q6.** What does stacking do differently from a simple voting ensemble?
<details><summary>Answer</summary>
Instead of combining base predictors' outputs with a fixed rule (majority vote or average), stacking trains a separate "blender" model whose inputs are the base predictors' predictions — it *learns* the best way to combine them.
</details>

**Q7.** Why does soft voting usually outperform hard voting?
<details><summary>Answer</summary>
Soft voting averages predicted class *probabilities*, giving more influence to confident predictions, whereas hard voting only counts each classifier's single vote equally regardless of how confident it was.
</details>

---

### Chapter 8 — Dimensionality Reduction

**Q1.** What is the curse of dimensionality?
<details><summary>Answer</summary>
As the number of features grows, data points become increasingly sparse and far apart from each other, and the amount of training data needed to maintain the same sample density grows explosively — making overfitting more likely and distance-based reasoning less reliable.
</details>

**Q2.** What's the difference between projection and manifold learning as dimensionality-reduction strategies?
<details><summary>Answer</summary>
Projection flattens data onto a lower-dimensional subspace (like squashing a rolled-up sheet flat, which can tangle different layers together). Manifold learning tries to "unroll" the data's true lower-dimensional curved structure instead, preserving local relationships.
</details>

**Q3.** What does PCA maximize, and why does that also minimize reconstruction error?
<details><summary>Answer</summary>
PCA finds the axes that maximize preserved variance. Because total spread splits into "spread along the chosen axis" plus "spread of the projection error" (a fixed total, like the Pythagorean theorem), maximizing the first term automatically minimizes the second.
</details>

**Q4.** What does `explained_variance_ratio_` tell you?
<details><summary>Answer</summary>
The proportion of the dataset's total variance captured by each principal component — letting you decide how many components to keep to retain a target amount of information (e.g., 95%).
</details>

**Q5.** When would you use Incremental PCA instead of regular PCA?
<details><summary>Answer</summary>
When the dataset is too large to fit in memory all at once — Incremental PCA lets you feed it mini-batches via `.partial_fit()`, processing data in chunks streamed from disk.
</details>

**Q6.** Why can't plain PCA properly "unroll" a Swiss-roll-shaped dataset?
<details><summary>Answer</summary>
PCA is a *linear* projection — it flattens the data onto a hyperplane, which squashes different layers of a curved (nonlinear) manifold on top of each other instead of unrolling it. Kernel PCA or LLE handle this correctly.
</details>

---

### Chapter 9 — Unsupervised Learning Techniques

**Q1.** Why is inertia a bad metric for choosing k in K-Means?
<details><summary>Answer</summary>
Inertia (mean squared distance to the nearest centroid) monotonically decreases as k increases — it would always favor more clusters, up to k = number of instances, which is useless. Use the elbow method or silhouette score instead.
</details>

**Q2.** What are the two main assumptions K-Means makes about cluster shape/size?
<details><summary>Answer</summary>
It assumes clusters are roughly spherical and of similar size/density — it struggles badly with elongated clusters or clusters of very different sizes.
</details>

**Q3.** How does DBSCAN decide if a point is an anomaly?
<details><summary>Answer</summary>
If a point isn't a "core instance" (doesn't have enough neighbors within ε) and isn't within any core instance's neighborhood, it's flagged as noise/anomaly (label -1).
</details>

**Q4.** What's the difference between a "hard" and "soft" clustering assignment?
<details><summary>Answer</summary>
Hard assignment puts each instance into exactly one cluster (like K-Means). Soft assignment gives each instance a probability of belonging to each cluster (like a Gaussian Mixture Model).
</details>

**Q5.** Why can a Gaussian Mixture Model generate new data, while K-Means cannot?
<details><summary>Answer</summary>
A GMM models the actual probability distribution the data was drawn from (a mix of Gaussians with learned means/covariances/weights), so you can literally sample new points from that learned distribution. K-Means only learns centroid locations, not a full generative distribution.
</details>

**Q6.** Why use BIC/AIC instead of inertia or silhouette score to choose the number of GMM components?
<details><summary>Answer</summary>
Inertia and silhouette assume roughly spherical, similarly-sized clusters — an assumption GMMs don't share. BIC/AIC instead balance how well the model fits the data against model complexity (number of parameters), which suits GMM's probabilistic nature.
</details>

---

## Part II — Neural Networks and Deep Learning

### Chapter 10 — Introduction to ANNs with Keras

**Q1.** Why can't a single Perceptron solve the XOR problem?
<details><summary>Answer</summary>
A Perceptron's decision boundary is purely linear (a weighted sum + step function), and XOR isn't linearly separable — no straight line can separate its classes.
</details>

**Q2.** Why does a neural network need nonlinear activation functions between layers?
<details><summary>Answer</summary>
Without them, stacking any number of linear layers mathematically collapses into a single linear transformation — no matter how "deep" the network looks, it could only ever learn linear relationships.
</details>

**Q3.** What loss/activation combination would you use for: multiclass classification, binary classification, and regression?
<details><summary>Answer</summary>
Multiclass (exclusive classes): softmax + (sparse) categorical crossentropy. Binary: sigmoid + binary crossentropy. Regression: linear (no activation) + MSE (or sigmoid + MSE if the target is bounded).
</details>

**Q4.** What's the key difference between the Sequential, Functional, and Subclassing APIs in Keras?
<details><summary>Answer</summary>
Sequential: simplest, a straight stack of layers. Functional: flexible — supports multiple inputs/outputs, skip connections, shared layers (still fully inspectable/savable). Subclassing: full imperative Python control (loops, conditionals), at the cost of losing easy introspection/saving.
</details>

**Q5.** What does `EarlyStopping(restore_best_weights=True)` do?
<details><summary>Answer</summary>
It stops training once validation performance hasn't improved for a set number of epochs (`patience`), and rolls the model back to the weights from its best-performing epoch, rather than keeping the final (possibly overfit) epoch's weights.
</details>

**Q6.** Why is the learning rate often considered the single most important hyperparameter to tune?
<details><summary>Answer</summary>
It has an outsized effect on both training speed and final performance — too high and training diverges/bounces; too low and training is painfully slow or gets stuck. Getting it right (or using a schedule) often matters more than most architectural choices.
</details>

---

### Chapter 11 — Training Deep Neural Networks

**Q1.** What causes the vanishing gradients problem, and name two fixes.
<details><summary>Answer</summary>
It's caused by activation functions (like sigmoid) that saturate, combined with naive weight initialization, causing gradients to shrink toward zero as they propagate backward through many layers. Fixes: better initialization (He/Glorot/LeCun matched to the activation), nonsaturating activations (ReLU family, ELU, SELU), and Batch Normalization.
</details>

**Q2.** What is the "dying ReLU" problem, and how does Leaky ReLU address it?
<details><summary>Answer</summary>
A ReLU neuron can end up outputting negative (i.e., zeroed) values for every training instance, making its gradient permanently zero — it "dies" and never updates again. Leaky ReLU gives negative inputs a small nonzero slope, so the neuron can never fully die.
</details>

**Q3.** What does Batch Normalization do, and why does it also act as a regularizer?
<details><summary>Answer</summary>
It normalizes each layer's inputs using the current mini-batch's statistics, then learns a scale/shift to undo that if useful — stabilizing and speeding up training. It regularizes somewhat because the batch statistics introduce a bit of noise into training, similar in spirit to Dropout.
</details>

**Q4.** How does Dropout behave differently during training vs. at inference time?
<details><summary>Answer</summary>
During training, each neuron has a probability of being temporarily ignored at each step, forcing redundant/robust representations. At inference, dropout is turned off and the full network is used (with appropriate scaling).
</details>

**Q5.** What's the key difference between Momentum and Adam as optimizers?
<details><summary>Answer</summary>
Momentum accumulates a "velocity" from past gradients to accelerate consistent directions, like a ball rolling downhill. Adam combines that momentum idea with an *adaptive*, per-parameter learning rate (borrowed from RMSProp) — generally needing less manual learning-rate tuning.
</details>

**Q6.** When fine-tuning a pretrained model, why use a lower learning rate on the reused layers?
<details><summary>Answer</summary>
The reused layers already hold useful, well-trained weights — a normal/high learning rate risks quickly destroying that useful structure ("catastrophic forgetting") before the new layers have learned anything useful to guide it.
</details>

---

### Chapter 12 — Custom Models and Training with TensorFlow

**Q1.** Why must a custom Keras loss function return one value per instance, not the mean?
<details><summary>Answer</summary>
Keras handles averaging internally (and needs per-instance values to correctly apply sample/class weights) — returning a single averaged scalar breaks that machinery.
</details>

**Q2.** What does `tf.GradientTape` do?
<details><summary>Answer</summary>
It records every operation performed on `Variable`s within its context, so that `.gradient()` can compute exact gradients via reverse-mode automatic differentiation afterward — the engine underlying Keras's `fit()`.
</details>

**Q3.** Why would you write a custom training loop instead of using `model.fit()`?
<details><summary>Answer</summary>
For genuinely unusual training needs `fit()` doesn't support — e.g., using two different optimizers for two different parts of a network. It should be a last resort, since it's more code and more error-prone than `fit()`.
</details>

**Q4.** What does `get_config()` do for a custom layer or loss?
<details><summary>Answer</summary>
It lets the layer/loss's hyperparameters be saved and correctly restored when the model is loaded later, instead of requiring you to manually re-supply `custom_objects` with hardcoded values.
</details>

**Q5.** What does `@tf.function` do to a Python function?
<details><summary>Answer</summary>
It traces the function once and converts it into an optimized, portable TensorFlow computation graph — typically faster to run and exportable to run without a Python interpreter.
</details>

---

### Chapter 13 — Loading and Preprocessing Data with TensorFlow

**Q1.** What's the typical order of operations in a `tf.data` pipeline?
<details><summary>Answer</summary>
Load → preprocess (`map`) → `cache()` (if it fits in RAM) → `shuffle` → `repeat` → `batch` → `prefetch`.
</details>

**Q2.** What does `.prefetch()` actually speed up?
<details><summary>Answer</summary>
It lets the CPU prepare the *next* batch while the GPU is still training on the *current* one, instead of the GPU sitting idle waiting for data — often a big, nearly-free speedup for I/O-bound pipelines.
</details>

**Q3.** When would you use an embedding instead of one-hot encoding for a categorical feature?
<details><summary>Answer</summary>
When the feature has high cardinality (many unique values, like zip codes or words) — one-hot encoding would produce huge, mostly-empty vectors, whereas an embedding learns a compact, trainable, dense representation.
</details>

**Q4.** What is "training/serving skew," and how do Keras preprocessing layers help avoid it?
<details><summary>Answer</summary>
It's when preprocessing logic used at serving time subtly differs from what was used during training, silently breaking the model. Bundling preprocessing layers *inside* the model itself ensures the exact same logic ships and runs wherever the model is deployed.
</details>

---

### Chapter 14 — Deep Computer Vision Using CNNs

**Q1.** Why is weight sharing in a convolutional layer so much more parameter-efficient than a Dense layer on the same image?
<details><summary>Answer</summary>
A conv filter's weights are reused (slid) across the entire image rather than having a unique weight per pixel position, drastically cutting parameter count while letting the same learned pattern be detected anywhere in the image.
</details>

**Q2.** What trade-off does pooling introduce — what does it gain, and what does it cost?
<details><summary>Answer</summary>
It gains computational/memory savings and a degree of translation invariance (helpful for classification). It costs spatial precision/detail — a real problem for tasks needing pixel-exact output, like segmentation.
</details>

**Q3.** What problem do ResNet's skip connections solve?
<details><summary>Answer</summary>
They let a block default to approximating the identity function at initialization (rather than an arbitrary function), making very deep networks (100+ layers) much easier to train by keeping gradients flowing well even before every layer has learned something useful.
</details>

**Q4.** Why is max pooling's translation invariance bad for semantic segmentation, even though it helps classification?
<details><summary>Answer</summary>
Segmentation needs *equivariance* (shift the input, shift the output the same way) to produce pixel-precise boundaries — invariance actively discards the positional information segmentation depends on.
</details>

**Q5.** When doing transfer learning with a pretrained CNN, why freeze the base layers first before unfreezing?
<details><summary>Answer</summary>
The new top layer(s) start randomly initialized; training everything at once with normal learning rates risks sending large, destructive gradients back through the well-trained pretrained layers before the new layer has learned anything sensible to guide it.
</details>

---

### Chapter 15 — Processing Sequences Using RNNs and CNNs

**Q1.** What is the "short-term memory problem" in plain RNNs, and how does LSTM address it?
<details><summary>Answer</summary>
Information from early time steps gradually dilutes as it passes through many transformations, so plain RNNs struggle with long-range dependencies. LSTM adds a separate long-term state (a "conveyor belt") plus gates that control what's added, forgotten, and output — letting information persist much further.
</details>

**Q2.** Why is `return_sequences=True` needed on all but the last recurrent layer in a stack?
<details><summary>Answer</summary>
Without it, an intermediate recurrent layer only passes its *final* output to the next layer, instead of the full sequence of outputs the next recurrent layer needs to process.
</details>

**Q3.** Why is tanh often preferred over ReLU inside RNN cells?
<details><summary>Answer</summary>
Because the same weights are reused at every time step, ReLU's unbounded outputs can compound across steps and make training *more* unstable in RNNs than in deep feedforward nets — unlike feedforward networks, where ReLU is usually the safer default.
</details>

**Q4.** What's the practical difference between LSTM and GRU?
<details><summary>Answer</summary>
GRU is a simplified LSTM — it merges the long-/short-term states into one and uses only two gates instead of three, making it cheaper to compute while often performing comparably.
</details>

**Q5.** Why does WaveNet use dilated convolutions?
<details><summary>Answer</summary>
Doubling the dilation rate at each successive layer makes the receptive field grow exponentially with depth, letting the network capture very long-range patterns with only a handful of layers — while still processing the whole sequence in parallel, unlike a recurrent approach.
</details>

---

### Chapter 16 — Natural Language Processing with RNNs and Attention

**Q1.** What bottleneck problem does Attention solve in a plain Encoder-Decoder?
<details><summary>Answer</summary>
A plain Encoder-Decoder compresses the entire source sequence into one fixed-size vector, which becomes a severe information bottleneck for long sequences. Attention lets the decoder look back at *all* encoder outputs, weighted by relevance, shortening the effective path from any input to its corresponding output.
</details>

**Q2.** What are Query, Key, and Value in Scaled Dot-Product Attention?
<details><summary>Answer</summary>
It's framed as a differentiable dictionary lookup: the Query is what you're looking for, Keys are compared against the Query (via dot product) to get relevance weights, and the Values (weighted by those scores) are combined into the output.
</details>

**Q3.** Why does the Transformer need positional encoding?
<details><summary>Answer</summary>
Attention has no inherent sense of sequence order — it only looks at relationships between elements, not their positions — so a fixed sine/cosine-based positional signal is added to each embedding to inject order information.
</details>

**Q4.** What's the key difference between GPT's and BERT's pretraining approach?
<details><summary>Answer</summary>
GPT is decoder-only and unidirectional (masked self-attention, predicts the next word using only leftward context). BERT is bidirectional, pretrained via Masked Language Modeling (predict randomly masked words using context from both directions) plus Next Sentence Prediction.
</details>

**Q5.** Why does the Transformer decoder use a "causal mask" on its self-attention?
<details><summary>Answer</summary>
To prevent a position from attending to (seeing) future tokens it's supposed to be predicting — without it, the model could "cheat" during training and would fail at real generation, where future tokens genuinely don't exist yet.
</details>

---

### Chapter 17 — Autoencoders and GANs

**Q1.** What forces an undercomplete autoencoder to learn something useful instead of just copying its input?
<details><summary>Answer</summary>
Its bottleneck (latent representation) has fewer dimensions than the input, so it physically cannot pass all the information through unchanged — it's forced to discover and retain only the most important underlying structure.
</details>

**Q2.** How does a Variational Autoencoder differ from a plain autoencoder, and why does that let it generate new data?
<details><summary>Answer</summary>
A VAE's encoder outputs a *distribution* (mean and standard deviation) rather than a fixed point, and a latent-loss term (KL divergence) pushes all codings toward a shared, simple Gaussian "cloud." This organizes the whole latent space meaningfully, so sampling a random point and decoding it produces a plausible new instance.
</details>

**Q3.** In GAN training, what is the generator optimizing for, and what is the discriminator optimizing for?
<details><summary>Answer</summary>
The discriminator is trained to correctly classify real vs. generated (fake) data. The generator is trained to fool the discriminator into classifying its fake output as real — the generator never sees real data directly, only gradients flowing back through the discriminator.
</details>

**Q4.** What is mode collapse?
<details><summary>Answer</summary>
When the generator discovers that producing one (or very few) type of output reliably fools the discriminator, and collapses to generating mostly that, losing the diversity of the real data distribution.
</details>

**Q5.** Why does a denoising autoencoder tend to learn better features than a plain one?
<details><summary>Answer</summary>
Since the input is corrupted differently each time but the target is always the clean original, the network can't rely on trivial pixel-level copying — it's forced to learn genuinely meaningful, robust structure in the data.
</details>

---

### Chapter 18 — Reinforcement Learning

**Q1.** What's the difference between on-policy and off-policy learning, using Policy Gradients and Q-Learning as examples?
<details><summary>Answer</summary>
Policy Gradients is on-policy: it improves the very policy it uses to explore. Q-Learning is off-policy: it can learn the optimal Q-values from experience gathered under a *different* (even random) exploration policy.
</details>

**Q2.** What does the discount factor (gamma) control?
<details><summary>Answer</summary>
How much future rewards are "worth" compared to immediate rewards — a value close to 1 weighs the long-term future heavily, while a smaller value makes the agent more short-sighted/myopic.
</details>

**Q3.** Why is a replay buffer important for Deep Q-Learning?
<details><summary>Answer</summary>
Training on the most recent, highly-correlated experiences directly would badly destabilize Gradient Descent; sampling random batches from a buffer of past experiences breaks that correlation and lets old experiences be reused.
</details>

**Q4.** Why is loss a poor progress indicator in RL, compared to supervised learning?
<details><summary>Answer</summary>
Loss can decrease while the agent's actual performance gets worse (e.g., overfitting to a narrow region of the state space), or increase while performance improves (e.g., correctly revising previously-underestimated Q-values) — track actual episode reward instead.
</details>

**Q5.** What trade-off does an epsilon-greedy policy balance?
<details><summary>Answer</summary>
Exploration (acting randomly, to discover new/better strategies) vs. exploitation (acting greedily on the best-known strategy so far) — epsilon typically starts high (mostly exploring) and decays over training (mostly exploiting).
</details>

---

### Chapter 19 — Training and Deploying TensorFlow Models at Scale

**Q1.** Why should preprocessing be bundled inside an exported model rather than reimplemented separately in the serving application?
<details><summary>Answer</summary>
It guarantees the exact same preprocessing logic is used at training and serving time, eliminating training/serving skew — a subtle, common source of production bugs when the two implementations drift apart.
</details>

**Q2.** What's the difference between data parallelism and model parallelism?
<details><summary>Answer</summary>
Data parallelism replicates the whole model across devices, splitting the *data* (each replica trains on a different batch, gradients are averaged). Model parallelism splits the *model itself* across devices — trickier, with heavy cross-device communication, and generally only worthwhile for specific architectures.
</details>

**Q3.** What does quantization trade away in exchange for a smaller/faster model?
<details><summary>Answer</summary>
It reduces numeric precision (e.g., 32-bit floats down to 8-bit integers), which adds some noise to weights/activations — usually a small accuracy cost, though it can be significant enough to require quantization-aware training to recover.
</details>

**Q4.** Why do returns from adding more GPUs to training typically diminish?
<details><summary>Answer</summary>
"Bandwidth saturation" — the time spent communicating (averaging) gradients between devices eventually competes with, and can exceed, the time saved by parallelizing computation, especially for large, dense models.
</details>

**Q5.** What's the difference between TF Serving and TFLite — when would you use each?
<details><summary>Answer</summary>
TF Serving is a production model server for hosting models on servers/cloud, supporting REST/gRPC and multi-version deployment. TFLite is for shrinking/optimizing models to run directly on mobile or embedded devices, with limited compute/memory/power.
</details>

---

*That's all 19 chapters. Pair this with the practice exercises in each chapter's notes (open-ended, hands-on) and the interactive quizzes (multiple-choice, chapter by chapter) for three different ways to test the same material.*