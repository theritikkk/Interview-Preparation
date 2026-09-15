# Chapter 19 — Training and Deploying TensorFlow Models at Scale

## 🎯 Why this chapter matters
A trained model sitting in a Jupyter notebook delivers **zero business value**. This final chapter closes the loop: how to **serve** predictions reliably, **shrink** models for phones/embedded devices, and **scale training** across multiple GPUs or entire clusters when one machine isn't enough.

---

## 🚀 Serving a TensorFlow Model

For quick internal use, `model.predict()` is fine. But at real scale, you want a dedicated **model server** — decoupling the model from the rest of your infrastructure so you can version it, scale it independently, A/B test it, and guarantee every service uses a consistent model version.

### Exporting to SavedModel format
```python
model_version = "0001"
model_path = os.path.join("my_mnist_model", model_version)
tf.saved_model.save(model, model_path)
# equivalently: model.save(model_path)   — as long as the path doesn't end in .h5
```
A SavedModel directory bundles the **computation graph** (`saved_model.pb`) and the **variable values** (weights) — everything needed to run the model with no Python training code required.

> 💡 **Best practice**: bundle your preprocessing steps (scaling, tokenizing, etc.) **inside** the exported model itself (e.g., using Keras preprocessing layers from Ch. 13) so production only ever needs to send raw data — this avoids **training/serving skew**, a subtle and common source of production bugs.

⚠️ SavedModel only works for models expressible as a pure TensorFlow computation graph — this **excludes** dynamic Subclassing-API models (Ch. 10) and any use of `tf.py_function` (arbitrary Python code) — those need a different serving approach (e.g., a Flask app).

### TensorFlow Serving (TF Serving)
A production-grade, C++, high-performance model server that can host multiple models/versions simultaneously and **auto-detect new model versions** dropped into a watched directory — zero-downtime updates. Typically run via Docker, and queried through either:
- **REST API**: simple, HTTP + JSON, easy to debug, less efficient for large payloads.
- **gRPC API**: more complex, more efficient (binary protobufs) — better for high-throughput/low-latency needs.

```python
import requests
response = requests.post(
    "http://localhost:8501/v1/models/my_mnist_model:predict",
    data=json.dumps({"signature_name": "serving_default", "instances": X_new.tolist()}))
y_proba = np.array(response.json()["predictions"])
```

### Deploying on a managed cloud platform
Cloud ML platforms (e.g., Google Cloud AI Platform, AWS SageMaker) handle hosting, autoscaling, and versioning for you — you upload the SavedModel, and the platform exposes a prediction API, scaling replicas up/down with traffic automatically. This trades some cost/control for far less operational burden than self-hosting TF Serving.

---

## 📱 Deploying to Mobile and Embedded Devices — TFLite

Big models are impractical on phones/microcontrollers: slow to download, RAM-hungry, drains battery, causes lag. **TFLite** (TensorFlow Lite) addresses this with three goals: **shrink the model**, **reduce computation per prediction**, and **adapt to device-specific constraints**.

```python
converter = tf.lite.TFLiteConverter.from_saved_model(saved_model_path)
tflite_model = converter.convert()
with open("converted_model.tflite", "wb") as f:
    f.write(tflite_model)
```
The converter automatically **prunes** unneeded (e.g., training-only) operations, **fuses** compatible operations (folding Batch Norm into the preceding layer, simplifying arithmetic), and packages everything into the compact **FlatBuffers** format — loadable straight into RAM, no parsing overhead.

### Quantization — the biggest size/speed lever
- **Reduced float precision**: 16-bit floats instead of 32-bit halves model size, speeds up training/inference, at a usually-small accuracy cost.
- **Post-training quantization to 8-bit integers**: maps the float weight range to the integer range [-127, 127] — roughly a **4× size reduction** versus 32-bit floats. Simple and effective for shrinking download/storage size, though by default weights are converted back to floats at runtime (no compute-speed benefit from this alone).
- **Full integer quantization (weights *and* activations)**: needed for real latency/power savings, and **mandatory** for some accelerator hardware (e.g., Edge TPUs) that can only process integers. Requires a brief **calibration step** on representative sample data to measure activation ranges.
- ⚠️ **Quantization always adds some noise** (equivalent to injecting noise into weights/activations) — if the accuracy hit is too large, use **quantization-aware training**: simulate quantization *during* training so the network learns weights that are naturally robust to it.

