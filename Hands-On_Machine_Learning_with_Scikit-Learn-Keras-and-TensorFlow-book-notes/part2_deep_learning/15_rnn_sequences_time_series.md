# Chapter 15 — Processing Sequences Using RNNs and CNNs

## 🎯 Why this chapter matters
Some data has an inherent **order** — stock prices over time, sensor readings, sentences (a sequence of words). **Recurrent Neural Networks (RNNs)** are built with a "memory" of what came before, letting them model sequences of *any* length. This chapter also sets up the vocabulary (encoder-decoder, LSTM, GRU) you'll need for Chapter 16's NLP and Transformers.

---

## 🧠 Recurrent Neurons and Layers

A **recurrent neuron** feeds its own output from the previous time step back into itself as an extra input:

$$
\mathbf{y}^{(t)} = \phi\left(\mathbf{W}_x^\top\mathbf{x}^{(t)} + \mathbf{W}_y^\top\mathbf{y}^{(t-1)} + \mathbf{b}\right)
$$

Since $\mathbf{y}^{(t)}$ depends on $\mathbf{y}^{(t-1)}$, which depends on $\mathbf{y}^{(t-2)}$, and so on — the neuron's output at any time step is effectively a function of **every input it has seen so far**. This gives it a form of **memory** — the general term for any network component that preserves state across time steps is a **memory cell**.

**Visualizing this**: "unrolling the network through time" — draw the same neuron/layer once per time step, with arrows showing information flowing forward across steps.

### Input/output sequence patterns (memorize these four)

