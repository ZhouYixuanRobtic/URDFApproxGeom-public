# SpherizedURDFGenerator — distributable runtime image.
#
# Multi-stage build that ships a working `urdf-approx-geom` CLI:
#   * builder  — compiles the C++ library + pybind11 extension against the
#                irmv base image and `cmake --install`s into /usr/local.
#   * runtime  — same base (carries irmv_core / urdfdom / Eigen runtime libs),
#                copies only the installed artifacts, pip-installs the pure
#                Python package, and sets the CLI as entrypoint.
#
# Build context is the repo root:
#   docker build -t urdfapprox -f docker/Dockerfile .
# Or pull the published image:
#   docker pull irmv-docker-hub-registry.cn-shanghai.cr.aliyuncs.com/manipulation/urdfapprox:1.5.0
#
# Base (imc:base-2.3.0, Ubuntu 22.04) already provides irmv_core 1.3.0,
# urdfdom, yaml-cpp, tinyxml2, Eigen, pybind11, and the non-root `irmv` user.
# Only libcgal-dev + libgmp-dev are added (convex-hull path via
# igl/copyleft/cgal). libigl / sphere_tree / ManifoldPlus are vendored under
# third_party/ and built by the project.

ARG BASE=irmv-docker-hub-registry.cn-shanghai.cr.aliyuncs.com/manipulation/imc:base-2.3.0

# ──────────────────────────────── builder ────────────────────────────────
FROM ${BASE} AS builder

USER root
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        libcgal-dev \
        libgmp-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

# Release build with pybind11 extension; tests/docs off. Install to /usr/local
# so the runtime stage can copy a single tree (libs, headers, CLI binaries,
# CMake package config).
RUN cmake -S . -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DCOMPILE_URDFApproxGeom_PYBINDING=ON \
        -DENABLE_TEST=OFF \
        -DENABLE_DOXYGEN=OFF \
        -DURDFApproxGeom_CONFIG_PATH=/usr/local/share/URDFApproxGeom/config \
    && cmake --build build -j"$(nproc)" \
    && cmake --install build

# ──────────────────────────────── runtime ────────────────────────────────
FROM ${BASE} AS runtime

# Runtime needs the cgal/gmp shared libs (same tiny apt layer as builder).
USER root
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        libcgal-dev \
        libgmp-dev \
    && rm -rf /var/lib/apt/lists/*

# Installed C++ artifacts: libURDFApproxGeom + third-party targets, headers,
# spherized/convex/capsuleized binaries, CMake config under /usr/local/share.
COPY --from=builder /usr/local/ /usr/local/
RUN ldconfig

# Stage the pure-Python package, then drop the compiled pybind11 extension and
# the preset config/ tree INSIDE the package dir. pip then packs them via
# package-data, so _extension.py and presets.py find them through package-
# relative resolution (no sys.path hacks, no parents[N] assumptions).
COPY --from=builder /src/build/python/_urdf_approx_geom*.so /tmp/ext/
COPY python/ /src/python/
COPY config/ /src/python/urdf_approx_geom/config/

RUN cp /tmp/ext/_urdf_approx_geom*.so /src/python/urdf_approx_geom/ \
    && pip3 install --no-cache-dir --retries 5 --timeout 120 --upgrade pip setuptools \
    && pip3 install --no-cache-dir --retries 5 --timeout 120 /src/python \
    && rm -rf /src /tmp/ext

# Standard system PATH even if the base image overrides it; /usr/local/lib on
# the loader path as a belt-and-suspenders fallback to ldconfig.
ENV PATH=/usr/local/cuda/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin \
    LD_LIBRARY_PATH=/usr/local/lib

USER irmv
WORKDIR /workspace
ENTRYPOINT ["urdf-approx-geom"]
CMD ["--help"]
