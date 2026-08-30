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

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# Resolve a relative BUILD_PY against the repo root so the script is cwd-independent.
case "$BUILD_PY" in
  /*) ;;
  *) BUILD_PY="$ROOT/$BUILD_PY" ;;
esac

if [[ ! -d "$BUILD_PY" ]]; then
  echo "error: built Python extension directory not found: $BUILD_PY" >&2
  exit 1
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

# Verify the staged extension matches the requested edition, so a research-built
# .so can never leak into a commercial wheel (or vice versa).
if ! python3 - "$BUILD_PY" "$EDITION" <<'PY'
import glob, importlib.util, sys
ext_dir, edition = sys.argv[1], sys.argv[2]
so_files = glob.glob(f"{ext_dir}/_urdf_approx_geom*.so")
if len(so_files) != 1:
    sys.exit(f"error: expected exactly one _urdf_approx_geom*.so in {ext_dir}, found {len(so_files)}")
spec = importlib.util.spec_from_file_location("_urdf_approx_geom", so_files[0])
if spec is None or spec.loader is None:
    sys.exit(f"error: cannot import {so_files[0]}")
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
expected = edition == "research"
if hasattr(mod, "spherized") != expected:
    sys.exit(f"error: {so_files[0]} is for the {'research' if hasattr(mod, 'spherized') else 'commercial'} edition, not '{edition}'")
PY
then
  exit 1
fi

# Stage a clean package tree.
cp -a "$ROOT/python/." "$TMP/"
mkdir -p "$TMP/urdf_approx_geom/config"
cp -a "$ROOT/config/." "$TMP/urdf_approx_geom/config/"
mapfile -t SO_FILES < <(compgen -G "$BUILD_PY/_urdf_approx_geom*.so" || true)
if [[ "${#SO_FILES[@]}" -ne 1 ]]; then
  echo "error: expected exactly one _urdf_approx_geom*.so in $BUILD_PY, found ${#SO_FILES[@]}" >&2
  exit 1
fi
cp "${SO_FILES[0]}" "$TMP/urdf_approx_geom/"
# Prune caches/artifacts staged from the working tree.
find "$TMP" -type d -name __pycache__ -prune -exec rm -rf {} +
find "$TMP" -name '*.egg-info' -o -name '*.pyc' | xargs -r rm -rf

if [[ "$EDITION" == "commercial" ]]; then
  # Commercial wheel must not ship the multi-sphere default preset.
  rm -f "$TMP/urdf_approx_geom/config/sphereTree/default.yml"
  cp "$ROOT/LICENSE.commercial" "$TMP/LICENSE"
  python3 - "$TMP/pyproject.toml" <<'PY'
import sys
from pathlib import Path
p = Path(sys.argv[1])
s = p.read_text().replace('name = "urdf-approx-geom"', 'name = "urdf-approx-geom-commercial"')
if 'urdf-approx-geom-commercial' not in s:
    raise SystemExit('failed to rename distribution in pyproject.toml')
p.write_text(s)
PY
else
  cp "$ROOT/LICENSE" "$TMP/LICENSE"
fi

if ! python3 -c "import build" 2>/dev/null; then
  echo "error: 'python3 -m build' requires the 'build' package (pip3 install build)" >&2
  exit 1
fi

mkdir -p "$ROOT/dist"
cd "$TMP"
# Force a platform/ABI-specific wheel tag: the .so is CPython/OS-specific, and a
# py3-none-any wheel would install (then crash) anywhere pip accepts it.
PY_TAG="cp$(python3 -c 'import sys; print(f"{sys.version_info.major}{sys.version_info.minor}")')"
PLAT="$(python3 -c 'import sysconfig; print(sysconfig.get_platform())')"
python3 -m build --wheel --outdir "$ROOT/dist" \
  --config-setting=--python-tag="$PY_TAG" --config-setting=--plat-name="$PLAT" >/dev/null

echo "Built $EDITION wheel:"
ls -1t "$ROOT"/dist/*.whl | head -1
