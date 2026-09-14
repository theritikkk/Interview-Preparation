# Chapter 17 — Autoencoders and GANs

## 🎯 Why this chapter matters
So far every network has needed labels. This chapter covers **unsupervised representation learning** — networks that learn useful, compressed representations of data with **no labels at all** — and **generative models**, networks that can create brand-new, realistic data. These ideas are the direct ancestors of today's image-generation and diffusion models.

---

## 🗜️ Autoencoders — Efficient Data Representations

**Core idea:** train a network to **reconstruct its own input**, but force the information to pass through a **bottleneck** (a smaller internal representation) along the way. To succeed despite the bottleneck, the network is *forced* to discover and exploit the most important patterns/structure in the data, discarding noise and redundancy.

> 💡 **Analogy**: chess experts can memorize a mid-game board in 5 seconds — not because they have better raw memory, but because they recognize *patterns* (real game structures) that compress what needs to be remembered. Random piece placements (no pattern) are just as hard for them as for anyone else. An autoencoder does the same thing with data.

An autoencoder always has two parts:
- **Encoder** (recognition network): compresses the input into a **latent representation** (a.k.a. **codings**).
- **Decoder** (generative network): reconstructs the output from that latent representation.

Since the latent space has **fewer dimensions than the input** (an **undercomplete** autoencoder), the network can't just trivially copy input to output — it must learn a genuinely useful compression.

```python
encoder = keras.models.Sequential([keras.layers.Dense(2, input_shape=[3])])
decoder = keras.models.Sequential([keras.layers.Dense(3, input_shape=[2])])
autoencoder = keras.models.Sequential([encoder, decoder])
autoencoder.compile(loss="mse", optimizer=keras.optimizers.SGD(lr=1.5))
autoencoder.fit(X_train, X_train, epochs=20)   # note: input IS the target!
```
A **linear** undercomplete autoencoder trained with MSE turns out to perform essentially the same dimensionality reduction as **PCA** (Ch. 8)! Stack multiple nonlinear layers (a **stacked autoencoder**) and it can capture much richer, nonlinear structure than PCA ever could.

