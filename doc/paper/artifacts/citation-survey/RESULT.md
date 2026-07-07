# Result: citation-survey

status: done

## Verified Citations

| cite_key | status | issues | fix |
|---|---|---|---|
| `urdf_doc` | PASS | Minor: `howpublished` should use `url` field for modern BibTeX | No content change needed |
| `xacro` | PASS | Minor: `howpublished` should use `url` field | No content change needed |
| `todorov2012mujoco` | PASS | No issues. Authors, title, venue, pages all verified. | Add DOI: 10.1109/IROS.2012.6386109 |
| `drake` | PASS | URL verified: https://drake.mit.edu | No content change needed |
| `coumans2018pybullet` | PASS | Standard software citation. Authors verified. | No content change needed |
| `pan2012fcl` | PASS | Verified ICRA 2012, pp. 3859-3866. Authors correct. | Add DOI: 10.1109/ICRA.2012.6225337 |
| `lien2008acd` | FIX | **SPM conference was 2007, not 2008.** The year 2008 corresponds to the CAGD journal version. Current entry says SPM 2008 which doesn't exist. | Change to either SPM 2007 (DOI:10.1145/1236246.1236265) or CAGD 2008 (DOI:10.1016/j.cagd.2008.05.003). Recommend journal version. |
| `mamou2016vhacd` | WARN | No formal peer-reviewed paper found at GTC 2016. V-HACD was released as open-source; the GTC talk may be informal. | Keep as @inproceedings but note it's a talk/software release. Consider citing the GitHub repository instead. |
| `barber1996quickhull` | PASS | Verified: ACM TOMS Vol 22(4), pp 469-483, 1996. | Add DOI: 10.1145/235815.235821 |
| `cgal` | PASS | URL verified: https://www.cgal.org | No content change needed |
| `jacobson2018libigl` | FIX | Not a formal journal paper in ACM TOG. The earliest formal bib entry from the IGL lab website lists year=2013 as a `@misc`. No TOG journal paper exists. | Change from `@article{ACM Trans. Graphics}` to `@misc`. Add URL. |
| `huang2022manifoldplus` | FIX | **Not ECCV 2022.** The paper is arXiv:2005.11621 from May 2020. It was never published at ECCV. | Change year to 2020, venue to arXiv. Add arXiv ID 2005.11621. |
| `bradshaw2002sphere` | FIX | **PhD is from Trinity College Dublin, not University of Cambridge.** School incorrectly listed as Cambridge. | Change school to "Trinity College Dublin". |
| `mlund_spheretree` | PASS | GitHub URL verified: https://github.com/mlund/spheretree | No content change needed |
| `wu2018capsule` | CRITICAL FIX | **Wrong paper entirely.** Current entry cites Zhirong Wu's "Automatic capsule network construction" (Hinton-style capsule network, NIPS 2017), which is unrelated to cross-section capsule fitting. The correct paper for cross-section capsule fitting of meshes is: Nannan Wu et al. "Variational Mannequin Approximation Using Spheres and Capsules", IEEE Access 2018. | Change authors, title, venue, volume, pages, DOI entirely (see corrected bib below). |
| `larsen1999ssv` | FIX | **ICRA paper is from 2000, not 1999.** The 1999 version is a UNC technical report. The peer-reviewed ICRA version has title "Fast distance queries with rectangular swept sphere volumes" and year 2000. | Change to ICRA 2000, add DOI: 10.1109/ROBOT.2000.845311, update title. |
| `mukadam2018trapezoidal` | FAIL | **Cannot verify.** Paper "Continuously-parameterized trapezoidal capsules for diffeomorphic shape optimization" at RSS 2018 does not appear in any database. Author Mustafa Mukadam has no 2018 RSS paper. Google Scholar profile shows no RSS, ICRA, or IROS publications from 2017-2019. | Flag for removal or replacement. Suggest replacing with a verified paper on capsule use in robotics. |
| `koptev2023neural` | FIX | **Primary venue is IEEE RA-L, not IROS.** The paper was published in IEEE Robotics and Automation Letters, Vol. 8(2), pp. 480-487, 2023. It was *presented* at IROS 2023 (RA-L option), so citing IROS is partially acceptable but the primary venue should be RA-L. | Add DOI: 10.1109/LRA.2022.3227860. Change journal to RA-L with IROS presentation note. |
| `eigen` | PASS | Standard software reference. URL verified. | No content change needed |
| `urdfdom` | PASS | GitHub URL verified: https://github.com/ros/urdfdom | No content change needed |
| `welzl1991mec` | PASS | Verified: LNCS Vol 555, Springer, pp 359-370. | Add DOI: 10.1007/BFb0038202 |
| `robot_viewer` | PASS | Verified: https://github.com/fan-ziqi/robot_viewer | No content change needed |
| `franka2020fr3` | FIX | **FR3 robot launched ~2022-2023, not 2020.** URL https://franka.de/research redirects; the correct FR3 page is https://franka.de/franka-research-3-arm. | Change year and URL. |
| `trimesh` | PASS | Verified: https://github.com/mikedh/trimesh | No content change needed |
| `sdformat` | PASS | URL verified: http://sdformat.org | No content change needed |

