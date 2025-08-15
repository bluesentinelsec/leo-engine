cmake_minimum_required(VERSION 3.25)

# ---- Project ----
set(PROJECT_NAME "leo")
set(ALIAS_NAME "Leo::Runtime")
set(TEST_RUNNER "test_leo_runtime")

project(leo 
    VERSION 0.1.0
    DESCRIPTION "Leo Engine Runtime"
    HOMEPAGE_URL "https://github.com/bluesentinelsec/leo-engine"
    LANGUAGES C CXX
)

# ---- Standards & policies ----
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# ---- Build Options ----
option(LEO_BUILD_SHARED "Build Leo as a shared library" ON)
option(LEO_BUILD_TESTS  "Build unit tests" ON)

# ---- Package Dependencies ----
FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.16
)
FetchContent_MakeAvailable(SDL3)

# ---- Runtime Entrypoint ----
include(sources.cmake) # <<<<<<<<<<<<< source code files are set here

if (LEO_BUILD_SHARED)
  add_library(${PROJECT_NAME} SHARED ${LEO_SOURCES})
else ()
  add_library(${PROJECT_NAME} STATIC ${LEO_SOURCES})
endif ()
add_library(${ALIAS_NAME} ALIAS ${PROJECT_NAME})

set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

# ---- Includes and library dependency linkage ----
target_include_directories(leo
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)

# ---- Installation Rules ----
# Install the library
install(TARGETS ${PROJECT_NAME}
  EXPORT LeoTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

# Install public headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
  DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)

# Generate and install export file for CMake package
install(EXPORT LeoTargets
  FILE LeoTargets.cmake
  NAMESPACE ${ALIAS_NAME}::
  DESTINATION lib/cmake/leo
)

# Generate CMake package configuration files
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/LeoConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LeoConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/LeoConfig.cmake"
  INSTALL_DESTINATION lib/cmake/leo
)

# Install CMake package configuration files
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/LeoConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/LeoConfigVersion.cmake"
  DESTINATION lib/cmake/leo
)

# ---- Test setup (toggle) ----
if (LEO_BUILD_TESTS)
  include(CTest)
  enable_testing()

  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.9.0
  )
  FetchContent_MakeAvailable(Catch2)

  add_executable(${TEST_RUNNER}
    ${LEO_TEST_SOURCES}
  )

  target_link_libraries(${TEST_RUNNER}
    PRIVATE
      ${ALIAS_NAME}
      Catch2::Catch2WithMain
  )

  include(Catch)
  catch_discover_tests(${TEST_RUNNER})

  if (LEO_BUILD_SHARED)
    add_custom_command(
      TARGET ${TEST_RUNNER} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              $<TARGET_FILE:${ALIAS_NAME}>
              $<TARGET_FILE_DIR:${TEST_RUNNER}>
    )
  endif ()
endif ()
