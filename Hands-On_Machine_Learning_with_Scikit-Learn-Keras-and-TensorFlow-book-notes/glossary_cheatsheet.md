# 📖 Glossary & Master Cheat Sheet

*Every key term and formula from all 19 chapters, in one place — organized by topic, not by chapter, so you can jump straight to what you need before an interview, exam, or project.*

---

## 🧭 Core Vocabulary (Ch. 1)

| Term | Meaning |
|---|---|
| **Supervised / Unsupervised / Semisupervised / RL** | Learning with labels / without labels / with a little of both / via reward signals from an environment |
| **Classification / Regression** | Predict a category / predict a continuous number |
| **Batch / Online learning** | Train on everything at once, redeploy / train incrementally, on the fly (tune via **learning rate**) |
| **Instance-based / Model-based** | Memorize + compare via similarity (e.g., kNN) / fit parameters of a function (e.g., Linear Regression) |
| **Overfitting / Underfitting** | Model too complex for the data (fix: simplify, more/cleaner data, regularize) / model too simple (fix: more powerful model, better features) |
| **Training / Validation / Test set** | Fit the model / tune hyperparameters & pick the model / final, one-time generalization estimate |
| **No Free Lunch theorem** | No single algorithm is best for every problem — you must experiment |

---

## 📊 Evaluation Metrics (Ch. 2–3)

| Metric | Formula / Idea | Use when |
|---|---|---|
| **RMSE** | $\sqrt{\frac{1}{m}\sum(h(x)-y)^2}$ — ℓ2 norm of errors | Default for regression; penalizes large errors heavily |
| **MAE** | $\frac{1}{m}\sum\lvert h(x)-y\rvert$ — ℓ1 norm | Regression with outliers you don't want to over-penalize |
| **Precision** | $TP/(TP+FP)$ — "how trustworthy are positive calls?" | False positives are costly (e.g., content filters) |
| **Recall** | $TP/(TP+FN)$ — "how many real positives caught?" | False negatives are costly (e.g., disease screening) |
| **F1 score** | Harmonic mean of precision & recall | Need one balanced number |
| **ROC-AUC** | Area under TPR-vs-FPR curve | Roughly balanced classes |
| **PR curve** | Precision vs. recall across thresholds | Imbalanced / rare positive class |
| **Silhouette score** | $(b-a)/\max(a,b)$ per instance | Picking $k$ for K-Means |
| **BIC / AIC** | Fit quality penalized by model complexity | Picking # components for a GMM |

**Confusion matrix**: rows = actual, columns = predicted → TN / FP / FN / TP. Raising the classification **threshold** always trades recall down for precision up (and vice versa) — there's no free lunch here either.

---

## 📐 Classic ML Model Formulas (Ch. 4–9)

- **Linear Regression**: $\hat y = \boldsymbol\theta \cdot \mathbf{x}$. Closed form: **Normal Equation** $\hat{\boldsymbol\theta}=(\mathbf X^\top\mathbf X)^{-1}\mathbf X^\top\mathbf y$ ($O(n^3)$ in features). Iterative: **Gradient Descent** (Batch/Stochastic/Mini-batch).
- **Regularization**: **Ridge** (ℓ2, shrinks smoothly) / **Lasso** (ℓ1, zeroes out features) / **Elastic Net** (mix — usually safer than pure Lasso). **Early stopping** = free regularization for iterative training.
- **Logistic Regression**: sigmoid $\sigma(t)=1/(1+e^{-t})$ + log loss, binary classification. **Softmax Regression**: multiclass generalization (one class per instance, not multilabel).
- **SVM**: maximize the margin ("widest street"); **soft margin** controlled by `C` (low C = more tolerant/more regularized); **kernel trick** (poly, RBF) adds nonlinearity without computing expanded features explicitly; `gamma` in RBF also acts as a regularizer (high = overfit-prone).
- **Decision Trees (CART)**: greedy recursive splitting minimizing **Gini impurity** ($1-\sum p_k^2$) or entropy; unconstrained trees overfit almost by default — regularize with `max_depth`/`min_samples_leaf`/etc.
- **Ensembles**: **Bagging** (w/ replacement) / **Pasting** (w/o) — parallel, reduces variance; **Random Forest** = bagged trees + random feature subset per split; **Boosting** (AdaBoost reweights instances, Gradient Boosting fits residuals) — sequential, can't parallelize; **Stacking** = learn how to combine base models instead of a fixed vote.
- **PCA**: find orthogonal axes of max variance via SVD; `explained_variance_ratio_`; Kernel PCA / LLE for nonlinear manifolds.
- **Clustering**: **K-Means** (hard assignment, needs $k$, assumes spherical clusters) / **DBSCAN** (density-based, no $k$, arbitrary shapes, flags noise) / **GMM** (soft, probabilistic, generative — can sample new data).

