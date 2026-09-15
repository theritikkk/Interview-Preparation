import sys
sys.path.insert(0, "/home/claude")
from nb_helper import md, code, make_notebook

OUT = "/mnt/user-data/outputs/ML_Notes/code/part2_deep_learning"

# ============================================================ Chapter 15
cells = [
md("""# Chapter 15 — Processing Sequences Using RNNs and CNNs
Companion notebook to `part2_deep_learning/15_rnn_sequences_time_series.md`. No downloads needed
(synthetic time series)."""),

code("""import numpy as np
import matplotlib.pyplot as plt
import tensorflow as tf
from tensorflow import keras"""),

md("## Generate a synthetic time series"),
code("""def generate_time_series(batch_size, n_steps):
    freq1, freq2, offsets1, offsets2 = np.random.rand(4, batch_size, 1)
    time = np.linspace(0, 1, n_steps)
    series = 0.5 * np.sin((time - offsets1) * (freq1 * 10 + 10))
    series += 0.2 * np.sin((time - offsets2) * (freq2 * 20 + 20))
    series += 0.1 * (np.random.rand(batch_size, n_steps) - 0.5)
    return series[..., np.newaxis].astype(np.float32)

n_steps = 50
series = generate_time_series(10000, n_steps + 1)
X_train, y_train = series[:7000, :n_steps], series[:7000, -1]
X_valid, y_valid = series[7000:9000, :n_steps], series[7000:9000, -1]
X_test, y_test = series[9000:, :n_steps], series[9000:, -1]

plt.plot(series[0])
plt.title("One example time series")
plt.show()"""),

md("## Baseline: naive forecast & a plain linear model"),
code("""y_pred_naive = X_valid[:, -1]
naive_mse = np.mean((y_pred_naive - y_valid) ** 2)
print("Naive forecast MSE:", naive_mse)

linear_model = keras.models.Sequential([
    keras.layers.Flatten(input_shape=[n_steps, 1]),
    keras.layers.Dense(1),
])
linear_model.compile(loss="mse", optimizer="adam")
linear_model.fit(X_train, y_train, epochs=10, validation_data=(X_valid, y_valid), verbose=0)
print("Linear model MSE:", linear_model.evaluate(X_valid, y_valid))"""),

md("## SimpleRNN vs LSTM vs GRU"),
code("""def build_and_eval(cell_type):
    Cell = {"SimpleRNN": keras.layers.SimpleRNN, "LSTM": keras.layers.LSTM, "GRU": keras.layers.GRU}[cell_type]
    model = keras.models.Sequential([
        Cell(20, return_sequences=True, input_shape=[None, 1]),
        Cell(20),
        keras.layers.Dense(1),
    ])
    model.compile(loss="mse", optimizer="adam")
    model.fit(X_train, y_train, epochs=10, validation_data=(X_valid, y_valid), verbose=0)
    mse = model.evaluate(X_valid, y_valid, verbose=0)
    print(f"{cell_type:10s} validation MSE = {mse:.5f}")
    return model

for cell_type in ["SimpleRNN", "LSTM", "GRU"]:
    build_and_eval(cell_type)"""),

md("""## ✍️ Practice
1. Compare naive baseline, linear, SimpleRNN, LSTM, GRU MSE above — which wins, by how much?
2. In your own words, explain what each of the LSTM's three gates is "deciding."
3. Try forecasting 10 steps ahead instead of 1 (either by feeding predictions back in, or training a multi-output model).
4. Debug: SimpleRNN performs well on 10-step sequences but badly on 200-step sequences even with more units — what's the likely cause, and the fix?"""),
code("# Your code / notes here\n"),
]
make_notebook(cells, f"{OUT}/15_rnn_sequences_time_series.ipynb")

# ============================================================ Chapter 16
cells = [
md("""# Chapter 16 — Natural Language Processing with RNNs and Attention
Companion notebook to `part2_deep_learning/16_nlp_attention_transformers.md`.
Downloads the IMDB dataset the first time (via Keras)."""),

code("""import numpy as np
import tensorflow as tf
from tensorflow import keras"""),

md("## Sentiment analysis: Embedding + GRU on IMDB reviews"),
code("""vocab_size = 10000
(X_train, y_train), (X_test, y_test) = keras.datasets.imdb.load_data(num_words=vocab_size)

X_train = keras.preprocessing.sequence.pad_sequences(X_train, maxlen=200)
X_test = keras.preprocessing.sequence.pad_sequences(X_test, maxlen=200)

model = keras.models.Sequential([
    keras.layers.Embedding(vocab_size, 32, input_shape=[None], mask_zero=True),
    keras.layers.GRU(32, return_sequences=True),
    keras.layers.GRU(32),
    keras.layers.Dense(1, activation="sigmoid"),
])
model.compile(loss="binary_crossentropy", optimizer="adam", metrics=["accuracy"])
history = model.fit(X_train, y_train, epochs=3, validation_split=0.2, batch_size=64)
print("Test accuracy:", model.evaluate(X_test, y_test)[1])"""),

md("## A minimal self-attention block (the core of the Transformer)"),
code("""# keras.layers.MultiHeadAttention implements Scaled Dot-Product Attention with multiple heads.
seq_len, d_model, num_heads = 10, 32, 4

sample_input = tf.random.normal([2, seq_len, d_model])   # (batch, seq_len, d_model)

mha = keras.layers.MultiHeadAttention(num_heads=num_heads, key_dim=d_model // num_heads)
# self-attention: query, value, and key are all the same sequence
attn_output = mha(query=sample_input, value=sample_input, key=sample_input)
print("Self-attention output shape:", attn_output.shape)   # same shape as input"""),

md("## Positional encoding (sine/cosine), so attention knows word order"),
code("""def positional_encoding(max_len, d_model):
    positions = np.arange(max_len)[:, np.newaxis]
    dims = np.arange(d_model)[np.newaxis, :]
    angle_rates = 1 / np.power(10000, (2 * (dims // 2)) / np.float32(d_model))
    angle_rads = positions * angle_rates
    angle_rads[:, 0::2] = np.sin(angle_rads[:, 0::2])
    angle_rads[:, 1::2] = np.cos(angle_rads[:, 1::2])
    return tf.cast(angle_rads, dtype=tf.float32)

pe = positional_encoding(50, 32)
import matplotlib.pyplot as plt
plt.pcolormesh(pe.numpy(), cmap="RdBu")
plt.xlabel("Depth"); plt.ylabel("Position"); plt.colorbar()
plt.title("Positional encoding")
plt.show()"""),

md("""## ✍️ Practice
1. Explain why a plain Encoder-Decoder (no attention) struggles more with long sentences than short ones.
2. Above: try a Bidirectional wrapper (`keras.layers.Bidirectional(keras.layers.GRU(32))`) — does accuracy improve?
3. Debug: a Transformer decoder without a causal mask trains with suspiciously great loss but generates poorly — why?
4. Why does the Transformer need positional encoding when RNNs never needed anything like it?"""),
code("# Your code / notes here\n"),
]
make_notebook(cells, f"{OUT}/16_nlp_attention_transformers.ipynb")

print("Chapters 15-16 done")