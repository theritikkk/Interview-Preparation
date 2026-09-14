# Chapter 16 — Natural Language Processing with RNNs and Attention

## 🎯 Why this chapter matters
This chapter takes Chapter 15's sequence tools and applies them to **text** — arguably the most impactful application of deep learning in the last decade. You'll build a text-generating Char-RNN, a sentiment classifier, a machine-translation Encoder-Decoder, and then meet **Attention** and the **Transformer** — the architecture behind GPT, BERT, and essentially every modern large language model.

---

## ✍️ Generating Text with a Character RNN

**Idea:** train an RNN to predict the **next character**, given the preceding ones. Train on all of Shakespeare's work, and the model learns spelling, grammar, and style well enough to generate plausible (if nonsensical) new "Shakespeare."

```python
tokenizer = keras.preprocessing.text.Tokenizer(char_level=True)
tokenizer.fit_on_texts([shakespeare_text])
[encoded] = np.array(tokenizer.texts_to_sequences([shakespeare_text])) - 1   # IDs from 0
```
**Preparing training windows**: chop the (very long) encoded text into many overlapping fixed-length windows (e.g., 100 characters), where the target for each window is the *same window shifted one character to the right* — i.e., the network always predicts "what comes next."

```python
model = keras.models.Sequential([
    keras.layers.GRU(128, return_sequences=True, input_shape=[None, max_id], dropout=0.2, recurrent_dropout=0.2),
    keras.layers.GRU(128, return_sequences=True, dropout=0.2, recurrent_dropout=0.2),
    keras.layers.TimeDistributed(keras.layers.Dense(max_id, activation="softmax")),
])
```
**Generating new text**: feed a seed string, sample the next character from the predicted probability distribution (a `temperature` parameter controls randomness — low temperature → safe/repetitive, high temperature → more creative/error-prone), append it, and repeat.

**Stateful RNN**: normally each training batch starts with a hidden state reset to zero. A **stateful RNN** *preserves* the hidden state between batches — letting it in principle learn patterns longer than one window — but requires careful, non-shuffled, sequentially-ordered batches, and only works within one continuous sequence at a time (more setup complexity for a real, but modest, potential benefit).

---

## 😊 Sentiment Analysis (Sequence → Vector)

The IMDb movie review dataset (50,000 labeled positive/negative reviews) is the "hello world" of NLP. Keras provides it **pre-tokenized**: each review is a list of integers, one per word, indexed by frequency.

```python
(X_train, y_train), (X_test, y_test) = keras.datasets.imdb.load_data()
```
**In real projects, you preprocess text yourself.** Word-level tokenization via spaces works for English but breaks for languages without spaces (Chinese) or with different conventions (German compound words). A more robust, language-independent approach is **subword tokenization** (e.g., SentencePiece) — it can reasonably handle words it's never seen by decomposing them into known sub-pieces (e.g., "smartest" → "smart" + "est").

```python
model = keras.models.Sequential([
    keras.layers.Embedding(vocab_size, embed_size, input_shape=[None]),   # learn word representations
    keras.layers.GRU(128, return_sequences=True),
    keras.layers.GRU(128),
    keras.layers.Dense(1, activation="sigmoid"),   # binary: positive/negative
])
```
### Masking — ignoring padding
Since sentences have different lengths, batches get **padded** with a filler token (usually 0) up to the longest sentence in the batch — but the model shouldn't "learn" anything from padding. Set `mask_zero=True` on the `Embedding` layer, and Keras automatically propagates a mask through subsequent layers that support it, so padded positions are simply skipped.

### Reusing pretrained embeddings/language models
Just like Chapter 11/14's transfer learning, you rarely need to learn word representations from scratch — reuse **pretrained embeddings** (e.g., trained on huge text corpora) or even entire **pretrained language models** (see "Recent Innovations" below) and fine-tune on your smaller labeled dataset.

---

## 🌐 Encoder–Decoder for Neural Machine Translation (NMT)

