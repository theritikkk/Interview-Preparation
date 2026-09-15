import sys
sys.path.insert(0, "/home/claude")
from nb_helper import md, code, make_notebook

OUT = "/mnt/user-data/outputs/ML_Notes/code/part2_deep_learning"

# ============================================================ Chapter 17
cells = [
md("""# Chapter 17 — Autoencoders and GANs
Companion notebook to `part2_deep_learning/17_autoencoders_gans.md`."""),

code("""import numpy as np
import matplotlib.pyplot as plt
import tensorflow as tf
from tensorflow import keras

fashion_mnist = keras.datasets.fashion_mnist
(X_train_full, y_train_full), (X_test, y_test) = fashion_mnist.load_data()
X_train_full = X_train_full.astype(np.float32) / 255.0
X_test = X_test.astype(np.float32) / 255.0
X_valid, X_train = X_train_full[:5000], X_train_full[5000:]"""),

md("## A simple stacked (undercomplete) autoencoder"),
code("""stacked_encoder = keras.models.Sequential([
    keras.layers.Flatten(input_shape=[28, 28]),
    keras.layers.Dense(100, activation="selu"),
    keras.layers.Dense(30, activation="selu"),
])
stacked_decoder = keras.models.Sequential([
    keras.layers.Dense(100, activation="selu", input_shape=[30]),
    keras.layers.Dense(28 * 28, activation="sigmoid"),
    keras.layers.Reshape([28, 28]),
])
stacked_ae = keras.models.Sequential([stacked_encoder, stacked_decoder])
stacked_ae.compile(loss="binary_crossentropy", optimizer="nadam")
history = stacked_ae.fit(X_train, X_train, epochs=10, validation_data=(X_valid, X_valid))"""),

code("""def plot_reconstructions(model, images, n=5):
    reconstructions = model.predict(images[:n])
    fig, axes = plt.subplots(2, n, figsize=(2 * n, 4))
    for i in range(n):
        axes[0, i].imshow(images[i], cmap="binary"); axes[0, i].axis("off")
        axes[1, i].imshow(reconstructions[i], cmap="binary"); axes[1, i].axis("off")
    axes[0, 0].set_ylabel("Original"); axes[1, 0].set_ylabel("Reconstructed")
    plt.show()

plot_reconstructions(stacked_ae, X_valid)"""),

md("## A small GAN, trained on Fashion MNIST"),
code("""codings_size = 30

generator = keras.models.Sequential([
    keras.layers.Dense(100, activation="selu", input_shape=[codings_size]),
    keras.layers.Dense(150, activation="selu"),
    keras.layers.Dense(28 * 28, activation="sigmoid"),
    keras.layers.Reshape([28, 28]),
])
discriminator = keras.models.Sequential([
    keras.layers.Flatten(input_shape=[28, 28]),
    keras.layers.Dense(150, activation="selu"),
    keras.layers.Dense(100, activation="selu"),
    keras.layers.Dense(1, activation="sigmoid"),
])
gan = keras.models.Sequential([generator, discriminator])

discriminator.compile(loss="binary_crossentropy", optimizer="rmsprop")
discriminator.trainable = False
gan.compile(loss="binary_crossentropy", optimizer="rmsprop")"""),

code("""batch_size = 64
dataset = tf.data.Dataset.from_tensor_slices(X_train).shuffle(1000).batch(batch_size, drop_remainder=True).prefetch(1)

def train_gan(gan, dataset, batch_size, codings_size, n_epochs=5):
    generator, discriminator = gan.layers
    for epoch in range(n_epochs):
        for X_batch in dataset:
            noise = tf.random.normal(shape=[batch_size, codings_size])
            generated_images = generator(noise)
            X_fake_and_real = tf.concat([generated_images, X_batch], axis=0)
            y1 = tf.constant([[0.]] * batch_size + [[1.]] * batch_size)
            discriminator.trainable = True
            discriminator.train_on_batch(X_fake_and_real, y1)

            noise = tf.random.normal(shape=[batch_size, codings_size])
            y2 = tf.constant([[1.]] * batch_size)
            discriminator.trainable = False
            gan.train_on_batch(noise, y2)
        print(f"Epoch {epoch + 1}/{n_epochs} done")

train_gan(gan, dataset, batch_size, codings_size, n_epochs=5)"""),

code("""noise = tf.random.normal(shape=[10, codings_size])
generated = generator(noise)
fig, axes = plt.subplots(1, 10, figsize=(15, 2))
for i in range(10):
    axes[i].imshow(generated[i], cmap="binary"); axes[i].axis("off")
plt.suptitle("Freshly generated (fake) Fashion-MNIST-like images")
plt.show()"""),

md("""## ✍️ Practice
1. Visualize a few reconstructions above — how much detail is lost at 30 latent dimensions?
2. Why does a denoising autoencoder tend to learn more useful features than a plain one?
3. Train the GAN longer (20-50 epochs) — does image quality visibly improve? Watch for mode collapse (all generated images looking alike).
4. Debug: after training, the generator only produces one type of image regardless of noise input — what's this called, name one mitigation."""),
code("# Your code / notes here\n"),
]
make_notebook(cells, f"{OUT}/17_autoencoders_gans.ipynb")