---

## 🧠 Neural Network Fundamentals (Ch. 10–11)

- **Perceptron**: weighted sum + step function; can't solve XOR (linear only). **MLP**: stacked layers + nonlinear activation solves this, trained via **backpropagation** (forward pass → error → backward chain-rule gradient computation → Gradient Descent update).
- **Output layer conventions**: regression → linear/sigmoid + MSE; binary/multilabel → sigmoid + binary crossentropy; multiclass → softmax + categorical crossentropy.
- **Activations**: sigmoid/tanh (saturate, vanishing gradients) → **ReLU** (fast, can "die") → **Leaky ReLU / ELU** (fix dying ReLU) → **SELU** (self-normalizing, but only for plain sequential Dense stacks).
- **Weight init**: match to activation — **Glorot/Xavier** (tanh/sigmoid/softmax), **He** (ReLU family), **LeCun** (SELU).
- **Batch Normalization**: normalizes + learns scale/shift per layer; fixes vanishing/exploding gradients, allows higher learning rates, also regularizes.
- **Optimizers** (fastest to slowest convergence, roughly): **Adam/Nadam** (momentum + adaptive learning rate, best modern default) > RMSProp > Momentum/NAG > plain SGD. Avoid AdaGrad for deep nets (stops too early).
- **Regularization**: ℓ1/ℓ2, **Dropout** (randomly zero neurons during training only — MC Dropout keeps it on at inference for uncertainty estimates), early stopping, max-norm.
- **Transfer learning**: reuse a pretrained model's lower (general) layers, freeze first, then selectively unfreeze with a **lower learning rate**.

---

## 🖼️ Computer Vision (Ch. 14)

- **Conv layer**: local receptive field + weight sharing → feature maps; far fewer params than Dense. `padding="same"` keeps size; stride > 1 shrinks it.
- **Pooling** (esp. max pooling): shrinks feature maps, adds translation invariance — good for classification, bad for pixel-precise tasks (segmentation needs equivariance instead).
- Typical architecture: [Conv, Conv, Pool] × N (filters grow as space shrinks) → Flatten → Dense → output.
- **ResNet skip connections**: block learns the *residual* $f(x)=h(x)-x$; lets very deep (100+ layer) nets train by defaulting near the identity function at init.
- Beyond classification: **localization** (class + bbox regression) → **object detection** (YOLO/FCN, multiple objects, non-max suppression) → **semantic segmentation** (per-pixel classification).

---

## 🔁 Sequences, NLP & Attention (Ch. 15–16)

- **RNN**: feedback loop gives it memory across time steps; trained via **Backpropagation Through Time**. Suffers *more* from unstable gradients than feedforward nets (tanh > ReLU here; use Layer Norm not Batch Norm) and from **short-term memory** (info dilutes over long sequences).
- **LSTM**: long-term state ($\mathbf c$) + short-term state ($\mathbf h$) + forget/input/output gates — the standard fix for long-range dependencies. **GRU**: simpler, cheaper, comparable performance.
- **Encoder-Decoder**: seq→vector→seq (e.g., translation); **Bidirectional** encoders and **Beam search** decoding both help.
- **Attention**: decoder looks back at *all* encoder outputs, weighted by relevance — solves the "one fixed vector" bottleneck.
- **Transformer**: attention-only, no recurrence. **Self-attention** (words attend to each other in one sequence); **Scaled Dot-Product Attention** = $\text{softmax}(QK^\top/\sqrt{d_k})V$; **Multi-Head Attention** (parallel attention "angles"); **Positional encoding** (sine/cosine, injects word order); decoder uses a **causal mask** (no peeking at future tokens).
- **GPT** (decoder-only, unidirectional, zero-shot capable) vs. **BERT** (bidirectional, Masked Language Model + Next Sentence Prediction pretraining).

