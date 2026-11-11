cmake_minimum_required(VERSION 3.24.2)

project(imp CXX)

if(${CMAKE_GENERATOR} STREQUAL "Unix Makefiles")
  set(CMAKE_MAKE_PROGRAM "${PROJECT_SOURCE_DIR}/scripts/make-parallel.sh" CACHE STRING "" FORCE)
endif()

if(${PROJECT_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "Imp must be built outside its source directory, e.g. `cmake -B build`.")
endif()

include(etc/build_type.cmake)
include(etc/cflags.cmake)
include(etc/scanners.cmake)
include(etc/tests.cmake)

include_directories("${PROJECT_SOURCE_DIR}/util")
include_directories("${PROJECT_SOURCE_DIR}/src")

add_subdirectory("${PROJECT_SOURCE_DIR}/util")
add_subdirectory("${PROJECT_SOURCE_DIR}/src")
add_subdirectory("${PROJECT_SOURCE_DIR}/tests")
add_subdirectory("${PROJECT_SOURCE_DIR}/apps")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Additional boundary/oracle checks; the provided tests are unchanged.
add_executable(check0_extra EXCLUDE_FROM_ALL checks/check0_extra.cc)
target_compile_options(check0_extra PRIVATE ${SANITIZING_FLAGS})
target_link_options(check0_extra PRIVATE ${SANITIZING_FLAGS})
target_link_libraries(check0_extra PRIVATE imp_sanitized util_sanitized)
add_test(NAME check0_extra COMMAND check0_extra)
