import sys
sys.path.insert(0, "/home/claude")
from nb_helper import md, code, make_notebook

OUT = "/mnt/user-data/outputs/ML_Notes/code/part1_fundamentals"

# ============================================================ Chapter 1
cells = [
md("""# Chapter 1 — The Machine Learning Landscape
Companion notebook to `part1_fundamentals/01_ml_landscape.md`.
Run cells top to bottom. Needs: `numpy`, `matplotlib`, `scikit-learn`."""),

code("""import numpy as np
import matplotlib.pyplot as plt
from sklearn.linear_model import LinearRegression
from sklearn.neighbors import KNeighborsRegressor"""),

md("## Model-based learning: a simple Linear Regression"),
code("""# Toy dataset: GDP per capita ($1000s) vs a life-satisfaction score (0-10)
gdp_per_capita = np.array([[9.5], [15.8], [26.8], [34.3], [37.9], [50.9]])
life_satisfaction = np.array([4.9, 5.9, 6.9, 7.3, 7.2, 7.8])

model = LinearRegression()
model.fit(gdp_per_capita, life_satisfaction)

print("intercept (theta0):", model.intercept_)
print("slope     (theta1):", model.coef_[0])

X_new = [[22.587]]
print("Predicted life satisfaction:", model.predict(X_new))

plt.scatter(gdp_per_capita, life_satisfaction)
plt.plot(gdp_per_capita, model.predict(gdp_per_capita), color="red")
plt.xlabel("GDP per capita ($1000s)")
plt.ylabel("Life satisfaction")
plt.title("Model-based learning: Linear Regression")
plt.show()"""),

md("## Instance-based learning: swap in k-Nearest Neighbors\nSame workflow, different algorithm family — notice how little code changes."),
code("""knn_model = KNeighborsRegressor(n_neighbors=3)
knn_model.fit(gdp_per_capita, life_satisfaction)
print("kNN predicted life satisfaction:", knn_model.predict(X_new))"""),

md("""## ✍️ Practice (from the notes — try these yourself below)
1. For each of: house price prediction, customer segmentation, robot walking via trial/error,
   and semi-supervised photo tagging — name the ML type and category.
2. Compare the Linear Regression vs kNN predictions above for a few different `X_new` values.
   Where do they diverge most, and why?
3. A model gets 99.8% training accuracy but 62% on new data — name the problem and two fixes.
4. Design question: fraud detection with monthly-changing patterns — batch or online learning?"""),

code("""# Your code / notes here
"""),
]
make_notebook(cells, f"{OUT}/01_ml_landscape.ipynb")