---

## 🎨 Generative Models (Ch. 17)

- **Autoencoder**: encoder (compress) + decoder (reconstruct), trained against its own input; undercomplete bottleneck forces useful compression (linear version ≈ PCA).
- **Denoising** (reconstruct clean from corrupted input) / **Sparse** (penalize average activation) autoencoders — both strong regularizers.
- **VAE**: encoder outputs a *distribution* (μ, σ), not a point; KL-divergence latent loss organizes latent space into a samplable Gaussian → generate new data by sampling random points and decoding.
- **GAN**: Generator (noise → fake data) vs. Discriminator (real-or-fake classifier), trained adversarially. Watch for **mode collapse** (diversity collapse) and instability; mitigations: experience replay, mini-batch discrimination. DCGAN → Progressive Growing → StyleGAN lineage.

---

## 🎮 Reinforcement Learning (Ch. 18)

- **Policy**: the agent's action-selection strategy; goal = maximize cumulative (discounted) reward.
- **Policy Gradients (REINFORCE)**: play episodes, compute discounted+normalized advantages, scale saved gradients, apply — on-policy.
- **MDP + Bellman equation** → Value/Q-Value Iteration (needs known dynamics). **Q-Learning**: learns Q-values from experience alone — off-policy; use **ε-greedy** exploration (decay ε over time).
- **Deep Q-Learning (DQN)**: neural net approximates Q-values; **replay buffer** decorrelates training data; watch **actual reward**, not loss, for progress; beware **catastrophic forgetting**.
- **Actor-Critic family** (A3C, A2C, SAC, PPO): combine policy learning with a learned value estimator for much better sample efficiency than plain PG.

---

## 🛠️ Data Engineering & Deployment (Ch. 12–13, 19)

- **`tf.GradientTape`**: records ops on `Variable`s; `.gradient()` computes exact gradients via reverse-mode autodiff in one pass.
- **`tf.data.Dataset`** pipeline order: load → preprocess (`map`) → `cache()` (if it fits in RAM) → `shuffle` → `repeat` → `batch` → `prefetch`.
- **Embeddings**: trainable dense vectors for high-cardinality categories/words — learned jointly with the network (representation learning); one-hot only suits low-cardinality categories.
- **SavedModel** → **TF Serving** (REST/gRPC) for production serving; **TFLite** (+ quantization) for mobile/embedded; **TensorFlow.js** for the browser.
- **Data parallelism** (replicate model, split data, average gradients — generally preferred) vs. **Model parallelism** (split the model itself — tricky, heavy cross-device communication, only wins for specific architectures).
- `tf.distribute.MirroredStrategy()` + `with strategy.scope():` = the easy way to multi-GPU-train a Keras model.

---

## 🔑 The Big Recurring Themes (worth internalizing above all the formulas)

1. **The bias/variance trade-off** shows up everywhere: simpler models underfit (high bias), complex models overfit (high variance) — nearly every regularization technique in this book is managing this one trade-off.
2. **Always hold out a test set, touch it once.** Use cross-validation / a validation set for every tuning decision.
3. **Scale your features** before anything Gradient-Descent-based (linear models, SVMs, neural nets) — trees are the main exception.
4. **Reuse before you retrain from scratch.** Transfer learning (Ch. 11, 14, 16) is almost always faster and better than training a big model from zero when a related pretrained model exists.
5. **Precision/recall-style trade-offs are unavoidable** — the "right" balance is a business decision, not a math decision.
6. **Loss curves and cross-validation scores are your instruments** — learning curves diagnose over/underfitting; in RL, track actual reward instead of loss.

---

*This file is a fast-recall companion — for the "why" behind any line here, open the matching chapter file in `part1_fundamentals/` or `part2_deep_learning/`.*