### TensorFlow.js — running models in the browser
Lets predictions happen **client-side**, in JavaScript — useful when connectivity is unreliable, when latency must be near-zero (e.g., games), or when privacy requires user data to **never leave their device**. Convert a SavedModel/Keras model with `tensorflowjs_converter`, then load it via the `@tensorflow/tfjs` JS library.

---

## ⚡ Using GPUs to Speed Up Computation

Even with Chapter 11's training speedups, a large network can take **days to weeks** to train on CPU alone. GPUs (and TPUs) massively parallelize the matrix operations neural nets are built from.

**Getting GPU access**: buy your own card (cost/setup effort, but no ongoing rental cost, good for heavy sustained use), or rent a **GPU-equipped cloud VM** (pay-as-you-go, easy to scale up/down), or use **Google Colab** (free/cheap GPU access, zero setup — great for learning and experimentation, this book's own recommendation for readers without local GPUs).

**Managing GPU RAM**: by default, TensorFlow grabs *all* available GPU RAM at startup — set `tf.config.experimental.set_memory_growth(gpu, True)` to instead let it grow allocation as needed, letting you run multiple processes on one GPU. You can also explicitly assign operations/variables to specific devices with `tf.device("/gpu:0")`.

---

## 🖧 Training Across Multiple Devices

When a single GPU isn't enough, you have two fundamentally different strategies:

### Model Parallelism — split the *model* across devices
⚠️ **Trickier than it sounds, and often disappointing.** Splitting a fully-connected network by placing different layers (or halves of layers) on different devices forces heavy **cross-device communication** at every step (later layers need output from *both* halves) — this communication overhead frequently cancels out any parallelism benefit, especially across separate machines (slow interconnects). Some architectures suit it better:
- **CNNs** (Ch. 14) have layers only partially connected to previous ones, making a clean device split more natural.
- **Deep RNNs** (Ch. 15) split more efficiently horizontally (layer-per-device) — but in practice, a plain LSTM stack on a *single* GPU often still runs faster overall once communication overhead is accounted for.

> 🔑 **Rule of thumb**: reach for model parallelism only when a model genuinely can't fit on one device, or for specific architectures known to split well — it needs a lot of care and tuning to pay off.

### Data Parallelism — the generally-preferred, simpler approach
**Replicate the entire model** on every device; each replica trains on a **different mini-batch simultaneously**; then **average the gradients** across replicas and apply the update everywhere. Two main flavors:

- **Mirrored strategy**: every device keeps an identical, fully-synced copy of all parameters. Averaging gradients across devices efficiently is done via an **AllReduce** algorithm (multiple nodes collaboratively compute a reduction like "mean," ending with every node holding the same result) — NVIDIA's **NCCL** library is a common efficient implementation. This is TensorFlow's default, most common approach on a single multi-GPU machine.
- **Centralized parameters**: store the master copy of parameters on a central location (e.g., CPU), with GPU "workers" pulling parameters, computing gradients on their mini-batch, and pushing gradients back. Can be **synchronous** (wait for every worker each step — no stale gradients, but speed limited by the slowest worker/"straggler") or **asynchronous** (apply updates as they arrive, no waiting — faster throughput, but risks **stale gradients**, where a worker computes gradients based on now-outdated parameters, which can actually *slow convergence* despite the raw speed gain).
- ⚠️ **Bandwidth saturation**: at some point, adding more GPUs no longer helps because the time spent **communicating** gradients between devices exceeds the time saved by parallel computation — a real, practical ceiling, especially pronounced for large, dense models (large models with sparser gradients scale better).

### The Distribution Strategies API — TensorFlow makes this easy
```python
distribution = tf.distribute.MirroredStrategy()

with distribution.scope():
    model = keras.models.Sequential([...])
    model.compile([...])

model.fit(X_train, y_train, epochs=10)   # batch_size must be divisible by the number of replicas
```
That's genuinely (almost) the entire code change — Keras automatically becomes "distribution-aware" inside the `scope()` context, replicating variables/operations across every available GPU. Swap `MirroredStrategy` for `tf.distribute.experimental.CentralStorageStrategy()` to use centralized parameters instead.

**Scaling further**: if a single machine's GPUs aren't enough, TensorFlow supports training across an entire **cluster** of machines (a "TensorFlow cluster"), and cloud platforms like **Google Cloud AI Platform** let you launch large distributed training jobs (and even run **black-box hyperparameter tuning** at scale) without managing the cluster infrastructure yourself.

---

## ⚠️ Common Pitfalls

- Deploying a model without bundling preprocessing into it — invites training/serving skew when the serving application's preprocessing code inevitably drifts from the training pipeline's.
- Using a Subclassing-API (dynamic) Keras model and expecting to export it as a SavedModel — it isn't supported; use Sequential/Functional models for anything you plan to serve this way.
- Quantizing a model aggressively without checking the resulting accuracy drop — always validate on a held-out set after conversion.
- Choosing model parallelism by default "because it sounds more powerful" — for most fully-connected architectures, data parallelism is simpler and faster in practice.
- Setting a training batch size that isn't evenly divisible by the number of replicas when using `MirroredStrategy`.
- Assuming more GPUs always means proportionally faster training — bandwidth saturation and straggler effects mean returns diminish, sometimes sharply.
- Forgetting to re-enter a `distribution.scope()` when *loading* a saved mirrored model if you want it to run across multiple devices again.

---

## 🔑 Cheat Sheet

- **SavedModel** = portable format (graph + weights) exported via `tf.saved_model.save()` / `model.save()`; bundle preprocessing inside to avoid train/serve skew.
- **TF Serving**: production model server, supports multi-version hosting, REST (simple) and gRPC (efficient) APIs. Managed cloud ML platforms trade control for less ops burden.
- **TFLite**: shrinks/optimizes models for mobile/embedded via FlatBuffers + pruning/fusing + **quantization** (16-bit floats, post-training 8-bit int, or full int quantization for accelerator hardware); use quantization-aware training if accuracy drops too much.
- **TensorFlow.js**: run models client-side in the browser — good for offline use, low latency, and privacy.
- **Model parallelism**: split the model across devices — tricky, heavy cross-device communication, only clearly wins for specific architectures (CNNs, sometimes RNNs).
- **Data parallelism**: replicate the model, split the data, average gradients — generally the simpler, more broadly effective default. **Mirrored strategy** (synced replicas, AllReduce) vs. **centralized parameters** (sync = no staleness but straggler-limited; async = faster but risks stale gradients).
- `tf.distribute.MirroredStrategy()` + `with strategy.scope():` = the easy, idiomatic way to multi-GPU-train a Keras model.
- Watch out for **bandwidth saturation** — communication overhead eventually limits how much parallelism actually helps.

---

## ✍️ Practice

1. **Hands-on:** Export a trained Keras MNIST model as a SavedModel. Inspect it with `saved_model_cli show --dir ... --all`. What are the input/output tensor names and shapes?
2. **Conceptual:** Explain why bundling preprocessing logic *inside* an exported model reduces the risk of training/serving skew, compared to reimplementing preprocessing separately in the serving application.
3. **Hands-on:** Convert the same model to TFLite with post-training 8-bit quantization. Compare file size and test-set accuracy before and after quantization.
4. **Debug-the-scenario:** You add a second GPU to your training job expecting a ~2× speedup, but you only observe a 1.3× speedup. What's the most likely explanation, and is this surprising?
   <details><summary>Answer</summary>Not surprising — this is a normal symptom of communication overhead (gradient averaging across devices) eating into the theoretical speedup; as more devices are added, this "bandwidth saturation" effect tends to grow, so returns diminish.</details>
5. **Conceptual:** Why is a purely fully-connected network usually a poor candidate for model parallelism, while a CNN is comparatively more amenable to it?

---

🎉 **Congratulations — you've completed all 19 chapters!** You now have a working map of the entire modern ML/DL toolkit: from Linear Regression to Transformers, from a single scikit-learn `.fit()` call to distributed training across a GPU cluster. The `ml_project_checklist.md` and `glossary_cheatsheet.md` files round out this note set as quick references for your next real project — go build something!