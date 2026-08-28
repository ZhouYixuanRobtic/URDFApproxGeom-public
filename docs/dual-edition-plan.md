# URDFApproxGeom 商用版 / 科研版 双版本重构计划

状态：已执行（仓库内可执行项完成；待外部许可确认与正式 SBOM/发布流水线）
范围：C++ 核心、Python 包、CMake、Docker、CI、合规制品
原则：不引入重量级依赖；两版代码路径尽量一致；sphere-tree 为唯一版本差异。

---

## 1. 背景与目标

当前仓库将以下组件无条件编译并链接进同一个库和三个 CLI：

- ManifoldPlus：non-commercial only，且仓库内未随附 LICENSE 原文。
- sphere_tree：仅允许 educational/research/non-profit，且内嵌 GPL-2.0-or-later 的 gdiam。
- CGAL：convex-hull 组件受 GPL / 商业双许可约束。
- irmv_core：IRMV 自有依赖，默认保留。

目标：

1. 发布两个版本：
   - **Research**：convex + capsule + single-sphere + 完整 sphere-tree。
   - **Commercial**：convex + capsule + single-sphere；不含 sphere-tree。
2. 从两个版本中**彻底移除 ManifoldPlus**。
3. 用**header-only QuickHull** 替换 CGAL 凸包，两个版本一致。
4. 网格预处理统一走 **Python + trimesh**；C++ 侧只接收 OBJ/STL，并对不满足要求的网格返回**可操作的报错信息**。
5. sphere-tree 仅 Research 编译、链接、打包。
6. 补齐许可证声明、THIRD_PARTY_NOTICES 与 SBOM。

---

## 2. 关键决策

| 编号 | 决策 | 理由 |
|---|---|---|
| D1 | 移除 ManifoldPlus，两个版本都不保留 | 许可不可商用；capsule 截面算法不要求水密拓扑，单球不需要水密，多球由 Python 预处理保证闭合 |
| D2 | 网格清洗/修复统一在 Python 层，使用 trimesh | trimesh 为 MIT，且 Python 侧已经用于 DAE→OBJ 转换；不新增 C++ 依赖 |
| D3 | C++ 侧对需要闭合网格的模式做校验，失败返回明确错误 | 避免 C++ app 直接吃坏网格产生静默劣化 |
| D4 | CGAL 替换为 header-only QuickHull，两个版本一致 | 消除 GPL/商业许可问题；只影响一个调用点 |
| D5 | sphere-tree 是两版唯一功能差异 | 用户已确认；商用版不构建 STG/gdiam/qHull |
| D6 | irmv_core 保留在两个版本 | IRMV 自有依赖，权利链已确认 |
| D7 | libigl 裁剪到实际使用子集 | 减少 1,200+ vendored 文件的审计面和制品体积 |
| D8 | 商用版默认只接受 OBJ/STL；DAE 等格式必须走 Python 预处理 | C++ 不引入 collada/assimp 等重依赖 |

---

## 3. 目标依赖矩阵

### 3.1 Research 版

| 组件 | 许可证 | 用途 | 状态 |
|---|---|---|---|
| Eigen3 | MPL-2.0 | 线性代数 | 系统依赖 |
| yaml-cpp | MIT | 配置解析 | 系统依赖 |
| urdfdom | BSD | URDF 解析/写入 | 系统依赖 |
| tinyxml2 | Zlib | XML | 系统依赖 |
| pybind11 | BSD | Python 绑定 | 系统依赖 |
| GTest | BSD | 测试 | 仅测试 |
| libigl 裁剪子集 | MPL-2.0 | OBJ/STL 读写、简化、体积/惯性 | vendored |
| nlohmann/json | MIT | JSON sidecar | vendored |
| quickhull 单头文件 | 以 LICENSE 为准 | 3D 凸包 | vendored |
| trimesh / numpy | MIT / BSD | DAE 转换、修复、水密检测 | Python 依赖 |
| irmv_core | IRMV 自有 | ErrorInfo / 日志 / 工厂 | 保留 |
| sphere_tree + STG + gdiam + qHull | 非商用 / GPL | 多球树 | 仅 Research |

### 3.2 Commercial 版

与 Research 相同，但：

- 不编译、不链接、不安装 `third_party/sphere_tree`；
- 不编译、不链接、不安装 `bot_utils/sphereTreeWrapper`；
- 不提供 `spherized` / `spherized_pair` 绑定和多球 sphere preset；
- Python `sphere` 模式只支持 `single` preset；请求多球树时返回明确错误。

