# Chapter 10 — Introduction to Artificial Neural Networks with Keras

## 🎯 Why this chapter matters
Welcome to Part II! Everything from here on builds on **neural networks** — the backbone of modern computer vision, NLP, and generative AI. This chapter takes you from "what is a neuron" all the way to building, training, and saving real neural nets in **Keras**, TensorFlow's beginner-friendly high-level API. This is the single most practically important chapter in the whole book for hands-on fluency.

---

## 🧠 From Biological to Artificial Neurons

Artificial Neural Networks (ANNs) were loosely inspired by the brain: billions of interconnected **biological neurons**, each a simple unit that becomes "activated" (fires) once its combined input signal crosses a threshold. Modern ANNs have drifted far from strict biological realism, but the "network of simple interconnected units" idea remains.

### The Perceptron — the simplest ANN
Built from **Threshold Logic Units (TLUs)**: a TLU computes a **weighted sum** of its inputs, then applies a **step function** to that sum:

$$
z = w_1x_1 + w_2x_2 + \dots + w_nx_n = \mathbf{x}^\top\mathbf{w}, \qquad h(\mathbf{x}) = \text{step}(z)
$$

A **Perceptron** is one layer of TLUs, each connected to every input (a **fully connected**, or **dense**, layer). A single TLU can do simple linear binary classification — just like Logistic Regression or a linear SVM.

**Training rule (Hebbian-inspired — "cells that fire together, wire together"):** for every wrongly-predicted output, nudge the weights that *would have* produced the right answer:

$$
w_{i,j}^{\text{next step}} = w_{i,j} + \eta(y_j - \hat{y}_j)x_i
$$

```python
from sklearn.linear_model import Perceptron
per_clf = Perceptron()
per_clf.fit(X, y)
```
⚠️ **Perceptrons have a serious limitation**: their decision boundary is linear, so — like Logistic Regression — they **cannot solve the XOR problem** (or anything not linearly separable). This limitation famously caused a wave of disillusionment with neural nets in the late 1960s ("AI Winter").

### The Multilayer Perceptron (MLP) — stacking fixes XOR
Stack multiple layers of TLUs — an **input layer**, one or more **hidden layers**, and an **output layer** — and suddenly the network *can* solve XOR and much more complex problems. This is the **Multilayer Perceptron**, and training it requires **backpropagation**.

### Backpropagation — how a network actually learns
In plain English: **run the network forward** to get a prediction, **measure the error**, then **work backward through the network, layer by layer, computing how much each individual weight contributed to that error** (using the calculus **chain rule** — this is what "backpropagation" refers to), and **nudge every weight a little in the direction that reduces the error** (Gradient Descent). Repeat for many batches and epochs.

This requires activation functions to be **differentiable** — which is why modern MLPs replaced the Perceptron's step function with smoother options:
- **Sigmoid** $\sigma(z) = \frac{1}{1+e^{-z}}$ — smooth, S-shaped, has a well-defined nonzero gradient everywhere (unlike the step function, whose gradient is 0 almost everywhere — useless for Gradient Descent).
- **Hyperbolic tangent (tanh)** — similar S-shape, but ranges from −1 to 1 instead of 0 to 1; often helps training converge faster since its output is centered around 0.
- **ReLU** (Rectified Linear Unit): $\text{ReLU}(z) = \max(0, z)$ — simple, fast to compute, and works very well in practice; it's the modern default for hidden layers (more on its quirks in Ch. 11).

> 🧭 **Why do we need a nonlinear activation function at all?** If every layer just applied a linear transformation, stacking many layers would mathematically collapse into one single linear transformation — no matter how "deep" the network looked, it would only ever be able to learn linear relationships. Nonlinear activation functions are what let deep stacks of layers approximate arbitrarily complex functions.

### MLP output layer conventions (memorize this table)

| Task | Output layer | Activation | Typical loss |
|---|---|---|---|
| Regression (single value) | 1 neuron | none (linear) | MSE |
| Regression (bounded, e.g. 0-1) | 1 neuron | sigmoid | MSE |
| Binary classification | 1 neuron | sigmoid | binary crossentropy |
| Multilabel binary classification | 1 neuron per label | sigmoid (independently) | binary crossentropy |
| Multiclass classification (exclusive) | 1 neuron per class | softmax | (sparse) categorical crossentropy |

---

## 💻 Implementing MLPs with Keras

Keras is TensorFlow's official high-level deep learning API (`tf.keras`) — built for speed of experimentation without sacrificing flexibility.

