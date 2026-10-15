#!/usr/bin/env python3
"""
LLM vendor-document extraction: schema-constrained output, evidence grounding,
deterministic validation, human-review routing, and a measured eval harness.

Assessment topic 9 (Certa FDE, "production LLM experience"). Runnable with no API key:
the provider call is stubbed by `stub_model`, which *deliberately misbehaves* the way real
models do (fenced JSON, prose around the JSON, invalid enum, invented value) so you can
see the repair loop, the validator and the eval harness do real work.

Run:
    python3 llm_extraction.py            # pipeline + eval report
    python3 llm_extraction.py --json     # machine-readable output
"""

from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass, field
from datetime import date, datetime
from decimal import Decimal, InvalidOperation
from typing import Any, Callable, Iterable, Mapping, Sequence

try:
    import jsonschema  # pip install jsonschema
except ImportError:  # pragma: no cover
    jsonschema = None  # type: ignore[assignment]


# --------------------------------------------------------------------------- #
# 1. The contract we ask the model to satisfy
# --------------------------------------------------------------------------- #
PROPERTIES: dict[str, Any] = {
    "vendor_name": {"type": "string", "minLength": 1},
    "risk_tier": {"type": "string", "enum": ["low", "medium", "high", "critical"]},
    "data_processing": {"type": "boolean"},
    "jurisdiction": {"type": "string"},
    "contract_start": {"type": "string", "format": "date"},
    "contract_end": {"type": "string", "format": "date"},
    "annual_spend": {"type": "number", "minimum": 0},
    "subprocessors": {"type": "integer", "minimum": 0},
    "evidence": {
        "type": "object",
        "description": "verbatim snippet per extracted field; required because unsupported claims must be reviewable",
        "additionalProperties": {"type": "string", "maxLength": 300},
    },
}
SCHEMA: dict[str, Any] = {
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "additionalProperties": False,
    # Only fields that are guaranteed to exist are required. Marking an
    # optional field "required" pressures the model to invent a value to
    # satisfy the schema -- a fabricated risk tier is worse than a null one.
    "required": ["vendor_name"],
    "properties": PROPERTIES,
}

PROMPT_TEMPLATE = """You extract structured facts from third-party risk documents.

Rules:
1. Output ONLY a JSON object matching this schema. No prose, no markdown fences.
{schema}
2. For every field you fill, add a verbatim snippet from the document to "evidence" under the field name.
3. If a field is not stated in the document, OMIT it. Never guess, never infer from the filename.
4. Unknown risk tier -> omit "risk_tier". Do not map to "medium" by default.

Document:
\"\"\"
{document}
\"\"\"
"""


# --------------------------------------------------------------------------- #
# 2. Provider call (stub) + JSON parsing that survives real-world output
# --------------------------------------------------------------------------- #
def stub_model(prompt: str, attempt: int) -> str:
    """Stand-in for an OpenAI/Anthropic call so this file runs offline.

    Real call shape:
        resp = client.chat.completions.create(
            model="gpt-4.1-mini",
            messages=[{"role": "system", "content": SYSTEM}, {"role": "user", "content": prompt}],
            response_format={"type": "json_schema", "json_schema": {"name": "vendor", "schema": SCHEMA, "strict": True}},
            temperature=0,
        )
        return resp.choices[0].message.content
    """
    if "Acme GmbH" in prompt:
        if attempt == 1:  # classic first-attempt failure: prose + fences + an enum the schema forbids
            return (
                "Here is the extracted data:\n```json\n"
                '{"vendor_name": "Acme GmbH", "risk_tier": "Medium-High", "data_processing": "yes",'
                ' "jurisdiction": "DE", "annual_spend": "EUR 250,000", "evidence": {}}\n```\n'
                "Let me know if you need anything else!"
            )
        return json.dumps(
            {
                "vendor_name": "Acme GmbH",
                "risk_tier": "high",
                "data_processing": True,
                "jurisdiction": "DE",
                "contract_start": "2024-03-01",
                "contract_end": "2027-02-28",
                "annual_spend": 250000,
                "subprocessors": 3,
                "evidence": {
                    "vendor_name": "Acme GmbH (\"Supplier\")",
                    "risk_tier": "Designated High risk due to production support access",
                    "data_processing": "Supplier processes personal data on behalf of Customer",
                    "annual_spend": "EUR 250,000 per annum",
                    "contract_end": "ending 28 February 2027",
                },
            }
        )
    if "Northwind" in prompt:
        # hallucination: a tier and a spend that the document never states, plus a
        # fabricated "evidence" snippet -- the validator must catch both.
        return json.dumps(
            {
                "vendor_name": "Northwind Analytics",
                "risk_tier": "medium",
                "annual_spend": 90000,
                "evidence": {
                    "vendor_name": "Northwind Analytics provides reporting services",
                    "risk_tier": "Northwind is assessed as a medium risk vendor",
                    "annual_spend": "Annual contract value of USD 90,000",
                },
            }
        )
    if "Baltic" in prompt:
        # honest partial extraction: two fields found, everything else omitted
        return json.dumps(
            {
                "vendor_name": "Baltic Logistics OÜ",
                "jurisdiction": "EE",
                "data_processing": True,
                "evidence": {
                    "vendor_name": "Baltic Logistics OÜ",
                    "jurisdiction": "warehousing in Estonia",
                    "data_processing": "Personal data transferred to one subprocessor",
                },
            }
        )
    return "{}"


