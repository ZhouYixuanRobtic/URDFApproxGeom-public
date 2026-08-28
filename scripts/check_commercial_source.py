#!/usr/bin/env python3
# Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
# All Rights Reserved.

"""Check that commercial edition source paths are free of forbidden dependencies.

Run from the repository root:

    python3 scripts/check_commercial_source.py
"""

from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

FORBIDDEN = re.compile(
    r"#include\s*[<\"](ManifoldPlus|igl/copyleft)|"
    r"add_subdirectory\(\s*third_party/ManifoldPlus\s*\)|"
    r"libcgal-dev|libgmp-dev|target_link_libraries\([^)]*ManifoldPlus"
)

SCAN_DIRS = ["CMakeLists.txt", "app", "bot_utils", "interface", "python", "src", "include", "test", "docker"]
SCAN_SUFFIXES = {".txt", ".cmake", ".cpp", ".h", ".hpp", "Dockerfile"}


def main() -> int:
    errors: list[str] = []
    for name in SCAN_DIRS:
        path = ROOT / name
        if path.is_dir():
            files = [p for p in path.rglob("*") if p.suffix in SCAN_SUFFIXES or p.name == "Dockerfile"]
        elif path.is_file():
            files = [path]
        else:
            continue
        for p in files:
            if "__pycache__" in p.parts:
                continue
            for i, line in enumerate(p.read_text(errors="ignore").splitlines(), 1):
                if FORBIDDEN.search(line):
                    errors.append(f"{p.relative_to(ROOT)}:{i}: {line.strip()}")
    if errors:
        print("\n".join(errors))
        print("Commercial source scan failed.", file=sys.stderr)
        return 1
    print("Commercial source scan passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
