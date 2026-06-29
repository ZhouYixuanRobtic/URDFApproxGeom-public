# pybind11 extension exposing the 3 collision-geometry generators to Python.
# Built only when COMPILE_<PROJECT_NAME>_PYBINDING=ON. Output is the importable
# module `urdf_approx_geom` under build/python (use via PYTHONPATH=build/python).

find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(urdf_approx_geom urdf_approx_geom.cpp)
target_link_libraries(urdf_approx_geom PRIVATE ${PROJECT_NAME})

set_target_properties(urdf_approx_geom PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python")
