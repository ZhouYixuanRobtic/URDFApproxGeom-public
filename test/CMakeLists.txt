find_package(GTest REQUIRED)

add_executable(test_simplify test_simplify.cpp)
target_link_libraries(
  test_simplify PRIVATE GTest::GTest GTest::Main pthread ${PROJECT_NAME}
                        ${irmv_core_LIBRARIES})

add_executable(test_capsule test_capsule.cpp)
target_link_libraries(
  test_capsule PRIVATE GTest::GTest GTest::Main pthread ${PROJECT_NAME}
                       ${irmv_core_LIBRARIES})

add_executable(test_mesh_validation test_mesh_validation.cpp)
target_link_libraries(
  test_mesh_validation PRIVATE GTest::GTest GTest::Main pthread ${PROJECT_NAME}
                              ${irmv_core_LIBRARIES})

if(URDFApproxGeom_ENABLE_SPHERE_TREE)
  add_executable(test_spheretree test_spheretree.cpp)
  target_link_libraries(
    test_spheretree PRIVATE GTest::GTest GTest::Main pthread ${Boost_LIBRARIES}
                          sphereTreeWrapper ${irmv_core_LIBRARIES})

  add_executable(test_urdf_genderator test_urdf_genderator.cpp)
  target_link_libraries(
    test_urdf_genderator
    PRIVATE GTest::GTest
            GTest::Main
            pthread
            ${PROJECT_NAME}
            ${Boost_LIBRARIES}
            sphereTreeWrapper
            ${irmv_core_LIBRARIES})
endif()

include(GoogleTest)
gtest_discover_tests(test_simplify)
gtest_discover_tests(test_capsule)
gtest_discover_tests(test_mesh_validation)
if(URDFApproxGeom_ENABLE_SPHERE_TREE)
  gtest_discover_tests(test_spheretree)
  gtest_discover_tests(test_urdf_genderator)
endif()
