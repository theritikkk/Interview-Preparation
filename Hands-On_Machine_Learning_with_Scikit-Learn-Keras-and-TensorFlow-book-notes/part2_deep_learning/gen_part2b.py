import sys
sys.path.insert(0, "/home/claude")
from nb_helper import md, code, make_notebook

OUT = "/mnt/user-data/outputs/ML_Notes/code/part2_deep_learning"

# ============================================================ Chapter 12
cells = [
md("""# Chapter 12 — Custom Models and Training with TensorFlow
Companion notebook to `part2_deep_learning/12_custom_models_tensorflow.md`."""),

code("""import numpy as np
import tensorflow as tf
from tensorflow import keras
from sklearn.datasets import fetch_california_housing
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler

housing = fetch_california_housing()
X_train_full, X_test, y_train_full, y_test = train_test_split(housing.data, housing.target, random_state=42)
X_train, X_valid, y_train, y_valid = train_test_split(X_train_full, y_train_full, random_state=42)
scaler = StandardScaler()
X_train = scaler.fit_transform(X_train).astype(np.float32)
X_valid = scaler.transform(X_valid).astype(np.float32)
X_test = scaler.transform(X_test).astype(np.float32)
y_train, y_valid, y_test = y_train.astype(np.float32), y_valid.astype(np.float32), y_test.astype(np.float32)"""),

md("## Custom loss function: Huber loss, from scratch"),
code("""def huber_fn(y_true, y_pred):
    error = y_true - y_pred
    is_small_error = tf.abs(error) < 1
    squared_loss = tf.square(error) / 2
    linear_loss = tf.abs(error) - 0.5
    return tf.where(is_small_error, squared_loss, linear_loss)

model = keras.models.Sequential([
    keras.layers.Dense(30, activation="relu", input_shape=X_train.shape[1:]),
    keras.layers.Dense(1),
])
model.compile(loss=huber_fn, optimizer="nadam")
model.fit(X_train, y_train, epochs=5, validation_data=(X_valid, y_valid))"""),

md("## Custom layer with trainable weights"),
code("""class MyDense(keras.layers.Layer):
    def __init__(self, units, activation=None, **kwargs):
        super().__init__(**kwargs)
        self.units = units
        self.activation = keras.activations.get(activation)

    def build(self, batch_input_shape):
        self.kernel = self.add_weight(
            name="kernel", shape=[batch_input_shape[-1], self.units],
            initializer="glorot_normal")
        self.bias = self.add_weight(name="bias", shape=[self.units], initializer="zeros")
        super().build(batch_input_shape)

    def call(self, X):
        return self.activation(X @ self.kernel + self.bias)

custom_model = keras.models.Sequential([
    MyDense(30, activation="relu", input_shape=X_train.shape[1:]),
    MyDense(1),
])
custom_model.compile(loss="mse", optimizer="nadam")
custom_model.fit(X_train, y_train, epochs=5, validation_data=(X_valid, y_valid))"""),

md("## tf.GradientTape: computing gradients by hand"),
code("""w1, w2 = tf.Variable(5.), tf.Variable(3.)

def f(w1, w2):
    return 3 * w1 ** 2 + 2 * w1 * w2

with tf.GradientTape() as tape:
    z = f(w1, w2)
gradients = tape.gradient(z, [w1, w2])
print("Gradients:", [g.numpy() for g in gradients])"""),

md("## A custom training loop"),
code("""model2 = keras.models.Sequential([
    keras.layers.Dense(30, activation="elu", kernel_initializer="he_normal",
                        input_shape=X_train.shape[1:]),
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
        X_batch, y_batch = random_batch(X_train, y_train, batch_size)
        with tf.GradientTape() as tape:
            y_pred = model2(X_batch, training=True)
            main_loss = tf.reduce_mean(loss_fn(y_batch, tf.squeeze(y_pred)))
            loss = tf.add_n([main_loss] + model2.losses)
        gradients = tape.gradient(loss, model2.trainable_variables)
        optimizer.apply_gradients(zip(gradients, model2.trainable_variables))
        mean_loss(loss)
    print(f"Epoch {epoch}: mean loss = {mean_loss.result():.4f}")
    mean_loss.reset_states()"""),

md("""## ✍️ Practice
1. Implement a `log_cosh` custom loss and compare its behavior on outlier-heavy data vs MSE.
2. Why must a custom loss return one value per instance, not an averaged scalar?
3. Write a custom layer implementing a trainable weighted skip connection: `alpha*input + (1-alpha)*Dense(units)(input)`.
4. Debug: in a custom training loop, BatchNormalization always behaves as if in inference mode — what line is missing?"""),
code("# Your code / notes here\n"),
]
make_notebook(cells, f"{OUT}/12_custom_models_tensorflow.ipynb")