### 3.3 被移除组件

| 组件 | 处理 |
|---|---|
| ManifoldPlus | 两个版本全部移除 |
| CGAL / libcgal-dev / libgmp-dev | 两个版本全部移除，`gmp` 链接移除 |
| sphere_tree | Research 保留，Commercial 移除 |
| libigl `copyleft/` 及未使用目录 | 两个版本裁剪 |

---

## 4. 当前代码风险点基线

以下为本次重构需要处理的确认项：

1. **项目自身许可**：根 `LICENSE` 与 C++ 文件头为 CC BY-NC 4.0；Disclaimer 错误写成 Trinity College Dublin；Python 文件多数无 license header。商用版发布前必须由 IRMV/SJTU 明确商用再许可方式。
2. **ManifoldPlus 调用点**：
   - `src/CapsuleURDFGenerator.cpp:22`
   - `src/CapsuleURDFGenerator.cpp:187-188`
   - `src/CapsuleURDFGenerator.cpp:267-268`
   - `src/SphereTreeURDFGenerator.cpp:54`
   - `src/SphereTreeURDFGenerator.cpp:134-135`
   - `src/SphereTreeURDFGenerator.cpp:311-313`
   - `include/SphereTreeURDFGenerator.h:52`
3. **CGAL 调用点**：
   - `src/ConvexHullCollisionURDFGenerator.cpp:46`
   - `src/ConvexHullCollisionURDFGenerator.cpp:98`
   - `test/test_simplify.cpp:46`
4. **sphere_tree 非商用/GPL**：
   - `third_party/sphere_tree/LICENSE`
   - `third_party/sphere_tree/src/BBox/MVBB/gdiam.h:5-10`
   - `third_party/sphere_tree/src/BBox/MVBB/gdiam.cpp:5-12`
5. **构建无开关**：
   - `CMakeLists.txt:87-88` 无条件加入 include 目录；
   - `CMakeLists.txt:115-117` 无条件 add_subdirectory；
   - `CMakeLists.txt:120` 无条件放入 `THIS_PACKAGE_TARGETS`；
   - `CMakeLists.txt:123-139` 无条件编译链接；
   - `CMakeLists.txt:171-186` 无条件打包导出。
6. **libigl 过大**：约 1,207 个 vendored 文件，业务只使用 13 个入口头，且 `copyleft/` 含 GPL 相关代码。
7. **Python 包**：`python/pyproject.toml` 未声明 `trimesh`/`numpy`；包版本 2.0.0 与 C++ 项目 2.0.1 不一致。
8. **Docker**：`docker/Dockerfile:21` 依赖私有基础镜像；`docker/Dockerfile:52-58` runtime 安装 dev 包 `libcgal-dev`/`libgmp-dev`。
9. **资源模型**：`resources/fr3`、`resources/robots/panda` 无 LICENSE/NOTICE/来源说明。
10. **合规制品缺失**：无 `THIRD_PARTY_NOTICES.md`、`LICENSES/`、SBOM、SPDX 信息。

---

## 5. 统一网格输入流水线

### 5.1 Python 入口（主路径，两个版本一致）

```
URDF 输入
  -> 解析 URDF，收集每个 link 的 visual/collision mesh 引用
  -> 应用 --replace 路径替换
  -> 若为 .obj/.stl：
       直接传给 C++ 扩展
  -> 若为 .dae 或其他格式：
       1. trimesh.load(...)
       2. merge_vertices()
       3. remove_degenerate_faces()
       4. fix_normals()
       5. fill_holes()（仅存在边界边时）
       6. 再次检查 is_watertight / is_winding_consistent
       7. 通过：导出临时 .obj
       8. 失败：报错，指明 link、文件、失败原因和建议
```

建议新增独立模块：

```
python/urdf_approx_geom/mesh_prep.py
```

提供：

```python
PreparedMesh = ...
prepare_visual_meshes(input_urdf, replace_pairs) -> (pairs, temp_dir, warnings)
validate_watertight(path) -> bool
```

### 5.2 C++ 入口（兼容路径，两个版本一致）

C++ app 直接接收 OBJ/STL：

