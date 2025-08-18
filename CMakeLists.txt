cmake_minimum_required(VERSION 3.25)

# =========================
# Project & global settings
# =========================
set(PROJECT_NAME "leo")
set(ALIAS_NAME   "Leo::Runtime")
set(TEST_RUNNER  "test_leo_runtime")

project(leo
  VERSION 0.1.0
  DESCRIPTION "Leo Engine Runtime"
  HOMEPAGE_URL "https://github.com/bluesentinelsec/leo-engine"
  LANGUAGES C CXX)

if(APPLE)
  enable_language(OBJC)          # allow .m files when building on macOS
endif()

# Standards
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =========================
# RPATH (relocatable builds)
# =========================
# Prefer origin-relative RPATHs so binaries remain movable.
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)        # @loader_path (macOS) / $ORIGIN (ELF)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE) # fold linked install paths into install RPATH

if(APPLE)
  set(CMAKE_INSTALL_RPATH "@loader_path" "@loader_path/../lib")
else() # Linux, BSD, etc.
  set(CMAKE_INSTALL_RPATH "$ORIGIN" "$ORIGIN/../lib")
endif()

# =========================
# Options
# =========================
option(LEO_BUILD_SHARED "Build Leo as a shared library" ON)
option(LEO_BUILD_TESTS  "Build unit tests"             ON)
option(LEO_VENDOR_SDL   "Bundle SDL3 into install"     ON)  # ON for plug-and-play, OFF for system SDL

# =========================
# Dependencies (SDL3)
# =========================
include(FetchContent)

# When vendoring, enable SDL's own install() rules so it installs into our prefix.
if(LEO_VENDOR_SDL)
  set(SDL_INSTALL ON  CACHE BOOL "" FORCE)
else()
  set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
endif()
set(SDL_TEST   OFF CACHE BOOL "" FORCE)

# Match SDL linkage form with our build type.
if(LEO_BUILD_SHARED)
  set(SDL_SHARED ON  CACHE BOOL "" FORCE)
  set(SDL_STATIC OFF CACHE BOOL "" FORCE)
else()
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON  CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG        release-3.2.16
)
FetchContent_MakeAvailable(SDL3)

# =========================
# Sources
# =========================
# This file is expected to define: LEO_SOURCES and (optionally) LEO_TEST_SOURCES
include(sources.cmake)

# =========================
# Library target
# =========================
if(LEO_BUILD_SHARED)
  add_library(${PROJECT_NAME} SHARED ${LEO_SOURCES})
  target_compile_definitions(${PROJECT_NAME} PRIVATE LEO_BUILDING_DLL)  # for __declspec(dllexport)
else()
  add_library(${PROJECT_NAME} STATIC ${LEO_SOURCES})
endif()

add_library(${ALIAS_NAME} ALIAS ${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

# Public link so consumers inherit SDL usage requirements (include dirs, link, etc.)
target_link_libraries(${PROJECT_NAME} PUBLIC SDL3::SDL3)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Build-tree RPATHs so local runs find deps without env tweaks.
# Use origin, plus the SDL target's build dir to avoid hardcoding _deps paths.
set(_ORIGIN  "$<IF:$<PLATFORM_ID:Darwin>,@loader_path,$ORIGIN>")
set(_RPATHS
  "${_ORIGIN}"
  "${_ORIGIN}/../lib"
  "$<TARGET_FILE_DIR:SDL3::SDL3>"
)
set_target_properties(${PROJECT_NAME} PROPERTIES
  BUILD_RPATH "${_RPATHS}"
  MACOSX_RPATH ON
  INSTALL_NAME_DIR "@rpath"
)

# =========================
# Tests
# =========================
if(LEO_BUILD_TESTS)
  include(CTest)
  enable_testing()

  # Catch2 (header-only runner + library for main)
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.9.0
  )
  FetchContent_MakeAvailable(Catch2)

  add_executable(${TEST_RUNNER} ${LEO_TEST_SOURCES})

  target_link_libraries(${TEST_RUNNER}
    PRIVATE
      ${ALIAS_NAME}
      Catch2::Catch2WithMain
      SDL3::SDL3
  )

  target_include_directories(${TEST_RUNNER} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  )

  include(Catch)

  # On Windows, discovery runs immediately after link; point it at DLL dirs so it doesn't fail.
  catch_discover_tests(${TEST_RUNNER}
    WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_RUNNER}>
    DL_PATHS
      $<TARGET_FILE_DIR:${PROJECT_NAME}>
      $<TARGET_FILE_DIR:SDL3::SDL3>
  )

  # ---- Post-build runtime handling for local runs ----

  # Windows: copy every dependent DLL (includes leo.dll, SDL3.dll, etc.).
  if(WIN32 AND LEO_BUILD_SHARED)
    add_custom_command(TARGET ${TEST_RUNNER} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              $<TARGET_RUNTIME_DLLS:${TEST_RUNNER}>
              $<TARGET_FILE_DIR:${TEST_RUNNER}>
      COMMAND_EXPAND_LISTS
      COMMENT "Copying runtime DLLs next to ${TEST_RUNNER}")
  endif()

  # macOS: copy dylibs next to the exe and prefer the exe dir via RPATH.
  if(APPLE AND LEO_BUILD_SHARED)
    add_custom_command(TARGET ${TEST_RUNNER} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              $<TARGET_FILE:${PROJECT_NAME}>
              $<TARGET_FILE:SDL3::SDL3>
              $<TARGET_FILE_DIR:${TEST_RUNNER}>
      COMMENT "Copying dylibs (Leo, SDL3) next to ${TEST_RUNNER}")
    set_target_properties(${TEST_RUNNER} PROPERTIES
      BUILD_RPATH "@loader_path"
    )
  endif()

  # Linux/ELF: copy .so next to the exe and prefer the exe dir via RPATH.
  if(UNIX AND NOT APPLE AND LEO_BUILD_SHARED)
    add_custom_command(TARGET ${TEST_RUNNER} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              $<TARGET_FILE:${PROJECT_NAME}>
              $<TARGET_FILE:SDL3::SDL3>
              $<TARGET_FILE_DIR:${TEST_RUNNER}>
      COMMENT "Copying shared objects (Leo, SDL3) next to ${TEST_RUNNER}")
    set_target_properties(${TEST_RUNNER} PROPERTIES
      BUILD_RPATH "$ORIGIN"
    )
  endif()
endif()