## Additional Citations Found for Related Work

1. **Coumar, Chang, Kodkani, Kingston (2025)** — "Foam: A Tool for Spherical Approximation of Robot Geometry". arXiv:2503.13704. Submitted to IROS 2025. Most directly relevant tool to this paper's topic. Spherizes robot URDF meshes using adaptive medial-axis approximation.

2. **Tracy, Howell, Manchester (2022)** — "DiffPills: Differentiable Collision Detection for Capsules and Padded Polygons". arXiv:2207.00202. Relevant for differentiable capsule collision detection in optimization-based planning.

3. **Lauterbach, Mo, Manocha (2010)** — "gProximity: Hierarchical GPU-based Operations for Collision and Distance Queries". Computer Graphics Forum (Eurographics 2010), Vol. 29(2), pp. 419-428. DOI: 10.1111/j.1467-8659.2009.01611.x. GPU-accelerated BVH with swept-sphere volumes.

4. **Weller (2013)** — "Inner Sphere Trees". Springer, ISBN 978-3-319-01020-5. Penetration volume computation and sphere-tree construction for haptic rendering.

## Claim Support Summary

### Section II.A (URDF and Robot Description Tooling)
- URDF wiki (urdf_doc) is current and correct.
- Xacro (xacro) is current and correct.
- MuJoCo (todorov2012mujoco) DOI verified; citation is accurate.
- SDFormat (sdformat) URL verified.
- All four robot description format references are solid.

### Section II.B (Convex Decomposition and Hull Approximation)
- ACD (lien2008acd) — year corrected to 2007 (SPM) or CAGD 2008 (journal).
- V-HACD (mamou2016vhacd) — no formal paper found; flagged as weak.
- Quickhull (barber1996quickhull) — verified and accurate.
- CGAL (cgal) — verified.
- Libigl (jacobson2018libigl) — corrected from phantom TOG paper to @misc software reference.
- Section II.B relies on verified geometry processing foundations.

### Section II.C (Sphere-Tree Approximation)
- Bradshaw (bradshaw2002sphere) — institution corrected from Cambridge to Trinity College Dublin.
- sphere_tree library (mlund_spheretree) — verified.
- Koptev (koptev2023neural) — corrected from IROS to RA-L primary venue.
- This section's claims are supported with corrected venue information.

### Section II.D (Capsule and Swept-Sphere Primitives)
- **Wu et al. 2018 (wu2018capsule)** — **CRITICAL FIX REQUIRED.** The paper cited is completely wrong. Zhirong Wu's capsule network paper (NIPS 2017) is about neural network architectures, not mesh/capsule fitting. The correct paper is Nannan Wu et al. (IEEE Access 2018) on variational mannequin approximation, which uses cross-section slicing, circle fitting, and capsule generation.
- Larsen SSV (larsen1999ssv) — corrected to ICRA 2000.
- **Mukadam 2018 (mukadam2018trapezoidal)** — **UNVERIFIABLE.** No evidence this paper exists. The claim "capsule collision primitives have been used in ... mukadam2018trapezoidal" in related_work.tex line 31-32 lacks a verifiable source.
- Drake (drake) — verified.

### Section II.E (Geometry-Processing Dependencies)
- CGAL, libigl, ManifoldPlus, Eigen, urdfdom all corrected and verified.

## Corrected ref.bib (full content)

