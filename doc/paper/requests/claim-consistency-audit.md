# Implementation Request: claim-consistency-audit

request_id: claim-consistency-audit
source: gatekeeper-review
status: open
owner: implement

## Paper Claim
All quantitative and guarantee-like statements in `doc/paper/sections/*.tex` must be traceable to artifact data, code behavior, or must be weakened.

## Task Type
code_audit

## Required Outputs
- `doc/paper/artifacts/claim-consistency-audit/RESULT.md`
- A Markdown table listing every claim containing: coverage, guarantee, conservative, complete, fastest, tightest, low-count, runtime, primitive count, capV/aabb, r/binMed, vertex gap, or sidecar.
- For each claim: file path, line number, exact phrase, support source, status `supported | partially_supported | unsupported | wording_risk`.
- A list of required writer edits.

## Metrics
- number_of_claims_checked
- number_supported
- number_partially_supported
- number_unsupported
- number_wording_risk

## Inputs
- `doc/paper/sections/*.tex`
- `doc/paper/outline.md`
- `doc/paper/artifacts/*/RESULT.md`
- `doc/paper/artifacts/*/data/*.csv`
- `doc/paper/artifacts/*/data/*.json`

## Acceptance Criteria
- Every numeric value in Tables 2 and 3 is checked against artifact CSV/JSON data.
- Every occurrence of guarantee-like wording is classified.
- RESULT.md includes concrete file/line edits for writer.
- The audit explicitly checks whether artifact rows disagree with table rows.

## Writer Integration Target
- `doc/paper/sections/abstract.tex`
- `doc/paper/sections/introduction.tex`
- `doc/paper/sections/experiments.tex`
- `doc/paper/sections/discussion.tex`
- `doc/paper/sections/conclusion.tex`
