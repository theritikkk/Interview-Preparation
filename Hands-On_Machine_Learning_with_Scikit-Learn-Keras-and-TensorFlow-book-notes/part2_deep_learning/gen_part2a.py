import sys
sys.path.insert(0, "/home/claude")
from nb_helper import md, code, make_notebook

OUT = "/mnt/user-data/outputs/ML_Notes/code/part2_deep_learning"

# ============================================================ Chapter 10
cells = [
md("""# Chapter 10 — Introduction to Artificial Neural Networks with Keras
Companion notebook to `part2_deep_learning/10_ann_with_keras.md`.
Needs `tensorflow` installed. Downloads Fashion MNIST the first time (via Keras)."""),

code("""import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import tensorflow as tf
from tensorflow import keras

print("TensorFlow version:", tf.__version__)"""),

md("## Load & prep Fashion MNIST"),
code("""fashion_mnist = keras.datasets.fashion_mnist
(X_train_full, y_train_full), (X_test, y_test) = fashion_mnist.load_data()

X_valid, X_train = X_train_full[:5000] / 255.0, X_train_full[5000:] / 255.0
y_valid, y_train = y_train_full[:5000], y_train_full[5000:]
X_test = X_test / 255.0

class_names = ["T-shirt/top", "Trouser", "Pullover", "Dress", "Coat",
               "Sandal", "Shirt", "Sneaker", "Bag", "Ankle boot"]

plt.imshow(X_train[0], cmap="binary")
plt.title(class_names[y_train[0]])
plt.axis("off")
plt.show()"""),

md("## Sequential API"),
code("""model = keras.models.Sequential([
    keras.layers.Flatten(input_shape=[28, 28]),
    keras.layers.Dense(300, activation="relu"),
    keras.layers.Dense(100, activation="relu"),
    keras.layers.Dense(10, activation="softmax"),
])
model.summary()"""),

code("""model.compile(loss="sparse_categorical_crossentropy",
              optimizer="sgd",
              metrics=["accuracy"])

history = model.fit(X_train, y_train, epochs=15,
                     validation_data=(X_valid, y_valid))"""),

code("""pd.DataFrame(history.history).plot(figsize=(8, 5))
plt.grid(True); plt.gca().set_ylim(0, 1)
plt.title("Learning curves")
plt.show()

model.evaluate(X_test, y_test)"""),

code("""y_proba = model.predict(X_test[:5])
y_pred = y_proba.argmax(axis=-1)
print("Predicted:", [class_names[i] for i in y_pred])
print("Actual:   ", [class_names[i] for i in y_test[:5]])"""),

md("## Functional API: Wide & Deep on California housing"),
code("""from sklearn.datasets import fetch_california_housing
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler

housing = fetch_california_housing()
X_train_full_h, X_test_h, y_train_full_h, y_test_h = train_test_split(housing.data, housing.target, random_state=42)
X_train_h, X_valid_h, y_train_h, y_valid_h = train_test_split(X_train_full_h, y_train_full_h, random_state=42)

scaler = StandardScaler()
X_train_h = scaler.fit_transform(X_train_h)
X_valid_h = scaler.transform(X_valid_h)
X_test_h = scaler.transform(X_test_h)

input_ = keras.layers.Input(shape=X_train_h.shape[1:])
hidden1 = keras.layers.Dense(30, activation="relu")(input_)
hidden2 = keras.layers.Dense(30, activation="relu")(hidden1)
concat = keras.layers.Concatenate()([input_, hidden2])
output = keras.layers.Dense(1)(concat)
wide_deep_model = keras.Model(inputs=[input_], outputs=[output])

wide_deep_model.compile(loss="mean_squared_error", optimizer="sgd")
history = wide_deep_model.fit(X_train_h, y_train_h, epochs=20,
                               validation_data=(X_valid_h, y_valid_h))
print("Test MSE:", wide_deep_model.evaluate(X_test_h, y_test_h))"""),

md("""## ✍️ Practice
1. Above: is the Fashion MNIST model overfitting, underfitting, or about right? Look at the learning curves.
2. Why can't a network with no/only-linear activations do anything a single linear model couldn't?
3. Add 2 more hidden layers (100 units, relu) to the Fashion MNIST model — does validation accuracy improve? Add EarlyStopping — how many epochs before it stops?
4. Debug: a regression MLP's loss goes `nan` after a few batches — name 2 likely causes.
5. Compare the Wide & Deep model's test MSE above against a plain 2-hidden-layer Sequential model on the same data."""),
code("# Your code / notes here\n"),
]
make_notebook(cells, f"{OUT}/10_ann_with_keras.ipynb")