# ============================================================ Chapter 2
cells = [
md("""# Chapter 2 — End-to-End Machine Learning Project
Companion notebook to `part1_fundamentals/02_end_to_end_project.md`.

Uses scikit-learn's built-in `fetch_california_housing` (a close cousin of the book's
original `housing.csv`, minus the categorical `ocean_proximity` column). Needs internet
access the first time, to download the dataset."""),

code("""import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from sklearn.datasets import fetch_california_housing
from sklearn.model_selection import train_test_split, StratifiedShuffleSplit, cross_val_score, GridSearchCV
from sklearn.pipeline import Pipeline
from sklearn.impute import SimpleImputer
from sklearn.preprocessing import StandardScaler
from sklearn.compose import ColumnTransformer
from sklearn.linear_model import LinearRegression
from sklearn.tree import DecisionTreeRegressor
from sklearn.ensemble import RandomForestRegressor
from sklearn.metrics import mean_squared_error"""),

md("## 1-2. Get the data & look at it"),
code("""housing = fetch_california_housing(as_frame=True).frame
housing = housing.rename(columns={"MedHouseVal": "median_house_value"})
housing.head()"""),
code("""housing.info()
housing.describe()"""),
code("""housing.hist(bins=50, figsize=(12, 8))
plt.tight_layout()
plt.show()"""),

md("## Stratified train/test split (stratify on income, the most predictive feature)"),
code("""housing["income_cat"] = pd.cut(housing["MedInc"],
                                bins=[0., 1.5, 3.0, 4.5, 6., np.inf],
                                labels=[1, 2, 3, 4, 5])

split = StratifiedShuffleSplit(n_splits=1, test_size=0.2, random_state=42)
for train_index, test_index in split.split(housing, housing["income_cat"]):
    strat_train_set = housing.loc[train_index]
    strat_test_set = housing.loc[test_index]

for set_ in (strat_train_set, strat_test_set):
    set_.drop("income_cat", axis=1, inplace=True)

print(len(strat_train_set), "train +", len(strat_test_set), "test")"""),

md("## 3. Explore (training set only!)"),
code("""housing_expl = strat_train_set.copy()
housing_expl.plot(kind="scatter", x="Longitude", y="Latitude", alpha=0.4,
                   s=housing_expl["Population"] / 100, label="population", figsize=(10, 7),
                   c="median_house_value", cmap="jet", colorbar=True)
plt.legend()
plt.show()"""),
code("""corr_matrix = housing_expl.corr(numeric_only=True)
corr_matrix["median_house_value"].sort_values(ascending=False)"""),
code("""# Feature engineering: ratios often beat raw counts
housing_expl["rooms_per_household"] = housing_expl["AveRooms"] / housing_expl["AveOccup"]
housing_expl.corr(numeric_only=True)["median_house_value"].sort_values(ascending=False)"""),

md("## 4. Prepare the data (reusable pipeline)"),
code("""housing_tr = strat_train_set.drop("median_house_value", axis=1)
housing_labels = strat_train_set["median_house_value"].copy()

num_attribs = list(housing_tr)   # all columns are numeric in this dataset

num_pipeline = Pipeline([
    ("imputer", SimpleImputer(strategy="median")),
    ("std_scaler", StandardScaler()),
])

full_pipeline = ColumnTransformer([
    ("num", num_pipeline, num_attribs),
])

housing_prepared = full_pipeline.fit_transform(housing_tr)
housing_prepared.shape"""),

md("## 5. Select & train a model, compare with cross-validation"),
code("""lin_reg = LinearRegression()
lin_reg.fit(housing_prepared, housing_labels)
lin_rmse = mean_squared_error(housing_labels, lin_reg.predict(housing_prepared), squared=False)
print("Linear Regression training RMSE:", lin_rmse)"""),
code("""for name, model in [("Linear Regression", LinearRegression()),
                     ("Decision Tree", DecisionTreeRegressor(random_state=42)),
                     ("Random Forest", RandomForestRegressor(n_estimators=100, random_state=42))]:
    scores = cross_val_score(model, housing_prepared, housing_labels,
                              scoring="neg_mean_squared_error", cv=10)
    rmse_scores = np.sqrt(-scores)
    print(f"{name:18s} mean RMSE = {rmse_scores.mean():.0f}  std = {rmse_scores.std():.0f}")"""),

md("## 6. Fine-tune with GridSearchCV"),
code("""param_grid = [
    {"n_estimators": [30, 100], "max_features": [4, 6, 8]},
]
forest_reg = RandomForestRegressor(random_state=42)
grid_search = GridSearchCV(forest_reg, param_grid, cv=5,
                            scoring="neg_mean_squared_error", return_train_score=True)
grid_search.fit(housing_prepared, housing_labels)
print(grid_search.best_params_)
best_model = grid_search.best_estimator_"""),
code("""# Which features matter most?
importances = best_model.feature_importances_
sorted(zip(importances, num_attribs), reverse=True)"""),

md("## Final evaluation — touch the test set exactly once"),
code("""X_test = strat_test_set.drop("median_house_value", axis=1)
y_test = strat_test_set["median_house_value"].copy()

X_test_prepared = full_pipeline.transform(X_test)   # transform only, never fit again!
final_predictions = best_model.predict(X_test_prepared)
final_rmse = mean_squared_error(y_test, final_predictions, squared=False)
print("Final test RMSE:", final_rmse)"""),

md("""## ✍️ Practice
1. Compare Linear Regression, Decision Tree, and Random Forest 10-fold CV RMSE — which wins, by how much?
2. Explain why fitting a `StandardScaler` on train+test combined is a mistake.
3. Your `DecisionTreeRegressor` gets RMSE=0 on train but poor CV score — what's happening, and what's one fix?
4. Design: if predictions only need to feed a "cheap/medium/expensive" downstream system, how would framing change?"""),
code("# Your code / notes here\n"),
]
make_notebook(cells, f"{OUT}/02_end_to_end_project.ipynb")

print("Part 1 (chapters 1-2) done")