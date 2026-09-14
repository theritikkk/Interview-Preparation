# Chapter 14 — Deep Computer Vision Using Convolutional Neural Networks

## 🎯 Why this chapter matters
Feeding raw pixels into a plain Dense network throws away one of the most important facts about images: **nearby pixels are related**. **Convolutional Neural Networks (CNNs)** exploit this spatial structure directly, and are the architecture behind essentially all modern computer vision — image classification, object detection, self-driving car perception, medical imaging, and more.

---

## 🧠 Why not just use a Dense network on images?

A fully-connected layer on a modest 150×100 color image would need $150 \times 100 \times 3 \times (\text{neurons in next layer})$ weights — **hundreds of millions of parameters** for one layer alone. Beyond being computationally wasteful, it also ignores a crucial insight: a useful feature detector (like "edge" or "eye") is useful **no matter where in the image it appears**. CNNs are (loosely) inspired by the visual cortex's structure: neurons respond only to a small local region (their **receptive field**), and these local detectors are reused across the whole image.

---

## 🧱 Convolutional Layers

Instead of connecting every neuron to every input pixel, a CNN neuron connects only to a **small rectangular region** of the previous layer — its **receptive field**. Stacking layers this way lets the network build a **hierarchy**: early layers detect simple local patterns (edges, corners), later layers combine them into increasingly complex, larger-scale features (textures, object parts, whole objects) — matching how real-world images are structured.

### Key vocabulary
- **Zero padding**: add a border of zeros around the input so the output can keep the same height/width as the input (`padding="same"` in Keras) — otherwise the image shrinks with every conv layer (`padding="valid"`).
- **Stride**: how far the receptive field shifts between neurons. Stride 1 = dense overlapping coverage; **stride 2+ skips positions**, shrinking the output size and dramatically cutting computation.
- **Filter (a.k.a. convolution kernel)**: a small set of learnable weights, shaped like a tiny image, that gets slid across the input. A filter that's a vertical bright line, for instance, will "light up" wherever the input has a vertical edge.
- **Feature map**: the output produced by sliding *one* filter across the whole input — a 2D grid showing *where* that filter's pattern was detected.
- A real convolutional layer applies **many filters at once**, so its output is genuinely 3D: (height, width, number of filters). Neurons within the *same* feature map **share the same weights** — this parameter sharing is what makes CNNs so much more parameter-efficient than Dense layers, and what lets a pattern learned in one part of the image be recognized *anywhere* in the image.
- You don't hand-design filters — **the network learns the most useful filters automatically** during training, just like it learns any other weights.

```python
from tensorflow import keras

conv_layer = keras.layers.Conv2D(filters=32, kernel_size=3, strides=1,
                                  padding="same", activation="relu")
```
⚠️ **Memory requirements add up fast**: each feature map takes real memory, especially in early layers with large spatial dimensions — a CNN can easily need gigabytes of RAM during training, which is why GPUs (with lots of fast memory) are so important for computer vision.

> 🔑 **Rule of thumb**: prefer **stacking several small kernels** (e.g., two 3×3 layers) over one large kernel (e.g., one 5×5) — fewer total parameters, less computation, and usually better performance. Exception: the very **first** layer can reasonably use a larger kernel (e.g., 7×7) with a stride ≥ 2, to shrink the spatial size quickly without losing much, since there are only 3 input channels there (cheap).

---

## 🏊 Pooling Layers

**Goal:** shrink (subsample) the feature maps — reducing computation, memory, parameter count, and overfitting risk. Unlike conv layers, **pooling neurons have no weights** — they just aggregate their receptive field with a fixed function (usually **max**).

