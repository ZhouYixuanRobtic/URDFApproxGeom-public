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

# CMake commands are case-insensitive and ManifoldPlus may appear with extra
# arguments, so match the token within the call instead of exact syntax.
FORBIDDEN = re.compile(
    r"#include\s*[<\"](ManifoldPlus|igl/copyleft)|"
    r"(?:add_subdirectory|target_link_libraries)\s*\([^)]*\bManifoldPlus\b|"
    r"libcgal-dev|libgmp-dev",
    re.IGNORECASE,
)

# .github is deliberately absent: ci.yml already greps the whole repo
# (including *.cmake) with the same forbidden patterns, and scanning the
# workflow itself would false-positive on its own inline grep strings.
SCAN_DIRS = [
    "CMakeLists.txt", "cmake", "app", "bot_utils", "interface",
    "python", "src", "include", "test", "docker",
]
SCAN_SUFFIXES = {".txt", ".cmake", ".cpp", ".h", ".hpp", ".py", ".yml", ".yaml"}


def main() -> int:
    errors: list[str] = []
    scanned = 0
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
            scanned += 1
            for i, line in enumerate(p.read_text(errors="ignore").splitlines(), 1):
                if line.strip().startswith(("#", "//")):
                    continue  # comments are not forbidden dependencies
                if FORBIDDEN.search(line):
                    errors.append(f"{p.relative_to(ROOT)}:{i}: {line.strip()}")
    if not scanned:
        print("error: no files scanned — SCAN_DIRS all missing; refusing to pass.", file=sys.stderr)
        return 1
    if errors:
        print("\n".join(errors))
        print("Commercial source scan failed.", file=sys.stderr)
        return 1
    print(f"Commercial source scan passed ({scanned} files).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
