# 📘 Hands-On Machine Learning — Study Notes
### Based on *Hands-On Machine Learning with Scikit-Learn, Keras & TensorFlow (2nd Ed.)* — Aurélien Géron

Welcome! These notes rebuild the entire book into a **beginner-friendly, practice-first course**. You don't need to already know ML — every chapter starts from "what problem are we solving and why should I care" before it touches a single line of code.

---

## How these notes are organized

```
ML_Notes/
├── 00_START_HERE_Roadmap.md        ← you are here
├── part1_fundamentals/             ← classic ML (no neural nets yet)
│   ├── 01_ml_landscape.md
│   ├── 02_end_to_end_project.md
│   ├── 03_classification.md
│   ├── 04_training_models.md
│   ├── 05_support_vector_machines.md
│   ├── 06_decision_trees.md
│   ├── 07_ensemble_random_forests.md
│   ├── 08_dimensionality_reduction.md
│   └── 09_unsupervised_learning.md
├── part2_deep_learning/            ← neural networks, Keras & TensorFlow
│   ├── 10_ann_with_keras.md
│   ├── 11_training_deep_neural_nets.md
│   ├── 12_custom_models_tensorflow.md
│   ├── 13_loading_preprocessing_data.md
│   ├── 14_cnn_computer_vision.md
│   ├── 15_rnn_sequences_time_series.md
│   ├── 16_nlp_attention_transformers.md
│   ├── 17_autoencoders_gans.md
│   ├── 18_reinforcement_learning.md
│   └── 19_training_deploying_at_scale.md
├── ml_project_checklist.md         ← printable checklist for any real project (Appendix B)
├── glossary_cheatsheet.md          ← every key term & formula in one place, for fast revision
└── code/                           ← one runnable Jupyter notebook per chapter (see code/README.md)
    ├── part1_fundamentals/
    └── part2_deep_learning/
```

📓 **Each chapter has a matching notebook in `code/`** — the same workflow, but as code you actually run, not just read. See `code/README.md` for setup instructions (Google Colab is the easiest way to start, no installation needed).

Every chapter file follows the **same skeleton**, so once you know the pattern you can navigate any file instantly:

| Section | What it gives you |
|---|---|
| 🎯 Why this chapter matters | The one-paragraph "so what" — skip nothing, read this first |
| 🧠 Core Concepts | Plain-English explanations, analogies before jargon |
| 📐 Key Formulas / How it Works | The math, but walked through, not just dropped on you |
| 💻 Code Walkthrough | Runnable, commented Python (scikit-learn / Keras / TensorFlow) |
| ⚠️ Common Pitfalls | Mistakes beginners make, and how to spot them |
| 🔑 Cheat Sheet | Bullet-point recap — this is what you re-read before an interview or exam |
| ✍️ Practice | Exercises with hints, so you actually *do* something, not just read |

---

## Prerequisites (from the book's own preface)

You should be comfortable with:
- **Basic Python** (functions, loops, lists/dicts)
- **NumPy / pandas basics** (arrays, DataFrames) — if these are new to you, spend a day on them first; every chapter leans on them
- **High-school-to-early-college math**: enough algebra to read a formula, and comfort with the *idea* of a derivative/vector (you do **not** need to be able to prove theorems — every formula in these notes is explained in words too)

If you've never coded in Python: pause and do a 2–3 day crash course first. Everything else compounds on that.

---

## Environment setup (do this once)

```bash
# Create an isolated environment (recommended)
python -m venv ml_env
source ml_env/bin/activate      # Windows: ml_env\Scripts\activate

# Core libraries used throughout Part I
pip install numpy pandas matplotlib scikit-learn jupyter

# Needed from Part II onward (deep learning)
pip install tensorflow
```

> 💡 **Tip:** Run everything inside a **Jupyter notebook**. ML is exploratory — you want to re-run one cell 20 times while tweaking a hyperparameter, not re-run a whole script. If you've never used Jupyter: `jupyter notebook` in your terminal opens it in the browser.

> 💡 **No GPU? No problem for Part I.** Classic ML (chapters 1–9) runs fine on any laptop CPU. Deep learning (chapters 10+) gets slow without a GPU for the bigger examples — use **Google Colab** (free GPU, nothing to install) if your machine doesn't have one.

---

## Suggested study path

This is a marathon, not a sprint — the book itself is ~850 pages. A realistic pace for a beginner studying alongside a job/school:

| Week(s) | Focus | Goal |
|---|---|---|
| 1 | Ch. 1–2 | Understand *what* ML is + do one full project end-to-end |
| 2 | Ch. 3–4 | Classification metrics + how models actually "learn" (gradient descent) |
| 3 | Ch. 5–7 | SVMs, trees, ensembles — the classic non-neural toolbox |
| 4 | Ch. 8–9 | Dimensionality reduction, clustering — working with unlabeled data |
| 5–6 | Ch. 10–11 | Neural network fundamentals, training them without them breaking |
| 7 | Ch. 12–13 | Going "under the hood" of TensorFlow, efficient data pipelines |
| 8 | Ch. 14 | Computer vision (CNNs) |
| 9 | Ch. 15–16 | Sequences, text, attention/Transformers |
| 10 | Ch. 17–18 | Generative models (autoencoders/GANs) + reinforcement learning |
| 11 | Ch. 19 | Shipping a model to production |

**Rule of thumb:** don't move to the next chapter until you've *typed* (not copy-pasted) at least one code example yourself and gotten it running.

---

## The single biggest idea in this book

> Machine Learning = writing a program that **improves at a task by looking at data**, instead of you hand-coding every rule.

Almost everything else — every algorithm, every chapter — is a different answer to: *"given this kind of data and this kind of task, what's the smartest way for a computer to find the pattern itself?"* Keep that question in your head as your compass through all 19 chapters.

Now head into `part1_fundamentals/01_ml_landscape.md` and let's start. 🚀
