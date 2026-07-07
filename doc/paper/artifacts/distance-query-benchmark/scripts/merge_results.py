#!/usr/bin/env python3
"""merge_results.py — Merge analytic timing CSV with mesh timing CSV into final output."""

import csv
from pathlib import Path

_script = Path(__file__).resolve()
DATA_DIR = _script.parents[1] / "data"

analytic_csv = DATA_DIR / "distance_query_timing.csv"
mesh_csv = DATA_DIR / "mesh_timing.csv"
output_csv = DATA_DIR / "distance_query_timing.csv"

# Read mesh data: link -> row
mesh_rows = {}
with open(mesh_csv) as f:
    reader = csv.DictReader(f)
    for row in reader:
        link = row["link"]
        # Keep only the columns we need
        mesh_rows[link] = {
            "total_time_ms": row["total_time_ms"],
            "ns_per_point_query": row["ns_per_point_query"],
            "ns_per_point_query_p95": row["ns_per_point_query_p95"],
            "ns_per_point_query_std": row["ns_per_point_query_std"],
            "backend": "mesh",
            "warmup": row["warmup"],
            "repeats": row["repeats"],
            "note": "",
        }

# Read analytic CSV, replace BLOCKED mesh rows with real data
rows_out = []
with open(analytic_csv) as f:
    reader = csv.DictReader(f)
    fieldnames = reader.fieldnames.copy()

    for row in reader:
        if row["backend"] == "mesh" and row["ns_per_point_query"] == "":
            # BLOCKED row — replace with real mesh data if available
            link = row["link"]
            if link in mesh_rows:
                m = mesh_rows[link]
                row["total_time_ms"] = m["total_time_ms"]
                row["ns_per_point_query"] = m["ns_per_point_query"]
                row["ns_per_point_query_p95"] = m["ns_per_point_query_p95"]
                row["ns_per_point_query_std"] = m["ns_per_point_query_std"]
                row["backend"] = m["backend"]
                row["warmup"] = m["warmup"]
                row["repeats"] = m["repeats"]
                row["note"] = m["note"]
            else:
                row["note"] = "BLOCKED: no mesh timing data"
        # Ensure integer fields
        if row.get("warmup") == "":
            row["warmup"] = ""
        if row.get("repeats") == "":
            row["repeats"] = ""
        rows_out.append(row)

# Write merged CSV
with open(output_csv, "w", newline="") as f:
    writer = csv.DictWriter(f, fieldnames=fieldnames, extrasaction="ignore")
    writer.writeheader()
    writer.writerows(rows_out)

print(f"Merged {len(rows_out)} rows to {output_csv}")

# Summary stats
print("\n=== Summary ===")
for row in rows_out:
    if row["backend"] == "mesh":
        print(f"  Mesh: {row['link']} = {row['ns_per_point_query']} ns/query")
