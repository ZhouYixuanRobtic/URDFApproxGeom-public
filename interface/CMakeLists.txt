# pybind11 extension exposing the C++ generators. The public Python package is
# python/urdf_approx_geom; this compiled module stays private.
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(_urdf_approx_geom urdf_approx_geom.cpp)
target_link_libraries(_urdf_approx_geom PRIVATE ${PROJECT_NAME})

set_target_properties(_urdf_approx_geom PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python")