# ============================================================ Chapter 18
cells = [
md("""# Chapter 18 — Reinforcement Learning
Companion notebook to `part2_deep_learning/18_reinforcement_learning.md`.
Needs the `gymnasium` (or classic `gym`) package: `pip install gymnasium`."""),

code("""import numpy as np
import tensorflow as tf
from tensorflow import keras
from collections import deque

try:
    import gymnasium as gym
except ImportError:
    import gym

env = gym.make("CartPole-v1")
print("Observation space:", env.observation_space)
print("Action space:", env.action_space)"""),

md("## Policy Gradients (REINFORCE)"),
code("""model = keras.models.Sequential([
    keras.layers.Dense(5, activation="elu", input_shape=[4]),
    keras.layers.Dense(1, activation="sigmoid"),
])

def play_one_step(env, obs, model, loss_fn):
    with tf.GradientTape() as tape:
        left_proba = model(obs[np.newaxis])
        action = (tf.random.uniform([1, 1]) > left_proba)
        y_target = tf.constant([[1.]]) - tf.cast(action, tf.float32)
        loss = tf.reduce_mean(loss_fn(y_target, left_proba))
    grads = tape.gradient(loss, model.trainable_variables)
    result = env.step(int(action[0, 0].numpy()))
    obs, reward, done = result[0], result[1], result[2]
    if len(result) > 3 and isinstance(result[3], bool):   # gymnasium: (obs, r, terminated, truncated, info)
        done = done or result[3]
    return obs, reward, done, grads

def play_multiple_episodes(env, n_episodes, n_max_steps, model, loss_fn):
    all_rewards, all_grads = [], []
    for episode in range(n_episodes):
        current_rewards, current_grads = [], []
        obs = env.reset()
        if isinstance(obs, tuple):   # gymnasium reset() returns (obs, info)
            obs = obs[0]
        for step in range(n_max_steps):
            obs, reward, done, grads = play_one_step(env, obs, model, loss_fn)
            current_rewards.append(reward)
            current_grads.append(grads)
            if done:
                break
        all_rewards.append(current_rewards)
        all_grads.append(current_grads)
    return all_rewards, all_grads

def discount_rewards(rewards, discount_factor):
    discounted = np.array(rewards, dtype=float)
    for step in range(len(rewards) - 2, -1, -1):
        discounted[step] += discounted[step + 1] * discount_factor
    return discounted

def discount_and_normalize_rewards(all_rewards, discount_factor):
    all_discounted_rewards = [discount_rewards(r, discount_factor) for r in all_rewards]
    flat = np.concatenate(all_discounted_rewards)
    mean, std = flat.mean(), flat.std()
    return [(dr - mean) / (std + 1e-8) for dr in all_discounted_rewards]"""),

code("""n_iterations = 50
n_episodes_per_update = 10
n_max_steps = 200
discount_factor = 0.95
optimizer = keras.optimizers.Adam(learning_rate=0.01)
loss_fn = keras.losses.binary_crossentropy

for iteration in range(n_iterations):
    all_rewards, all_grads = play_multiple_episodes(env, n_episodes_per_update, n_max_steps, model, loss_fn)
    total_rewards = sum(map(sum, all_rewards))
    if iteration % 10 == 0:
        print(f"Iteration {iteration}: mean reward = {total_rewards / n_episodes_per_update:.1f}")
    all_final_rewards = discount_and_normalize_rewards(all_rewards, discount_factor)
    all_mean_grads = []
    for var_index in range(len(model.trainable_variables)):
        mean_grads = tf.reduce_mean(
            [final_reward * all_grads[ep][step][var_index]
             for ep, final_rewards in enumerate(all_final_rewards)
             for step, final_reward in enumerate(final_rewards)], axis=0)
        all_mean_grads.append(mean_grads)
    optimizer.apply_gradients(zip(all_mean_grads, model.trainable_variables))"""),

md("""## ✍️ Practice
1. Plot mean episode reward over training iterations — how many iterations until it's consistently near 200?
2. Explain the difference between on-policy (Policy Gradients) and off-policy (Q-Learning) learning.
3. Implement basic Deep Q-Learning with a replay buffer (see the notes for the full recipe) on CartPole. Do you see catastrophic forgetting?
4. Debug: DQN loss steadily decreases but game score gets worse — what should you track instead of loss?"""),
code("# Your code / notes here\n"),
]
make_notebook(cells, f"{OUT}/18_reinforcement_learning.ipynb")

