# Chapter 11 — Training Deep Neural Networks

## 🎯 Why this chapter matters
Chapter 10 got a small network training. But **deep** networks (many layers) bring new problems: gradients that vanish or explode, training that takes forever, and models with millions of parameters that overfit easily. This chapter is your **complete toolbox** for training deep networks that actually work — and it ends with a practical "just use this" cheat sheet you'll come back to constantly.

---

## 📉 The Vanishing/Exploding Gradients Problem

Backpropagation propagates error gradients from the output layer back to the input layer. In deep networks, these gradients often **get smaller and smaller** as they travel backward — so early (lower) layers barely update at all, and training stalls. This is the **vanishing gradients problem**. Sometimes the opposite happens — gradients **grow explosively** — and training diverges (the **exploding gradients problem**, especially common in RNNs, Ch. 15).

**Root cause (Glorot & Bengio, 2010):** the old default — sigmoid activation + naive random weight initialization — causes each layer's output variance to be *larger* than its input variance. Stack enough layers, and by the top layers, neurons **saturate** (sigmoid squashes everything toward 0 or 1), where the gradient is essentially **zero** — nothing left to propagate backward.

### Fix 1 — Better weight initialization (Glorot/Xavier & He initialization)
The idea: keep each layer's **output variance ≈ its input variance**, in both the forward pass and the backward (gradient) pass. Different activation functions need different initialization scale:

| Initialization | Best for | Variance (normal dist.) |
|---|---|---|
| **Glorot (Xavier)** | tanh, sigmoid/logistic, softmax, none | $1/\text{fan}_{\text{avg}}$ |
| **He** | ReLU and variants | $2/\text{fan}_{\text{in}}$ |
| **LeCun** | SELU | $1/\text{fan}_{\text{in}}$ |

*(“fan-in”/“fan-out” = number of inputs/outputs of the layer.)* Keras defaults to Glorot with a uniform distribution; switch to He for ReLU-family activations:
```python
keras.layers.Dense(10, activation="relu", kernel_initializer="he_normal")
```

### Fix 2 — Better (nonsaturating) activation functions

- **ReLU**: $\max(0, z)$ — fast, doesn't saturate for positive inputs. ⚠️ **Dying ReLU problem**: if a neuron's weights end up producing negative output for *every* training instance, its gradient is permanently 0 — it "dies" and never updates again (common with large learning rates).
- **Leaky ReLU**: $\max(\alpha z, z)$, with small $\alpha$ (e.g. 0.01–0.2) — the small negative slope means neurons never fully die, just go into a "coma" they can wake up from. Generally outperforms plain ReLU.
- **ELU** (Exponential Linear Unit): smooth, negative values for $z<0$ (mean output closer to 0, which helps counter vanishing gradients further), nonzero gradient everywhere — often trains faster and generalizes better than ReLU variants, but is slower to *compute* (uses `exp`).
- **SELU** (Scaled ELU): if a network is **a plain sequential stack of Dense layers**, uses **LeCun normal initialization**, and has **standardized inputs**, then using SELU everywhere makes the network **self-normalize** — each layer's output automatically preserves mean 0/std 1 — powerfully solving vanishing/exploding gradients. ⚠️ Breaks down with non-sequential architectures (skip connections, RNNs) — self-normalization isn't guaranteed there.