### Loading data
```python
from tensorflow import keras

fashion_mnist = keras.datasets.fashion_mnist
(X_train_full, y_train_full), (X_test, y_test) = fashion_mnist.load_data()

# Carve out a validation set, and scale pixel values to [0, 1] (neural nets train much
# better on small, well-scaled inputs — this matters even more here than in Part I)
X_valid, X_train = X_train_full[:5000] / 255.0, X_train_full[5000:] / 255.0
y_valid, y_train = y_train_full[:5000], y_train_full[5000:]
X_test = X_test / 255.0

class_names = ["T-shirt/top", "Trouser", "Pullover", "Dress", "Coat",
               "Sandal", "Shirt", "Sneaker", "Bag", "Ankle boot"]
```

### Building a model — the Sequential API (simplest, most common)
```python
model = keras.models.Sequential([
    keras.layers.Flatten(input_shape=[28, 28]),   # 28x28 image -> flat 784-value vector
    keras.layers.Dense(300, activation="relu"),
    keras.layers.Dense(100, activation="relu"),
    keras.layers.Dense(10, activation="softmax"), # 10 classes, mutually exclusive
])

model.summary()   # see every layer, its output shape, and parameter count
```
`model.summary()` is invaluable for sanity-checking your architecture — e.g., that first `Dense(300)` layer alone has $784 \times 300 + 300 = 235{,}500$ parameters (weights + biases)! Lots of capacity to fit data, but also lots of capacity to **overfit** if you don't have enough training data.

### Compiling — specify how the model should learn
```python
model.compile(loss="sparse_categorical_crossentropy",
              optimizer="sgd",
              metrics=["accuracy"])
```
- **`loss`**: `"sparse_categorical_crossentropy"` when labels are plain integer class indices (0–9); use `"categorical_crossentropy"` instead if your labels are already one-hot vectors; use `"binary_crossentropy"` for binary/multilabel classification (with `sigmoid` output).
- **`optimizer`**: `"sgd"` is plain Stochastic Gradient Descent (Keras runs backprop = reverse-mode autodiff + Gradient Descent under the hood). Chapter 11 covers much better optimizers.
- **`metrics`**: extra numbers to track (not used for training itself, just for you to monitor).

### Training
```python
history = model.fit(X_train, y_train, epochs=30,
                     validation_data=(X_valid, y_valid))
```
`fit()` returns a `History` object — `history.history` is a dict of loss/metric values per epoch, perfect for plotting learning curves:
```python
import pandas as pd
pd.DataFrame(history.history).plot(figsize=(8, 5))
```
💡 If training/validation curves are close together and both improving → good sign, not much overfitting. If training loss keeps dropping but validation loss climbs → overfitting; consider more data, simpler model, or regularization (Ch. 11).