Recall Ch. 15: feed the **encoder** the source sentence (word by word), producing a final "summary" hidden state; feed that state (plus a start token) to the **decoder**, which generates the translation one word at a time, feeding each output word back in as the next input (**teacher forcing** during training: feed the *true* previous target word, not the model's own possibly-wrong prediction, so errors don't compound during learning).

**Improvements to the basic Encoder-Decoder:**
- **Bidirectional RNNs**: for the *encoder* (which sees the whole input sentence upfront, unlike the decoder which must generate sequentially), run one RNN forward and another backward over the sequence, then combine their outputs at each step — captures context from *both* directions, typically boosting NMT performance.
- **Beam search**: instead of greedily picking the single most probable next word at each decoder step (which can commit to an early mistake that dooms the whole sentence), track the top-$k$ most probable partial sequences ("beams") at each step, extending each and keeping only the best $k$ overall — usually produces noticeably better full-sentence translations than greedy decoding.

---

## 🎯 Attention Mechanisms — the breakthrough idea

**The problem attention solves:** in a plain Encoder-Decoder, the *entire* source sentence gets compressed into **one fixed-size vector** — a serious bottleneck for long sentences (by the time the decoder needs an early word's information, it's traveled through many steps and gotten diluted — Chapter 15's short-term memory problem again).

**The fix (Bahdanau, 2014):** instead of only passing the encoder's *final* state to the decoder, pass **all** of the encoder's per-word outputs. At each decoder step, compute a **weighted sum** of all encoder outputs — the weights $\alpha^{(t,i)}$ say how much to "attend to" input word $i$ while producing output word $t$. This dramatically **shortens the effective path** from any input word to its corresponding output word, largely sidestepping the short-term-memory limitation.

$$
\tilde{\mathbf{y}}^{(t)} = \sum_i \alpha^{(t,i)} \mathbf{y}_{(i)}, \qquad \alpha^{(t,i)} = \frac{\exp(e^{(t,i)})}{\sum_{i'}\exp(e^{(t,i')})}
$$

where $e^{(t,i)}$ is a **similarity score** between the decoder's current state and encoder output $i$ — computed either by a small trained neural net (**Bahdanau/"concatenative" attention**) or, more simply and now more commonly, by a plain **dot product** (**Luong/"multiplicative" attention** — faster, and works about as well or better).

> 🔑 **Intuition**: attention lets the decoder "look back" at the *specific* source words most relevant to what it's generating right now, instead of relying on everything being squeezed through one bottleneck vector — much like how a human translator re-reads the specific part of the source sentence relevant to the phrase they're currently translating.

---

## 🏛️ The Transformer — "Attention Is All You Need" (2017)

A landmark architecture that dropped recurrence and convolution **entirely**, relying purely on attention (plus embeddings, Dense layers, and normalization) — and it trains **faster** (fully parallelizable across time steps, unlike RNNs which must process sequentially) while achieving **better** results.

### Key components

**Self-Attention**: instead of only the decoder attending to the encoder, each word in a sentence attends to **every other word in the same sentence** — e.g., in "They welcomed the Queen of the United Kingdom," the representation of "Queen" ends up shaped heavily by "United" and "Kingdom," much more than by "They" or "welcomed." This lets the model build rich, context-aware word representations in one step.

