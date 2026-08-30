# GenerateBuildScript.cmake Template for generating build.sh script for
# different projects This generates a universal script that users can control
# via command line options

# Function to generate build.sh script
function(generate_build_script)
  # Parse arguments
  cmake_parse_arguments(
    ARG "ENABLE_PYBINDING;ENABLE_TEST;ENABLE_DOXYGEN"
    "PROJECT_NAME;PROJECT_DESCRIPTION;AUTHOR;AUTHOR_EMAIL;VERSION" "" ${ARGN})

  # Re-bind the parsed values to the un-prefixed names so configure_file's
  # @PROJECT_NAME@-style substitution actually picks them up (the plain names
  # are the project() defaults when the argument is omitted).
  if(ARG_PROJECT_NAME)
    set(PROJECT_NAME "${ARG_PROJECT_NAME}")
  endif()
  if(ARG_PROJECT_DESCRIPTION)
    set(PROJECT_DESCRIPTION "${ARG_PROJECT_DESCRIPTION}")
  endif()
  if(ARG_AUTHOR)
    set(AUTHOR "${ARG_AUTHOR}")
  endif()
  if(ARG_AUTHOR_EMAIL)
    set(AUTHOR_EMAIL "${ARG_AUTHOR_EMAIL}")
  endif()
  if(ARG_VERSION)
    set(PROJECT_VERSION "${ARG_VERSION}")
  endif()

  # Use configure_file to process the template. Generate into the build tree:
  # writing scripts/ into the source tree dirties git status and fails on
  # read-only checkouts.
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/template/build.sh.in"
                 "${CMAKE_CURRENT_BINARY_DIR}/scripts/build.sh" @ONLY)

  # Make the script executable (portable; chmod is Unix-only)
  file(
    CHMOD "${CMAKE_CURRENT_BINARY_DIR}/scripts/build.sh"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE)

  message(
    STATUS
      "Generated build.sh script at ${CMAKE_CURRENT_BINARY_DIR}/scripts/build.sh"
  )
endfunction()
