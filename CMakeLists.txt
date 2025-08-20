cmake_minimum_required(VERSION 3.25)

# =========================
# Project & global settings
# =========================
set(PROJECT_NAME "leo")
set(ALIAS_NAME "Leo::Runtime")
set(TEST_RUNNER "test_leo_runtime")

project(leo
        VERSION 0.1.0
        DESCRIPTION "Leo Engine Runtime"
        HOMEPAGE_URL "https://github.com/bluesentinelsec/leo-engine"
        LANGUAGES C CXX)

if (APPLE)
    enable_language(OBJC)          # allow .m files on macOS
endif ()

# Language standards
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (EMSCRIPTEN)
    set(CMAKE_C_EXTENSIONS ON)
    set(CMAKE_CXX_EXTENSIONS ON)
endif ()

# =========================
# RPATH (relocatable builds)
# =========================
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)        # @loader_path (macOS) / $ORIGIN (ELF)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if (APPLE)
    set(CMAKE_INSTALL_RPATH "@loader_path" "@loader_path/../lib")
else ()
    set(CMAKE_INSTALL_RPATH "$ORIGIN" "$ORIGIN/../lib")
endif ()

# =========================
# Options
# =========================
option(LEO_BUILD_SHARED "Build Leo as a shared library" ON)
option(LEO_BUILD_TESTS "Build unit tests" ON)
option(LEO_VENDOR_SDL "Bundle SDL3 into install" ON)  # ON: vendored SDL; OFF: system/parent SDL

if (EMSCRIPTEN)
    set(LEO_BUILD_SHARED OFF CACHE BOOL "" FORCE)
endif ()
# =========================
# Dependencies (SDL3)
# =========================
include(FetchContent)

if (LEO_VENDOR_SDL)
    set(SDL_INSTALL ON CACHE BOOL "" FORCE)   # let SDL install itself into our prefix
else ()
    set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
endif ()
set(SDL_TEST OFF CACHE BOOL "" FORCE)

# Match SDL linkage form with our build type
if (LEO_BUILD_SHARED)
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)
else ()
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
endif ()


find_package(SDL3 CONFIG QUIET)

if (NOT SDL3_FOUND AND LEO_VENDOR_SDL)
    include(FetchContent)
    set(SDL_INSTALL ON CACHE BOOL "" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    # Toggle shared/static only for native; Emscripten is static anyway
    if (NOT EMSCRIPTEN)
        if (LEO_BUILD_SHARED)
            set(SDL_SHARED ON CACHE BOOL "" FORCE)
            set(SDL_STATIC OFF CACHE BOOL "" FORCE)
        else ()
            set(SDL_SHARED OFF CACHE BOOL "" FORCE)
            set(SDL_STATIC ON CACHE BOOL "" FORCE)
        endif ()
    endif ()

    FetchContent_Declare(
            SDL3
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-3.2.16
    )
    FetchContent_MakeAvailable(SDL3)
endif ()

if (NOT TARGET SDL3::SDL3)
    message(FATAL_ERROR "SDL3::SDL3 not found. Set LEO_VENDOR_SDL=ON or provide SDL3.")
endif ()

# =========================
# Sources
# =========================
# Expected to define: LEO_SOURCES and (optionally) LEO_TEST_SOURCES
include(sources.cmake)

# =========================
# Library target
# =========================
if (LEO_BUILD_SHARED)
    add_library(${PROJECT_NAME} SHARED ${LEO_SOURCES})
    target_compile_definitions(${PROJECT_NAME} PRIVATE LEO_BUILDING_DLL)  # for __declspec(dllexport)
else ()
    add_library(${PROJECT_NAME} STATIC ${LEO_SOURCES})
endif ()

add_library(${ALIAS_NAME} ALIAS ${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

# Public so consumers inherit SDL usage requirements
target_link_libraries(${PROJECT_NAME} PUBLIC SDL3::SDL3)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)


# Build-tree RPATH so local runs find deps without env tweaks
set(_ORIGIN "$<IF:$<PLATFORM_ID:Darwin>,@loader_path,$ORIGIN>")
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

if (LEO_BUILD_TESTS)
    include(CTest)
    enable_testing()

    FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.9.0
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

    # Windows discovery runs immediately after link; ensure DLL dirs are on PATH.
    catch_discover_tests(${TEST_RUNNER}
            WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_RUNNER}>
            DL_PATHS
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
            $<TARGET_FILE_DIR:SDL3::SDL3>
    )

    if (EMSCRIPTEN)
        # Build for Node environment and allow exit() to terminate process.
        target_link_options(${TEST_RUNNER} PRIVATE
                "-sENVIRONMENT=node"
                "-sEXIT_RUNTIME=1"
                "-sALLOW_MEMORY_GROWTH=1"
        )
    endif ()

    # ---- Post-build runtime handling (shared builds only) ----
    if (LEO_BUILD_SHARED)
        if (WIN32)
            # Copy all dependent DLLs (leo, SDL3, etc.) next to the test exe
            add_custom_command(TARGET ${TEST_RUNNER} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_RUNTIME_DLLS:${TEST_RUNNER}>
                    $<TARGET_FILE_DIR:${TEST_RUNNER}>
                    COMMAND_EXPAND_LISTS
                    COMMENT "Copying runtime DLLs next to ${TEST_RUNNER}")
        elseif (APPLE)
            # Copy dylibs and prefer the exe dir via RPATH
            add_custom_command(TARGET ${TEST_RUNNER} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:${PROJECT_NAME}>
                    $<TARGET_FILE:SDL3::SDL3>
                    $<TARGET_FILE_DIR:${TEST_RUNNER}>
                    COMMENT "Copying dylibs (Leo, SDL3) next to ${TEST_RUNNER}")
            set_target_properties(${TEST_RUNNER} PROPERTIES
                    BUILD_RPATH "@loader_path"
            )
        elseif (UNIX)
            # Copy .so and prefer the exe dir via RPATH
            add_custom_command(TARGET ${TEST_RUNNER} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:${PROJECT_NAME}>
                    $<TARGET_FILE:SDL3::SDL3>
                    $<TARGET_FILE_DIR:${TEST_RUNNER}>
                    COMMENT "Copying shared objects (Leo, SDL3) next to ${TEST_RUNNER}")
            set_target_properties(${TEST_RUNNER} PROPERTIES
                    BUILD_RPATH "$ORIGIN"
            )
        endif ()
    endif ()
endif ()

target_compile_definitions(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:DEBUG>
        $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
)

if (LEO_BUILD_TESTS)
    target_compile_definitions(${TEST_RUNNER} PRIVATE
            $<$<CONFIG:Debug>:DEBUG>
            $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
    )
endif ()