# --------------------------------------------------------------------------- #
# 3. Parse -> validate -> repair loop
# --------------------------------------------------------------------------- #
_FENCE = re.compile(r"```(?:json)?\s*(.*?)```", re.DOTALL | re.IGNORECASE)


def parse_json_loose(text: str) -> dict[str, Any]:
    """Tolerate fences and prose; take the first balanced JSON object."""
    text = text.strip()
    fenced = _FENCE.search(text)
    if fenced:
        text = fenced.group(1).strip()
    if text.startswith("{"):
        candidate = text
    else:
        start = text.find("{")
        if start == -1:
            raise ValueError("no JSON object in model output")
        depth, end = 0, -1
        for index in range(start, len(text)):
            if text[index] == "{":
                depth += 1
            elif text[index] == "}":
                depth -= 1
                if depth == 0:
                    end = index
                    break
        if end == -1:
            raise ValueError("unbalanced JSON in model output")
        candidate = text[start : end + 1]
    parsed = json.loads(candidate)
    if not isinstance(parsed, dict):
        raise ValueError(f"expected a JSON object, got {type(parsed).__name__}")
    return parsed


def validate_schema(instance: Mapping[str, Any], schema: Mapping[str, Any]) -> list[str]:
    """Return human-readable schema violations (empty list == conformant)."""
    if jsonschema is not None:
        validator = jsonschema.Draft202012Validator(schema)
        return [f"{'/'.join(str(p) for p in err.absolute_path) or '<root>'}: {err.message}" for err in validator.iter_errors(instance)]

    errors: list[str] = []
    for key in schema.get("required", []):
        if key not in instance:
            errors.append(f"<root>: '{key}' is a required property")
    for key, value in instance.items():
        spec = schema.get("properties", {}).get(key)
        if spec is None:
            if schema.get("additionalProperties") is False:
                errors.append(f"{key}: additional properties are not allowed")
            continue
        expected = spec.get("type")
        ok = {
            "string": isinstance(value, str),
            "boolean": isinstance(value, bool),
            "number": isinstance(value, (int, float)) and not isinstance(value, bool),
            "integer": isinstance(value, int) and not isinstance(value, bool),
            "object": isinstance(value, Mapping),
        }.get(expected, True)
        if not ok:
            errors.append(f"{key}: {value!r} is not of type '{expected}'")
        if "enum" in spec and value not in spec["enum"]:
            errors.append(f"{key}: {value!r} is not one of {spec['enum']}")
        if isinstance(value, str) and spec.get("minLength") and len(value) < spec["minLength"]:
            errors.append(f"{key}: string too short")
    return errors


@dataclass
class ExtractionResult:
    data: dict[str, Any]
    attempts: int
    schema_errors: list[str] = field(default_factory=list)
    raw_first_attempt: str = ""


def extract_with_repair(
    document: str,
    *,
    schema: Mapping[str, Any],
    model: Callable[[str, int], str],
    max_attempts: int = 3,
) -> ExtractionResult:
    """Ask the model, validate, and feed the validator's complaint back on retry."""
    prompt = PROMPT_TEMPLATE.format(schema=json.dumps(schema, indent=2), document=document.strip())
    first_raw = ""
    last_errors: list[str] = []
    for attempt in range(1, max_attempts + 1):
        raw = model(prompt if attempt == 1 else _repair_prompt(prompt, raw, last_errors), attempt)
        if attempt == 1:
            first_raw = raw
        try:
            candidate = parse_json_loose(raw)
        except (ValueError, json.JSONDecodeError) as exc:
            last_errors = [f"<root>: output was not parseable JSON ({exc})"]
            continue
        errors = validate_schema(candidate, schema)
        if not errors:
            return ExtractionResult(candidate, attempt, [], first_raw)
        last_errors = errors
    return ExtractionResult({}, max_attempts, last_errors, first_raw)


