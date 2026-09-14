# Chapter 13 — Loading and Preprocessing Data with TensorFlow

## 🎯 Why this chapter matters
Real datasets are often **too big to fit in RAM**, and loading/preprocessing data naively can leave your expensive GPU sitting idle, waiting for the CPU to prepare the next batch. The **`tf.data` API** solves this: build an efficient, multi-threaded, prefetching input pipeline that keeps your GPU fed continuously. This chapter also covers **embeddings**, one of deep learning's most important and reusable ideas.

---

## 🔗 The Data API — `tf.data.Dataset`

Everything revolves around a `Dataset` object: a sequence of data items, usually streamed from disk rather than loaded all at once.

```python
import tensorflow as tf

X = tf.range(10)
dataset = tf.data.Dataset.from_tensor_slices(X)   # 10 items: 0, 1, ..., 9

for item in dataset:
    print(item)
```

### Chaining transformations
Every dataset method returns a **new** dataset (originals are never modified — you must reassign, e.g. `dataset = dataset.repeat(...)`), so you can chain them fluently:

```python
dataset = dataset.repeat(3).batch(7)     # repeat the sequence 3x, then group into batches of 7
dataset = dataset.map(lambda x: x * 2)   # apply a transformation to every item — this is where preprocessing goes
dataset = dataset.filter(lambda x: x < 10)
for item in dataset.take(3):             # peek at just the first 3 items
    print(item)
```
- `.repeat()` with no argument repeats **forever** — your training loop then decides when to stop (e.g., via `steps_per_epoch`).
- `.map(fn, num_parallel_calls=N)` can run preprocessing across multiple CPU threads — essential for anything nontrivial (image decoding/resizing, text tokenizing, etc.).

### Shuffling
Gradient Descent works best on independent, identically distributed data — `.shuffle(buffer_size)` fills a buffer, then randomly pulls items from it, refilling from the source as it goes:

```python
dataset = tf.data.Dataset.range(10).repeat(3)
dataset = dataset.shuffle(buffer_size=5, seed=42).batch(7)
```
⚠️ **The buffer must be reasonably large relative to the dataset**, or shuffling is weak (imagine shuffling only the top 3 cards of a sorted deck — barely random). For datasets too big to buffer-shuffle well, a common combo is: **shuffle the source data at the file level** (split into many files, read a random subset simultaneously with `interleave()`), **then** apply a shuffling buffer on top of that.

### Prefetching — the key to keeping your GPU busy
```python
dataset = dataset.prefetch(1)    # or tf.data.experimental.AUTOTUNE to let TF pick automatically
```
While the GPU is training on the *current* batch, `.prefetch()` makes the CPU work **in parallel** on preparing the *next* batch (reading from disk, decoding, augmenting) — instead of the GPU sitting idle waiting. Combined with multithreaded `map()`/`interleave()`, this can keep the GPU close to 100% utilized, which is often the single biggest practical speedup available for I/O-bound training.

**If your dataset fits in RAM**, call `.cache()` right after loading/preprocessing (but before shuffling/repeating/batching) — each instance is then read/preprocessed only **once total**, not once per epoch.

```python
# A typical full pipeline for reading and preparing CSV files:
def csv_reader_dataset(filepaths, batch_size=32, n_readers=5, shuffle_buffer_size=10000):
    dataset = tf.data.Dataset.list_files(filepaths).repeat()
    dataset = dataset.interleave(
        lambda filepath: tf.data.TextLineDataset(filepath).skip(1),
        cycle_length=n_readers, num_parallel_calls=tf.data.experimental.AUTOTUNE)
    dataset = dataset.shuffle(shuffle_buffer_size)
    dataset = dataset.map(preprocess_fn, num_parallel_calls=tf.data.experimental.AUTOTUNE)
    dataset = dataset.batch(batch_size)
    return dataset.prefetch(1)

train_set = csv_reader_dataset(train_filepaths)
model.fit(train_set, epochs=10)   # Keras accepts a Dataset directly!
```

---

## 💾 The TFRecord Format

TensorFlow's own efficient **binary** file format for large datasets — much faster to read than CSV/text, especially for complex data like images.

