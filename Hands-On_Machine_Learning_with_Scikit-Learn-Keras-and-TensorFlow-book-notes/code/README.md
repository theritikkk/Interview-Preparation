# 💻 Code Notebooks — Hands-On Machine Learning

One runnable Jupyter notebook per chapter, matching the notes in `../part1_fundamentals/` and `../part2_deep_learning/`. Each notebook is a hands-on companion, not a copy of the notes — it has the working code so you can run, break, and modify it directly, plus the same "✍️ Practice" prompts at the bottom (unsolved — that's for you) with an empty cell to work in.

```
code/
├── part1_fundamentals/       01–09: classic ML (scikit-learn only)
└── part2_deep_learning/      10–19: neural nets (TensorFlow/Keras)
```

## How to run these

**Easiest: Google Colab.** Upload any `.ipynb` to [colab.research.google.com](https://colab.research.google.com) — everything needed (scikit-learn, TensorFlow, a free GPU) is already installed. This is what the notes recommend for Part II.

**Locally:**
```bash
python -m venv ml_env
source ml_env/bin/activate      # Windows: ml_env\Scripts\activate

# Part I (chapters 1-9)
pip install numpy pandas matplotlib scikit-learn jupyter

# Part II (chapters 10-19) — everything above, plus:
pip install tensorflow

# Chapter 18 (Reinforcement Learning) additionally needs:
pip install gymnasium

jupyter notebook
```

## Notes on the code

- **Internet access is needed the first time** you run most notebooks — they download standard datasets (MNIST, Fashion MNIST, IMDB, California housing) via scikit-learn/Keras's built-in fetchers, exactly like the book does.
- **Chapter 2** uses scikit-learn's built-in `fetch_california_housing()` rather than the book's original `housing.csv`, since that file isn't bundled here — it's the same data source, just missing the book's `ocean_proximity` category column. If you'd rather use the exact original CSV, swap it in; the pipeline code will need the categorical-encoding block back (shown in the Chapter 2 notes).
- **Chapter 14**'s transfer-learning example uses `MobileNetV2` instead of the book's `Xception` purely for a smaller, faster download — the freeze → train → unfreeze → fine-tune recipe is identical either way.
- **Chapter 18** uses `gymnasium` (the maintained fork of the original `gym`) with a small compatibility shim, since `gym` itself is no longer actively updated.
- A few heavier cells (full GAN training, RL training loops, CNN transfer learning) will run faster on a GPU — Colab's free tier is the easiest way to get one.
- Every notebook is independent — you don't need to run them in order, though within Part II, concepts do build on each other (Ch. 11's techniques reappear used in Ch. 14, etc.), same as the notes.

Run the **Setup** cell(s) at the top of each notebook first — everything downstream depends on the imports and data-loading there.
