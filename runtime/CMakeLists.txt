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

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# ---- Build Options ----

option(LEO_BUILD_SHARED "Build Leo as a shared library" ON)
option(LEO_BUILD_TESTS  "Build unit tests" ON)

# ---- Runtime Entrypoint ----
include(sources.cmake)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ---- Test setup (toggle) ----
if (LEO_BUILD_TESTS)
  include(CTest)
  enable_testing()

  include(FetchContent)
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

  # Copy DLL for Windows shared library testing
  if (WIN32 AND LEO_BUILD_SHARED)
    add_custom_command(
      TARGET ${TEST_RUNNER} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              $<TARGET_FILE:${ALIAS_NAME}>
              $<TARGET_FILE_DIR:${TEST_RUNNER}>
    )
  endif ()
endif ()

# TODO: export compile commands
# TODO: set python find_package
# TODO: add OBJC to languages for macOS?
# TODO: Add dependencies: Lua, SDL3
# TODO: Add sanitizers