> 🔑 **Rule of thumb ranking (usually, YMMV): SELU > ELU > Leaky ReLU (and variants) > ReLU > tanh > sigmoid.** If runtime speed matters most, plain ReLU is still very reasonable (it's the most hardware-optimized).

```python
model = keras.models.Sequential([
    keras.layers.Dense(10, kernel_initializer="he_normal"),
    keras.layers.LeakyReLU(alpha=0.2),          # apply as its own layer after Dense
    # ... or for SELU:
    keras.layers.Dense(10, activation="selu", kernel_initializer="lecun_normal"),
])
```

### Fix 3 — Batch Normalization (BN)
Add a **BatchNormalization** layer before/after a hidden layer's activation. It **zero-centers and normalizes** each input using the current mini-batch's mean/std, then **learns** a scale ($\gamma$) and shift ($\beta$) to undo that normalization if useful:

$$
\hat{x}^{(i)} = \frac{x^{(i)} - \mu_B}{\sqrt{\sigma_B^2 + \varepsilon}}, \qquad z^{(i)} = \gamma \otimes \hat{x}^{(i)} + \beta
$$

At **test time**, there's no "batch" to compute stats from, so Keras uses a **moving average** of $\mu$/$\sigma$ collected during training instead.

```python
model = keras.models.Sequential([
    keras.layers.Flatten(input_shape=[28, 28]),
    keras.layers.BatchNormalization(),
    keras.layers.Dense(300, activation="elu", kernel_initializer="he_normal"),
    keras.layers.BatchNormalization(),
    keras.layers.Dense(100, activation="elu", kernel_initializer="he_normal"),
    keras.layers.BatchNormalization(),
    keras.layers.Dense(10, activation="softmax"),
])
```
**Why it's so effective:** massively reduces vanishing/exploding gradients (even letting you use saturating activations again if you want), makes the network far less sensitive to weight initialization, lets you use much larger learning rates, and **acts as a regularizer** (reducing the need for other techniques like dropout) — but adds a bit of runtime cost per layer and a new hyperparameter set (though defaults work well). Networks with BN often need **more time per epoch but far fewer epochs overall** — usually a net win in wall-clock time.

### Fix 4 — Gradient Clipping
Simply **cap the gradient** during backprop so it can never exceed some threshold — common in RNNs (where BN is awkward to apply):
```python
optimizer = keras.optimizers.SGD(clipvalue=1.0)   # clip each gradient component to [-1, 1]
# or:
optimizer = keras.optimizers.SGD(clipnorm=1.0)    # clip the whole gradient vector by its norm (preserves direction)
```

---

## ♻️ Reusing Pretrained Layers — Transfer Learning

**Don't train a big network from scratch if a similar problem's pretrained network already exists** — reuse its lower layers (which tend to learn general, broadly-useful features like edges/textures), and replace/retrain the top ones (which tend to be task-specific).

**Practical recipe:**
1. Remove the original output layer (wrong number of classes for your task); add a new one.
2. **Freeze** the reused layers first (`layer.trainable = False`) — train just the new top layer(s) and see how it does.
3. Try **unfreezing** the top one or two reused layers and continuing training (with a **lower learning rate**, to avoid wrecking their already-good weights) — more unfreezing tends to help more as *your* training data grows.
4. The **more similar** the original and new tasks are, the more layers you should reuse.

```python
model_A = keras.models.load_model("my_model_A.h5")
model_B_on_A = keras.models.Sequential(model_A.layers[:-1])   # reuse all but the output layer
model_B_on_A.add(keras.layers.Dense(1, activation="sigmoid")) # new output layer for the new task

for layer in model_B_on_A.layers[:-1]:
    layer.trainable = False    # freeze reused layers
model_B_on_A.compile(loss="binary_crossentropy", optimizer="sgd", metrics=["accuracy"])
# ... train a few epochs, then unfreeze and fine-tune with a lower learning rate
```
⚠️ Always **clone** the original model (`keras.models.clone_model`) before reusing its layers if you still need the original model intact — otherwise, changes to the shared layers affect both.

**No similar pretrained model available?** Two alternatives:
- **Unsupervised pretraining**: if you have lots of *unlabeled* data (common!) but little labeled data, train an unsupervised model (e.g., an autoencoder, Ch. 17) on the unlabeled data first, reuse its lower layers, then fine-tune on your small labeled set.
- **Pretraining on an auxiliary task**: train a first network on an easier, related, but differently-labeled task where labeled data is abundant/cheap (or even synthetically generated), then reuse its lower layers for your real (data-scarce) task.

---

## 🚀 Faster Optimizers

Plain (mini-batch) SGD is rarely the fastest choice. All of the following improve on it:

| Optimizer | Core idea | Keras |
|---|---|---|
| **Momentum** | Accumulate a "velocity" from past gradients (like a bowling ball rolling downhill) instead of reacting only to the current gradient; hyperparameter $\beta$ (typically 0.9) acts like friction. Can end up ~10× faster than plain SGD. | `SGD(momentum=0.9)` |
| **Nesterov Accelerated Gradient (NAG)** | Momentum, but measures the gradient slightly *ahead*, in the direction the momentum is already pushing — usually a bit faster than plain momentum. | `SGD(momentum=0.9, nesterov=True)` |
| **AdaGrad** | Scales down the gradient more aggressively along steep dimensions — automatically adapts the learning rate per-parameter. ⚠️ Tends to stop too early on neural nets (learning rate decays too much) — **avoid for deep nets**, but fine for simpler models like Linear Regression. | `Adagrad(...)` |
| **RMSProp** | Fixes AdaGrad's "stops too early" flaw by only accumulating *recent* gradients (exponential decay, $\beta \approx 0.9$) instead of all of history. Reliable, was the go-to before Adam. | `RMSprop(rho=0.9)` |
| **Adam** ("adaptive moment estimation") | **Momentum + RMSProp combined**: tracks a decaying average of past gradients *and* past squared gradients. Needs little learning-rate tuning (default 0.001 usually works). The most popular modern default. | `Adam(beta_1=0.9, beta_2=0.999)` |
| **Nadam** | Adam + the Nesterov trick — often slightly faster than Adam. | `Nadam(...)` |
| **AdaMax** | A variant of Adam using the $\ell_\infty$ norm instead of $\ell_2$ — sometimes more stable, usually Adam is still better. | `Adamax(...)` |

⚠️ **Caveat:** a 2017 paper (Wilson et al.) showed adaptive methods (RMSProp/Adam/Nadam) can sometimes **generalize worse** than plain SGD+Nesterov Momentum on certain datasets — if your model underperforms expectations, it's worth trying plain Momentum/NAG as an alternative.

### Learning Rate Scheduling
A **constant** learning rate is rarely ideal — too high and training diverges or bounces; too low and it converges painfully slowly, or gets stuck near (but not at) a good solution. **Schedules** reduce the learning rate over training:
- **Power scheduling**: LR drops as a power-law function of the iteration number.
- **Exponential scheduling**: LR multiplies by a constant factor every few epochs.
- **Piecewise constant scheduling**: use a fixed LR for N epochs, then a smaller fixed LR for the next N, etc.
- **Performance scheduling**: reduce LR whenever the validation error stops improving (`ReduceLROnPlateau` in Keras).
- **1cycle scheduling**: start low, ramp *up* to a large LR partway through training, then ramp back down even lower than the start — often trains faster and generalizes better than any monotonic schedule (recommended as a strong default).

```python
def exponential_decay(lr0, s):
    def fn(epoch):
        return lr0 * 0.1 ** (epoch / s)
    return fn

lr_scheduler = keras.callbacks.LearningRateScheduler(exponential_decay(lr0=0.01, s=20))
model.fit(X_train, y_train, epochs=25, callbacks=[lr_scheduler])
```

---

## 🛡️ Avoiding Overfitting Through Regularization

Deep nets often have **thousands to millions of parameters** — extreme flexibility means extreme overfitting risk without countermeasures. Chapter 4's ℓ1/ℓ2 regularization and early stopping still apply directly (`keras.regularizers.l2(0.01)` on a layer's `kernel_regularizer`), plus two deep-learning-specific favorites:

### Dropout — the most popular DNN regularizer
At **every training step**, each neuron (except output neurons) has probability $p$ (the **dropout rate**, typically 0.1–0.5) of being **temporarily entirely ignored** for that step — it might come back next step. After training, dropout does nothing (full network is used, with inputs scaled to compensate).

```python
model = keras.models.Sequential([
    keras.layers.Flatten(input_shape=[28, 28]),
    keras.layers.Dropout(rate=0.2),
    keras.layers.Dense(300, activation="elu", kernel_initializer="he_normal"),
    keras.layers.Dropout(rate=0.2),
    keras.layers.Dense(100, activation="elu", kernel_initializer="he_normal"),
    keras.layers.Dropout(rate=0.2),
    keras.layers.Dense(10, activation="softmax"),
])
```
**Why does randomly deleting neurons help?** Neurons can't "co-adapt" (over-rely on specific neighbors) — each must become independently useful, forcing more robust, redundant, spread-out representations. It's mathematically similar to training a huge ensemble of many different "thinned" sub-networks and averaging them, all sharing weights.

⚠️ Dropout only applies during training — so training loss (measured *with* dropout active) can look artificially worse than validation loss (measured *without* it) during training; don't panic if that flips your expectations. If underfitting, **reduce** the dropout rate (or apply it to fewer layers); if overfitting, increase it.

### Monte Carlo (MC) Dropout — a clever bonus, no retraining needed
Run the *same trained* dropout model multiple times at prediction time, with dropout still **active** (instead of off, as normal), then average the predictions — this gives you both a better point estimate and a genuine **uncertainty estimate** (how much the predictions vary across runs), useful for risk-sensitive applications.

### Max-Norm Regularization
After each training step, rescale each neuron's incoming weight vector so its norm never exceeds a chosen max value — constrains weights without adding a term to the loss function itself (implemented via a Keras constraint, e.g. `kernel_constraint=keras.constraints.max_norm(1.0)`).

---

## 📋 Summary and Practical Guidelines

**Default configuration that works well in most cases (start here):**

