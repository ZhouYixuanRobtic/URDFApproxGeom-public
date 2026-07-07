# Gatekeeper Review: 2026-07-06

## Verdict

Reject in current form. The manuscript is salvageable as a rigorous tool paper, but only after central claims are weakened and new evidence is produced.

## Fatal Issues To Address

1. The draft repeatedly uses "coverage-preserving" or equivalent guarantee language while its own FR3 artifacts show every capsule preset leaves positive vertex gaps.
2. The phrase "fundamental representation trade-off" is mathematically invalid because a sufficiently inflated sphere or capsule can cover any bounded vertex set; the actual trade-off is tightness versus over-coverage under a primitive budget and the current heuristic.
3. The originality claim is too broad. The manuscript must acknowledge that cross-section capsule fitting is conceptually inherited from prior work; the defensible contribution is integration into a URDF toolchain with sidecar analytic parameters and validation workflows.
4. The experiment section is too narrow for broad robotics claims: it uses only FR3 arm links and does not measure downstream collision or distance-query performance.
5. Vertex-only coverage is not enough to establish conservative surface coverage. The metric must be named honestly and complemented with surface sampling if conservative collision claims remain.
6. Artifact numbers and paper tables must be rechecked after every writer pass.

## Required Direction

Gate A: make the current paper truthful with FR3-only evidence.

Gate B: add evidence requests for surface coverage, synthetic morphology stress tests, downstream distance-query timing, and prior-art positioning.