# ============================================================ Chapter 13
cells = [
md("""# Chapter 13 — Loading and Preprocessing Data with TensorFlow
Companion notebook to `part2_deep_learning/13_loading_preprocessing_data.md`."""),

code("""import numpy as np
import tensorflow as tf
from tensorflow import keras"""),

md("## tf.data.Dataset basics: chaining transformations"),
code("""dataset = tf.data.Dataset.from_tensor_slices(tf.range(10))
dataset = dataset.repeat(3).batch(7)
for item in dataset:
    print(item)"""),

code("""dataset2 = dataset.map(lambda x: x * 2)
dataset2 = dataset2.filter(lambda x: tf.reduce_sum(x) < 50)
for item in dataset2.take(2):
    print(item)"""),

md("## Shuffling + a full pipeline pattern"),
code("""dataset3 = tf.data.Dataset.range(10).repeat(3)
dataset3 = dataset3.shuffle(buffer_size=5, seed=42).batch(7)
for item in dataset3:
    print(item)"""),

code("""# A typical end-to-end pipeline over an in-memory numpy array (stand-in for reading from files)
X = np.random.rand(1000, 8).astype(np.float32)
y = np.random.randint(0, 2, size=(1000,)).astype(np.int32)

full_dataset = tf.data.Dataset.from_tensor_slices((X, y))
full_dataset = (full_dataset
                .cache()
                .shuffle(buffer_size=1000)
                .batch(32)
                .prefetch(tf.data.experimental.AUTOTUNE))

model = keras.models.Sequential([
    keras.layers.Dense(16, activation="relu", input_shape=[8]),
    keras.layers.Dense(1, activation="sigmoid"),
])
model.compile(loss="binary_crossentropy", optimizer="adam", metrics=["accuracy"])
model.fit(full_dataset, epochs=3)   # Keras accepts a Dataset directly!"""),

md("## Embeddings for high-cardinality categorical features"),
code("""vocab_size = 500       # e.g. 500 possible zip codes
embedding_dim = 8

embedding_layer = keras.layers.Embedding(input_dim=vocab_size, output_dim=embedding_dim)

sample_ids = tf.constant([3, 47, 210, 499])
embedded = embedding_layer(sample_ids)
print("Embedded shape:", embedded.shape)   # (4, 8) -- one 8-dim trainable vector per category"""),

md("## Keras preprocessing layer: Normalization"),
code("""normalization_layer = keras.layers.Normalization()
normalization_layer.adapt(X)   # learn mean/std from data, like a Scaler's .fit()

norm_model = keras.models.Sequential([
    normalization_layer,
    keras.layers.Dense(16, activation="relu"),
    keras.layers.Dense(1, activation="sigmoid"),
])
norm_model.compile(loss="binary_crossentropy", optimizer="adam", metrics=["accuracy"])
norm_model.fit(X, y, epochs=3)"""),

md("""## ✍️ Practice
1. Build a `tf.data` pipeline over several CSV-like numpy chunks (interleave + shuffle + map + batch + prefetch).
2. Why should `.cache()` generally go before `.shuffle()`/`.repeat()`, not after?
3. Compare model size/training speed: one-hot encoding a 500-category feature vs an Embedding(output_dim=16).
4. Debug: `fit()` throughput looks fine but GPU utilization is only 40% — what's the likely bottleneck, first fix to try?"""),
code("# Your code / notes here\n"),
]
make_notebook(cells, f"{OUT}/13_loading_preprocessing_data.ipynb")

