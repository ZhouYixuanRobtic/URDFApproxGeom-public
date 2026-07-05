#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys
import tempfile
import xml.etree.ElementTree as ET

import trimesh

ROOT = pathlib.Path(__file__).resolve().parents[1]
FR3_URDF = ROOT / "resources/fr3/urdf/fr3.urdf"
ARM_LINKS = {f"fr3_link{i}" for i in range(8)}


def import_project_modules():
    sys.path.insert(0, str(ROOT / "python"))
    sys.path.insert(0, str(ROOT / "build/python"))
    scripts_dir = str(ROOT / "scripts")
    if scripts_dir not in sys.path:
        sys.path.insert(0, scripts_dir)
    from urdf_approx_geom import generate
    from check_capsule_coverage import evaluate_capsules
    return generate, evaluate_capsules


def visual_meshes(urdf_path: pathlib.Path) -> dict[str, pathlib.Path]:
    tree = ET.parse(urdf_path)
    out: dict[str, pathlib.Path] = {}
    for link in tree.iter("link"):
        name = link.get("name")
        if name not in ARM_LINKS:
            continue
        visual = link.find("visual")
        if visual is None:
            continue
        mesh = visual.find("geometry/mesh")
        if mesh is None or not mesh.get("filename"):
            continue
        fn = pathlib.Path(mesh.get("filename"))
        if str(fn).startswith("/workspace/"):
            fn = ROOT / str(fn).removeprefix("/workspace/")
        out[name] = fn
    return out


def mesh_volume(path: pathlib.Path) -> float:
    mesh = trimesh.load(path, force="mesh")
    if isinstance(mesh, trimesh.Scene):
        mesh = trimesh.util.concatenate(tuple(mesh.geometry.values()))
    volume = float(abs(mesh.volume))
    if volume <= 0.0:
        hull = mesh.convex_hull
        volume = float(abs(hull.volume))
    return volume


def capsule_rows(volume_samples: int) -> dict[str, dict[str, float]]:
    generate, evaluate_capsules = import_project_modules()
    meshes = visual_meshes(FR3_URDF)
    mesh_volumes = {link: mesh_volume(path) for link, path in meshes.items()}

    work = pathlib.Path(tempfile.mkdtemp(prefix="fr3_capsule_union_report_"))
    rows: dict[str, dict[str, float]] = {}
    for preset in ("single", "default", "high_detail"):
        out = work / f"fr3_capsule_{preset}.urdf"
        result = generate("capsule", FR3_URDF, out, preset=preset, mesh_source="visual")
        metrics = evaluate_capsules(
            str(result.json_path),
            str(FR3_URDF),
            mesh_source="visual",
            volume_samples=volume_samples,
        )
        worst = 0.0
        for row in metrics["links"]:
            link = row["link"]
            if link not in ARM_LINKS or link not in mesh_volumes:
                continue
            denom = mesh_volumes[link]
            if denom > 0.0:
                worst = max(worst, float(row["capsule_union_volume"]) / denom)
        rows[preset] = {
            "primitives": float(result.primitive_count),
            "worst_vol_dae": worst,
        }
    return rows


def markdown_capsule_rows(rows: dict[str, dict[str, float]]) -> dict[str, str]:
    labels = {
        "single": "capsule `single`",
        "default": "capsule `default`",
        "high_detail": "capsule `high_detail`",
    }
    return {
        preset: f"| {labels[preset]:27} | {int(values['primitives']):3d} | {values['worst_vol_dae']:.2f} |"
        for preset, values in rows.items()
    }


def update_readme(readme: pathlib.Path, rows: dict[str, dict[str, float]]) -> None:
    text = readme.read_text()
    replacements = markdown_capsule_rows(rows)
    patterns = {
        "single": r"\| capsule `single`\s+\|\s+\d+\s+\|\s+[0-9.]+\s+\|",
        "default": r"\| capsule `default`\s+\|\s+\d+\s+\|\s+[0-9.]+\s+\|",
        "high_detail": r"\| capsule `high_detail`\s+\|\s+\d+\s+\|\s+[0-9.]+\s+\|",
    }
    for preset, pattern in patterns.items():
        text = re.sub(pattern, replacements[preset], text)
    text = text.replace(
        "`vol/dae` = approximation volume ÷ `.dae` volume",
        "`vol/dae` = approximation occupied volume ÷ `.dae` volume; capsule rows use sampled capsule-union volume",
    )
    readme.write_text(text)


def update_config_presets(path: pathlib.Path, rows: dict[str, dict[str, float]]) -> None:
    text = path.read_text()
    default = rows["default"]["worst_vol_dae"]
    high = rows["high_detail"]["worst_vol_dae"]
    sentence = (
        f"- **`high_detail`**: more axial sections (`NSections: 6`) and a larger capsule budget "
        f"(`MaxCapsulesPerLink: 16`). Raises the detail ceiling but does **not** guarantee tighter "
        f"output than `default`; union-volume scoring removes overlap, but coverage growth and "
        f"section matching can still make one preset larger on a specific link. On FR3, worst "
        f"`vol/dae` by sampled capsule union is `default` {default:.2f} vs `high_detail` {high:.2f}."
    )
    text = re.sub(r"- \*\*`high_detail`\*\*:.*", sentence, text)
    path.write_text(text)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--volume-samples", type=int, default=96)
    parser.add_argument("--update", action="store_true")
    args = parser.parse_args()

    rows = capsule_rows(args.volume_samples)
    print(json.dumps(rows, indent=2, sort_keys=True))
    print()
    for line in markdown_capsule_rows(rows).values():
        print(line)

    if args.update:
        update_readme(ROOT / "README.md", rows)
        update_config_presets(ROOT / "docs/config-presets.md", rows)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