1. `loadedIntoIGL` 读入 V/F。
2. 执行轻量 `validateMeshForMode(V, F, mode)`：
   - 合并重复顶点（哈希索引）；
   - 删除退化面；
   - 统计每条无向边的 valence；
   - 对需要闭合网格的模式（capsule、multi-sphere），要求所有边 valence == 2 且可定向；
   - 不满足则返回错误，不静默降级。
3. 错误信息必须包含：
   - link 名；
   - 文件路径；
   - 具体原因（如 "found 123 boundary edges"）；
   - 修复建议：
     - 优先使用 Python CLI/API；
     - 或使用 `--mesh-source collision`；
     - 或先修复模型后重试。

### 5.3 各模式的水密要求

| 模式 | 水密要求 |
|---|---|
| `convex` | 不要求，只需顶点集合 |
| `capsule` | 建议要求；不满足时 C++ 返回错误，Python 路径先尝试修复 |
| `sphere/single` | 不要求，只需顶点集合 |
| `sphere/default`（多球） | 要求；仅 Research 版存在 |
| `generate_all` | 按各模式要求分别校验 |

---

## 6. ManifoldPlus 移除方案

### 6.1 代码改动

1. `src/CapsuleURDFGenerator.cpp`
   - 删除 `#include <ManifoldPlus/Manifold.h>`；
   - `run()` 中删除 `Manifold::ProcessManifold`，改为 `validateMeshForMode(V, F, MeshMode::Capsule)`；
   - `runMulti()` 同样处理。
2. `src/SphereTreeURDFGenerator.cpp`
   - 删除 ManifoldPlus include；
   - `buildSingleSphereModel()` 直接使用读取到的 `V`，不调用任何水密处理；
   - `buildSphereModel()` 在多球路径调用 `validateMeshForMode(V, F, MeshMode::SphereTree)`。
3. `include/SphereTreeURDFGenerator.h`
   - 删除 `#include "ManifoldPlus/Manifold.h"`；
   - sphere_tree 相关 include 移到 `.cpp` 或用宏保护，公共头不再暴露第三方非商用头。
4. `CMakeLists.txt`
   - 删除 `add_subdirectory(third_party/ManifoldPlus)`；
   - include 目录、`THIS_PACKAGE_TARGETS`、install/package 目标中删除 `ManifoldPlus`；
   - 删除 `gmp` 链接。
5. `third_party/ManifoldPlus/` 目录从两个版本的源码包和镜像中移除。

### 6.2 测试替换

- 删除或改写 `test/test_manifoldplus.cpp`；
- 新增 `test/test_mesh_validation.cpp`：
  - 水密 cube：通过；
  - 缺面 cube：capsule/multi-sphere 报错；
  - 重复顶点：合并后通过；
  - 非流形边：报错；
  - convex/single-sphere：非水密输入仍可运行。

---

## 7. CGAL 替换方案：header-only QuickHull

### 7.1 候选

优先评估并 vendored 以下任一单头实现：

