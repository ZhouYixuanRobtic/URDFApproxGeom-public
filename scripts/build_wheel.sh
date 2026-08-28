#!/usr/bin/env bash
# Build a Python wheel and rename it for the requested edition.
# Usage: scripts/build_wheel.sh [research|commercial]
set -euo pipefail

EDITION="${1:-research}"
if [[ "$EDITION" != "research" && "$EDITION" != "commercial" ]]; then
  echo "usage: $0 [research|commercial]" >&2
  exit 2
fi

rm -rf dist build/python
python3 -m pip install --quiet build 2>/dev/null || true
python3 -m build --wheel

# Rename to the edition-specific wheel name.
for wheel in dist/urdf_approx_geom-*.whl; do
  [[ -e "$wheel" ]] || continue
  new_name="${wheel/urdf_approx_geom-/urdf_approx_geom_${EDITION}-}"
  mv "$wheel" "$new_name"
  echo "built $new_name"
done
