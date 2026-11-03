# Python practice environment — setup notes

This folder supports **Module 24: Python for Data Analysis and Analyst Automation**.

> All data is **fictional** and was generated for interview practice. It is not Trinity data, not real company data, and not indicative of any real market.

## Install

From the pack root:

```bash
python3 -m venv .venv
source .venv/bin/activate          # macOS / Linux
# .venv\Scripts\activate           # Windows

python -m pip install --upgrade pip
python -m pip install -r data/python/requirements.txt
```

Check:

```bash
python -c "import pandas as pd, numpy as np, matplotlib; print(pd.__version__, np.__version__, matplotlib.__version__)"
```

## Data

Module 24 uses the shared practice CSVs in `data/sql/csv/`:

| File | Rows | Grain |
|---|---:|---|
| `sales.csv` | 72 | month × region × product |
| `regions.csv` | 4 | one row per region |
| `products.csv` | 3 | one row per product |
| `reps.csv` | 12 | one row per sales representative |
| `physicians.csv` | 42 | one row per physician |
| `survey.csv` | 42 | one row per survey response |
| `survey_items.csv` | 126 | response × Likert item |
| `calls.csv` | 120 | one row per rep call |
| `prescriptions.csv` | 276 | physician × product × month with TRx > 0 |
| `quotas.csv` | 5 | one row per awareness stage |

Run Python from the pack root so relative paths work:

```python
from pathlib import Path
import pandas as pd

DATA = Path("data/sql/csv")
sales = pd.read_csv(DATA / "sales.csv")
```

## Ready-to-run report

`build_report.py` loads the shared CSVs, runs 11 validation checks, builds the KPI/region/product/survey/Rx tables, and writes a workbook plus tidy CSVs to `outputs/`:

```bash
python data/python/build_report.py
```

Expected output: `outputs/trinity_report.xlsx`, seven report CSVs, and `outputs/qa_log.csv`. The script is a teaching example for Module 24; it is not a production pipeline.

## Deliberate data-quality issues

The shared CSVs contain the same quirks as the SQL practice database:

- Two duplicate survey respondents: `P-007`, `P-023`.
- One out-of-window response: `SR-032` / `P-030` (2025-12-15).
- Invalid Likert records: `SR-020` Access = `7`; `SR-021` Efficacy = `N/A`.
- One blank monthly TRx: `SR-030` / `P-028`.
- One straight-liner: `SR-022` / `P-021`.
- Two blank sales targets; three blank call counts.
- Raw region variants `north ` and `N`; one trailing space in `specialty`.

These are exercises, not accidents. Every analysis in Module 24 states how it handles them.