# ============================================================ Chapter 14
cells = [
md("""# Chapter 14 — Deep Computer Vision Using CNNs
Companion notebook to `part2_deep_learning/14_cnn_computer_vision.md`."""),

code("""import numpy as np
import matplotlib.pyplot as plt
import tensorflow as tf
from tensorflow import keras

fashion_mnist = keras.datasets.fashion_mnist
(X_train_full, y_train_full), (X_test, y_test) = fashion_mnist.load_data()
X_valid, X_train = X_train_full[:5000] / 255.0, X_train_full[5000:] / 255.0
y_valid, y_train = y_train_full[:5000], y_train_full[5000:]
X_test = X_test / 255.0

# CNNs expect a channel dimension
X_train = X_train[..., np.newaxis]
X_valid = X_valid[..., np.newaxis]
X_test = X_test[..., np.newaxis]"""),

md("## A typical CNN architecture"),
code("""cnn_model = keras.models.Sequential([
    keras.layers.Conv2D(64, 7, activation="relu", padding="same", input_shape=[28, 28, 1]),
    keras.layers.MaxPooling2D(2),
    keras.layers.Conv2D(128, 3, activation="relu", padding="same"),
    keras.layers.Conv2D(128, 3, activation="relu", padding="same"),
    keras.layers.MaxPooling2D(2),
    keras.layers.Flatten(),
    keras.layers.Dense(128, activation="relu"),
    keras.layers.Dropout(0.5),
    keras.layers.Dense(10, activation="softmax"),
])
cnn_model.summary()"""),

code("""cnn_model.compile(loss="sparse_categorical_crossentropy", optimizer="nadam", metrics=["accuracy"])
history = cnn_model.fit(X_train, y_train, epochs=5, validation_data=(X_valid, y_valid))
print("Test accuracy:", cnn_model.evaluate(X_test, y_test)[1])
print("Total params:", cnn_model.count_params())"""),

md("## Transfer learning with a pretrained model (MobileNetV2, lightweight)"),
code("""# Using MobileNetV2 here instead of Xception purely for a smaller download / faster demo.
# The recipe (freeze -> train top -> unfreeze -> fine-tune) is identical either way.
base_model = keras.applications.MobileNetV2(weights="imagenet", include_top=False, input_shape=(96, 96, 3))

avg = keras.layers.GlobalAveragePooling2D()(base_model.output)
output = keras.layers.Dense(5, activation="softmax")(avg)   # e.g. 5 classes of your own
transfer_model = keras.Model(inputs=base_model.input, outputs=output)

for layer in base_model.layers:
    layer.trainable = False

transfer_model.compile(loss="sparse_categorical_crossentropy", optimizer="nadam", metrics=["accuracy"])
transfer_model.summary()
print("(Fit this on your own image dataset resized to 96x96x3 -- frozen base first, then unfreeze top layers with a lower learning rate.)")"""),

md("""## ✍️ Practice
1. Compare the CNN's test accuracy/parameter count above against the plain Dense MLP from Chapter 10.
2. Why does a CNN need far fewer parameters than a Dense network for the same image?
3. Fine-tune the transfer-learning model on a real small image dataset (e.g. `tf_flowers` via `tensorflow_datasets`) — freeze first, then unfreeze the top few layers with a lower LR.
4. Debug: a segmentation model built on a heavily-pooled classification backbone produces blurry boundaries — what property is responsible, and what's the general fix?"""),
code("# Your code / notes here\n"),
]
make_notebook(cells, f"{OUT}/14_cnn_computer_vision.ipynb")

print("Chapters 12-14 done")