# ============================================================ Chapter 19
cells = [
md("""# Chapter 19 — Training and Deploying TensorFlow Models at Scale
Companion notebook to `part2_deep_learning/19_training_deploying_at_scale.md`."""),

code("""import os
import numpy as np
import tensorflow as tf
from tensorflow import keras

# A small trained model to export (reuse Fashion MNIST from Chapter 10/11 if you have it in memory,
# otherwise train a quick one here)
fashion_mnist = keras.datasets.fashion_mnist
(X_train, y_train), (X_test, y_test) = fashion_mnist.load_data()
X_train, X_test = X_train / 255.0, X_test / 255.0

model = keras.models.Sequential([
    keras.layers.Flatten(input_shape=[28, 28]),
    keras.layers.Dense(100, activation="relu"),
    keras.layers.Dense(10, activation="softmax"),
])
model.compile(loss="sparse_categorical_crossentropy", optimizer="adam", metrics=["accuracy"])
model.fit(X_train, y_train, epochs=3, validation_split=0.1)"""),

md("## Export to SavedModel format"),
code("""model_version = "0001"
model_name = "my_mnist_model"
model_path = os.path.join(model_name, model_version)
tf.saved_model.save(model, model_path)
print("Saved to:", model_path)

# Inspect it
os.system(f"saved_model_cli show --dir {model_path} --all")"""),

md("## Load it back two ways"),
code("""# 1. As a raw SavedModel
raw_model = tf.saved_model.load(model_path)
y_pred_raw = raw_model(tf.constant(X_test[:3], dtype=tf.float32))
print("Raw SavedModel prediction shape:", y_pred_raw.shape)

# 2. As a Keras model
keras_model = keras.models.load_model(model_path)
print("Keras reloaded model accuracy:", keras_model.evaluate(X_test, y_test)[1])"""),

md("## Convert to TFLite (for mobile/embedded) with post-training quantization"),
code("""converter = tf.lite.TFLiteConverter.from_saved_model(model_path)
converter.optimizations = [tf.lite.Optimize.DEFAULT]   # enables post-training quantization
tflite_model = converter.convert()

with open("converted_model.tflite", "wb") as f:
    f.write(tflite_model)

original_size = sum(os.path.getsize(os.path.join(dp, f))
                     for dp, _, fs in os.walk(model_path) for f in fs)
tflite_size = os.path.getsize("converted_model.tflite")
print(f"SavedModel size: {original_size/1024:.1f} KB   TFLite size: {tflite_size/1024:.1f} KB")"""),

md("## Data parallelism across multiple GPUs (pattern — needs actual multi-GPU hardware to see the speedup)"),
code("""distribution = tf.distribute.MirroredStrategy()
print("Number of devices:", distribution.num_replicas_in_sync)

with distribution.scope():
    mirrored_model = keras.models.Sequential([
        keras.layers.Flatten(input_shape=[28, 28]),
        keras.layers.Dense(100, activation="relu"),
        keras.layers.Dense(10, activation="softmax"),
    ])
    mirrored_model.compile(loss="sparse_categorical_crossentropy", optimizer="adam", metrics=["accuracy"])

mirrored_model.fit(X_train, y_train, epochs=2, batch_size=100, validation_split=0.1)"""),

md("""## ✍️ Practice
1. Inspect the SavedModel above with `saved_model_cli show --dir ... --all` — what are the input/output tensor names and shapes?
2. Why does bundling preprocessing inside an exported model reduce training/serving skew risk?
3. Compare file size and test accuracy of the model before and after TFLite quantization above.
4. Debug: adding a 2nd GPU only gives a 1.3x speedup instead of ~2x — what's the likely explanation? Is this surprising?"""),
code("# Your code / notes here\n"),
]
make_notebook(cells, f"{OUT}/19_training_deploying_at_scale.ipynb")

print("Chapters 17-19 done")