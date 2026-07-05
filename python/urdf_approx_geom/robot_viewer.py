"""robot_viewer integration.

`robot_viewer` is a web URDF/MJCF viewer (https://github.com/fan-ziqi/robot_viewer).
It loads a URDF plus its mesh directory via drag-and-drop and renders the
``<visual>`` and ``<collision>`` geometry with a toggle, which makes it the
default way to compare the high-precision ``.dae`` visual meshes against the
generated capsule/sphere/convex collision approximation.

The generated URDFs reference absolute ``/workspace/...`` mesh paths that do
not exist on a host. :func:`bundle` rewrites every mesh reference to a path
relative to the bundle and copies the referenced files alongside the URDF, so
the bundle is self-contained and portable.
"""

from __future__ import annotations

import os
import pathlib
import shutil
import subprocess
import urllib.request
import xml.etree.ElementTree as ET

DEFAULT_ROOT_CANDIDATES = (
    "/home/admin1/ref/robot_viewer",
)


def repo_root() -> pathlib.Path:
    return pathlib.Path(__file__).resolve().parents[2]


def find_robot_viewer_root() -> pathlib.Path | None:
    """Locate a robot_viewer checkout usable as a dev server, or None."""
    env = os.environ.get("ROBOT_VIEWER_ROOT")
    candidates: list[pathlib.Path] = []
    if env:
        candidates.append(pathlib.Path(env))
    candidates.append(repo_root() / "robot_viewer")
    candidates.extend(pathlib.Path(c) for c in DEFAULT_ROOT_CANDIDATES)
    for cand in candidates:
        if (cand / "package.json").is_file():
            return cand.resolve()
    return None


def _collect_mesh_paths(urdf_path: pathlib.Path) -> tuple[list[tuple[ET.Element, pathlib.Path]], ET.ElementTree]:
    """Return (element, resolved_source) pairs for every resolvable mesh ref, plus the parsed tree."""
    tree = ET.parse(urdf_path)
    pairs: list[tuple[ET.Element, pathlib.Path]] = []
    for mesh in tree.iter("mesh"):
        fn = mesh.get("filename")
        if not fn:
            continue
        src = pathlib.Path(fn)
        if not src.is_absolute():
            src = urdf_path.parent / src
        if src.exists():
            pairs.append((mesh, src.resolve()))
    return pairs, tree


def bundle(urdf_path: str | pathlib.Path, out_dir: str | pathlib.Path) -> pathlib.Path:
    """Copy *urdf_path* + every referenced mesh into *out_dir* with relative paths.

    Meshes land in ``<out_dir>/meshes/<basename>``; the URDF's ``filename``
    attributes are rewritten to ``meshes/<basename>``. The returned URDF path is
    self-contained: drop the bundle directory onto robot_viewer's file tree.
    """
    return bundle_many([urdf_path], out_dir)[0]


def bundle_many(urdf_paths: list[str | pathlib.Path], out_dir: str | pathlib.Path) -> list[pathlib.Path]:
    """Bundle multiple URDFs into one shared *out_dir* with a single deduped ``meshes/`` tree.

    Mesh files referenced by more than one URDF (e.g. the shared ``.dae`` visual
    meshes) are copied once. Each URDF is rewritten with relative mesh paths and
    written next to the others, so the whole directory loads as one drag-and-drop
    into robot_viewer and the user can click between approximation variants.
    """
    out_dir = pathlib.Path(out_dir).resolve()
    mesh_dir = out_dir / "meshes"
    mesh_dir.mkdir(parents=True, exist_ok=True)
    copied: dict[str, str] = {}  # resolved source -> relative target
    outputs: list[pathlib.Path] = []
    for raw in urdf_paths:
        urdf_path = pathlib.Path(raw).resolve()
        if not urdf_path.is_file():
            raise FileNotFoundError(f"URDF not found: {urdf_path}")
        pairs, tree = _collect_mesh_paths(urdf_path)
        for mesh, src in pairs:
            key = str(src)
            if key in copied:
                mesh.set("filename", copied[key])
                continue
            dst = mesh_dir / src.name
            stem, suffix = src.stem, src.suffix
            counter = 0
            while dst.exists() and dst.stat().st_size != src.stat().st_size:
                counter += 1
                dst = mesh_dir / f"{stem}_{counter}{suffix}"
            if not dst.exists():
                shutil.copy2(src, dst)
            rel = "meshes/" + dst.name
            copied[key] = rel
            mesh.set("filename", rel)
        out_urdf = out_dir / urdf_path.name
        tree.write(out_urdf, encoding="utf-8", xml_declaration=True)
        outputs.append(out_urdf)
    return outputs


def _dev_server_up(url: str) -> bool:
    try:
        urllib.request.urlopen(url, timeout=0.5)
        return True
    except Exception:
        return False


def open_in_robot_viewer(
    bundle_urdf: str | pathlib.Path,
    *,
    launch: bool = True,
    port: str | None = None,
) -> int:
    """Print drag-and-drop instructions for *bundle_urdf*; launch dev server if possible.

    Returns 0 on success. If no local robot_viewer checkout is found, falls back
    to the hosted demo URL (model stays in the browser).
    """
    bundle_dir = pathlib.Path(bundle_urdf).resolve().parent
    print(f"bundle ready: {bundle_dir}")
    root = find_robot_viewer_root()
    if root is None:
        print(
            "robot_viewer checkout not found locally.\n"
            "  Option A (hosted): open http://viewer.robotsfan.com and drag the bundle dir onto the file tree.\n"
            "  Option B (local):  set ROBOT_VIEWER_ROOT to a robot_viewer checkout."
        )
        return 0

    port = port or os.environ.get("ROBOT_VIEWER_PORT", "5173")
    url = f"http://localhost:{port}"
    if launch:
        if _dev_server_up(url):
            print(f"robot_viewer dev server already running at {url}")
        else:
            try:
                subprocess.Popen(
                    ["pnpm", "run", "dev", "--port", port, "--host"],
                    cwd=str(root),
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL,
                    start_new_session=True,
                )
                print(f"launched robot_viewer dev server at {url} (root={root})")
            except FileNotFoundError:
                print(
                    f"pnpm not available here; start the server manually:\n"
                    f"  pnpm --dir {root} run dev"
                )
    print(
        f"open {url} in a browser, then drag this directory onto the file tree:\n"
        f"  {bundle_dir}\n"
        f"toggle Visual / Collision in the viewer to compare the .dae meshes against the approximation."
    )
    return 0
