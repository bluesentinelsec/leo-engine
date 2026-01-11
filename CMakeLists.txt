cmake_minimum_required(VERSION 3.20)

set(PROJECT_VERSION 0.1.0)
set(EXECUTABLE_NAME "leo-engine")

project(LeoEngine VERSION ${PROJECT_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Fetch SDL3
include(FetchContent)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.0
    GIT_SHALLOW TRUE
)
set(SDL_STATIC ON CACHE BOOL "Build SDL3 as static library" FORCE)
set(SDL_SHARED OFF CACHE BOOL "Don't build SDL3 as shared library" FORCE)
FetchContent_MakeAvailable(SDL3)

# Fetch Catch2
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.12.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Catch2)

# Fetch tmxlite
FetchContent_Declare(
    tmxlite
    GIT_REPOSITORY https://github.com/fallahn/tmxlite.git
    GIT_TAG v1.4.4
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR tmxlite
)
set(TMXLITE_STATIC_LIB ON CACHE BOOL "Build tmxlite as static library" FORCE)
FetchContent_MakeAvailable(tmxlite)

# Fetch Lua
FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/marovira/lua.git
    GIT_TAG 5.4.4
    GIT_SHALLOW TRUE
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build Lua as static library" FORCE)
FetchContent_MakeAvailable(lua)

# Fetch PhysFS
FetchContent_Declare(
    physfs
    GIT_REPOSITORY https://github.com/icculus/physfs.git
    GIT_TAG 5ee045551a5d3404822f8ea08486b74d1b283858
)
set(PHYSFS_BUILD_STATIC ON CACHE BOOL "Build PhysFS as static library" FORCE)
set(PHYSFS_BUILD_SHARED OFF CACHE BOOL "Don't build PhysFS as shared library" FORCE)
FetchContent_MakeAvailable(physfs)

# Fetch miniaudio (header-only)
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.23
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(miniaudio)

# Shared source files
set(CORE_SOURCES
    src/math_utils.cpp
    src/stb_impl.cpp
)

# Main executable
add_executable(${EXECUTABLE_NAME}
    src/main.cpp
    ${CORE_SOURCES}
)

target_include_directories(${EXECUTABLE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/CLI11-v2.6.1
    ${CMAKE_CURRENT_SOURCE_DIR}/external/stb-10jan2026
    ${tmxlite_SOURCE_DIR}/tmxlite/include
    ${miniaudio_SOURCE_DIR}
)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE SDL3::SDL3 tmxlite lua::lua physfs-static)

# Tests
add_executable(leo-engine-tests
    tests/test_math_utils.cpp
    tests/test_version.cpp
    ${CORE_SOURCES}
)

target_include_directories(leo-engine-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/stb-10jan2026
)

target_link_libraries(leo-engine-tests PRIVATE Catch2::Catch2WithMain SDL3::SDL3)

enable_testing()
add_test(NAME leo-engine-tests COMMAND leo-engine-tests)