### What autoencoders are used for
- **Dimensionality reduction / visualization** (like PCA, but more powerful).
- **Unsupervised pretraining**: train a stacked autoencoder on abundant *unlabeled* data, then reuse the encoder's lower layers for a supervised task with limited labeled data (Ch. 11's transfer learning strategy, applied when no directly-relevant pretrained model exists).
- **Anomaly detection**: an autoencoder trained only on "normal" data will reconstruct normal inputs well but reconstruct *anomalous* inputs poorly — high reconstruction error flags anomalies (similar spirit to Ch. 9's GMM-based anomaly detection).
- **Generating new data** (with the *variational* variant, below).

### Tying weights (a common trick to halve parameters)
For a symmetric stacked autoencoder, you can force the decoder's weights to be the **transpose** of the corresponding encoder's weights — halving the number of trainable parameters, speeding up training, and reducing overfitting risk, with a custom Keras layer that reuses `encoder_layer.weights[0]` transposed.

### Convolutional & Recurrent Autoencoders
When inputs are images, build the encoder from **Conv2D + pooling** layers and the decoder from **transposed convolutions** (upsampling back to the original resolution) — a **Convolutional Autoencoder**. For sequences, use recurrent layers in both encoder and decoder — a **Recurrent Autoencoder** (an RNN sequence-to-vector encoder + vector-to-sequence decoder, echoing Ch. 15/16's Encoder-Decoder pattern).

### Denoising Autoencoders — a powerful, simple regularization trick
Feed the encoder a **corrupted** version of the input (Gaussian noise, or randomly zeroed-out pixels via Dropout), but still train it to reconstruct the **original, clean** input. This forces the network to learn genuinely meaningful structure (you can't just memorize/copy if the input is corrupted differently every time) rather than trivial pixel-level shortcuts.

```python
denoising_encoder = keras.models.Sequential([
    keras.layers.Flatten(input_shape=[28, 28]),
    keras.layers.GaussianNoise(0.2),
    keras.layers.Dense(100, activation="selu"),
    keras.layers.Dense(30, activation="selu"),
])
```

### Sparse Autoencoders — another regularization angle
Add a penalty (during training) that pushes the *average activation* of each coding neuron across the training batch toward a small target value (e.g., 0.05) — the KL-divergence between the target and actual average activation is a common choice. This pushes the network toward using only a **small number of active neurons** per instance, similar in spirit to ℓ1 regularization (Ch. 4) — the result tends to be a more meaningfully-structured, interpretable representation.

---

## 🎲 Variational Autoencoders (VAEs) — the first real generative model in this chapter

VAEs differ from everything above in two crucial ways:
- **Probabilistic**: outputs involve genuine randomness, even after training.
- **Generative**: you can sample brand-new, plausible instances — not just reconstruct existing ones.

**How it works:** instead of the encoder outputting one fixed coding, it outputs a **mean $\mu$ and standard deviation $\sigma$** describing a small Gaussian distribution. The actual coding fed to the decoder is then **randomly sampled** from that Gaussian. Training uses two combined loss terms:
1. **Reconstruction loss** (as usual — did the decoder reproduce the input well?).
2. **Latent loss**: pushes the distribution of codings to resemble a simple **standard Gaussian** — computed via **KL divergence** between the target and actual coding distribution.

$$
\mathcal{L}_{\text{latent}} = -\frac{1}{2}\sum_{i=1}^{K}\left(1 + \log(\sigma_i^2) - \sigma_i^2 - \mu_i^2\right)
$$

**Why this is the magic trick:** by pushing every training instance's coding distribution toward a *shared*, simple Gaussian "cloud" in latent space, the *whole* latent space becomes meaningful and densely populated — so after training, you can **generate a brand new instance** just by sampling a random point from a standard Gaussian and running it through the decoder:

```python
codings = tf.random.normal(shape=[12, codings_size])
new_images = variational_decoder(codings)
```
```python
class Sampling(keras.layers.Layer):
    def call(self, inputs):
        mean, log_var = inputs
        return keras.backend.random_normal(tf.shape(log_var)) * keras.backend.exp(log_var / 2) + mean

codings_size = 10
inputs = keras.layers.Input(shape=[28, 28])
z = keras.layers.Flatten()(inputs)
z = keras.layers.Dense(150, activation="selu")(z)
z = keras.layers.Dense(100, activation="selu")(z)
codings_mean = keras.layers.Dense(codings_size)(z)       # μ
codings_log_var = keras.layers.Dense(codings_size)(z)    # log(σ²) — more numerically stable than σ directly
codings = Sampling()([codings_mean, codings_log_var])
variational_encoder = keras.Model(inputs=[inputs], outputs=[codings_mean, codings_log_var, codings])
```

---

## ⚔️ Generative Adversarial Networks (GANs)

**Core idea (Goodfellow et al., 2014):** pit **two networks against each other** in a competitive game, and the competition pushes both to improve dramatically:

- **Generator**: takes random noise as input, outputs fake data (e.g., an image) — functionally like a VAE's decoder, but trained completely differently.
- **Discriminator**: a binary classifier — given an image (real or generator-made), guess whether it's real or fake.

### Training — two alternating phases, every iteration
1. **Train the discriminator**: feed it a batch of real images (label 1) + an equal batch of freshly-generated fake images (label 0); update **only the discriminator's** weights via binary cross-entropy.
2. **Train the generator**: generate a fresh batch of fake images, run them through the (now frozen) discriminator, but label them all as **1** ("real") — the generator is rewarded for fooling the discriminator. Only the **generator's** weights get updated (the discriminator is frozen for this step).

```python
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
discriminator.trainable = False          # frozen when training through the combined `gan` model
gan.compile(loss="binary_crossentropy", optimizer="rmsprop")

def train_gan(gan, dataset, batch_size, codings_size, n_epochs=50):
    generator, discriminator = gan.layers
    for epoch in range(n_epochs):
        for X_batch in dataset:
            # Phase 1: train the discriminator
            noise = tf.random.normal(shape=[batch_size, codings_size])
            generated_images = generator(noise)
            X_fake_and_real = tf.concat([generated_images, X_batch], axis=0)
            y1 = tf.constant([[0.]] * batch_size + [[1.]] * batch_size)
            discriminator.trainable = True
            discriminator.train_on_batch(X_fake_and_real, y1)
            # Phase 2: train the generator (via the combined gan model)
            noise = tf.random.normal(shape=[batch_size, codings_size])
            y2 = tf.constant([[1.]] * batch_size)
            discriminator.trainable = False
            gan.train_on_batch(noise, y2)
```
> 🔑 **Remarkable fact**: the generator **never sees a single real image** — it only ever receives gradients flowing back *through* the discriminator. Yet as the discriminator gets better at spotting fakes, those gradients carry more and more useful signal, and the generator gradually learns to produce convincing images anyway.

### The Difficulties of Training GANs (this is real, and important to know)
GAN training is a **zero-sum game** between generator and discriminator — in game theory terms, it can reach a **Nash equilibrium** (a state where neither side benefits from changing strategy alone). GANs have exactly one such equilibrium in theory: the generator produces perfectly realistic images, and the discriminator is reduced to a coin flip (50/50). But **nothing guarantees training will ever reach it** — in practice:

- **Mode collapse**: the generator discovers one category (or a narrow range) that reliably fools the discriminator, and increasingly outputs *only* that — diversity collapses. The generator may then cycle between a few modes without ever mastering the full data distribution.
- **Training instability**: because the two networks are constantly adapting to each other, parameters can oscillate or suddenly diverge with no obvious warning — GANs are notoriously **hyperparameter-sensitive**.
- **Mitigations**: **experience replay** (train the discriminator against a buffer of past generated images, not just the newest batch, so it can't simply overfit to the current generator); **mini-batch discrimination** (let the discriminator see *batch-level* diversity statistics, so it can reject overly-uniform batches, pushing the generator toward more variety); and various alternative loss functions and architectural tweaks proposed in the (very active) research literature.

### Deep Convolutional GANs (DCGANs) and beyond
Replacing Dense layers with **Conv2D**/transposed-Conv2D layers (following empirically-discovered stability guidelines — e.g., strided convolutions instead of pooling, Batch Norm in most layers, no fully-connected hidden layers) made GANs viable for larger, more realistic images. Later advances pushed image quality further:
- **Progressive growing of GANs**: start training on tiny (e.g., 4×4) images, then progressively add layers to grow both networks to handle larger resolutions — much more stable than training at full resolution from the start.
- **StyleGAN**: separates a "mapping network" (turns the input noise into an intermediate style representation) from a "synthesis network" (generates the image, injecting style information at multiple resolutions) — produces remarkably high-quality, controllable image generation (famous for hyper-realistic generated faces).

---

## ⚠️ Common Pitfalls

- Forgetting that an autoencoder's **target is its own input** (`fit(X_train, X_train)`), not a separate label set.
- Making the latent space *too large* (overcomplete without other constraints) — the network can then trivially learn the identity function and learns nothing useful.
- Expecting a plain (non-variational) autoencoder to generate good new samples — the latent space of a plain autoencoder isn't organized for meaningful sampling; that's specifically what the VAE's latent-loss term provides.
- Forgetting to freeze the discriminator (`trainable = False`, recompiled appropriately) when training the generator through the combined GAN model — without it, you'd be fighting yourself.
- Treating GAN training instability/mode collapse as "something's wrong with my code" — it's a well-documented, inherent challenge of adversarial training, not necessarily a bug.
- Expecting GAN training to behave like ordinary supervised training (steadily decreasing loss) — oscillating or non-monotonic losses are normal in adversarial training; image quality is a better signal to watch than loss curves alone.

---

## 🔑 Cheat Sheet

- **Autoencoder** = encoder (compress) + decoder (reconstruct); trained by minimizing reconstruction error against its own input.
- **Undercomplete** bottleneck forces genuinely useful compression; a linear undercomplete autoencoder ≈ PCA.
- Uses: dimensionality reduction/visualization, unsupervised pretraining, anomaly detection (via reconstruction error), and (variational) generation.
- **Denoising autoencoder**: reconstruct clean input from corrupted input — strong, simple regularizer. **Sparse autoencoder**: penalize average neuron activation to encourage sparse, interpretable codings.
- **Variational Autoencoder (VAE)**: encoder outputs a distribution (μ, σ), not a point; latent loss (KL divergence) organizes the latent space into a usable Gaussian "cloud" → sample random points to generate new data.
- **GAN**: Generator (noise → fake data) vs. Discriminator (real-or-fake classifier), trained adversarially in alternating phases; generator only ever learns from gradients through the discriminator.
- GAN training challenges: **mode collapse** (diversity collapses) and **instability**; mitigations include experience replay and mini-batch discrimination.
- DCGAN → Progressive Growing → StyleGAN: the lineage of increasingly stable, high-quality image-generating GANs.

---

## ✍️ Practice

1. **Hands-on:** Train a simple undercomplete stacked autoencoder on Fashion MNIST (784 → 100 → 30 → 100 → 784). Visualize a few reconstructions next to their originals. How much detail is lost at 30 latent dimensions?
2. **Conceptual:** Explain why a denoising autoencoder tends to learn more *useful* features than a plain autoencoder with the same architecture and bottleneck size.
3. **Hands-on:** Build and train the VAE described above on Fashion MNIST. Sample 12 random codings from a standard Gaussian and decode them — do the generated images look like plausible (if imperfect) clothing items?
4. **Debug-the-scenario:** After 30 epochs, your GAN's generator produces only one type of image (e.g., always sneakers), regardless of the input noise. What's this phenomenon called, and name one mitigation technique.
   <details><summary>Answer</summary>Mode collapse. Mitigations: experience replay (train the discriminator against a buffer of past generated images) or mini-batch discrimination (let the discriminator penalize low-diversity batches).</details>
5. **Conceptual:** Why can't you train a GAN the normal way, with one combined loss function and one call to `fit()`? What's fundamentally different about its training objective compared to a standard supervised model?

Next: `18_reinforcement_learning.md` — a completely different learning paradigm: agents that learn from trial, error, and reward.