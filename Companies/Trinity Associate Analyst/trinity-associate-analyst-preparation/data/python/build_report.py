"""Build a small commercial + survey report from the fictional practice CSVs.

Run from anywhere:

    python data/python/build_report.py

Outputs are written to ``outputs/`` at the pack root. This is a teaching
script, not a production pipeline: it demonstrates loading, validation,
transformation, assertions, and export in one reproducible entry point.
"""

from __future__ import annotations

import logging
from pathlib import Path

import pandas as pd

PACK_ROOT = Path(__file__).resolve().parents[2]
DATA_DIR = PACK_ROOT / "data" / "sql" / "csv"
OUT_DIR = PACK_ROOT / "outputs"

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s %(levelname)s %(message)s",
)
logger = logging.getLogger(__name__)


def load_tables(data_dir: Path = DATA_DIR) -> dict[str, pd.DataFrame]:
    """Load the shared practice CSVs with explicit date and code parsing."""
    logger.info("loading data from %s", data_dir)
    return {
        "sales": pd.read_csv(data_dir / "sales.csv"),
        "regions": pd.read_csv(data_dir / "regions.csv"),
        "products": pd.read_csv(data_dir / "products.csv"),
        "physicians": pd.read_csv(data_dir / "physicians.csv"),
        "survey": pd.read_csv(data_dir / "survey.csv", parse_dates=["response_date"]),
        "items": pd.read_csv(
            data_dir / "survey_items.csv",
            dtype={"score_raw": "string"},
            keep_default_na=False,
        ),
        "prescriptions": pd.read_csv(data_dir / "prescriptions.csv"),
    }


def validate(tables: dict[str, pd.DataFrame]) -> pd.DataFrame:
    """Run hard checks. Raise before exporting a wrong report."""
    sales = tables["sales"]
    survey = tables["survey"]
    items = tables["items"]
    rx = tables["prescriptions"]

    clean_survey = (
        survey.loc[survey["response_date"].between("2026-01-15", "2026-03-31")]
        .sort_values(["physician_id", "response_id"])
        .drop_duplicates("physician_id", keep="first")
    )
    items = items.assign(score_num=pd.to_numeric(items["score_raw"], errors="coerce"))
    invalid_items = int((~items["score_num"].isin([1, 2, 3, 4, 5])).sum())

    checks = [
        ("sales_rows", len(sales), 72, len(sales) == 72),
        ("sales_revenue", int(sales["revenue"].sum()), 2_683_320,
         int(sales["revenue"].sum()) == 2_683_320),
        ("revenue_reconciliation", int((sales["revenue"] - sales["units"] * sales["unit_price"]).abs().sum()),
         0, int((sales["revenue"] - sales["units"] * sales["unit_price"]).abs().sum()) == 0),
        ("survey_raw_rows", len(survey), 42, len(survey) == 42),
        ("survey_unique_physicians", survey["physician_id"].nunique(), 40,
         survey["physician_id"].nunique() == 40),
        ("survey_clean_rows", len(clean_survey), 39, len(clean_survey) == 39),
        ("survey_duplicate_rows", int(survey.duplicated("physician_id").sum()), 2,
         int(survey.duplicated("physician_id").sum()) == 2),
        ("invalid_survey_items", invalid_items, 2, invalid_items == 2),
        ("rx_rows", len(rx), 276, len(rx) == 276),
        ("rx_total", int(rx["trx"].sum()), 1_699, int(rx["trx"].sum()) == 1_699),
        ("june_trx", int(rx.loc[rx["month"] == "2026-06-01", "trx"].sum()), 344,
         int(rx.loc[rx["month"] == "2026-06-01", "trx"].sum()) == 344),
    ]

    qa = pd.DataFrame(checks, columns=["check", "actual", "expected", "passed"])
    failed = qa.loc[~qa["passed"]]
    if not failed.empty:
        raise AssertionError(f"validation failed:\n{failed.to_string(index=False)}")
    logger.info("all %d validation checks passed", len(qa))
    return qa


