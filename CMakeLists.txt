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

if(APPLE)
  enable_language(OBJC)
endif()

# --- RPATH defaults ---
# Prefer relative rpaths so binaries are movable:
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)          # -> @loader_path (macOS) / $ORIGIN (ELF)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)   # add linked libs' install paths automatically

# Where installed binaries should look for their dylibs/so's:
if(APPLE)
  set(CMAKE_INSTALL_RPATH "@loader_path" "@loader_path/../lib")
else() # Linux, BSD, etc.
  set(CMAKE_INSTALL_RPATH "$ORIGIN" "$ORIGIN/../lib")
endif()

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
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_TEST    OFF CACHE BOOL "" FORCE)

# If Leo is built as a shared lib, prefer shared SDL; otherwise use static SDL
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

# ---- Runtime Entrypoint ----
include(sources.cmake) # sets LEO_SOURCES and (optionally) LEO_TEST_SOURCES

if (LEO_BUILD_SHARED)
  add_library(${PROJECT_NAME} SHARED ${LEO_SOURCES})
else ()
  add_library(${PROJECT_NAME} STATIC ${LEO_SOURCES})
endif ()
add_library(${ALIAS_NAME} ALIAS ${PROJECT_NAME})

set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

# --- Build-tree rpaths so targets can find each other in place ---
# Origin for the exe dir; sibling ../lib for typical layouts; plus explicit build dirs
set(_ORIGIN "$<IF:$<PLATFORM_ID:Darwin>,@loader_path,$ORIGIN>")
set(_RPATHS
  "${_ORIGIN}"
  "${_ORIGIN}/../lib"
  "${CMAKE_BINARY_DIR}"
  "${CMAKE_BINARY_DIR}/_deps/sdl3-build"    # where SDL3::SDL3 .dylib/.so ends up in FetchContent
)

# Let the library know where dependents will look during build
set_target_properties(${PROJECT_NAME} PROPERTIES
  BUILD_RPATH "${_RPATHS}"
  MACOSX_RPATH ON             # macOS: enable @rpath
  INSTALL_NAME_DIR "@rpath"   # give the dylib an @rpath install_name
)

# ---- Includes and library dependency linkage ----
target_include_directories(leo
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)

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
      SDL3::SDL3
  )

  target_include_directories(${TEST_RUNNER} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  )

  include(Catch)
  catch_discover_tests(${TEST_RUNNER})

  # --- Use local shared libs for tests (copy next to exe + rpath to current dir) ---

  # Windows: copy all runtime DLLs (includes SDL3 and leo if dynamic)
  if(WIN32 AND LEO_BUILD_SHARED)
    add_custom_command(TARGET ${TEST_RUNNER} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              $<TARGET_RUNTIME_DLLS:${TEST_RUNNER}>
              $<TARGET_FILE_DIR:${TEST_RUNNER}>
      COMMAND_EXPAND_LISTS)
    # On Windows, no rpath. Exe naturally loads DLLs from its own folder.
  endif()

  # macOS: copy specific dylibs and make the exe search its own directory first
  if(APPLE AND LEO_BUILD_SHARED)
    add_custom_command(TARGET ${TEST_RUNNER} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              $<TARGET_FILE:${PROJECT_NAME}>
              $<TARGET_FILE:SDL3::SDL3>
              $<TARGET_FILE_DIR:${TEST_RUNNER}>
    )
    set_target_properties(${TEST_RUNNER} PROPERTIES
      BUILD_RPATH "@loader_path"     # prefer current exe directory
    )
  endif()

  # Linux/other ELF: copy specific .so and make the exe search its own directory first
  if(UNIX AND NOT APPLE AND LEO_BUILD_SHARED)
    add_custom_command(TARGET ${TEST_RUNNER} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              $<TARGET_FILE:${PROJECT_NAME}>
              $<TARGET_FILE:SDL3::SDL3>
              $<TARGET_FILE_DIR:${TEST_RUNNER}>
    )
    set_target_properties(${TEST_RUNNER} PROPERTIES
      BUILD_RPATH "$ORIGIN"          # prefer current exe directory
    )
  endif()

endif ()