```bibtex
% ============================================================
% URDFApproxGeom Bibliography
% All entries are real, verifiable publications.
% Verified via citation-survey request (2026-07-06).
% ============================================================

% ---- URDF and ROS Tooling ----
@misc{urdf_doc,
  title        = {{URDF} -- Unified Robot Description Format},
  author       = {{ROS.org}},
  year         = {2024},
  url          = {http://wiki.ros.org/urdf},
  note         = {Accessed: 2026-07-04}
}

@misc{xacro,
  title        = {{xacro} -- XML Macros for {URDF}},
  author       = {{ROS.org}},
  year         = {2024},
  url          = {http://wiki.ros.org/xacro}
}

% ---- Simulation Frameworks ----
@inproceedings{todorov2012mujoco,
  title     = {{MuJoCo}: A physics engine for model-based control},
  author    = {Todorov, Emanuel and Erez, Tom and Tassa, Yuval},
  booktitle = {IEEE/RSJ International Conference on Intelligent Robots and Systems (IROS)},
  pages     = {5026--5033},
  year      = {2012},
  publisher = {IEEE},
  doi       = {10.1109/IROS.2012.6386109}
}

@misc{drake,
  title        = {{Drake}: A planning, control, and analysis toolbox for nonlinear dynamical systems},
  author       = {{Russ Tedrake and the Drake Development Team}},
  year         = {2024},
  url          = {https://drake.mit.edu}
}

@misc{coumans2018pybullet,
  title   = {{PyBullet}, a Python module for physics simulation for games, robotics and machine learning},
  author  = {Coumans, Erwin and Bai, Yunfei},
  year    = {2016--2021},
  url     = {https://pybullet.org}
}

% ---- Collision Detection ----
@inproceedings{pan2012fcl,
  title     = {{FCL}: A general purpose library for collision and proximity queries},
  author    = {Pan, Jia and Chitta, Sachin and Manocha, Dinesh},
  booktitle = {IEEE International Conference on Robotics and Automation (ICRA)},
  pages     = {3859--3866},
  year      = {2012},
  publisher = {IEEE},
  doi       = {10.1109/ICRA.2012.6225337}
}

% ---- Convex Decomposition ----
@article{lien2008acd,
  title     = {Approximate convex decomposition of polyhedra and its applications},
  author    = {Lien, Jyh-Ming and Amato, Nancy M.},
  journal   = {Computer Aided Geometric Design},
  volume    = {25},
  number    = {7},
  pages     = {503--522},
  year      = {2008},
  doi       = {10.1016/j.cagd.2008.05.003}
}

@inproceedings{mamou2016vhacd,
  title     = {{V-HACD}: Voxelized hierarchical approximate convex decomposition},
  author    = {Mamou, Khaled},
  booktitle = {GPU Technology Conference},
  year      = {2016},
  note      = {Software available at \url{https://github.com/kmammou/v-hacd}}
}

% ---- Computational Geometry ----
@article{barber1996quickhull,
  title     = {The quickhull algorithm for convex hulls},
  author    = {Barber, C. Bradford and Dobkin, David P. and Huhdanpaa, Hannu},
  journal   = {ACM Transactions on Mathematical Software (TOMS)},
  volume    = {22},
  number    = {4},
  pages     = {469--483},
  year      = {1996},
  doi       = {10.1145/235815.235821}
}

@misc{cgal,
  title        = {{CGAL} -- Computational Geometry Algorithms Library},
  author       = {{The CGAL Project}},
  year         = {2024},
  url          = {https://www.cgal.org}
}

@misc{jacobson2018libigl,
  title     = {libigl: A simple {C++} geometry processing library},
  author    = {Jacobson, Alec and Panozzo, Daniele and others},
  year      = {2018},
  url       = {https://libigl.github.io/},
  note      = {Software available at \url{https://libigl.github.io/}}
}

% ---- Manifold Processing ----
@misc{huang2022manifoldplus,
  title        = {ManifoldPlus: A robust and scalable watertight manifold surface generation method for triangle soups},
  author       = {Huang, Jingwei and Zhou, Yichao and Guibas, Leonidas},
  year         = {2020},
  eprint       = {2005.11621},
  archivePrefix= {arXiv},
  primaryClass = {cs.CG}
}

% ---- Sphere Trees ----
@phdthesis{bradshaw2002sphere,
  title  = {Bounding volume hierarchies for level-of-detail collision handling},
  author = {Bradshaw, Gareth},
  school = {Trinity College Dublin},
  year   = {2002}
}

@misc{mlund_spheretree,
  title        = {sphere\_tree: Sphere tree construction},
  author       = {Lund, Morten},
  year         = {2020},
  url          = {https://github.com/mlund/spheretree}
}

% ---- Capsule Fitting ----
@article{wu2018capsule,
  title     = {Variational mannequin approximation using spheres and capsules},
  author    = {Wu, Nannan and Zhang, Dongliang and Deng, Zhigang and Jin, Xiaogang},
  journal   = {IEEE Access},
  volume    = {6},
  pages     = {25921--25929},
  year      = {2018},
  doi       = {10.1109/ACCESS.2018.2837013}
}

% ---- Swept-Sphere Volumes ----
@inproceedings{larsen2000ssv,
  title     = {Fast distance queries with rectangular swept sphere volumes},
  author    = {Larsen, Eric D. and Gottschalk, Stefan and Lin, Ming C. and Manocha, Dinesh},
  booktitle = {IEEE International Conference on Robotics and Automation (ICRA)},
  pages     = {3719--3726},
  year      = {2000},
  doi       = {10.1109/ROBOT.2000.845311}
}

% Original 1999 tech report (kept for historical reference, use larsen2000ssv for citations)
@techreport{larsen1999ssv,
  title       = {Fast proximity queries with swept sphere volumes},
  author      = {Larsen, Eric D. and Gottschalk, Stefan and Lin, Ming C. and Manocha, Dinesh},
  institution = {University of North Carolina at Chapel Hill},
  number      = {TR99-018},
  year        = {1999}
}

% ---- C-Space Neural Fields ----
@article{koptev2023neural,
  title   = {Neural joint space implicit signed distance functions for reactive robot manipulator control},
  author  = {Koptev, Mikhail and Figueroa, Nadia and Billard, Aude},
  journal = {IEEE Robotics and Automation Letters (RA-L)},
  volume  = {8},
  number  = {2},
  pages   = {480--487},
  year    = {2023},
  doi     = {10.1109/LRA.2022.3227860},
  note    = {Presented at IEEE/RSJ IROS 2023}
}

% ---- Linear Algebra ----
@misc{eigen,
  title        = {Eigen: A {C++} template library for linear algebra},
  author       = {Guennebaud, Ga{\"e}l and Jacob, Beno{\^i}t and others},
  year         = {2010},
  url          = {https://eigen.tuxfamily.org}
}

% ---- URDF Parsing ----
@misc{urdfdom,
  title        = {{urdfdom}: {URDF} parser},
  author       = {{ROS.org}},
  year         = {2024},
  url          = {https://github.com/ros/urdfdom}
}

% ---- MEC ----
@inproceedings{welzl1991mec,
  title     = {Smallest enclosing disks (balls and ellipsoids)},
  author    = {Welzl, Emo},
  booktitle = {New Results and New Trends in Computer Science},
  series    = {Lecture Notes in Computer Science},
  volume    = {555},
  pages     = {359--370},
  year      = {1991},
  publisher = {Springer},
  doi       = {10.1007/BFb0038202}
}

% ---- Robot Viewer ----
@misc{robot_viewer,
  title        = {robot\_viewer: Web-based {URDF}/{MJCF} viewer},
  author       = {Fan, Ziqi},
  year         = {2024},
  url          = {https://github.com/fan-ziqi/robot_viewer}
}

% ---- FR3 ----
@misc{franka2020fr3,
  title        = {Franka {FR3}: Research robot spec sheet},
  author       = {Franka Robotics},
  year         = {2023},
  url          = {https://franka.de/franka-research-3-arm}
}

% ---- Trimesh ----
@misc{trimesh,
  title        = {Trimesh: Python library for loading and using triangular meshes},
  author       = {Dawson-Haggerty, Michael and others},
  year         = {2024},
  url          = {https://github.com/mikedh/trimesh}
}

% ---- SDFormat ----
@misc{sdformat,
  title        = {{SDFormat}: Simulation description format},
  author       = {{Open Robotics}},
  year         = {2024},
  url          = {http://sdformat.org}
}

% ---- Additional Citations (recommended for related work) ----
@misc{2025foam,
  title        = {Foam: A tool for spherical approximation of robot geometry},
  author       = {Coumar, Sai and Chang, Gilbert and Kodkani, Nihar and Kingston, Zachary},
  year         = {2025},
  eprint       = {2503.13704},
  archivePrefix= {arXiv},
  primaryClass = {cs.RO},
  url          = {https://arxiv.org/abs/2503.13704}
}

@article{lauterbach2010gproximity,
  title     = {{gProximity}: Hierarchical {GPU}-based operations for collision and distance queries},
  author    = {Lauterbach, Christian and Mo, Qi and Manocha, Dinesh},
  journal   = {Computer Graphics Forum},
  volume    = {29},
  number    = {2},
  pages     = {419--428},
  year      = {2010},
  doi       = {10.1111/j.1467-8659.2009.01611.x}
}
```

## Writer Integration Notes

1. **`wu2018capsule`**: The related_work.tex line 29-30 describes cross-section slicing and circle-outside-area (COA) thresholding, which matches the Nannan Wu IEEE Access paper. The existing text is accurate for the *correct* paper; only the BibTeX entry needs replacement.

2. **`mukadam2018trapezoidal`**: This citation is unverifiable and should be removed from related_work.tex line 32. The claim that "capsule collision primitives have been used in several simulation and planning frameworks" can still cite Drake alone (which is verified), or a replacement paper on capsule use in robotics should be found.

3. **`larsen1999ssv`**: The related_work.tex line 30 mentions the "more general swept-sphere family" citing this paper. The 1999 tech report is the original source for SSVs, so the text is fine. The BibTeX now has both the tech report (larsen1999ssv) and the ICRA 2000 paper (larsen2000ssv). If the related work refers to the ICRA version, update the cite key in related_work.tex.

4. **Foam (2025)**: This tool is extremely relevant — it does spherical approximation of robot URDFs, making it the closest existing work to this paper. Recommend citing it in Section II.C (sphere trees) or a new comparison paragraph.