- [akuukka/quickhull](https://github.com/akuukka/quickhull)
- [tomilov/quickhull](https://github.com/tomilov/quickhull)

采用条件：

1. LICENSE 允许闭源商用（需最终确认）；
2. 保留 LICENSE 原文于 `third_party/quickhull/LICENSE`；
3. 不修改算法主体，只在项目内做薄封装。

### 7.2 封装接口

```cpp
// include/ConvexHullBackend.h
void computeConvexHull3D(const Eigen::MatrixXd& V,
                         Eigen::MatrixXd& HV,
                         Eigen::MatrixXi& HF);
```

实现位于 `src/ConvexHullBackend.cpp`，内部调用 vendored `quickhull.hpp`。

### 7.3 改动点

- `src/ConvexHullCollisionURDFGenerator.cpp:46` 删除 `igl/copyleft/cgal/convex_hull.h`；
- `src/ConvexHullCollisionURDFGenerator.cpp:98` 改为 `computeConvexHull3D(V, CH_V, CH_F)`；
- `test/test_simplify.cpp` 同步改为调用新接口；
- CMake/Docker 移除 `libcgal-dev`、`libgmp-dev`、`gmp`。

### 7.4 回归验收

不对 CGAL 输出做逐顶点一致性比较，改为：

- 输出凸包为流形三角形网格；
- 所有输入顶点均在凸包内或容差范围内；
- 体积与 CGAL 版本误差在可接受阈值内；
- 输出 URDF 可被 `check_urdf` 或 urdfdom 正常解析。

---

## 8. sphere-tree 条件化方案

### 8.1 CMake 开关

```cmake
option(URDFApproxGeom_ENABLE_SPHERE_TREE "Enable sphere_tree backend (research only)" ON)
```

增加两个 preset：

```json
{
  "name": "research",
  "cacheVariables": {
    "URDFApproxGeom_ENABLE_SPHERE_TREE": "ON"
  }
},
{
  "name": "commercial",
  "cacheVariables": {
    "URDFApproxGeom_ENABLE_SPHERE_TREE": "OFF"
  }
}
```

商用版若显式 `-DURDFApproxGeom_ENABLE_SPHERE_TREE=ON`，configure 阶段直接 fatal error。

### 8.2 条件编译范围

- `third_party/sphere_tree`、`bot_utils/sphereTreeWrapper`：仅 `ON` 时 `add_subdirectory`。
- `THIS_PACKAGE_INCLUDE_DIRS`：
  - 商用版移除 `third_party/sphere_tree/src`、`bot_utils/sphereTreeWrapper/include`。
- `THIS_PACKAGE_TARGETS`：商用版移除 `${LIBS_TO_LINK_OUT}`、`sphereTreeWrapper`。
- `app/spherized`：仅 Research 构建。
- `test/test_spheretree.cpp`：仅 Research 构建。
- Python 绑定：
  - `spherized` / `spherized_pair` 仅 Research 注册；
  - 商用版 Python 层检测扩展能力，`sphere/default` 返回：
    `"sphere-tree backend is only available in the research edition; use sphere/single"`。

---

## 9. Python 包统一方案

### 9.1 依赖声明

`python/pyproject.toml`：

```toml
[project]
version = "2.0.1"   # 与 CMake PROJECT_VERSION 同步

dependencies = [
  "numpy>=1.23",
  "trimesh>=4.0",
]
```

### 9.2 入口行为

- 所有模式统一通过 Python API/CLI 进入；
- DAE/非 OBJ/STL 输入在 Python 层完成转换和修复；
- 修复失败返回非零退出码，并打印：
  - link 名；
  - 文件路径；
  - 水密检查结果；
  - 建议命令，例如：
    `try --mesh-source collision, or repair the mesh and rerun`。
- C++ app 仍保留给高级用户，但错误路径与 Python 层一致。

### 9.3 版本能力检测

`python/urdf_approx_geom/_extension.py` 或 `__init__.py`：

```python
_HAS_SPHERE_TREE = hasattr(ext, "spherized")
```

`sphere` 模式：
- 有 `spherized`：支持 `single` / `default`；
- 无 `spherized`：仅支持 `single`。

---

## 10. CMake / 打包 / Docker 方案

### 10.1 CMake 最终依赖

```
find_package(Eigen3 3.1 REQUIRED)
find_package(yaml-cpp 0.6 REQUIRED)
find_package(urdfdom REQUIRED)
find_package(pybind11 CONFIG REQUIRED)   # 仅 PYTHON 绑定开启时
find_package(GTest REQUIRED)             # 仅测试开启时
```

不再依赖：

- CGAL / GMP；
- ManifoldPlus；
- sphere_tree（商用版）。

### 10.2 打包

| 制品 | Research | Commercial |
|---|---|---|
| DEB 名称 | `urdfapprox-research` | `urdfapprox-commercial` |
| wheel 名称 | `urdf_approx_geom_research` | `urdf_approx_geom_commercial` |
| Docker image | `urdfapprox-research` | `urdfapprox-commercial` |
| sphere-tree | 包含 | 不包含 |
| SBOM | 必附 | 必附 |

### 10.3 Docker

- Commercial builder：`ubuntu:22.04` + apt 安装 Eigen/yaml-cpp/urdfdom/tinyxml2/pybind11/python3-dev + irmv_core（IRMV 内部源）。
- Research builder：可在同一 Dockerfile 基础上额外安装 sphere_tree 构建所需依赖。
- Runtime 只保留 `.so` 运行依赖，不装 `*-dev`。
- 两个镜像都必须有 `THIRD_PARTY_NOTICES.md` 和 SBOM。

---

## 11. 许可证与合规

### 11.1 需要新增/修改的文件

```
LICENSE                              # 明确双版本许可策略
LICENSES/MPL-2.0.txt
LICENSES/MIT.txt
LICENSES/BSD-2-Clause.txt
LICENSES/BSD-3-Clause.txt
LICENSES/Apache-2.0.txt
LICENSES/Zlib.txt
THIRD_PARTY_NOTICES.md
NOTICE                               # Franka / Panda 模型声明
```

### 11.2 文件头修复

- 所有 C++ 文件头移除错误的 Trinity College Dublin Disclaimer；
- Python 文件补 license header；
- 商用版文件头采用 IRMV/SJTU 确认的商用许可，不允许直接保留 CC BY-NC 4.0。

### 11.3 CI 门禁

1. `build-research`：完整构建 + 全部测试。
2. `build-commercial`：
   - configure 验证 `URDFApproxGeom_ENABLE_SPHERE_TREE=OFF`；
   - `ldd` / `nm -D` / `strings` 禁止出现 `ManifoldPlus`、`STG`、`gdiam`、`CGAL`；
   - 安装树禁止包含 `third_party/ManifoldPlus`、`third_party/sphere_tree`。
3. `license-scan`：
   - 扫描依赖树；
   - 商用版发现 `GPL`、`Non-commercial`、`CC BY-NC` 组件即失败。
4. `sbom`：
   - 使用 syft / trivy 生成 SPDX 或 CycloneDX，随 release 上传。

---

## 12. 实施阶段与顺序

### 阶段 0：许可确认
- 确认 IRMV/SJTU 对项目自身代码的商用再许可方式。
- 确认候选 QuickHull 的 LICENSE。
- 确认 irmv_core 在商用制品中的分发方式。
- 确认 Franka/Panda 模型声明。

### 阶段 1：CMake 开关与 preset
- 新增 `URDFApproxGeom_ENABLE_SPHERE_TREE`；
- 新增 research / commercial preset；
- 条件化子目录、include、targets、app、tests。

### 阶段 2：移除 ManifoldPlus
- 删除调用点；
- 新增 `validateMeshForMode`；
- 删除/改写 ManifoldPlus 测试；
- 删除第三方目录和 CMake 引用。

### 阶段 3：替换 CGAL
- vendored quickhull 单头文件；
- 新增 `ConvexHullBackend`；
- 替换 convex 与 test_simplify；
- 移除 CGAL/GMP。

### 阶段 4：Python 统一入口
- 新增 `mesh_prep.py`；
- pyproject 依赖与版本修正；
- DAE 修复 + 水密校验 + 错误文案；
- capability detection for sphere-tree。

### 阶段 5：Docker / 打包 / CI
- 双 Dockerfile 或单 Dockerfile + ARG EDITION；
- DEB/wheel/image 命名与内容差异；
- CI matrix 与 license/sbom 门禁。

### 阶段 6：合规制品与回归
- 生成 NOTICE/SBOM；
- 修复文件头；
- FR3/Panda 模型全量回归；
- 商用版二进制扫描。

---

## 13. 验收标准

1. Commercial 构建中不存在 ManifoldPlus、CGAL、sphere_tree、gdiam 的任何源码头文件、库和符号。
2. Research 构建保留完整 sphere-tree，全部现有研究功能通过。
3. Python CLI/API 在两个版本中行为一致，仅 sphere-tree 相关能力不同。
4. DAE visual 输入经 trimesh 修复后可运行 capsule/single-sphere；无法修复时错误信息包含 link、文件、原因、建议。
5. C++ app 对非水密 capsule/multi-sphere 输入返回明确错误，不崩溃、不静默输出。
6. Convex 输出与 CGAL 版本在体积/覆盖上满足回归阈值。
7. 两个 release 均附带 THIRD_PARTY_NOTICES、NOTICE、SBOM。
8. CI license scan 商用版无 GPL / Non-commercial / CC BY-NC 命中。

---

## 14. 开放问题

1. QuickHull 最终选择 `akuukka/quickhull` 还是 `tomilov/quickhull`，以 LICENSE 审查为准。
2. 商用版对非水密 C++ 输入是硬报错，还是提供 `--allow-open-mesh` 的显式降级选项。
3. 商用版 `sphere/single` 是否继续由 `SphereTreeURDFGenerator` 承载，还是拆成独立 `SingleSphereURDFGenerator`（推荐拆分，公共依赖更干净）。
4. 双版本共享同一仓库分支，还是商用版独立分支/仓库。
5. 私有 Docker 基础镜像是否可被 Amazon 环境拉取；如不能，需要提供 irmv_core 的离线安装包。
