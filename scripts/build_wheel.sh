#!/usr/bin/env bash
# Build an edition-specific Python wheel with the compiled extension bundled.
#
# Usage:
#   scripts/build_wheel.sh [research|commercial] [path-to-built-python-dir]
#
# Examples:
#   scripts/build_wheel.sh research build/research/python
#   scripts/build_wheel.sh commercial build/commercial/python
set -euo pipefail

EDITION="${1:-research}"
BUILD_PY="${2:-build/python}"

if [[ "$EDITION" != "research" && "$EDITION" != "commercial" ]]; then
  echo "usage: $0 [research|commercial] [path-to-built-python-dir]" >&2
  exit 2
fi

if [[ ! -d "$BUILD_PY" ]]; then
  echo "error: built Python extension directory not found: $BUILD_PY" >&2
  exit 1
fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

# Stage a clean package tree.
cp -a "$ROOT/python/." "$TMP/"
mkdir -p "$TMP/urdf_approx_geom/config"
cp -a "$ROOT/config/." "$TMP/urdf_approx_geom/config/"
cp "$BUILD_PY"/_urdf_approx_geom*.so "$TMP/urdf_approx_geom/"

if [[ "$EDITION" == "commercial" ]]; then
  # Commercial wheel must not ship the multi-sphere default preset.
  rm -f "$TMP/urdf_approx_geom/config/sphereTree/default.yml"
  cp "$ROOT/LICENSE.commercial" "$TMP/LICENSE"
  python3 - "$TMP/pyproject.toml" <<'PY'
import sys
from pathlib import Path
p = Path(sys.argv[1])
s = p.read_text().replace('name = "urdf-approx-geom"', 'name = "urdf-approx-geom-commercial"')
p.write_text(s)
PY
else
  cp "$ROOT/LICENSE" "$TMP/LICENSE"
fi

mkdir -p "$ROOT/dist"
cd "$TMP"
python3 -m build --wheel --outdir "$ROOT/dist" >/dev/null

echo "Built $EDITION wheel in $ROOT/dist"
ls -1 "$ROOT"/dist/urdf_approx_geom*"$EDITION"*.whl 2>/dev/null || ls -1 "$ROOT"/dist/*.whl
