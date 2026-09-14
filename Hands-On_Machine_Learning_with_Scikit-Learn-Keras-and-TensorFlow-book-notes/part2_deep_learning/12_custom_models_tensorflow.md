# Chapter 12 — Custom Models and Training with TensorFlow

## 🎯 Why this chapter matters
Keras's high-level API (Ch. 10–11) covers 95% of use cases. But when you need a **custom loss function**, an **exotic layer**, or a **training loop that does something `fit()` can't**, you need to drop to TensorFlow's lower-level API. This chapter shows you TensorFlow's NumPy-like core, and how to customize every piece of the training process — while still preferring Keras's convenience whenever you can.

---

## 🧮 A Quick Tour of TensorFlow — using it like NumPy

At its core, TensorFlow is a numerical computing library, very similar in spirit to NumPy, but with GPU/TPU support and automatic differentiation built in.

```python
import tensorflow as tf

t = tf.constant([[1., 2., 3.], [4., 5., 6.]])   # a tensor, like a NumPy array
t.shape       # TensorShape([2, 3])
t.dtype       # tf.float32

t + 10                       # element-wise ops work like NumPy
tf.square(t)
t @ tf.transpose(t)          # @ = matrix multiplication
```
Most NumPy operations have a TensorFlow equivalent, sometimes under a different name — e.g., `tf.reduce_mean()`/`tf.reduce_sum()`/`tf.reduce_max()` instead of `np.mean()`/`np.sum()`/`np.max()`. Tensors and NumPy arrays interoperate freely (`tf.constant(numpy_array)`, `tensor.numpy()`).