```python
max_pool = keras.layers.MaxPooling2D(pool_size=2)   # halves height & width
```
- **Max pooling** (most common): keeps only the strongest activation in each small region. This also introduces a degree of **translation invariance** — a small shift in the input often produces the *same* pooled output — useful for classification (where *exact* position shouldn't matter), but **undesirable for tasks needing precise spatial correspondence** (like semantic segmentation, where you actually want *equivariance*: shift the input → shift the output the same way).
- ⚠️ Pooling is **destructive** — a 2×2 pool with stride 2 throws away 75% of the values in each region. This trade-off (losing detail for efficiency + some invariance) is usually worth it for classification but not always for other tasks.

---

## 🏗️ Typical CNN Architecture

The classic pattern: **[Conv → Conv → Pool] × N**, with the number of filters **growing** as spatial dimensions **shrink** (commonly doubling filter count each time you halve spatial size — keeps total compute roughly balanced), then **flatten** and finish with a few **Dense** layers ending in the task's output layer (e.g., softmax for classification).

```python
model = keras.models.Sequential([
    keras.layers.Conv2D(64, 7, activation="relu", padding="same", input_shape=[28, 28, 1]),
    keras.layers.MaxPooling2D(2),
    keras.layers.Conv2D(128, 3, activation="relu", padding="same"),
    keras.layers.Conv2D(128, 3, activation="relu", padding="same"),
    keras.layers.MaxPooling2D(2),
    keras.layers.Conv2D(256, 3, activation="relu", padding="same"),
    keras.layers.Conv2D(256, 3, activation="relu", padding="same"),
    keras.layers.MaxPooling2D(2),
    keras.layers.Flatten(),
    keras.layers.Dense(128, activation="relu"),
    keras.layers.Dropout(0.5),
    keras.layers.Dense(64, activation="relu"),
    keras.layers.Dropout(0.5),
    keras.layers.Dense(10, activation="softmax"),
])
```
This simple architecture reaches over 92% accuracy on Fashion MNIST — noticeably better than the plain Dense MLP from Chapter 10.

---

## 🏛️ Famous CNN Architectures (know the story, not just the names)

Progress on the ImageNet benchmark (1,000 classes, ~1M+ images) drove most of the field's key architectural innovations:

- **LeNet-5** (1998, Yann LeCun): the original — small, used for handwritten digit recognition. Established the conv+pool+dense pattern.
- **AlexNet** (2012): won ImageNet by a landslide (17% vs. 26% top-5 error for the runner-up) — the breakthrough that reignited deep learning interest. Deeper/larger than LeNet-5, used ReLU and dropout.
- **GoogLeNet** (2014): introduced the **Inception module** — instead of choosing one filter size, run several sizes (1×1, 3×3, 5×5) *in parallel* on the same input and concatenate their outputs, letting the network learn which scales matter. Much more parameter-efficient than a simple deep stack.
- **VGGNet** (2014): a simple, very uniform architecture (just stacks of 3×3 convs + pooling) — easy to understand, if not the most efficient.
- **ResNet** (2015): won with a **152-layer** network and under 3.6% top-5 error — huge leap. The key innovation: **skip (residual) connections** (below).
- **Xception** (2016): pushed the Inception idea further with **depthwise separable convolutions** (splitting a standard convolution into a per-channel spatial step plus a cross-channel 1×1 step) — fewer parameters, often better performance.
- **SENet** (2017): adds a small "Squeeze-and-Excitation" block that lets the network learn to **reweight feature maps by importance** — a lightweight attention-like mechanism.

### ResNet's key idea — skip connections (worth understanding deeply)
Add the layer's **input** directly to its output (a "shortcut"), forcing the block to learn the **residual** $f(x) = h(x) - x$ instead of the full target function $h(x)$ directly:

$$
\text{output} = f(x) + x
$$

**Why this helps so much:** when weights are initialized near zero, a block with a skip connection starts out approximating the **identity function** (just passes its input through) rather than an arbitrary near-zero function — a much better, "do-no-harm" starting point. This also lets the training signal flow easily through very deep stacks even before every layer has "learned" anything — deep ResNets (100+ layers) train successfully where equally deep plain stacks would struggle badly with vanishing gradients. When a residual block changes the feature map size/depth, a small 1×1 convolution is used on the skip path so the shapes still match for the addition.

---

## 🎁 Using Pretrained Models — don't train from scratch if you don't have to

Keras ships ready-to-use, ImageNet-pretrained models in `keras.applications`:

```python
model = keras.applications.resnet50.ResNet50(weights="imagenet")

images_resized = tf.image.resize(images, [224, 224])           # match the model's expected input size
inputs = keras.applications.resnet50.preprocess_input(images_resized * 255)   # model-specific preprocessing

Y_proba = model.predict(inputs)
top_K = keras.applications.resnet50.decode_predictions(Y_proba, top=3)  # human-readable top-3 predictions
```
⚠️ Each pretrained model has its **own expected input size and preprocessing convention** (pixel range 0–1, –1 to 1, etc.) — always use the matching `preprocess_input()` function.

### Transfer learning for your own classes
If your classes aren't in ImageNet, reuse the pretrained model's **lower layers** (general visual features) and replace the top:

```python
base_model = keras.applications.xception.Xception(weights="imagenet", include_top=False)
avg = keras.layers.GlobalAveragePooling2D()(base_model.output)
output = keras.layers.Dense(n_classes, activation="softmax")(avg)
model = keras.Model(inputs=base_model.input, outputs=output)

for layer in base_model.layers:
    layer.trainable = False    # freeze pretrained layers first

model.compile(...)
model.fit(...)    # train just the new top layer(s) first

# then unfreeze some/all of base_model's layers and continue training with a lower learning rate
```
This is exactly Chapter 11's transfer-learning recipe, applied to vision — and it's how the overwhelming majority of real-world image classifiers get built in practice (rarely from scratch).

---

## 🎯 Beyond Classification: Localization, Detection, Segmentation

- **Classification + Localization**: predict a class *and* a bounding box (as 4 extra regression outputs: center x/y, height, width) — a combined classification + regression task, typically trained with a combined loss.
- **Object Detection**: locate and classify **multiple** objects in one image. Classic approach: slide a classifier over many regions (**sliding window**) — very expensive. Modern efficient approaches:
  - **Fully Convolutional Networks (FCN)**: replace a CNN's final Dense layers with equivalent Conv layers, letting the network process images of any size and output a full grid of predictions in one pass, instead of separately per window.
  - **YOLO** ("You Only Look Once"): an extremely fast single-pass FCN-based detector — divides the image into a grid, and each grid cell directly predicts bounding boxes + class probabilities. Fast enough for real-time video.
  - Non-max suppression is used to clean up duplicate/overlapping detected boxes for the same object (keep only the most confident box, suppress heavily-overlapping lower-confidence ones).
- **Semantic Segmentation**: classify **every pixel** by which object/class it belongs to (not just draw a box). Needs **equivariance**, not the translation invariance pooling provides — so architectures for this task carefully preserve (or explicitly restore) spatial resolution, often through upsampling/transposed-convolution layers that expand the shrunk feature maps back to full image size.

---

## ⚠️ Common Pitfalls

- Using very large convolution kernels by default — usually worse (more parameters, more compute) than stacking small ones.
- Feeding a pretrained model images without the model-specific `preprocess_input()` step — silently tanks accuracy.
- Forgetting `include_top=False` when reusing a pretrained model for transfer learning — you'd otherwise get ImageNet's 1,000-class output layer instead of your own.
- Expecting max pooling's translation invariance to help on pixel-precise tasks like segmentation — it actively hurts there.
- Fine-tuning a whole pretrained network on a very small new dataset with a normal learning rate (Ch. 11's transfer learning pitfall applies here too).
- Underestimating GPU memory needs — deep CNNs with high-resolution inputs can exhaust GPU memory fast; reduce batch size or image resolution if you hit out-of-memory errors.

---

## 🔑 Cheat Sheet

- **Conv layer**: local receptive fields + weight sharing → learns spatially-reusable filters, producing feature maps; far fewer parameters than a Dense layer on the same input.
- Zero padding (`"same"`) keeps spatial size; stride > 1 shrinks it (and cuts compute).
- **Pooling** (esp. max pooling): shrinks feature maps, adds some translation invariance, but is destructive and undesirable for pixel-precise tasks.
- Typical architecture: [Conv, Conv, Pool] × N (filters growing as space shrinks) → Flatten → Dense → output.
- **ResNet's skip connections**: let a block default to the identity function at initialization, enabling very deep (100+ layer) networks to train well.
- **Use `keras.applications` pretrained models** for both direct inference (ImageNet classes) and transfer learning (`include_top=False` + freeze/unfreeze recipe from Ch. 11).
- Beyond classification: localization (class + bbox regression), object detection (YOLO/FCN, multiple objects), semantic segmentation (per-pixel classification, needs equivariance not invariance).

---

## ✍️ Practice

1. **Hands-on:** Build and train the Fashion MNIST CNN above. Compare its test accuracy and parameter count against the plain Dense MLP from Chapter 10.
2. **Conceptual:** Explain in your own words why a CNN needs far fewer parameters than a Dense network to process the same image, and why that also helps it generalize better.
3. **Hands-on:** Use a pretrained `Xception` model (`include_top=False`) to build a flower classifier via transfer learning on the `tf_flowers` dataset (via `tensorflow_datasets`). Freeze the base first, then try unfreezing the top few layers with a lower learning rate — does accuracy improve?
4. **Debug-the-scenario:** You build a segmentation model using a standard classification CNN backbone (with several max-pooling layers) and are disappointed that fine object boundaries come out blurry/blocky. What architectural property is likely responsible, and what's the general fix?
   <details><summary>Answer</summary>Max pooling repeatedly shrinks spatial resolution and introduces translation invariance, throwing away the fine positional detail segmentation needs (equivariance, not invariance). Fix: use an architecture that restores resolution (transposed convolutions/upsampling, skip connections from earlier high-resolution layers — e.g., U-Net-style architectures) instead of (or in addition to) heavy pooling.</details>
5. **Conceptual:** Why does adding a skip connection make it easier to train a 100-layer network than a plain 100-layer stack, especially early in training?

Next: `15_rnn_sequences_time_series.md` — Recurrent Neural Networks, for data with a time or sequence dimension.