# GenerateBuildScript.cmake Template for generating build.sh script for
# different projects This generates a universal script that users can control
# via command line options

# Function to generate build.sh script
function(generate_build_script)
  # Parse arguments
  cmake_parse_arguments(
    ARG "ENABLE_PYBINDING;ENABLE_TEST;ENABLE_DOXYGEN"
    "PROJECT_NAME;PROJECT_DESCRIPTION;AUTHOR;AUTHOR_EMAIL;VERSION" "" ${ARGN})

  # Set defaults
  if(NOT ARG_PROJECT_NAME)
    set(ARG_PROJECT_NAME "${PROJECT_NAME}")
  endif()
  if(NOT ARG_PROJECT_DESCRIPTION)
    set(ARG_PROJECT_DESCRIPTION "${PROJECT_DESCRIPTION}")
  endif()
  if(NOT ARG_AUTHOR)
    set(ARG_AUTHOR "${AUTHOR}")
  endif()
  if(NOT ARG_AUTHOR_EMAIL)
    set(ARG_AUTHOR_EMAIL "${AUTHOR_EMAIL}")
  endif()
  if(NOT ARG_VERSION)
    set(ARG_VERSION "${PROJECT_VERSION}")
  endif()

  # Use configure_file to process the template
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/template/build.sh.in"
                 "${CMAKE_CURRENT_SOURCE_DIR}/scripts/build.sh" @ONLY)

  # Make the script executable
  execute_process(
    COMMAND chmod +x "${CMAKE_CURRENT_SOURCE_DIR}/scripts/build.sh"
    RESULT_VARIABLE CHMOD_RESULT)

  if(NOT CHMOD_RESULT EQUAL 0)
    message(WARNING "Failed to make build.sh executable")
  endif()

  message(
    STATUS
      "Generated build.sh script at ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build.sh"
  )
endfunction()