```python
with tf.io.TFRecordWriter("my_data.tfrecord") as f:
    f.write(b"This is the first record")
    f.write(b"And the second record")

dataset = tf.data.TFRecordDataset(["my_data.tfrecord"])
```
Files can be **compressed** (`options=tf.io.TFRecordOptions(compression_type="GZIP")`) — a good trade-off when data is downloaded over a slow network but read on a fast local machine.

**Structured records** are usually stored using **Protocol Buffers (protobufs)** — Google's compact, efficient, language-neutral binary serialization format. TensorFlow provides the `tf.train.Example` protobuf specifically for ML datasets (essentially a dict of named features, each a list of bytes/floats/int64s), and `tf.io.parse_single_example()` to decode records back into tensors when reading. For sequential/nested data (e.g., a list of tokenized sentences), there's `tf.train.SequenceExample`.

---

## 🧹 Preprocessing the Input Features

Neural nets need numeric, appropriately-scaled inputs — you've seen this since Ch. 2, but a few deep-learning-specific techniques matter a lot here:

### One-hot encoding (low-cardinality categories)
Same idea as Ch. 2, but done with TensorFlow ops so it fits inside a `tf.data` pipeline or a Keras preprocessing layer:
```python
vocab = ["<1H OCEAN", "INLAND", "NEAR OCEAN", "NEAR BAY", "ISLAND"]
indices = tf.range(len(vocab), dtype=tf.int64)
table_init = tf.lookup.KeyValueTensorInitializer(vocab, indices)
num_oov_buckets = 2   # "out-of-vocabulary" buckets catch categories not seen during training
table = tf.lookup.StaticVocabularyTable(table_init, num_oov_buckets)

cat_indices = table.lookup(tf.constant(["NEAR BAY", "DESERT"]))   # DESERT -> an OOV bucket
cat_one_hot = tf.one_hot(cat_indices, depth=len(vocab) + num_oov_buckets)
```
⚠️ One-hot encoding only works well for **low-cardinality** categories (few unique values) — for categories with thousands+ possible values (zip codes, words, user IDs...), it produces huge, mostly-empty vectors. Use embeddings instead.