⚠️ **Tensors created with `tf.constant()` are immutable** — you can't modify them in-place. For values that need to change during training (like model weights), use `tf.Variable`:
```python
v = tf.Variable([[1., 2., 3.], [4., 5., 6.]])
v.assign(2 * v)              # in-place update
v[0, 1].assign(42)
```
*(You'll rarely create `tf.Variable`s directly — Keras layers manage their own weight variables for you. This matters when you write a custom training loop, below.)*

**Other useful data types**: TensorFlow also has first-class support for **string tensors** (`tf.string`), **ragged tensors** (variable-length sequences), **sparse tensors**, and **sets** — worth knowing they exist for specialized data.

---

## 🎨 Custom Loss Functions

Suppose the built-in losses don't fit your problem — e.g., you want the **Huber loss** (quadratic for small errors, like MSE, but linear for large errors, like MAE — great for noisy data with outliers), and pretend it's not already built in:

```python
def huber_fn(y_true, y_pred):
    error = y_true - y_pred
    is_small_error = tf.abs(error) < 1
    squared_loss = tf.square(error) / 2
    linear_loss = tf.abs(error) - 0.5
    return tf.where(is_small_error, squared_loss, linear_loss)   # per-instance loss

model.compile(loss=huber_fn, optimizer="nadam")
```
⚠️ **Return one loss per instance, not the mean** — Keras handles averaging (and applying class/sample weights) for you. Use `tf.where` for vectorized "if/else" logic and only TensorFlow ops throughout, so the function benefits from graph compilation (below).

**Saving/loading models with custom components**: Keras saves the loss *function's name*, not its code — so when loading, you must tell Keras what that name refers to:
```python
model = keras.models.load_model("my_model.h5", custom_objects={"huber_fn": huber_fn})
```

**Need a *configurable* custom loss** (e.g., a tunable threshold)? Wrap it as a class extending `keras.losses.Loss`, implementing `get_config()` so its hyperparameters get saved/loaded automatically:
```python
class HuberLoss(keras.losses.Loss):
    def __init__(self, threshold=1.0, **kwargs):
        self.threshold = threshold
        super().__init__(**kwargs)

    def call(self, y_true, y_pred):
        error = y_true - y_pred
        is_small_error = tf.abs(error) < self.threshold
        squared_loss = tf.square(error) / 2
        linear_loss = self.threshold * tf.abs(error) - self.threshold ** 2 / 2
        return tf.where(is_small_error, squared_loss, linear_loss)

    def get_config(self):
        base_config = super().get_config()
        return {**base_config, "threshold": self.threshold}
```
The same class-based pattern (subclassing a base class, implementing `get_config()`) works for **custom activation functions, initializers, regularizers, constraints, and metrics** too.

### A subtlety with custom metrics: streaming vs. per-batch
A metric like accuracy should usually be tracked as a **running average across the whole epoch**, not recomputed fresh each batch — this is what a **streaming metric** (subclassing `keras.metrics.Metric`) does, maintaining internal state (`add_weight()`) that accumulates via `update_state()` and reports via `result()`, with `reset_states()` clearing it at epoch boundaries. Keras's built-in metrics (like `Precision`) already work this way.

---

## 🧱 Custom Layers

**No weights needed?** Wrap a plain function in a `Lambda` layer:
```python
exponential_layer = keras.layers.Lambda(lambda x: tf.exp(x))
```
**Need weights (a "stateful" layer)?** Subclass `keras.layers.Layer`:
```python
class MyDense(keras.layers.Layer):
    def __init__(self, units, activation=None, **kwargs):
        super().__init__(**kwargs)
        self.units = units
        self.activation = keras.activations.get(activation)

    def build(self, batch_input_shape):
        self.kernel = self.add_weight(
            name="kernel", shape=[batch_input_shape[-1], self.units],
            initializer="glorot_normal")
        self.bias = self.add_weight(name="bias", shape=[self.units], initializer="zeros")
        super().build(batch_input_shape)     # must be called at the end

    def call(self, X):
        return self.activation(X @ self.kernel + self.bias)

    def get_config(self):
        base_config = super().get_config()
        return {**base_config, "units": self.units,
                "activation": keras.activations.serialize(self.activation)}
```
- **`__init__`**: store hyperparameters.
- **`build()`**: create weights via `self.add_weight(...)` — called automatically the first time the layer sees real input, once Keras knows the input shape.
- **`call()`**: the actual forward computation.
- **`get_config()`**: needed for saving/loading, same pattern as custom losses.

**Repetitive architectures**: if a block of layers (A→B→C) repeats many times, wrap it as a single custom layer (or a small `keras.Model` subclass) so your top-level model reads as a clean `D, D, D` instead of a long repeated stack.

---

## ⚙️ Computing Gradients Using Autodiff — `tf.GradientTape`

This is the engine under Keras's `fit()`. **Reverse-mode automatic differentiation** computes exact gradients of any differentiable function efficiently — one forward pass, one backward pass, gradients for *all* parameters at once, regardless of how many there are (compare this to numerically approximating each partial derivative one at a time, which would require re-running the function once per parameter — intractable for a network with millions of weights).

```python
def f(w1, w2):
    return 3 * w1 ** 2 + 2 * w1 * w2

w1, w2 = tf.Variable(5.), tf.Variable(3.)
with tf.GradientTape() as tape:
    z = f(w1, w2)

gradients = tape.gradient(z, [w1, w2])   # [36.0, 10.0] — exact, not approximated
```
- The tape **records every operation involving a `Variable`** inside its `with` block.
- ⚠️ **The tape is erased after one call to `.gradient()`** — for multiple calls, use `tf.GradientTape(persistent=True)` and manually `del tape` when done.
- By default the tape only tracks `Variable`s, not `constant`s — call `tape.watch(tensor)` to track an arbitrary tensor too (useful for gradients with respect to *inputs*, e.g. for certain regularization tricks or interpretability techniques).
- `tf.stop_gradient(x)` passes `x` through unchanged on the forward pass, but blocks gradients from flowing back through it — useful for freezing part of a computation.

---

## 🔁 Custom Training Loops

⚠️ **Prefer `model.fit()` whenever possible** — a custom loop is longer, more error-prone, and harder to maintain. But sometimes you genuinely need it (e.g., using **two different optimizers** for two different parts of a network, as in the original Wide & Deep paper).

```python
# Build (but don't compile!) the model — we're managing training manually
model = keras.models.Sequential([
    keras.layers.Dense(30, activation="elu", kernel_initializer="he_normal"),
    keras.layers.Dense(1),
])

def random_batch(X, y, batch_size=32):
    idx = np.random.randint(len(X), size=batch_size)
    return X[idx], y[idx]

n_epochs, batch_size = 5, 32
n_steps = len(X_train) // batch_size
optimizer = keras.optimizers.Nadam(learning_rate=0.01)
loss_fn = keras.losses.mean_squared_error
mean_loss = keras.metrics.Mean()

for epoch in range(1, n_epochs + 1):
    for step in range(1, n_steps + 1):
        X_batch, y_batch = random_batch(X_train, y_train)
        with tf.GradientTape() as tape:
            y_pred = model(X_batch, training=True)               # forward pass
            main_loss = tf.reduce_mean(loss_fn(y_batch, y_pred))
            loss = tf.add_n([main_loss] + model.losses)           # + any regularization losses
        gradients = tape.gradient(loss, model.trainable_variables)
        optimizer.apply_gradients(zip(gradients, model.trainable_variables))
        mean_loss(loss)
    mean_loss.reset_states()
```
Walking through the key lines: forward pass and loss computation happen **inside** the `GradientTape` block; `tape.gradient()` computes gradients only with respect to `model.trainable_variables` (not all variables); `optimizer.apply_gradients()` performs the actual weight update step.

⚠️ **Things you must handle manually that `fit()` normally does for you**: passing `training=True`/`False` correctly (so layers like `BatchNormalization`/`Dropout` behave correctly), applying weight constraints after each update, and any gradient clipping (though `clipnorm`/`clipvalue` on the optimizer still work automatically).

---

## ⚡ TensorFlow Functions and Graphs — `@tf.function`

Plain Python functions run as regular (slow-ish) Python. Decorating a function with `@tf.function` tells TensorFlow to trace it once and convert it into a fast, optimized, portable **computation graph**:

```python
@tf.function
def my_function(x):
    return x ** 2 + 10

# equivalent, without the decorator syntax:
tf_my_function = tf.function(my_function)
```
- Keras automatically converts your `call()` methods (custom layers/models) into `tf.function`s for you — you get this speed boost "for free" in most normal usage.
- Graphs run faster (especially with repeated operations) and can be **exported and run on other platforms** without a Python interpreter at all — mobile, embedded devices, servers (this portability matters a lot in Ch. 19).
- Under the hood, TensorFlow uses **AutoGraph** to convert Python control flow (`for`, `while`, `if`) into equivalent graph operations (`tf.while_loop`, etc.) automatically.

⚠️ **TF Function rules to know**: prefer TensorFlow operations over arbitrary Python/NumPy calls inside a `tf.function` (non-TF operations may only run once, at trace time, not every call); avoid creating new `tf.Variable`s inside the function body on every call; and be cautious with Python side effects (`print()`, appending to lists) — they may only execute during the initial trace, not on every subsequent call.

---

## ⚠️ Common Pitfalls

- Reaching for a custom training loop by default "to feel in control," when `model.fit()` (with callbacks) would have done the job more reliably.
- Returning the **mean** loss from a custom loss function instead of one value **per instance** — breaks Keras's ability to apply sample/class weights correctly.
- Forgetting `get_config()` on custom losses/layers/metrics — the model will train fine, but **fail to reload correctly** later without manually re-supplying `custom_objects`.
- Calling `tape.gradient()` twice on a non-persistent tape.
- In a custom loop, forgetting to pass `training=True`/`False` appropriately — silently breaks BatchNorm/Dropout behavior.
- Mixing plain Python/NumPy logic heavily inside `@tf.function`-decorated code and being surprised when it doesn't re-execute as expected on each call.

---

## 🔑 Cheat Sheet

- TensorFlow tensors behave like NumPy arrays, but are GPU/TPU-accelerated; `tf.Variable` for mutable state (like model weights), `tf.constant`/tensors otherwise.
- Custom losses/metrics/layers: plain functions for simple cases; subclass `keras.losses.Loss` / `keras.metrics.Metric` / `keras.layers.Layer` (with `get_config()`) when you need configurability or saving support.
- `tf.GradientTape()`: records operations on `Variable`s; `.gradient(target, sources)` computes exact gradients via reverse-mode autodiff, in one pass, regardless of parameter count.
- Custom training loop = manual version of `fit()`: forward pass + loss inside the tape, `tape.gradient()`, `optimizer.apply_gradients()` — use only when `fit()` genuinely can't do what you need.
- `@tf.function` compiles a Python function into a fast, portable TensorFlow graph — Keras does this for you automatically in normal usage.

---

## ✍️ Practice

1. **Hands-on:** Implement a custom `log_cosh` loss function (`tf.math.log(tf.math.cosh(y_pred - y_true))`) and train a regression model with it. Compare its behavior on outlier-heavy data against plain MSE.
2. **Conceptual:** Why must a custom loss function return one value *per instance* rather than a single averaged scalar?
3. **Hands-on:** Write a custom layer that implements a simple "weighted skip connection": `output = alpha * input + (1 - alpha) * Dense(units)(input)`, where `alpha` is a *trainable* scalar weight. (Hint: create it in `build()` with `add_weight()`.)
4. **Debug-the-scenario:** Your custom training loop runs, but `BatchNormalization` layers behave as if they're always in inference mode, even during training. What line is almost certainly missing?
   <details><summary>Answer</summary>`model(X_batch, training=True)` inside the training loop — without explicitly passing `training=True`, layers like BatchNorm/Dropout default to inference-mode behavior.</details>
5. **Conceptual:** In your own words, explain why reverse-mode autodiff (as used by `GradientTape`) is so much more efficient than numerically approximating each partial derivative one at a time for a network with a million parameters.

Next: `13_loading_preprocessing_data.md` — the `tf.data` API, for feeding huge datasets into training efficiently.