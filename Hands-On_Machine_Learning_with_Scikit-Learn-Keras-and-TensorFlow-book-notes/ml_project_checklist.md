# 📋 Machine Learning Project Checklist
*A practical, printable checklist for any real ML project — based on Appendix B's 8-step framework, cross-referenced to the chapter that covers each step in depth.*

> Adapt freely — not every project needs every item, but scan the whole list before skipping something.

---

## 1. Frame the Problem (→ Ch. 1–2)
- [ ] Write down the business objective in plain terms — what decision or action will this feed into?
- [ ] Figure out exactly how the solution will be consumed downstream.
- [ ] Check what the current solution/workaround looks like today (even a manual or rule-based one) — it's your baseline.
- [ ] Decide the framing: supervised vs. unsupervised vs. RL? Classification vs. regression? Batch vs. online?
- [ ] Choose a performance measure, and confirm it's genuinely aligned with the business objective.
- [ ] Define the minimum performance that would actually be useful to the business.
- [ ] Look for comparable problems — can you reuse existing tools, code, or experience?
- [ ] Check whether in-house human expertise is available to sanity-check your approach.
- [ ] Sketch how you'd solve this manually, step by step — it often reveals what features/logic matter.
- [ ] List every assumption you're making, and verify the ones you can before building anything.

## 2. Get the Data (→ Ch. 2, 13)
- [ ] List exactly what data you need and roughly how much.
- [ ] Find and document every source you can pull it from.
- [ ] Estimate storage footprint before you commit to pulling it all.
- [ ] Check legal/compliance obligations and get any required authorization.
- [ ] Set up a workspace with enough room to grow.
- [ ] Convert data into a convenient working format — without altering the underlying values.
- [ ] Scrub or protect sensitive information (anonymize where required).
- [ ] Understand the data's shape and type (time series? cross-sectional? geographic?).
- [ ] **Carve out a test set immediately, and don't look at it again** until final evaluation — avoid data snooping bias.
- [ ] Automate this whole step as much as you can, so refreshing the dataset later is cheap.

## 3. Explore the Data (→ Ch. 2)
- [ ] Work on a (possibly downsampled) copy — never the original or the test set.
- [ ] Keep a running notebook documenting what you find as you go.
- [ ] Profile every feature: name, type, % missing, noise characteristics, likely usefulness, distribution shape.
- [ ] For supervised tasks, pin down exactly what the target variable(s) are.
- [ ] Visualize everything you reasonably can.
- [ ] Study pairwise correlations and interactions between features.
- [ ] Think again about how you'd solve this manually — compare notes with your earlier sketch.
- [ ] Note promising transformations to try later (ratios, logs, bucketing, etc.).
- [ ] Note any extra data sources that would help — loop back to Step 2 if needed.
- [ ] Get a domain expert to sanity-check your findings if at all possible.

## 4. Prepare the Data (→ Ch. 2, 4, 13)
> Work on copies, never the raw original. **Write every transformation as a reusable function/pipeline** — you'll need to repeat it identically on the test set, on future fresh data, and in production.
- [ ] **Clean**: fix or drop outliers (case by case); fill or drop missing values.
- [ ] **Select**: drop features that carry no real signal for the task.
- [ ] **Engineer**: bucket continuous features where useful; decompose composite features (dates, categories); add promising transforms (log, sqrt, polynomial, ratios); aggregate raw features into more informative ones.
- [ ] **Scale**: standardize or normalize numeric features.
- [ ] If working with huge data, consider a smaller sample to iterate faster (just remember this penalizes complex models like large ensembles/neural nets, which need more data to shine).

## 5. Shortlist Promising Models (→ Ch. 2–9)
- [ ] Quickly train several "quick and dirty" models from *different* families (linear, tree-based, SVM, naive Bayes, a simple neural net) using mostly default settings.
- [ ] Compare them fairly using N-fold cross-validation — record both the mean **and** the standard deviation of the score across folds.
- [ ] Inspect which features matter most for each model.
- [ ] Look at what *kinds* of errors each model tends to make — would a human have used different information to avoid them?
- [ ] Do a quick pass of extra feature selection/engineering based on what you just learned.
- [ ] Repeat the last few steps once or twice more.
- [ ] Shortlist your top 3–5 models — prefer models that tend to make *different kinds* of mistakes (they'll combine well later).

## 6. Fine-Tune the System (→ Ch. 2, 7, 11)
- [ ] Use as much data as you reasonably can for this stage.
- [ ] Tune hyperparameters via cross-validation — and treat your data-prep choices (impute with median vs. drop rows? etc.) as tunable hyperparameters too, wherever you're genuinely unsure.
- [ ] Prefer randomized search over exhaustive grid search unless the search space is tiny; consider Bayesian optimization if training runs are expensive.
- [ ] Try combining your best models into an ensemble — this often beats any single model.
- [ ] **Only once you're confident in the final model**, evaluate it on the held-out test set to estimate real-world generalization error.
- [ ] **Do not tweak anything after seeing the test score** — that's overfitting to the test set, and it defeats the entire point of holding it out.

## 7. Present Your Solution (→ Ch. 2)
- [ ] Document the full process, not just the final result.
- [ ] Build a clear presentation that leads with the big picture.
- [ ] Explicitly connect your solution back to the original business objective.
- [ ] Share what worked *and* what didn't — don't just show the highlight reel.
- [ ] List your assumptions and your system's known limitations, honestly.
- [ ] Distill key findings into memorable visuals or one-line takeaways non-technical stakeholders will actually retain.

## 8. Launch, Monitor, and Maintain (→ Ch. 2, 19)
- [ ] Get the solution production-ready: wire it to real data inputs, add tests.
- [ ] Build monitoring that checks live performance regularly and alerts on drops.
  - [ ] Watch for slow, silent **model rot** as real-world data drifts from training data.
  - [ ] Also monitor input **data quality** (a broken upstream sensor or a stale feed from another team can quietly wreck predictions) — especially critical for online-learning systems.
- [ ] Set up a regular retraining cadence on fresh data, automated as much as possible.

---

*Cross-reference: for the deep dive behind any step, see the matching chapter file in `part1_fundamentals/` or `part2_deep_learning/`.*
