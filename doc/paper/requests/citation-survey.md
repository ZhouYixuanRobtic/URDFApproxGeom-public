# Implementation Request: citation-survey

request_id: citation-survey
source: writer
status: open
owner: implement

## Paper Claim
Related-work positioning relies on verified, correctly-formatted BibTeX entries for all cited works.

## Task Type
citation_check

## Required Outputs
- Verified BibTeX entries for all citations in `doc/paper/ref.bib`, checking:
  - Author names are complete and correctly formatted.
  - Publication titles are correct.
  - Venue, year, volume, pages are accurate.
  - DOI or arXiv ID is present where available.
  - For software/online references: URL is still valid and correctly formatted.

- Additional citations to find and add. The writer has used the following citations that need verification:
  1. `urdf_doc` — URDF documentation, verify current URL.
  2. `xacro` — Xacro documentation, verify current URL.
  3. `todorov2012mujoco` — MuJoCo IROS 2012 paper.
  4. `drake` — Drake robotics library.
  5. `coumans2018pybullet` — PyBullet.
  6. `pan2012fcl` — FCL ICRA 2012.
  7. `lien2008acd` — Approximate convex decomposition SPM 2008.
  8. `mamou2016vhacd` — V-HACD GPU Technology Conference 2016.
  9. `barber1996quickhull` — Quickhull ACM TOMS 1996.
  10. `cgal` — CGAL library.
  11. `jacobson2018libigl` — libigl.
  12. `huang2022manifoldplus` — ManifoldPlus ECCV 2022.
  13. `bradshaw2002sphere` — Sphere tree PhD thesis Cambridge 2002.
  14. `mlund_spheretree` — sphere_tree library.
  15. `wu2018capsule` — Wu et al. ECCV 2018 capsule network (verify this is actually the cross-section/capsule-fitting paper, not the capsule network paper — may need a different target).
  16. `larsen1999ssv` — Swept sphere volumes ICRA 1999.
  17. `mukadam2018trapezoidal` — Trapezoidal capsules RSS 2018.
  18. `koptev2023neural` — Neural distance functions IROS 2023.
  19. `eigen` — Eigen library.
  20. `urdfdom` — urdfdom library.
  21. `welzl1991mec` — Welzl's smallest enclosing disk.
  22. `robot_viewer` — Fan Ziqi robot_viewer.
  23. `franka2020fr3` — Franka FR3.
  24. `trimesh` — Trimesh library.
  25. `sdformat` — SDFormat.

- Additional search targets for related work:
  - Automatic capsule fitting for robot collision geometry — are there other papers besides Wu2018?
  - Sphere-tree and bounding-volume hierarchy papers specific to robotics.
  - Works on URDF collision geometry authoring/simplification.
  - Any relevant papers from ICRA, IROS, RSS, TRO in 2023--2026 on collision geometry generation.

## Inputs
- Current bibliography: `doc/paper/ref.bib`
- Related work section: `doc/paper/sections/related_work.tex`

## Acceptance Criteria
- Every citation in `ref.bib` has been verified for correctness.
- Missing DOIs, URLs, or fields have been filled in.
- Any broken or outdated references have been flagged and replaced.
- Additional relevant citations for the related-work section have been identified.
- Results recorded in `doc/paper/artifacts/citation-survey/RESULT.md` with corrected BibTeX entries.

## Writer Integration Target
- Section II (Related Work), ref.bib (entire file).