def build_report(tables: dict[str, pd.DataFrame]) -> dict[str, pd.DataFrame]:
    """Build the headline tables. Each table is tidy and export-ready."""
    sales = tables["sales"]
    survey = tables["survey"]
    items = tables["items"]
    rx = tables["prescriptions"]

    sales_rp = (
        sales.merge(tables["regions"], on="region_id", how="left", validate="many_to_one")
        .merge(tables["products"], on="product_id", how="left", validate="many_to_one")
    )

    kpi = pd.DataFrame(
        [
            {"metric": "revenue", "value": sales["revenue"].sum()},
            {"metric": "target", "value": sales["target_revenue"].sum()},
            {"metric": "achievement_pct",
             "value": 100 * sales["revenue"].sum() / sales["target_revenue"].sum()},
            {"metric": "units", "value": sales["units"].sum()},
            {"metric": "rows", "value": len(sales)},
            {"metric": "rows_with_target", "value": sales["target_revenue"].notna().sum()},
            {"metric": "rows_at_or_above_target",
             "value": (sales["revenue"] >= sales["target_revenue"]).sum()},
        ]
    )

    region = (
        sales_rp.groupby("region_name", as_index=False)
        .agg(revenue=("revenue", "sum"), target=("target_revenue", "sum"),
             units=("units", "sum"))
    )
    region["achievement_pct"] = 100 * region["revenue"] / region["target"]
    region["share_pct"] = 100 * region["revenue"] / region["revenue"].sum()
    region = region.sort_values("share_pct", ascending=False).reset_index(drop=True)

    product = (
        sales_rp.groupby("product_name", as_index=False)
        .agg(revenue=("revenue", "sum"), target=("target_revenue", "sum"),
             units=("units", "sum"))
    )
    product["achievement_pct"] = 100 * product["revenue"] / product["target"]
    product["share_pct"] = 100 * product["revenue"] / product["revenue"].sum()
    product = product.sort_values("revenue", ascending=False).reset_index(drop=True)

    clean_survey = (
        survey.loc[survey["response_date"].between("2026-01-15", "2026-03-31")]
        .sort_values(["physician_id", "response_id"])
        .drop_duplicates("physician_id", keep="first")
    )

    stage = (
        clean_survey.groupby("awareness_stage", as_index=False)
        .agg(n=("response_id", "size"), trx=("monthly_trx", "sum"))
        .sort_values("awareness_stage")
        .reset_index(drop=True)
    )

    items = items.assign(score_num=pd.to_numeric(items["score_raw"], errors="coerce"))
    valid_items = items.loc[items["score_num"].isin([1, 2, 3, 4, 5])]
    valid_items = valid_items.merge(
        clean_survey[["response_id"]],
        on="response_id",
        how="inner",
        validate="many_to_one",
    )
    top2 = (
        valid_items.groupby("item", as_index=False)
        .agg(valid_n=("score_num", "size"),
             top2_pct=("score_num", lambda s: 100 * s.isin([4, 5]).mean()),
             mean_score=("score_num", "mean"))
        .sort_values("item")
        .reset_index(drop=True)
    )

    promoters = int((clean_survey["recommend_score"] >= 9).sum())
    passives = int(clean_survey["recommend_score"].between(7, 8).sum())
    detractors = int((clean_survey["recommend_score"] <= 6).sum())
    nps = pd.DataFrame(
        [
            {"metric": "promoters", "value": promoters},
            {"metric": "passives", "value": passives},
            {"metric": "detractors", "value": detractors},
            {"metric": "nps_unweighted",
             "value": 100 * (promoters - detractors) / len(clean_survey)},
            {"metric": "nps_weighted",
             "value": 100 * (
                 clean_survey.loc[clean_survey["recommend_score"] >= 9, "weight"].sum()
                 - clean_survey.loc[clean_survey["recommend_score"] <= 6, "weight"].sum()
             ) / clean_survey["weight"].sum()},
            {"metric": "effective_n",
             "value": clean_survey["weight"].sum() ** 2
             / (clean_survey["weight"] ** 2).sum()},
        ]
    )

    rx_monthly = (
        rx.groupby("month", as_index=False)
        .agg(trx=("trx", "sum"), new_rx=("new_rx", "sum"))
    )
    rx_monthly["mom_pct"] = rx_monthly["trx"].pct_change() * 100
    rx_monthly["running_total"] = rx_monthly["trx"].cumsum()
    rx_monthly["ma_3m"] = rx_monthly["trx"].rolling(3).mean()
    rx_monthly["new_rx_share_pct"] = 100 * rx_monthly["new_rx"] / rx_monthly["trx"]

    return {
        "kpi": kpi,
        "region": region,
        "product": product,
        "survey_stage": stage,
        "survey_top2": top2,
        "survey_nps": nps,
        "rx_monthly": rx_monthly,
    }


def export(report: dict[str, pd.DataFrame], qa: pd.DataFrame,
           out_dir: Path = OUT_DIR) -> None:
    """Write one workbook plus tidy CSVs for downstream use."""
    out_dir.mkdir(parents=True, exist_ok=True)
    workbook = out_dir / "trinity_report.xlsx"

    with pd.ExcelWriter(workbook, engine="openpyxl") as writer:
        report["kpi"].to_excel(writer, sheet_name="KPI", index=False)
        report["region"].to_excel(writer, sheet_name="Region", index=False)
        report["product"].to_excel(writer, sheet_name="Product", index=False)
        report["survey_stage"].to_excel(writer, sheet_name="Survey_Stage", index=False)
        report["survey_top2"].to_excel(writer, sheet_name="Survey_Top2", index=False)
        report["survey_nps"].to_excel(writer, sheet_name="Survey_NPS", index=False)
        report["rx_monthly"].to_excel(writer, sheet_name="Rx_Monthly", index=False)
        qa.to_excel(writer, sheet_name="QA_Log", index=False)

    for name, frame in report.items():
        frame.to_csv(out_dir / f"{name}.csv", index=False)
    qa.to_csv(out_dir / "qa_log.csv", index=False)

    logger.info("wrote %s and %d CSVs to %s", workbook.name, len(report) + 1, out_dir)


def main() -> None:
    tables = load_tables()
    qa = validate(tables)
    report = build_report(tables)
    export(report, qa)
    logger.info("report complete")


if __name__ == "__main__":
    main()