def _repair_prompt(original: str, bad_output: str, errors: Sequence[str]) -> str:
    return (
        f"{original}\n\nYour previous answer failed schema validation:\n"
        + "\n".join(f"- {e}" for e in errors)
        + f"\n\nPrevious answer:\n{bad_output}\n\nReturn corrected JSON only."
    )


# --------------------------------------------------------------------------- #
# 4. Deterministic post-checks (the layer that catches what the schema cannot)
# --------------------------------------------------------------------------- #
def _norm_text(value: str) -> str:
    return re.sub(r"[^a-z0-9]+", " ", value.lower()).strip()


def _norm_money(value: Any) -> Decimal | None:
    if isinstance(value, bool) or value is None:
        return None
    if isinstance(value, (int, float, Decimal)):
        return Decimal(str(value))
    digits = re.sub(r"[^0-9.]", "", str(value))
    try:
        return Decimal(digits) if digits else None
    except InvalidOperation:
        return None


def _norm_date(value: Any) -> date | None:
    if not isinstance(value, str):
        return None
    for fmt in ("%Y-%m-%d", "%d/%m/%Y", "%m/%d/%Y", "%d %B %Y", "%B %d, %Y"):
        try:
            return datetime.strptime(value.strip(), fmt).date()
        except ValueError:
            continue
    return None


def verify_evidence(data: Mapping[str, Any], document: str) -> list[str]:
    """Every claimed field must be traceable to a verbatim snippet in the document."""
    issues: list[str] = []
    evidence = data.get("evidence") or {}
    if not isinstance(evidence, Mapping):
        return ["evidence: must be an object mapping field -> snippet"]
    haystack = _norm_text(document)
    for key, value in data.items():
        if key == "evidence":
            continue
        snippet = evidence.get(key)
        if not snippet:
            issues.append(f"{key}: no evidence snippet supplied")
            continue
        if _norm_text(str(snippet)) not in haystack:
            issues.append(f"{key}: evidence snippet is not present verbatim in the document")
    return issues


def business_rules(data: Mapping[str, Any]) -> list[str]:
    """Cross-field and format rules that a JSON schema cannot express."""
    issues: list[str] = []
    start, end = _norm_date(data.get("contract_start")), _norm_date(data.get("contract_end"))
    if start and end and end < start:
        issues.append("contract_end: ends before contract_start")
    spend = _norm_money(data.get("annual_spend"))
    if spend is not None and spend < 0:
        issues.append("annual_spend: negative")
    if data.get("risk_tier") == "critical" and data.get("data_processing") is False:
        issues.append("risk_tier: critical tier with data_processing=false needs human confirmation")
    return issues


def normalise(data: Mapping[str, Any]) -> dict[str, Any]:
    """Canonical form for storage/compare: dates ISO-8601, money as Decimal."""
    out = dict(data)
    for key in ("contract_start", "contract_end"):
        parsed = _norm_date(out.get(key))
        if parsed:
            out[key] = parsed.isoformat()
    if "annual_spend" in out:
        spend = _norm_money(out["annual_spend"])
        if spend is not None:
            out["annual_spend"] = str(spend)
    return out


# --------------------------------------------------------------------------- #
# 5. Routing policy: auto-accept vs human review
# --------------------------------------------------------------------------- #
@dataclass
class Routing:
    decision: str  # "auto_accept" | "human_review" | "reject"
    reasons: list[str]
    data_gaps: list[str] = field(default_factory=list)


# Fields a compliance decision depends on. Missing they are not an extraction
# failure -- the workflow raises a task / questionnaire instead of guessing.
DECISION_FIELDS = ("risk_tier", "jurisdiction", "data_processing")

# Subset where a present-but-unsupported value is unacceptable without human eyes.
HUMAN_REVIEW_FIELDS = {"risk_tier", "jurisdiction", "data_processing"}


def route(data: Mapping[str, Any], schema_errors: Sequence[str], evidence_issues: Sequence[str], rule_issues: Sequence[str]) -> Routing:
    if schema_errors:
        return Routing("reject", [f"schema: {e}" for e in schema_errors][:5])
    reasons: list[str] = [f"evidence: {i}" for i in evidence_issues]
    reasons += [f"rule: {i}" for i in rule_issues]
    # missing evidence on a decision-driving field is always a review trigger
    for key in HUMAN_REVIEW_FIELDS & set(data):
        if not (data.get("evidence") or {}).get(key):
            reasons.append(f"review: {key} drives a compliance decision and has no evidence")
    gaps = [key for key in DECISION_FIELDS if key not in data]
    return Routing("human_review" if reasons else "auto_accept", reasons, gaps)