**Scaled Dot-Product Attention** — the mathematical core, framed as a **differentiable dictionary lookup**: given a **Query** (what am I looking for?), compare it against a set of **Keys** (what's available?) via dot product, softmax the similarity scores into weights, then return a weighted sum of the corresponding **Values**:

$$
\text{Attention}(Q, K, V) = \text{softmax}\left(\frac{QK^\top}{\sqrt{d_{\text{keys}}}}\right)V
$$

The $\sqrt{d_{\text{keys}}}$ scaling factor prevents the dot products from growing too large and pushing softmax into a near-zero-gradient regime.

**Multi-Head Attention**: instead of computing attention just once, apply *several* different learned linear projections of Q/K/V in parallel ("heads"), run scaled dot-product attention on each, then concatenate and project the results. **Why multiple heads?** A word's representation encodes many characteristics at once (part of speech, tense, semantic role...) — one attention operation can only "query" along one such angle at a time; multiple heads let the model attend to *different aspects* of the relationships between words simultaneously (e.g., one head might specialize in syntactic role, another in tense agreement).

**Positional Encoding**: since attention has **no inherent sense of word order** (it just looks at relationships, not sequence position), the Transformer adds a **positional embedding** to each word embedding — a vector uniquely encoding that word's position, built from sine/cosine functions at different frequencies:

$$
P_{p,2i} = \sin(p/10000^{2i/d}), \qquad P_{p,2i+1} = \cos(p/10000^{2i/d})
$$

This fixed (not learned) scheme generalizes to sentence lengths never seen during training, and its oscillating structure lets the model infer **relative** positions too, not just absolute ones.

### Full architecture (high level)
- **Encoder**: stack of $N$ identical blocks, each containing a Multi-Head Self-Attention layer + a Feed-Forward block, with skip connections and Layer Normalization around each (same residual-learning idea as ResNet, Ch. 14).
- **Decoder**: similar stack, but each block has *three* sublayers: **Masked** Multi-Head Self-Attention (a word can only attend to *earlier* words in the target — it mustn't "cheat" by seeing future words it hasn't generated yet), then Multi-Head Attention over the *encoder's* output (this is where the decoder looks back at the source sentence), then a Feed-Forward block.
- Final output: a Dense + softmax layer predicting the next word's probability distribution.

```python
# Simplified sketch (real Transformer needs multi-head + masking wrappers)
encoder_in = positional_encoding(embeddings(encoder_inputs))
Z = encoder_in
for _ in range(6):
    Z = keras.layers.Attention(use_scale=True)([Z, Z])       # self-attention
encoder_outputs = Z

decoder_in = positional_encoding(embeddings(decoder_inputs))
Z = decoder_in
for _ in range(6):
    Z = keras.layers.Attention(use_scale=True, causal=True)([Z, Z])   # masked self-attention
    Z = keras.layers.Attention(use_scale=True)([Z, encoder_outputs])  # attend to encoder
outputs = keras.layers.TimeDistributed(keras.layers.Dense(vocab_size, activation="softmax"))(Z)
```
*(`causal=True` enforces the "no peeking at future tokens" masking.)*

---

## 🚀 Recent Innovations (as of the book's writing) — the road to modern LLMs

2018 was called the **"ImageNet moment for NLP"**: massive Transformer/LSTM-based models trained on huge text corpora, then fine-tuned for specific tasks, began smashing benchmarks. Key milestones worth knowing by name:

- **ELMo (2018)**: contextualized word embeddings — the same word gets a *different* embedding depending on context (unlike static word2vec-style embeddings, "queen" in "Queen of England" vs. "queen bee" would differ).
- **ULMFiT (2018)**: showed the power of **unsupervised pretraining + fine-tuning** for NLP — pretrain an LSTM language model on huge unlabeled text (self-supervised: predict the next word, which needs no manual labels), then fine-tune on a specific task with far less labeled data than training from scratch would need.
- **GPT (2018) / GPT-2 (2019)**: OpenAI's decoder-only Transformer stack (masked self-attention only), pretrained via self-supervised next-word prediction on huge text. GPT-2, at 1.5 billion parameters, showed strong performance on many tasks **without any task-specific fine-tuning at all** — this is called **zero-shot learning**.
- **BERT (2018)**: Google's model — architecturally similar to GPT but uses **non-masked** (bidirectional) self-attention, so it can use context from **both directions** at once ("Bidirectional Encoder Representations from Transformers"). Pretrained with two clever self-supervised tasks:
  - **Masked Language Model (MLM)**: randomly mask ~15% of words, train the model to predict them from context in both directions.
  - **Next Sentence Prediction (NSP)**: train the model to judge whether two given sentences are genuinely consecutive in the original text — improves performance on downstream tasks like question answering.

> 🔭 **The throughline**: better (subword) tokenization + shifting from LSTMs to Transformers + self-supervised pretraining on huge corpora, then lightweight fine-tuning for specific tasks. This recipe — scaled up dramatically — is the direct ancestor of today's large language models.

---

## ⚠️ Common Pitfalls

- Forgetting `mask_zero=True` on the `Embedding` layer, letting padding tokens pollute training.
- Using greedy decoding for translation/generation and being surprised by locally-sensible-but-globally-poor output — beam search usually helps noticeably.
- Assuming word-level (space-based) tokenization works for all languages — it breaks for Chinese, is awkward for German compounds, etc. Subword tokenization is the more robust modern default.
- Confusing self-attention (words attending to other words in the *same* sequence) with encoder-decoder attention (decoder words attending to *encoder* outputs) — the Transformer decoder uses *both*, for different purposes.
- Omitting positional encoding — without it, attention-based models have literally no way to know word order ("dog bites man" and "man bites dog" would look identical to plain self-attention).
- Forgetting the *causal* mask in the decoder's self-attention — without it, the model can "see the answer" during training (future tokens), producing a model that looks great in training but fails at real generation.

---

## 🔑 Cheat Sheet

- **Char-RNN**: predict the next character; generate text by sampling repeatedly (temperature controls randomness).
- **Sentiment analysis**: sequence→vector RNN/GRU/LSTM on word/subword embeddings; use masking for padded sequences.
- **Encoder-Decoder**: seq→vector (encoder) + vector→seq (decoder), improved by **bidirectional** encoders and **beam search** decoding.
- **Attention**: lets the decoder look back at *all* encoder outputs, weighted by relevance — shortens the effective path length, easing the short-term memory problem. Bahdanau (learned similarity) vs. Luong (dot-product similarity, simpler/faster).
- **Transformer**: attention-only architecture (no recurrence/convolution) — **self-attention** (words attend to each other within a sequence), **Scaled Dot-Product Attention** (Query/Key/Value lookup), **Multi-Head Attention** (multiple parallel attention "angles"), **positional encoding** (injects word order via sine/cosine), **masked** self-attention in the decoder (no peeking at future tokens).
- **GPT** (decoder-only, unidirectional) vs. **BERT** (bidirectional, MLM + NSP pretraining) — both pretrain on huge unlabeled text, then fine-tune cheaply on specific tasks.

---

## ✍️ Practice

1. **Hands-on:** Build and train the Char-RNN on a text corpus of your choice (a book, song lyrics, etc.). Generate text at a few different `temperature` values — describe how the output changes.
2. **Conceptual:** Explain, in your own words, why a plain Encoder-Decoder (no attention) struggles more with *long* sentences than short ones, and how attention specifically addresses that.
3. **Hands-on:** Build a sentiment classifier on the IMDb dataset using an `Embedding` (with `mask_zero=True`) + bidirectional LSTM. Report test accuracy.
4. **Debug-the-scenario:** You implement a Transformer decoder but forget the causal mask on its self-attention layer. Training loss looks suspiciously excellent, but generated text at inference time is much worse than expected. What happened?
   <details><summary>Answer</summary>Without the causal mask, during training each position could attend to (effectively "see") future tokens it's supposed to be predicting — an information leak that makes training loss look artificially great. At real inference time, future tokens don't exist yet, so the model — never having learned to predict without that leak — performs far worse.</details>
5. **Conceptual:** Why does the Transformer need positional encoding at all, when RNNs (Ch. 15) never needed anything like it to know word order?

Next: `17_autoencoders_gans.md` — unsupervised representation learning and generative models: autoencoders, GANs, and diffusion models.