### Evaluating and predicting
```python
model.evaluate(X_test, y_test)          # final generalization estimate — use once, at the end

y_proba = model.predict(X_new)          # array of class probabilities per instance
y_pred = y_proba.argmax(axis=-1)        # the single most likely class per instance
```
*(Note: older Keras versions had a `model.predict_classes()` shortcut for this — it's been removed in modern TensorFlow, so use `np.argmax(model.predict(X_new), axis=-1)` instead.)*

### Regression MLPs
Same idea, different output layer and loss:
```python
model = keras.models.Sequential([
    keras.layers.Dense(30, activation="relu", input_shape=X_train.shape[1:]),
    keras.layers.Dense(30, activation="relu"),
    keras.layers.Dense(1),                       # single output, no activation (unbounded regression target)
])
model.compile(loss="mean_squared_error", optimizer="sgd")
```

---

## 🏗️ Beyond simple stacks — the Functional API

Not every good architecture is a straight stack of layers. The **Wide & Deep** architecture, for example, connects raw inputs *directly* to the output (the "wide" path, good at memorizing simple rules) **alongside** a deep stack of hidden layers (the "deep" path, good at learning complex patterns) — both feeding into the final prediction:

```python
input_ = keras.layers.Input(shape=X_train.shape[1:])
hidden1 = keras.layers.Dense(30, activation="relu")(input_)
hidden2 = keras.layers.Dense(30, activation="relu")(hidden1)
concat = keras.layers.Concatenate()([input_, hidden2])   # skip-connection: raw input + deep features
output = keras.layers.Dense(1)(concat)
model = keras.Model(inputs=[input_], outputs=[output])
```
Notice the style: you call each layer **like a function**, passing it its input — this is why it's the "Functional API." It's fully flexible: multiple inputs, multiple outputs, shared layers, skip connections — all possible.

**Multiple outputs** are useful for multi-task learning, or as a **regularization trick**: add an "auxiliary output" earlier in the network, forcing that part of the network to learn something useful on its own, not just as a stepping stone for later layers.
```python
output = keras.layers.Dense(1, name="main_output")(concat)
aux_output = keras.layers.Dense(1, name="aux_output")(hidden2)
model = keras.Model(inputs=[input_A, input_B], outputs=[output, aux_output])
model.compile(loss=["mse", "mse"], loss_weights=[0.9, 0.1], optimizer="sgd")   # care more about the main output
```

### The Subclassing API — full imperative flexibility
For architectures with loops, conditionals, or genuinely dynamic behavior (research-y stuff), subclass `keras.Model` directly:
```python
class WideAndDeepModel(keras.Model):
    def __init__(self, units=30, activation="relu", **kwargs):
        super().__init__(**kwargs)
        self.hidden1 = keras.layers.Dense(units, activation=activation)
        self.hidden2 = keras.layers.Dense(units, activation=activation)
        self.main_output = keras.layers.Dense(1)

    def call(self, inputs):
        input_A, input_B = inputs
        hidden1 = self.hidden1(input_B)
        hidden2 = self.hidden2(hidden1)
        concat = keras.layers.concatenate([input_A, hidden2])
        return self.main_output(concat)
```
⚠️ **Trade-off**: this flexibility costs you Keras's ability to inspect, save, clone, or plot the model's structure automatically — the architecture is hidden inside your `call()` method's Python code. **Default to Sequential or Functional API** unless you specifically need the dynamism.

---

## 💾 Saving, Callbacks, and TensorBoard

### Saving/loading
```python
model.save("my_keras_model.h5")            # (Sequential/Functional models only — full architecture + weights + optimizer state)
model = keras.models.load_model("my_keras_model.h5")
model.save_weights(...) / model.load_weights(...)   # weights only
```

### Callbacks — hook into the training loop
```python
checkpoint_cb = keras.callbacks.ModelCheckpoint("my_keras_model.h5", save_best_only=True)
early_stopping_cb = keras.callbacks.EarlyStopping(patience=10, restore_best_weights=True)

history = model.fit(X_train, y_train, epochs=100,
                     validation_data=(X_valid, y_valid),
                     callbacks=[checkpoint_cb, early_stopping_cb])
```
- **`ModelCheckpoint(save_best_only=True)`**: only overwrites the saved file when validation performance improves — a safety net against crashes *and* overfitting.
- **`EarlyStopping(patience=N, restore_best_weights=True)`**: automatically stops training after `N` epochs with no validation improvement, and rolls back to the best weights seen — this is Chapter 4's "early stopping" idea, essentially free. You can set `epochs` generously high and let this callback decide when to actually stop.
- You can write **custom callbacks** by subclassing `keras.callbacks.Callback` and implementing hooks like `on_epoch_end(self, epoch, logs)`.

### TensorBoard — visualize training
```python
tensorboard_cb = keras.callbacks.TensorBoard(log_dir="./my_logs")
model.fit(X_train, y_train, epochs=30, callbacks=[tensorboard_cb])
# then in a terminal: tensorboard --logdir=./my_logs
```
Gives you interactive learning-curve plots, graph visualization, and more — indispensable once you're running many experiments and want to compare them.

---

## 🎛️ Fine-Tuning Neural Network Hyperparameters

Neural nets have *many* hyperparameters — number of layers, neurons per layer, activation functions, weight initialization, learning rate, optimizer, batch size, and more. You can wrap a Keras model in `KerasClassifier`/`KerasRegressor` and use Scikit-Learn's `GridSearchCV` or `RandomizedSearchCV` from Chapter 2, or use purpose-built libraries like **Keras Tuner**, **Hyperopt**, or **Optuna** for smarter, more efficient search strategies.

**Practical rules of thumb:**

- **Number of hidden layers**: a single hidden layer can theoretically model even very complex functions if it has enough neurons (per the *Universal Approximation Theorem*), but in practice **deep networks (many layers, each relatively narrow) generalize better and train more efficiently** than one giant shallow layer — deeper networks can build up a **hierarchy of features** (early layers learn simple patterns like edges, later layers combine them into complex ones, like a face), which also transfers well to related tasks (**transfer learning**, Ch. 11). Start with 1–2 hidden layers for simple problems; ramp up depth as needed, ideally reusing parts of a pretrained network for complex tasks (images, speech) rather than training very deep nets from scratch.
- **Number of neurons per layer**: for a long time, people shrunk layer sizes progressively (a "funnel" shape) assuming this mimicked useful compression — in practice, using **the same size for every hidden layer** performs comparably or better and simplifies tuning to one number. A common strategy: pick a network noticeably larger than you need, then apply early stopping and other regularization (Ch. 11) to prevent it from overfitting — this is often more effective than painstakingly tuning the exact layer sizes ("the stretch pants approach").
- **Learning rate**: arguably **the single most important hyperparameter**. A good approach: train the model for a few hundred iterations while exponentially increasing the learning rate, plot loss vs. learning rate, and pick a rate a bit below where the loss started climbing sharply.
- **Optimizer**: plain SGD is rarely the best choice in practice — Chapter 11 covers better options (Momentum, Adam, etc.).
- **Batch size**: large batches (using hardware acceleration well) can speed up training but sometimes generalize worse; small batches train more slowly but sometimes generalize better. A common compromise is to try a large batch size with a **learning rate warmup**; if training is unstable, fall back to a smaller batch.
- **Activation function**: **ReLU is a good default** for hidden layers (fast, works well broadly); output layer activation is dictated by the task (see the table above).
- **Number of iterations (epochs)**: in most cases you don't need to tune this directly — just set it high and use **early stopping**.

---

## ⚠️ Common Pitfalls

- Forgetting to **scale input features** before training a neural net — Gradient Descent-based training is very sensitive to this (even more so than Chapter 4's linear models).
- Not specifying `input_shape` on the first layer, then being confused when `.summary()` or `.save()` doesn't work yet — Keras defers building the model until it knows the input shape.
- Comparing training loss (a running average *during* an epoch) directly against validation loss (computed *after* the full epoch) without accounting for that half-epoch offset — can make it look like validation is doing better early in training when it isn't really.
- Reaching for the Subclassing API "just in case flexibility is needed" — you lose easy saving/inspection/plotting for no benefit if a Sequential/Functional model would've done the job.
- Setting `epochs` too low and never noticing training hadn't converged, or too high with no `EarlyStopping` and wasting compute/overfitting.
- Forgetting `save_best_only=True` on `ModelCheckpoint`, ending up with the *last* (possibly overfit) epoch saved instead of the *best* one.

---

## 🔑 Cheat Sheet

- **TLU/Perceptron**: weighted sum + step function; single layer, linear boundary only, can't solve XOR.
- **MLP**: input layer + hidden layer(s) + output layer, trained via **backpropagation** (forward pass → measure error → backward pass computing gradients via chain rule → Gradient Descent update).
- Nonlinear activations (sigmoid, tanh, **ReLU**) are essential — without them, stacked layers collapse into one linear function.
- Output layer convention: regression → linear (or sigmoid if bounded) + MSE; binary/multilabel → sigmoid + binary crossentropy; multiclass exclusive → softmax + (sparse) categorical crossentropy.
- **Sequential API**: simplest, linear stack of layers. **Functional API**: flexible DAGs (multi-input/output, skip connections). **Subclassing API**: full imperative control, at the cost of introspectability.
- Always scale input features. `model.summary()` to sanity-check parameter counts.
- `ModelCheckpoint(save_best_only=True)` + `EarlyStopping(restore_best_weights=True)` = a nearly-free way to avoid overfitting and wasted training time.
- Learning rate is the most important hyperparameter to tune; prefer many same-sized layers + early stopping over painstakingly hand-tuned architecture sizes.

---

## ✍️ Practice

1. **Hands-on:** Build the Fashion MNIST Sequential model above. Train for 30 epochs with a validation set. Plot the learning curves. Is the model overfitting, underfitting, or about right?
2. **Conceptual:** Explain in your own words why a neural network with *no* activation functions (or only linear ones) can't do anything a single-layer linear model couldn't already do, no matter how many layers you stack.
3. **Hands-on:** Modify the Fashion MNIST model to have 4 hidden layers of 100 neurons each (all ReLU). Does it outperform the original 2-hidden-layer model on the validation set? Try adding `EarlyStopping` — how many epochs did it actually take before stopping?
4. **Debug-the-scenario:** Your regression MLP's loss is `nan` after a few batches of training. List two likely causes and how you'd check for them.
   <details><summary>Answer</summary>Learning rate too high (diverging weights) — try a much smaller learning rate. Unscaled input features with large values, producing huge activations/gradients — scale inputs. (Ch. 11 also covers "exploding gradients" as a deeper cause in very deep networks.)</details>
5. **Hands-on:** Build a Wide & Deep model (Functional API) for the California housing regression task. Compare its validation MSE against a plain 2-hidden-layer Sequential model.

Next: `11_training_deep_neural_nets.md` — deeper networks bring new problems (vanishing gradients, painfully slow training, overfitting) and this chapter's toolbox of fixes.