# --------------------------------------------------------------------------- #
# 6. Evaluation harness (accuracy is measured, not vibes)
# --------------------------------------------------------------------------- #
GOLDEN_DOCS: list[tuple[str, dict[str, Any]]] = [
    (
        "Acme GmbH (\"Supplier\") provides production support. Designated High risk due to production support access. "
        "Supplier processes personal data on behalf of Customer in Germany. Contract ending 28 February 2027, "
        "EUR 250,000 per annum. Three subprocessors are engaged.",
        {"vendor_name": "Acme GmbH", "risk_tier": "high", "data_processing": True, "jurisdiction": "DE", "annual_spend": "250000"},
    ),
    (
        "Northwind Analytics provides reporting services. No personal data is processed. Contract term: 12 months.",
        {"vendor_name": "Northwind Analytics", "data_processing": False},
    ),
    (
        "Baltic Logistics OÜ handles warehousing in Estonia. Personal data transferred to one subprocessor.",
        {"vendor_name": "Baltic Logistics OÜ", "jurisdiction": "EE", "data_processing": True},
    ),
]


def evaluate(field_names: Sequence[str]) -> dict[str, Any]:
    per_field = {name: {"tp": 0, "fp": 0, "fn": 0} for name in field_names}
    unsupported = 0
    total_claims = 0
    repair_attempts: list[int] = []
    for document, expected in GOLDEN_DOCS:
        outcome = extract_with_repair(document, schema=SCHEMA, model=stub_model)
        data = normalise(outcome.data)
        repair_attempts.append(outcome.attempts)
        for name in field_names:
            got = str(data.get(name)) if name in data else None
            want = str(expected[name]) if name in expected else None
            if got is not None:
                total_claims += 1
            if want is not None and got == want:
                per_field[name]["tp"] += 1
            elif want is not None and got != want:
                per_field[name]["fn"] += 1
            elif want is None and got is not None:
                per_field[name]["fp"] += 1
        unsupported += len(verify_evidence(data, document))

    def prf(counts: Mapping[str, int]) -> dict[str, float]:
        tp, fp, fn = counts["tp"], counts["fp"], counts["fn"]
        precision = tp / (tp + fp) if tp + fp else 1.0
        recall = tp / (tp + fn) if tp + fn else 1.0
        f1 = 2 * precision * recall / (precision + recall) if precision + recall else 0.0
        return {"precision": round(precision, 3), "recall": round(recall, 3), "f1": round(f1, 3)}

    return {
        "per_field": {name: prf(counts) for name, counts in per_field.items()},
        "unsupported_claim_rate": round(unsupported / total_claims, 3) if total_claims else 0.0,
        "avg_model_attempts": round(sum(repair_attempts) / len(repair_attempts), 2),
        "documents": len(GOLDEN_DOCS),
    }


# --------------------------------------------------------------------------- #
# 7. CLI
# --------------------------------------------------------------------------- #
def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()

    for document, _expected in GOLDEN_DOCS:
        outcome = extract_with_repair(document, schema=SCHEMA, model=stub_model)
        # validate the RAW model output, then normalise for storage: validate-then-transform
        schema_errors = validate_schema(outcome.data, SCHEMA)
        data = normalise(outcome.data)
        evidence_issues = verify_evidence(data, document)
        rule_issues = business_rules(data)
        routing = route(data, schema_errors, evidence_issues, rule_issues)
        report = {
            "document_head": document[:60] + "...",
            "attempts": outcome.attempts,
            "raw": outcome.data,
            "stored": data,
            "schema_errors": schema_errors,
            "evidence_issues": evidence_issues,
            "rule_issues": rule_issues,
            "routing": {"decision": routing.decision, "reasons": routing.reasons},
        }
        if args.json:
            print(json.dumps(report, default=str))
        else:
            print(f"\n=== {report['document_head']}")
            print(f"attempts: {outcome.attempts}   routing: {routing.decision}")
            print("stored:  ", json.dumps(data, default=str))
            if routing.data_gaps:
                print("data gaps (raise a task, do not guess):", routing.data_gaps)
            for issue in schema_errors + evidence_issues + rule_issues:
                print("  !", issue)
            for reason in routing.reasons:
                print("  ->", reason)

    print("\n=== eval report")
    print(json.dumps(evaluate(["vendor_name", "risk_tier", "data_processing", "jurisdiction", "annual_spend"]), indent=2))


if __name__ == "__main__":
    main()