| Pattern | Use case |
|---|---|
| **Sequence → Sequence** | Forecast a time series (feed last N days, predict next N days shifted by one) |
| **Sequence → Vector** | Sentiment analysis (feed a sentence word by word, output one score at the end) — ignore all outputs except the last |
| **Vector → Sequence** | Image captioning (feed a fixed input, e.g. a CNN's image features, generate a sequence of words) |
| **Encoder–Decoder** (Seq→Vector, then Vector→Seq) | Machine translation — read the *whole* source sentence into one vector (encoder) before generating the translation (decoder). Works much better than translating "on the fly," since the end of the source sentence can affect how you should translate the beginning. |

### Training: Backpropagation Through Time (BPTT)
Simple idea: **unroll the network through time**, then run ordinary backpropagation on the unrolled graph. Gradients flow backward through every output that's part of the cost function, and since the *same* weights are reused at every time step, their gradients get **summed** across all time steps. Keras handles all of this automatically.

---

## 📈 Forecasting a Time Series (a worked example)

```python
model = keras.models.Sequential([
    keras.layers.SimpleRNN(1, input_shape=[None, 1]),   # None = works with any sequence length!
])
```
Note `input_shape=[None, 1]`: RNNs can process sequences of **any length**, unlike Dense/Conv layers — you don't need to fix the sequence length in advance.

⚠️ **A single-neuron `SimpleRNN` is usually too weak** — it beats a naive "predict the last value" baseline, but typically loses to even a plain linear model, because it has very few parameters (one per input + one per hidden dimension, vs. a linear model's one parameter per time step). Stack more layers/units for real performance:

```python
model = keras.models.Sequential([
    keras.layers.SimpleRNN(20, return_sequences=True, input_shape=[None, 1]),
    keras.layers.SimpleRNN(20, return_sequences=True),
    keras.layers.SimpleRNN(1),
])
```
⚠️ **`return_sequences=True`** is essential on every recurrent layer *except the last* in a stack — otherwise intermediate layers only pass along their *final* output instead of the full sequence the next recurrent layer needs.

**Forecasting several steps ahead**: either (a) predict one step, feed that prediction back in as if it were real data, and repeat (errors can accumulate over many steps), or (b) train the network to directly output *all* future steps at once (generally more accurate for medium-length forecasts).

---

## 🌪️ Handling Long Sequences — two persistent problems

### Problem 1: Unstable gradients (worse here than in plain deep nets)
The *same* weights are reused at every time step, so if Gradient Descent nudges the output up slightly, that nudge compounds across every subsequent step — a recipe for exploding (or vanishing) values. Some Chapter 11 fixes transfer directly; some don't:
- **Nonsaturating activations (ReLU) can actually make RNNs *more* unstable** here (unlike in deep feedforward nets) — this is why **tanh remains the default** activation for RNNs.
- **Gradient Clipping** helps directly with exploding gradients.
- **Batch Normalization doesn't work well *within* an RNN's time steps** (only somewhat helpful *between* stacked recurrent layers) — instead, **Layer Normalization** (normalizing across the *feature* dimension instead of the batch dimension, computed independently per instance at each time step) tends to work much better for RNNs.

### Problem 2: Short-term memory
Information from early time steps gradually gets **diluted** as it passes through many transformations — by the time a long sequence ends, the network may have "forgotten" how it started. (Like the movie character Dory, who can't remember the beginning of a sentence by the time she finishes reading it!) This motivated smarter cell designs with genuine **long-term memory**.

---

## 🧠 LSTM — Long Short-Term Memory cells

The modern default recurrent cell, and one of deep learning's great success stories. An LSTM splits its state into two parts: $\mathbf{h}^{(t)}$ (**short-term state**) and $\mathbf{c}^{(t)}$ (**long-term state**), and uses three learned **gates** (each a small neural layer with a sigmoid output between 0 and 1, acting like a dial from "fully closed" to "fully open") to control the flow of information:

- **Forget gate**: decides what to *erase* from the long-term state.
- **Input gate**: decides what *new* information to *add* to the long-term state.
- **Output gate**: decides what part of the long-term state to *output* at this time step (becoming the new short-term state / the cell's output).

$$
\mathbf{c}^{(t)} = \mathbf{f}^{(t)} \otimes \mathbf{c}^{(t-1)} + \mathbf{i}^{(t)} \otimes \mathbf{g}^{(t)}, \qquad \mathbf{y}^{(t)} = \mathbf{h}^{(t)} = \mathbf{o}^{(t)} \otimes \tanh(\mathbf{c}^{(t)})
$$

**Intuition:** the long-term state $\mathbf{c}^{(t)}$ acts like a **conveyor belt** running through the whole sequence — information can be dropped onto it, ride along mostly unchanged, and be read off much later, largely bypassing the "dilution" problem that plagues simple RNN cells. This lets LSTMs learn dependencies spanning far more time steps than a basic cell can.

```python
model = keras.models.Sequential([
    keras.layers.LSTM(20, return_sequences=True, input_shape=[None, 1]),
    keras.layers.LSTM(20, return_sequences=True),
    keras.layers.TimeDistributed(keras.layers.Dense(10)),   # applies the same Dense layer at every time step
])
```
*(Variant: **peephole connections** let the gates also see the long-term state directly — sometimes helps, sometimes doesn't; worth trying if you have time to experiment.)*

### GRU — a simpler, popular LSTM alternative
The **Gated Recurrent Unit** simplifies LSTM: it merges the short-/long-term states into one vector, and uses only two gates (an update gate and a reset gate) instead of three. It's computationally cheaper and often performs comparably to LSTM.

```python
model = keras.models.Sequential([
    keras.layers.GRU(20, return_sequences=True, input_shape=[None, 1]),
    keras.layers.GRU(20, return_sequences=True),
    keras.layers.TimeDistributed(keras.layers.Dense(10)),
])
```

> 🔑 In practice: **start with GRU or LSTM, not plain `SimpleRNN`**, for anything beyond toy problems — basic cells are rarely used anymore precisely because of the short-term memory problem.

---

## 🌊 WaveNet — an alternative: 1D convolutions for sequences

Instead of recurrence, WaveNet stacks **1D convolutional layers**, with each successive layer's **dilation rate doubling** (skip every 1, then every 2, then every 4 inputs, etc.) — this lets the receptive field grow **exponentially** with depth, letting the network learn extremely long-range patterns using only a handful of layers, and (unlike RNNs) can process the whole sequence **in parallel** rather than step by step, making it much faster to train.

```python
model = keras.models.Sequential()
model.add(keras.layers.InputLayer(input_shape=[None, 1]))
for rate in (1, 2, 4, 8) * 2:
    model.add(keras.layers.Conv1D(filters=20, kernel_size=2, padding="causal",
                                   activation="relu", dilation_rate=rate))
model.add(keras.layers.Conv1D(filters=10, kernel_size=1))
```
`padding="causal"` ensures the layer only looks at *past* inputs, never future ones — essential for realistic forecasting (you can't peek at data you don't have yet in a real deployment).

---

## ⚠️ Common Pitfalls

- Forgetting `return_sequences=True` on all-but-the-last recurrent layer in a stack — silently breaks the intended sequence-to-sequence flow between layers.
- Using ReLU in an RNN and being surprised training is *less* stable than expected — tanh is the safer default here.
- Expecting `SimpleRNN` to handle long sequences well — its short-term memory problem makes LSTM/GRU the practical default for anything nontrivial.
- Applying Batch Normalization the same way inside an RNN as in a CNN/MLP — it doesn't transfer well across time steps; prefer Layer Normalization.
- Forgetting `padding="causal"` in a WaveNet-style model — without it, the model can "cheat" by looking at future time steps during training, which won't be available at real inference/deployment time.
- Feeding raw, unscaled time series values directly into an RNN — scale/normalize just like any other neural net input.

---

## 🔑 Cheat Sheet

- RNN = a network with a feedback loop; "memory cell" = any component preserving state across time steps.
- Four sequence I/O patterns: seq→seq, seq→vector, vector→seq, encoder-decoder (seq→vector→seq).
- Training = **Backpropagation Through Time** (unroll, then standard backprop).
- RNNs suffer *more* from unstable gradients than deep feedforward nets (tanh preferred over ReLU; use Gradient Clipping; Layer Norm > Batch Norm here) and from **short-term memory** (basic cells forget early inputs).
- **LSTM**: long-term state ($\mathbf{c}$) + short-term state ($\mathbf{h}$) + forget/input/output gates — the standard choice for capturing long-range dependencies.
- **GRU**: simpler, cheaper LSTM variant, often comparable performance.
- **WaveNet**: dilated causal 1D convolutions — parallelizable alternative to recurrence, with exponentially growing receptive field.
- `return_sequences=True` on every stacked recurrent layer except the last; `TimeDistributed(Dense(...))` to apply the same output layer at every time step.

---

## ✍️ Practice

1. **Hands-on:** Generate a synthetic sine-wave-plus-noise time series. Compare forecasting MSE across a naive baseline (predict last value), a linear model, a `SimpleRNN`, and an `LSTM`. Which wins, and by how much?
2. **Conceptual:** Explain, in plain language, what each of the LSTM's three gates is "deciding," using the analogy of a notebook you're allowed to write new notes in, erase old notes from, and selectively read from.
3. **Hands-on:** Build a sequence-to-vector RNN (LSTM or GRU) for sentiment classification on a small text dataset (e.g., IMDB reviews via `tensorflow_datasets`). Report validation accuracy.
4. **Debug-the-scenario:** Your `SimpleRNN`-based model performs well on short sequences (10 steps) but badly on long ones (200 steps), even after adding more units. What's the most likely explanation, and what's the first thing you'd try?
   <details><summary>Answer</summary>The short-term memory problem — basic RNN cells struggle to retain information over many steps. Switch to `LSTM` or `GRU` cells, which are specifically designed to preserve long-range dependencies.</details>
5. **Conceptual:** Why does WaveNet's *dilated* convolution approach let it capture long-range dependencies with far fewer layers than a plain (non-dilated) 1D CNN would need?

Next: `16_nlp_attention_transformers.md` — apply (and go beyond) these sequence tools for natural language: text generation, sentiment analysis, translation, and the Attention mechanism that led to Transformers.