| Hyperparameter | Default |
|---|---|
| Kernel initializer | He initialization |
| Activation function | ELU (or leaky ReLU for speed) |
| Normalization | None if shallow; **Batch Norm** if deep |
| Regularization | Early stopping (+ ℓ2 if needed) |
| Optimizer | Momentum (or RMSProp / Nadam) |
| Learning rate schedule | 1cycle |

**If your network is a plain stack of Dense layers (fully sequential, no skip connections), consider the self-normalizing configuration instead:**

| Hyperparameter | Default |
|---|---|
| Kernel initializer | LeCun initialization |
| Activation function | SELU |
| Normalization | None (self-normalizes) |
| Regularization | Alpha dropout, if needed |
| Optimizer | Momentum (or RMSProp / Nadam) |
| Learning rate schedule | 1cycle |

**Always:** normalize your input features. Reuse a pretrained model if one exists for a similar task. **Exceptions:** for a sparse model, use ℓ1 regularization; for low-latency inference, use fewer layers, fold BN into the preceding layer, prefer ReLU/leaky ReLU, and consider lower numeric precision (16/8-bit, Ch. 19); for risk-sensitive applications where inference speed matters less, consider MC Dropout.

---

## ⚠️ Common Pitfalls

- Using sigmoid/tanh throughout a deep network with naive initialization — classic recipe for vanishing gradients.
- Setting a high learning rate with ReLU and being surprised when many neurons "die" (permanently output 0).
- Forgetting that Batch Normalization behaves **differently at train vs. test time** — Keras handles this correctly automatically, but it's essential to understand when debugging.
- Comparing training loss to validation loss while dropout is active — training loss is measured under a "handicapped" network and can look artificially worse.
- Fine-tuning *all* reused pretrained layers with a normal (not reduced) learning rate — risks destroying useful pretrained weights.
- Using AdaGrad for deep neural networks — it tends to stall out before reaching a good solution.
- Treating a fixed learning rate as "good enough" — a schedule (or Adam-style adaptivity) very often trains faster and better.

---

## 🔑 Cheat Sheet

- **Vanishing/exploding gradients**: fixed by good initialization (Glorot/He/LeCun), nonsaturating activations (ReLU family, ELU, SELU), Batch Normalization, and Gradient Clipping (mainly RNNs).
- **Batch Norm**: normalizes + learns scale/shift per layer; also regularizes; uses batch stats at train time, moving-average stats at test time.
- **Transfer learning**: reuse lower layers of a similar pretrained model; freeze first, then selectively unfreeze with a lower learning rate.
- **Faster optimizers**: Momentum/NAG > plain SGD; Adam/Nadam/RMSProp adapt the learning rate automatically and are strong modern defaults; avoid AdaGrad for deep nets.
- **Learning rate schedules** (especially 1cycle) usually beat a constant learning rate.
- **Dropout**: randomly zero out neurons during training only; strong, popular regularizer; MC Dropout gives uncertainty estimates at inference time for free (by keeping dropout active and averaging repeated predictions).
- Default recipe: He init + ELU/leaky ReLU + Batch Norm (if deep) + early stopping + Momentum/Nadam + 1cycle schedule.

---

## ✍️ Practice

1. **Hands-on:** Take the Fashion MNIST model from Chapter 10. Add `BatchNormalization` after each `Dense` layer and switch the optimizer to `Nadam`. Compare convergence speed (epochs to reach 90% validation accuracy) against the original.
2. **Conceptual:** Explain, in your own words, why "dying ReLU" happens, and why Leaky ReLU fixes it.
3. **Hands-on:** Add `Dropout(0.3)` before each hidden layer in the same model. Does validation accuracy improve? Does training take more epochs to reach a comparable accuracy?
4. **Debug-the-scenario:** You unfreeze all layers of a pretrained model and fine-tune on a small new dataset (500 images) using the same learning rate as pretraining. Validation accuracy is *worse* than when everything was frozen. What likely went wrong, and what two things would you try?
   <details><summary>Answer</summary>Fine-tuning with too high a learning rate on too little data has likely destroyed the useful pretrained weights ("catastrophic forgetting"). Try: (1) freeze more layers and only unfreeze the top one or two, (2) use a much smaller learning rate when unfreezing.</details>
5. **Conceptual:** Why does SELU require the network to be a plain sequential stack of Dense layers to guarantee self-normalization? What breaks that guarantee?

Next: `12_custom_models_tensorflow.md` — go beneath Keras's high-level API to write custom loss functions, metrics, and training loops with raw TensorFlow.