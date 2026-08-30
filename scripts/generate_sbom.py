#!/usr/bin/env python3
# Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
# All Rights Reserved.

"""Generate a minimal CycloneDX SBOM for URDFApproxGeom.

This is a lightweight, dependency-free stand-in for a full syft/trivy SBOM.
It lists the known direct components from THIRD_PARTY_NOTICES.md plus the
project itself.  Run from the repository root:

    python3 scripts/generate_sbom.py [research|commercial] [output.json]
"""

from __future__ import annotations

import json
import re
import sys
import uuid
from pathlib import Path

COMPONENTS = [
    ("Eigen3", "3.1", "MPL-2.0"),
    ("yaml-cpp", "0.6", "MIT"),
    ("urdfdom", "1.0", "BSD-3-Clause"),
    ("tinyxml2", "8.0", "Zlib"),
    ("pybind11", "2", "BSD-3-Clause"),
    ("GoogleTest", "1", "BSD-3-Clause"),
    ("libigl", "2", "MPL-2.0"),
    ("nlohmann/json", "3", "MIT"),
    ("trimesh", "4.0", "MIT"),
    ("numpy", "1.23", "BSD-3-Clause"),
    ("quickhull.hpp", "1.0", "LicenseRef-Project"),
    ("sphere_tree", "1.0", "LicenseRef-NonCommercial"),
]

RESEARCH_ONLY = {"sphere_tree"}


def main(argv: list[str] | None = None) -> int:
    args = list(sys.argv[1:] if argv is None else argv)
    edition = "research"
    if args and args[0] in ("research", "commercial"):
        edition = args.pop(0)

    out = Path(args[0]) if args else Path(f"build/sbom-{edition}.cyclonedx.json")
    out.parent.mkdir(parents=True, exist_ok=True)

    components = COMPONENTS
    if edition == "commercial":
        components = [c for c in COMPONENTS if c[0] not in RESEARCH_ONLY]

    # Single source of truth for the version: pyproject.toml.
    pyproject = (Path(__file__).resolve().parents[1] / "python" / "pyproject.toml").read_text()
    version = re.search(r'^version\s*=\s*"([^"]+)"', pyproject, re.MULTILINE).group(1)

    bom = {
        "bomFormat": "CycloneDX",
        "specVersion": "1.5",
        # CycloneDX requires a unique serialNumber per BOM instance; a fixed
        # zero UUID would make every BOM look like the same document.
        "serialNumber": f"urn:uuid:{uuid.uuid4()}",
        "version": 1,
        "metadata": {
            "component": {
                "type": "application",
                "name": f"URDFApproxGeom-{edition}",
                "version": version,
            }
        },
        "components": [
            {
                "type": "library",
                "name": name,
                "version": version,
                # LicenseRef-* ids must be defined in the BOM; use a name
                # instead so validators can resolve the entry.
                "licenses": [
                    {"license": {"name": f"{license_id} (see THIRD_PARTY_NOTICES.md)"}}
                    if license_id.startswith("LicenseRef-")
                    else {"license": {"id": license_id}}
                ],
            }
            for name, version, license_id in components
        ],
    }
    out.write_text(json.dumps(bom, indent=2) + "\n")
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
