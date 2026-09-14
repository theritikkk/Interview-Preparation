# Chapter 2 — End-to-End Machine Learning Project

## 🎯 Why this chapter matters
This is the chapter where theory becomes muscle memory. You'll walk through **every stage a real ML project goes through**, using a real dataset (California district housing prices). This 8-step workflow is the one you'll reuse for the rest of your career — memorize the *shape* of it, not just the code.

## 🗺️ The 8-step workflow

1. Look at the big picture
2. Get the data
3. Explore and visualize the data to gain insights
4. Prepare the data for ML algorithms
5. Select a model and train it
6. Fine-tune your model
7. Present your solution
8. Launch, monitor, and maintain your system

We'll go through each with the California Housing dataset: predict a district's **median house value** from census data (population, median income, location, etc.) — a **regression** task.

---

## 1️⃣ Look at the Big Picture

Before writing code, answer these questions — they change *everything* downstream:

- **What's the business objective?** Not "predict housing prices" in the abstract — e.g., "feed predictions into another ML system that decides whether an area is worth investing in." This tells you what error is costly and how your output will be *consumed*.
- **What does the current solution look like (if any)?** Gives you a performance baseline and hints about the problem's structure.
- **Frame the problem:**
  - Supervised, unsupervised, or RL? → We have labels (`median_house_value`) → **supervised**.
  - Classification or regression? → Predicting a number → **regression**. Specifically **multiple regression** (many features are used to predict) and **univariate regression** (predicting a single value per district, as opposed to *multivariate* regression predicting several values at once).
  - Batch or online learning? → Data doesn't change fast, fits in memory → **plain batch learning**.
- **Select a performance measure.** For regression, the default is **Root Mean Square Error (RMSE)**:

$$
\text{RMSE}(\mathbf{X}, h) = \sqrt{\frac{1}{m}\sum_{i=1}^{m}\left(h(\mathbf{x}^{(i)}) - y^{(i)}\right)^2}
$$

  In words: for every prediction, measure the error, square it (so negative and positive errors don't cancel and big errors are punished more), average all those squared errors, then take the square root to bring the units back to "dollars" instead of "dollars²." RMSE corresponds to the **Euclidean norm** (ℓ2 norm) of the error vector.

  Alternative: **Mean Absolute Error (MAE)**, a.k.a. the ℓ1 norm — averages the *absolute* errors instead of squared ones. MAE is **more robust to outliers** than RMSE (RMSE's squaring makes it very sensitive to a few large errors). Higher-index norms (ℓk) focus increasingly on large values; RMSE > MAE in sensitivity to outliers, but RMSE is usually preferred when large errors are undesirable and the error distribution is roughly bell-shaped.

- **Check your assumptions.** Sounds trivial, but verify what "downstream" actually needs — e.g., if the next system treats your price output as a *category* (cheap/medium/expensive) rather than an exact number, you actually need **classification**, not regression! Catch this before you build the wrong thing.

---

## 2️⃣ Get the Data

**Take a quick first look** (`housing.head()`, `housing.info()`, `housing.describe()`, `housing["column"].value_counts()` for categorical columns, and `housing.hist(bins=50, figsize=(20,15))` to see every numeric distribution at once). Things to watch for right away:
- Missing values (`info()` shows non-null counts per column)
- Capped/clipped values (e.g., a histogram with a big spike at the max value — house values capped at $500,000 is a real issue in this dataset: it means the labels near the cap are wrong/uninformative for that range)
- Attributes on very different scales (feature scaling will matter later)
- Tail-heavy (skewed) distributions — some models handle these better after a log transform

### 🚨 Create a test set — *immediately*, before exploring further
This feels backwards ("shouldn't I understand the data first?") but it's critical: **your brain is a pattern-finder, and if you eyeball the whole dataset (including what becomes your test set) you risk unconsciously choosing a model that fits quirks of the test data too.** That's **data snooping bias**, and it makes your generalization estimate too optimistic.

```python
from sklearn.model_selection import train_test_split

train_set, test_set = train_test_split(housing, test_size=0.2, random_state=42)
```

**But plain random splitting has a subtle flaw:** if one feature is very important (say, `median_income`), a purely random split might accidentally produce a test set that isn't representative of the full income distribution — introducing **sampling bias**. The fix is **stratified sampling**: split the population into homogeneous groups ("strata") and sample from each proportionally, the way a well-run political poll makes sure its sample matches the population's known demographics.

```python
import numpy as np
import pandas as pd
from sklearn.model_selection import StratifiedShuffleSplit

# Turn a continuous feature into discrete income "buckets" so we can stratify on it
housing["income_cat"] = pd.cut(
    housing["median_income"],
    bins=[0., 1.5, 3.0, 4.5, 6., np.inf],
    labels=[1, 2, 3, 4, 5],
)

split = StratifiedShuffleSplit(n_splits=1, test_size=0.2, random_state=42)
for train_index, test_index in split.split(housing, housing["income_cat"]):
    strat_train_set = housing.loc[train_index]
    strat_test_set = housing.loc[test_index]

# Drop the helper column — we only needed it to stratify
for set_ in (strat_train_set, strat_test_set):
    set_.drop("income_cat", axis=1, inplace=True)
```

This produces a test set whose income distribution near-exactly matches the full dataset's — a plain random split would have been measurably skewed.

---

## 3️⃣ Discover and Visualize the Data (only the training set!)

Put the test set away in a drawer. Copy the training set and explore freely:

```python
housing = strat_train_set.copy()

# Geographical scatter, weighted by population size (s) and colored by price (c)
housing.plot(kind="scatter", x="longitude", y="latitude", alpha=0.4,
             s=housing["population"]/100, label="population", figsize=(10,7),
             c="median_house_value", cmap="jet", colorbar=True)
```

This single plot often reveals the story instantly — e.g., prices are clearly higher near the coast and near big cities, and there's a strong geographical/density cluster structure.

**Look for correlations.** For numeric columns, Pandas gives you the **standard correlation coefficient** (Pearson's r) between every pair of attributes:

```python
corr_matrix = housing.corr()
corr_matrix["median_house_value"].sort_values(ascending=False)
```

⚠️ **Correlation only measures *linear* relationships** and ranges from –1 (strong negative) to +1 (strong positive); 0 means *no linear* correlation — but a strong nonlinear relationship (like a "V" shape) can score close to 0. Always eyeball a scatter matrix too (`pandas.plotting.scatter_matrix`), because correlation numbers alone can hide the true shape of a relationship — and can hide **data artifacts** (like a hard price cap producing a suspicious straight horizontal line on a scatter plot).

**Experiment with attribute combinations.** Raw features are often less useful than *ratios* of them — e.g., "total rooms in a district" is less informative than "rooms per household," and this is where **feature engineering** creativity pays off:

```python
housing["rooms_per_household"] = housing["total_rooms"] / housing["households"]
housing["bedrooms_per_room"] = housing["total_bedrooms"] / housing["total_rooms"]
housing["population_per_household"] = housing["population"] / housing["households"]
```
These new features often correlate with the target *more strongly* than any of their raw ingredients.

---

## 4️⃣ Prepare the Data for ML Algorithms

**Golden rule: write functions/pipelines, don't do this by hand.** You'll need to repeat every transformation on new data (production, live pipelines) exactly, reproducibly, and in the same order every time.

```python
# separate predictors from labels
housing = strat_train_set.drop("median_house_value", axis=1)
housing_labels = strat_train_set["median_house_value"].copy()
```

**Data cleaning (missing values).** Most ML algorithms can't handle missing values. Three common options: drop the rows, drop the whole column, or **impute** (fill in) a value like the median:

```python
from sklearn.impute import SimpleImputer

imputer = SimpleImputer(strategy="median")   # only works on numeric columns
housing_num = housing.drop("ocean_proximity", axis=1)  # the one text column
imputer.fit(housing_num)          # computes the median of every column, stores it
X = imputer.transform(housing_num)  # actually fills in the missing values
```
Using `SimpleImputer` (instead of manually filling `NaN`s with `df.median()`) matters because it **remembers the medians it learned on the training set**, so you can apply the *exact same* fill values to the test set / production data later — you must never compute a fresh median from the test set.

**Handling categorical/text attributes.** ML models want numbers. `ocean_proximity` is a category like `"NEAR BAY"`, `"INLAND"`, etc.

```python
from sklearn.preprocessing import OneHotEncoder

cat_encoder = OneHotEncoder()
housing_cat_1hot = cat_encoder.fit_transform(housing[["ocean_proximity"]])
```
⚠️ Don't use a plain **ordinal encoder** (mapping categories to 0,1,2,3...) for *unordered* categories — the model would wrongly assume `"INLAND"` (say, 1) is "closer to" `"NEAR BAY"` (0) than `"ISLAND"` (4) is, when there's no such natural ordering. **One-hot encoding** creates one binary column per category instead, avoiding that false ordering (at the cost of more columns — for categories with *very* many values, embeddings, covered in Ch. 13, scale better).

**Feature scaling.** Most ML algorithms don't perform well when numeric features have very different scales (e.g., `total_rooms` ranges 6–39,320 while `median_income` ranges 0–15). Two common approaches:
- **Min-max scaling ("normalization")**: shifts/rescales values into [0, 1]. `sklearn.preprocessing.MinMaxScaler`.
- **Standardization**: subtract the mean and divide by the standard deviation → result has zero mean, unit variance. Doesn't bound values to a fixed range (can be an issue for algorithms like neural nets that expect [0,1]), but is **much less affected by outliers**. `sklearn.preprocessing.StandardScaler`.

⚠️ **Fit scalers only on the training data**, then use the *same* fitted scaler's `.transform()` on the validation/test/production data. Never `fit()` on the full dataset — that leaks test-set information into training.

**Put it all together with Pipelines.** Manually chaining these steps is error-prone; scikit-learn's `Pipeline` and `ColumnTransformer` automate it:

```python
from sklearn.pipeline import Pipeline
from sklearn.impute import SimpleImputer
from sklearn.preprocessing import StandardScaler, OneHotEncoder
from sklearn.compose import ColumnTransformer

num_attribs = list(housing_num)          # all numeric column names
cat_attribs = ["ocean_proximity"]

num_pipeline = Pipeline([
    ("imputer", SimpleImputer(strategy="median")),
    ("std_scaler", StandardScaler()),
])

full_pipeline = ColumnTransformer([
    ("num", num_pipeline, num_attribs),
    ("cat", OneHotEncoder(), cat_attribs),
])

housing_prepared = full_pipeline.fit_transform(housing)
```
One call — `full_pipeline.fit_transform(housing)` — cleans, engineers, scales, and encodes *everything*, in the right order, and can be reapplied identically to any future data with `full_pipeline.transform(new_data)`.

---

## 5️⃣ Select and Train a Model

Start simple, as a sanity-check baseline:

```python
from sklearn.linear_model import LinearRegression

lin_reg = LinearRegression()
lin_reg.fit(housing_prepared, housing_labels)
```

Measure RMSE **on the training set** first, just as a smoke test:

```python
from sklearn.metrics import mean_squared_error

predictions = lin_reg.predict(housing_prepared)
lin_rmse = mean_squared_error(housing_labels, predictions, squared=False)
```

If this error is large, you're likely **underfitting** — the features may not carry enough signal, or you need a more powerful model. Try a more flexible model:

```python
from sklearn.tree import DecisionTreeRegressor

tree_reg = DecisionTreeRegressor(random_state=42)
tree_reg.fit(housing_prepared, housing_labels)
```

If a `DecisionTreeRegressor` reports **zero training error**, don't celebrate — that's a huge red flag for **overfitting** (the tree has probably memorized the training data), not a great model. You can't tell the difference between "the model is great" and "the model memorized the data" just by looking at training error — which is exactly why you need...

**Better evaluation using cross-validation.** Instead of a single train/validation split, **K-fold cross-validation** splits the training set into *K* subsets ("folds"), trains K times, each time holding out a different fold for validation and training on the rest, then reports all K scores:

```python
from sklearn.model_selection import cross_val_score

scores = cross_val_score(tree_reg, housing_prepared, housing_labels,
                          scoring="neg_mean_squared_error", cv=10)
tree_rmse_scores = np.sqrt(-scores)
print(tree_rmse_scores.mean(), tree_rmse_scores.std())
```
*(Note: scikit-learn's cross-validation scoring convention expects a "higher is better" score, so it uses the* **negative** *MSE — hence the sign flip before the square root.)*

This gives you not just an average error but also its **standard deviation** — a measure of how sensitive/unstable the model's performance is. With cross-validation you'll typically discover the fancy Decision Tree is actually *worse* than plain Linear Regression once you stop being fooled by its zero training error — a classic overfitting story. Trying `RandomForestRegressor` (Ch. 7) next usually beats both.

> 🔑 **Try several models (Linear Regression, Decision Tree, Random Forest, SVM Regressor...) quickly, shortlisting the 2–5 most promising ones, before investing heavily in tuning any single one.**

---

## 6️⃣ Fine-Tune Your Model

**Grid Search** — tell scikit-learn which hyperparameter values to try, and it evaluates every combination via cross-validation:

```python
from sklearn.model_selection import GridSearchCV
from sklearn.ensemble import RandomForestRegressor

param_grid = [
    {"n_estimators": [3, 10, 30], "max_features": [2, 4, 6, 8]},
    {"bootstrap": [False], "n_estimators": [3, 10], "max_features": [2, 3, 4]},
]

forest_reg = RandomForestRegressor(random_state=42)
grid_search = GridSearchCV(forest_reg, param_grid, cv=5,
                            scoring="neg_mean_squared_error",
                            return_train_score=True)
grid_search.fit(housing_prepared, housing_labels)

print(grid_search.best_params_)
best_model = grid_search.best_estimator_
```

When the hyperparameter search space is large, **Randomized Search** (`RandomizedSearchCV`) is more efficient — it samples a fixed number of random combinations instead of trying every single one, letting you cover a much wider space in the same compute budget.

**Ensemble methods**: combining several models that each perform reasonably well often beats the single best model — this is the whole idea behind Random Forests (an ensemble of Decision Trees) and is explored fully in Ch. 7.

**Analyze the best model and its errors.** Many models (like `RandomForestRegressor`) expose `.feature_importances_` — inspect which features actually mattered, and consider dropping the useless ones or engineering better ones based on what you learn.

**Evaluate on the test set — exactly once, at the very end:**

```python
X_test = strat_test_set.drop("median_house_value", axis=1)
y_test = strat_test_set["median_house_value"].copy()

X_test_prepared = full_pipeline.transform(X_test)   # transform() only — never fit again!
final_predictions = best_model.predict(X_test_prepared)
final_rmse = mean_squared_error(y_test, final_predictions, squared=False)
```
Don't tweak hyperparameters after seeing this number — that would just start overfitting to the test set, defeating its entire purpose.

---

## 7️⃣ & 8️⃣ Present and Launch

- **Present**: document what worked, what didn't, your assumptions, and your system's limitations — highlight the big picture, not just the final RMSE.
- **Launch, monitor, maintain**: connect the trained pipeline to production data sources, write monitoring code (models silently degrade over time as real-world data drifts — this is called **model rot**), and set up a retraining process, ideally automated, that's triggered on a schedule or when monitored performance drops. Automate as much of the pipeline as reasonably possible (data collection, retraining, evaluation) so upkeep isn't fully manual forever.

---

## ⚠️ Common Pitfalls

- Doing exploratory analysis on the **whole** dataset instead of just the training set → data snooping bias.
- Fitting a `Scaler`/`Imputer`/encoder on the **test set** (or on train+test combined) instead of only on the training set.
- Judging a model by its **training error alone** (a Decision Tree can trivially reach ~0 training error by memorizing).
- Tuning hyperparameters using the **test set** score instead of cross-validation / a separate validation set.
- Forgetting that RMSE is sensitive to outliers — check whether MAE tells a different story.
- Treating `random_state` as a magic fix — it just makes results *reproducible*, not necessarily *correct* or *representative*.

---

## 🔑 Cheat Sheet

- **8-step project workflow**: big picture → get data → explore/visualize → prepare data → select & train model(s) → fine-tune → present → launch/monitor.
- Split off a test set **before** exploring; use **stratified sampling** on your most important feature if the dataset isn't huge.
- RMSE = ℓ2 norm of errors (sensitive to outliers); MAE = ℓ1 norm (more robust).
- `Pipeline` + `ColumnTransformer` = reproducible, reusable preprocessing (`fit_transform` on train, `transform` only on test/production).
- One-hot encode unordered categories; standardize/normalize numeric features (fit only on training data).
- Cross-validation (`cross_val_score`, `cv=k`) gives you both a mean error **and** its variability — always prefer this over a single train/val split for model comparison.
- `GridSearchCV` / `RandomizedSearchCV` automate hyperparameter tuning with built-in cross-validation.
- Touch the test set **exactly once**, at the very end.

---

## ✍️ Practice

1. **Hands-on (recommended dataset: scikit-learn's `fetch_california_housing()`, a close cousin of the book's dataset):** Build the pipeline above end-to-end. Compare `LinearRegression`, `DecisionTreeRegressor`, and `RandomForestRegressor` using 10-fold cross-validation RMSE. Which wins, and by how much?
2. **Conceptual:** Explain, in your own words, why fitting a `StandardScaler` on the *combined* train+test data before splitting is a mistake, even though it seems harmless ("it's just computing a mean/std").
3. **Debug-the-scenario:** Your `DecisionTreeRegressor` gets RMSE = 0 on the training set and RMSE = 68,000 in cross-validation. What's happening, and name one fix.
   <details><summary>Answer</summary>The tree has overfit (memorized) the training data. Fixes: limit tree depth (`max_depth`), require more samples per leaf (`min_samples_leaf`), or switch to a Random Forest (ensembling reduces overfitting).</details>
4. **Design:** You're told the housing-price model's predictions will feed into a downstream system that only needs to know "cheap / medium / expensive." Does this change your problem framing? What would you do differently starting from Step 1?

Next: `03_classification.md` — same workflow, but now the target is a category, not a number, and we dig into precision/recall.