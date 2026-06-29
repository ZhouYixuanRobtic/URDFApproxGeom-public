# SpherizedURDFGenerator development / build image
#
# Base (imc:base-2.3.0, Ubuntu 22.04) already provides:
#   irmv_core 1.3.0, urdfdom, yaml-cpp, tinyxml2, Eigen, gcc/g++,
#   clang-format, cppcheck, doxygen, pybind11, and a non-root `irmv` user.
# libigl / sphere_tree / ManifoldPlus are vendored under third_party/.
# Only two apt deps are missing for this project: libcgal-dev + libgmp-dev
# (used by the convex-hull path via igl/copyleft/cgal).
#
# piqp is NOT required: irmv_core 1.3.0 in the base image is self-contained for
# the symbols this project links.

FROM irmv-docker-hub-registry.cn-shanghai.cr.aliyuncs.com/manipulation/imc:base-2.3.0

USER root
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        libcgal-dev \
        libgmp-dev \
    && rm -rf /var/lib/apt/lists/*

USER irmv
WORKDIR /workspace

# Standard system PATH even if the base image overrides it (devcontainer/Cursor scripts
# rely on whoami/uname/tar being on PATH).
ENV PATH=/usr/local/cuda/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

CMD ["/bin/bash"]