# ============================================================ Chapter 11
cells = [
md("""# Chapter 11 — Training Deep Neural Networks
Companion notebook to `part2_deep_learning/11_training_deep_neural_nets.md`."""),

code("""import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import tensorflow as tf
from tensorflow import keras

fashion_mnist = keras.datasets.fashion_mnist
(X_train_full, y_train_full), (X_test, y_test) = fashion_mnist.load_data()
X_valid, X_train = X_train_full[:5000] / 255.0, X_train_full[5000:] / 255.0
y_valid, y_train = y_train_full[:5000], y_train_full[5000:]
X_test = X_test / 255.0"""),

md("## He init + ELU + Batch Normalization + Nadam"),
code("""def build_model():
    return keras.models.Sequential([
        keras.layers.Flatten(input_shape=[28, 28]),
        keras.layers.BatchNormalization(),
        keras.layers.Dense(300, activation="elu", kernel_initializer="he_normal"),
        keras.layers.BatchNormalization(),
        keras.layers.Dense(100, activation="elu", kernel_initializer="he_normal"),
        keras.layers.BatchNormalization(),
        keras.layers.Dense(10, activation="softmax"),
    ])

model_bn = build_model()
model_bn.compile(loss="sparse_categorical_crossentropy", optimizer="nadam", metrics=["accuracy"])
history_bn = model_bn.fit(X_train, y_train, epochs=10, validation_data=(X_valid, y_valid))"""),

md("## Dropout as regularization"),
code("""model_dropout = keras.models.Sequential([
    keras.layers.Flatten(input_shape=[28, 28]),
    keras.layers.Dropout(rate=0.2),
    keras.layers.Dense(300, activation="elu", kernel_initializer="he_normal"),
    keras.layers.Dropout(rate=0.2),
    keras.layers.Dense(100, activation="elu", kernel_initializer="he_normal"),
    keras.layers.Dropout(rate=0.2),
    keras.layers.Dense(10, activation="softmax"),
])
model_dropout.compile(loss="sparse_categorical_crossentropy", optimizer="nadam", metrics=["accuracy"])
history_dropout = model_dropout.fit(X_train, y_train, epochs=10, validation_data=(X_valid, y_valid))"""),

md("## Callbacks: ModelCheckpoint + EarlyStopping, and a learning-rate schedule"),
code("""model_cb = build_model()
model_cb.compile(loss="sparse_categorical_crossentropy", optimizer="nadam", metrics=["accuracy"])

checkpoint_cb = keras.callbacks.ModelCheckpoint("best_model.h5", save_best_only=True)
early_stopping_cb = keras.callbacks.EarlyStopping(patience=5, restore_best_weights=True)

history_cb = model_cb.fit(X_train, y_train, epochs=50,
                           validation_data=(X_valid, y_valid),
                           callbacks=[checkpoint_cb, early_stopping_cb])
print("Stopped after", len(history_cb.history["loss"]), "epochs")"""),

code("""def exponential_decay(lr0, s):
    def fn(epoch):
        return lr0 * 0.1 ** (epoch / s)
    return fn

lr_scheduler = keras.callbacks.LearningRateScheduler(exponential_decay(lr0=0.01, s=20))
model_lr = build_model()
model_lr.compile(loss="sparse_categorical_crossentropy",
                  optimizer=keras.optimizers.SGD(learning_rate=0.01), metrics=["accuracy"])
history_lr = model_lr.fit(X_train, y_train, epochs=15,
                           validation_data=(X_valid, y_valid), callbacks=[lr_scheduler])"""),

md("## Compare all variants"),
code("""for name, h in [("BatchNorm+Nadam", history_bn), ("Dropout", history_dropout),
                ("Checkpoint+EarlyStop", history_cb), ("LR schedule", history_lr)]:
    print(f"{name:22s} best val_accuracy = {max(h.history['val_accuracy']):.4f}")"""),

md("""## ✍️ Practice
1. Compare convergence speed (epochs to 90% val accuracy) of the BatchNorm+Nadam model vs a plain SGD model.
2. Explain why "dying ReLU" happens, and why Leaky ReLU fixes it.
3. Does the Dropout model's validation accuracy improve vs the plain model? Does it need more epochs?
4. Debug: fine-tuning ALL layers of a pretrained model on 500 images with the pretraining learning rate hurts accuracy — what's wrong, what 2 things would you try?"""),
code("# Your code / notes here\n"),
]
make_notebook(cells, f"{OUT}/11_training_deep_neural_nets.ipynb")

print("Chapters 10-11 done")