### Embeddings — the modern default for high-cardinality categories
An **embedding** is a **trainable, dense, low-dimensional vector** representing a category — initialized randomly, then **learned via Gradient Descent right alongside the rest of the network**. As training proceeds, embeddings of *similar* categories (in terms of how they affect the prediction) naturally end up **close together** in the embedding space — the network discovers useful structure on its own. This general idea is called **representation learning** (you'll see it again with autoencoders, Ch. 17).

```python
embedding_dim = 2
embed_init = tf.random.uniform([len(vocab) + num_oov_buckets, embedding_dim])
embedding_matrix = tf.Variable(embed_init)   # a trainable lookup table: one row per category

# In practice, use the Keras layer instead of doing this by hand:
embedding_layer = keras.layers.Embedding(input_dim=len(vocab) + num_oov_buckets,
                                          output_dim=embedding_dim)
```
**Rule of thumb**: embedding dimensionality is typically 10–300, depending on vocabulary size and task complexity — tune it like any hyperparameter.

> 💡 **Word embeddings** are the most famous application of this idea: train a network to predict nearby words in huge text corpora, and the resulting word vectors capture startlingly rich semantic structure — the classic example is $\vec{\text{King}} - \vec{\text{Man}} + \vec{\text{Woman}} \approx \vec{\text{Queen}}$. Pretrained word embeddings can often be reused directly for new NLP tasks (transfer learning again!) — see Ch. 16. ⚠️ Note also: embeddings trained on real-world text can absorb and reproduce **real-world biases** present in that text (e.g., gender stereotypes in occupation associations) — an active area of fairness research, worth being aware of before deploying any embedding-based system.

### Keras Preprocessing Layers
Modern Keras includes built-in layers for common preprocessing steps — the advantage is that they can be **included directly inside the model itself**, so preprocessing logic ships and saves *with* the model (no risk of train/serve mismatch): `Normalization` (standardization), `TextVectorization`, `Discretization`, `StringLookup`/`IntegerLookup` (category-to-index mapping), `CategoryEncoding` (one-hot/multi-hot), and more.

```python
normalization_layer = keras.layers.Normalization()
normalization_layer.adapt(X_train)     # learn the mean/std from training data, like a Scaler's .fit()

model = keras.models.Sequential([
    normalization_layer,
    keras.layers.Dense(30, activation="relu"),
    # ...
])
```

### TF Transform & TF Datasets (TFDS) — worth knowing exist
- **TF Transform (`tft`)**: for large production pipelines, lets you define preprocessing once and run it efficiently over huge datasets (via Apache Beam) at training time — with the *exact same* preprocessing logic exported into the serving graph, eliminating training/serving skew.
- **TensorFlow Datasets (TFDS)**: a library of ready-to-use, standard datasets (MNIST, CIFAR, IMDB reviews, etc.), each downloadable as a `tf.data.Dataset` in one line — great for experimentation and benchmarking.
```python
import tensorflow_datasets as tfds
dataset = tfds.load(name="mnist", batch_size=32, as_supervised=True)
```

---

## ⚠️ Common Pitfalls

- Forgetting to reassign the result of a dataset transformation (`dataset = dataset.shuffle(...)`) — datasets are immutable; the call does nothing useful if you discard the return value.
- Setting the shuffle buffer too small for a large dataset — shuffling looks like it's "working" but is much weaker than it appears.
- Putting `.cache()` **after** `.shuffle()`/`.batch()` instead of right after loading/preprocessing — defeats much of its purpose (or caches a fixed shuffle order across epochs).
- Skipping `.prefetch()` — an easy, nearly-free speedup that many pipelines forget.
- Using one-hot encoding for a high-cardinality categorical feature (e.g., zip code, user ID) instead of an embedding — wastes memory and gives the model little to work with.
- Doing preprocessing *outside* the model (e.g., in pandas) instead of with Keras preprocessing layers when you plan to deploy — risks **training/serving skew**, where production preprocessing subtly differs from training preprocessing.

---

## 🔑 Cheat Sheet

- `tf.data.Dataset`: chainable, lazy pipeline — `.map()` (preprocess), `.filter()`, `.shuffle(buffer_size)`, `.batch()`, `.repeat()`, `.prefetch()`, `.cache()`.
- Typical order: load → preprocess (`map`) → `cache()` (if it fits in RAM) → `shuffle` → `repeat` → `batch` → `prefetch`.
- `.prefetch(1)` (or `AUTOTUNE`) lets the CPU prepare the next batch while the GPU trains on the current one — often a big, easy speedup.
- **TFRecord**: TensorFlow's efficient binary format for large datasets, built on **Protocol Buffers** (`tf.train.Example`).
- **One-hot encoding**: fine for low-cardinality categories. **Embeddings**: trainable dense vectors, the standard choice for high-cardinality categories (and words) — learned jointly with the rest of the network via representation learning.
- **Keras preprocessing layers** (`Normalization`, `TextVectorization`, `StringLookup`, etc.) can live *inside* the model, avoiding training/serving skew.
- `tfds.load(...)` gives one-line access to many standard benchmark datasets.

---

## ✍️ Practice

1. **Hands-on:** Split the California housing dataset into several CSV files. Build a `tf.data` pipeline (`interleave` + `shuffle` + `map` for scaling + `batch` + `prefetch`) and train a Keras regression model directly on the resulting `Dataset`.
2. **Conceptual:** Explain why `.cache()` should generally go *before* `.shuffle()`/`.repeat()` in the pipeline, not after.
3. **Hands-on:** Take a categorical feature with ~500 unique values (or simulate one). Compare model size and training speed using one-hot encoding vs. a `keras.layers.Embedding(output_dim=16)`.
4. **Debug-the-scenario:** Your training loop is fast on paper (`fit()` reports high throughput) but GPU utilization (checked via `nvidia-smi`) sits at 40%. What's the most likely bottleneck, and what's the first thing you'd try?
   <details><summary>Answer</summary>The input pipeline (CPU-side data loading/preprocessing) probably can't keep up with the GPU. Try adding/tuning `.prefetch()`, increasing `num_parallel_calls` on `map()`/`interleave()`, and — if the dataset fits — adding `.cache()`.</details>
5. **Conceptual:** Why is $\vec{\text{King}} - \vec{\text{Man}} + \vec{\text{Woman}} \approx \vec{\text{Queen}}$ such a striking result? What does it suggest about what the embedding space has implicitly learned?

Next: `14_cnn_computer_vision.md` — Convolutional Neural Networks, the architecture that made deep learning dominate computer vision.