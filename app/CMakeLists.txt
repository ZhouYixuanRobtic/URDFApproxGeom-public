add_executable(convex Convex.cpp)
target_link_libraries(convex ${PROJECT_NAME} pthread)

add_executable(capsuleized Capsuleized.cpp)
target_link_libraries(capsuleized ${PROJECT_NAME} pthread)

if(URDFApproxGeom_ENABLE_SPHERE_TREE)
  add_executable(spherized Sphereized.cpp)
  target_link_libraries(spherized ${PROJECT_NAME} pthread)
endif()
