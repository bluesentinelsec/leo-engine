To ensure your `leo` engine runtime builds correctly across different platforms and configurations in a CI/CD pipeline, you need to test various build scenarios that cover the key variations in your `CMakeLists.txt`. Your CMake configuration supports:
- **Shared vs. Static Library**: Controlled by the `LEO_BUILD_SHARED` option.
- **Tests Enabled vs. Disabled**: Controlled by the `LEO_BUILD_TESTS` option.
- **Multiple Platforms**: Windows, macOS, Linux, and Emscripten.
- **C17 and C++17 Standards**: Enforced with extensions off.
- **Symbol Visibility**: Using `LEO_BUILD_DLL` and `C_VISIBILITY_PRESET hidden` for shared libraries.

Since you’re focused on verifying that all build scenarios work (not unit tests), the goal is to test that the library and test executable (when enabled) compile and link correctly across platforms and configurations. Below is a list of build scenarios to test in your CI/CD pipeline, along with a rationale for each and how to configure them.

### Build Scenarios to Test
Your CI/CD pipeline should test the following scenarios to cover all combinations of `LEO_BUILD_SHARED`, `LEO_BUILD_TESTS`, and the target platforms (Windows, macOS, Linux, Emscripten). Each scenario verifies that the build completes successfully and produces the expected artifacts (e.g., library, test executable).

#### 1. Windows Builds
- **Compiler**: MSVC (Microsoft Visual Studio, e.g., 2022, as per your previous output).
- **Scenarios**:
  1. **Shared Library with Tests**:
     - **Purpose**: Verifies that the shared library (`leo.dll`) builds, exports symbols correctly (`LEO_API __declspec(dllexport)`), and that the test executable (`test_leo_runtime.exe`) links and runs with the DLL (copied via `copy_if_different`).
     - **Command**:
       ```bash
       cmake -B build -S . -DLEO_BUILD_SHARED=ON -DLEO_BUILD_TESTS=ON -G "Ninja"
       cmake --build build
       ```
     - **Artifacts**:
       - `build/leo.dll` (shared library).
       - `build/test_leo_runtime.exe` (test executable).
       - Ensure DLL is copied to the test executable’s directory.
     - **CI Setup**: Use a Windows runner (e.g., `windows-latest` in GitHub Actions) with MSVC installed.

  2. **Shared Library without Tests**:
     - **Purpose**: Ensures the shared library builds without test dependencies (e.g., Catch2) and exports symbols correctly.
     - **Command**:
       ```bash
       cmake -B build -S . -DLEO_BUILD_SHARED=ON -DLEO_BUILD_TESTS=OFF -G "Ninja"
       cmake --build build
       ```
     - **Artifacts**: `build/leo.dll`.
     - **CI Setup**: Windows runner with MSVC.

  3. **Static Library with Tests**:
     - **Purpose**: Verifies that the static library (`leo.lib`) builds and links correctly with the test executable, without needing DLL-specific handling.
     - **Command**:
       ```bash
       cmake -B build -S . -DLEO_BUILD_SHARED=OFF -DLEO_BUILD_TESTS=ON -G "Ninja"
       cmake --build build
       ```
     - **Artifacts**:
       - `build/leo.lib` (static library).
       - `build/test_leo_runtime.exe` (test executable, statically linked).
     - **CI Setup**: Windows runner with MSVC.

  4. **Static Library without Tests**:
     - **Purpose**: Ensures the static library builds without test dependencies.
     - **Command**:
       ```bash
       cmake -B build -S . -DLEO_BUILD_SHARED=OFF -DLEO_BUILD_TESTS=OFF -G "Ninja"
       cmake --build build
       ```
     - **Artifacts**: `build/leo.lib`.
     - **CI Setup**: Windows runner with MSVC.

#### 2. macOS Builds
- **Compiler**: Clang (default on macOS, e.g., via Xcode).
- **Scenarios**:
  1. **Shared Library with Tests**:
     - **Purpose**: Verifies that the shared library (`libleo.dylib`) builds with `LEO_API __attribute__((visibility("default")))` and `C_VISIBILITY_PRESET hidden`, and that the test executable links and runs.
     - **Command**:
       ```bash
       cmake -B build -S . -DLEO_BUILD_SHARED=ON -DLEO_BUILD_TESTS=ON
       cmake --build build
       ```
     - **Artifacts**:
       - `build/libleo.dylib`.
       - `build/test_leo_runtime`.
     - **CI Setup**: Use a macOS runner (e.g., `macos-latest` in GitHub Actions) with Xcode and Clang.

  2. **Shared Library without Tests**:
     - **Purpose**: Ensures the shared library builds without tests and exports only `LEO_API` symbols.
     - **Command**:
       ```bash
       cmake -B build -S . -DLEO_BUILD_SHARED=ON -DLEO_BUILD_TESTS=OFF
       cmake --build build
       ```
     - **Artifacts**: `build/libleo.dylib`.
     - **CI Setup**: macOS runner.

  3. **Static Library with Tests**:
     - **Purpose**: Verifies the static library (`libleo.a`) builds and links with the test executable.
     - **Command**:
       ```bash
       cmake -B build -S . -DLEO_BUILD_SHARED=OFF -DLEO_BUILD_TESTS=ON
       cmake --build build
       ```
     - **Artifacts**:
       - `build/libleo.a`.
       - `build/test_leo_runtime`.
     - **CI Setup**: macOS runner.

  4. **Static Library without Tests**:
     - **Purpose**: Ensures the static library builds without tests.
     - **Command**:
       ```bash
       cmake -B build -S . -DLEO_BUILD_SHARED=OFF -DLEO_BUILD_TESTS=OFF
       cmake --build build
       ```
     - **Artifacts**: `build/libleo.a`.
     - **CI Setup**: macOS runner.

#### 3. Linux Builds
- **Compiler**: GCC or Clang (e.g., via Ubuntu’s toolchain).
- **Scenarios**:
  1. **Shared Library with Tests**:
     - **Purpose**: Verifies that the shared library (`libleo.so`) builds with `LEO_API __attribute__((visibility("default")))` and `C_VISIBILITY_PRESET hidden`, and that the test executable links and runs.
     - **Command**:
       ```bash
       cmake -B build -S . -DLEO_BUILD_SHARED=ON -DLEO_BUILD_TESTS=ON
       cmake --build build
       ```
     - **Artifacts**:
       - `build/libleo.so`.
       - `build/test_leo_runtime`.
     - **CI Setup**: Use a Linux runner (e.g., `ubuntu-latest` in GitHub Actions) with GCC or Clang installed.

  2. **Shared Library without Tests**:
     - **Purpose**: Ensures the shared library builds without tests and exports only `LEO_API` symbols.
     - **Command**:
       ```bash
       cmake -B build -S . -DLEO_BUILD_SHARED=ON -DLEO_BUILD_TESTS=OFF
       cmake --build build
       ```
     - **Artifacts**: `build/libleo.so`.
     - **CI Setup**: Linux runner.

  3. **Static Library with Tests**:
     - **Purpose**: Verifies the static library (`libleo.a`) builds and links with the test executable.
     - **Command**:
       ```bash
       cmake -B build -S . -DLEO_BUILD_SHARED=OFF -DLEO_BUILD_TESTS=ON
       cmake --build build
       ```
     - **Artifacts**:
       - `build/libleo.a`.
       - `build/test_leo_runtime`.
     - **CI Setup**: Linux runner.

  4. **Static Library without Tests**:
     - **Purpose**: Ensures the static library builds without tests.
     - **Command**:
       ```bash
       cmake -B build -S . -DLEO_BUILD_SHARED=OFF -DLEO_BUILD_TESTS=OFF
       cmake --build build
       ```
     - **Artifacts**: `build/libleo.a`.
     - **CI Setup**: Linux runner.

#### 4. Emscripten Builds
- **Compiler**: Emscripten (emcc).
- **Scenarios**:
  - Emscripten typically produces WebAssembly modules or shared objects, so `LEO_BUILD_SHARED=ON` is more relevant. Static libraries are less common but can be tested for completeness.
  - Emscripten requires additional linker flags for WebAssembly exports (e.g., `-sEXPORTED_FUNCTIONS`), which you can add conditionally.
  1. **Shared Library with Tests**:
     - **Purpose**: Verifies that the WebAssembly shared library (`leo.js`/`leo.wasm`) builds with `LEO_API __attribute__((visibility("default")))` and that the test executable (Node.js-compatible) links and runs.
     - **Command**:
       ```bash
       emcmake cmake -B build -S . -DLEO_BUILD_SHARED=ON -DLEO_BUILD_TESTS=ON
       cmake --build build
       ```
     - **Artifacts**:
       - `build/leo.js`, `build/leo.wasm` (WebAssembly module).
       - `build/test_leo_runtime.js`, `build/test_leo_runtime.wasm` (test executable).
     - **CI Setup**: Use a Linux runner with Emscripten installed (e.g., `emscripten/emcc`). Run tests with `node` or a headless browser.
     - **Note**: Add `-sEXPORTED_FUNCTIONS` for public functions (e.g., `_leo_sum`) in `CMakeLists.txt`:
       ```cmake
       if(EMSCRIPTEN AND LEO_BUILD_SHARED)
         target_link_options(${PROJECT_NAME} PRIVATE -sEXPORTED_FUNCTIONS=_leo_sum)
         target_link_options(${TEST_RUNNER} PRIVATE -sEXPORTED_FUNCTIONS=_main)
       endif()
       ```

  2. **Shared Library without Tests**:
     - **Purpose**: Ensures the WebAssembly module builds without test dependencies.
     - **Command**:
       ```bash
       emcmake cmake -B build -S . -DLEO_BUILD_SHARED=ON -DLEO_BUILD_TESTS=OFF
       cmake --build build
       ```
     - **Artifacts**: `build/leo.js`, `build/leo.wasm`.
     - **CI Setup**: Linux runner with Emscripten.

  3. **Static Library with Tests**:
     - **Purpose**: Verifies the static library builds and links with the test executable in an Emscripten context (less common but ensures compatibility).
     - **Command**:
       ```bash
       emcmake cmake -B build -S . -DLEO_BUILD_SHARED=OFF -DLEO_BUILD_TESTS=ON
       cmake --build build
       ```
     - **Artifacts**:
       - `build/libleo.a`.
       - `build/test_leo_runtime.js`, `build/test_leo_runtime.wasm`.
     - **CI Setup**: Linux runner with Emscripten.

  4. **Static Library without Tests**:
     - **Purpose**: Ensures the static library builds in an Emscripten context.
     - **Command**:
       ```bash
       emcmake cmake -B build -S . -DLEO_BUILD_SHARED=OFF -DLEO_BUILD_TESTS=OFF
       cmake --build build
       ```
     - **Artifacts**: `build/libleo.a`.
     - **CI Setup**: Linux runner with Emscripten.

### Total Scenarios
- **Windows**: 4 scenarios (Shared+Tests, Shared+NoTests, Static+Tests, Static+NoTests).
- **macOS**: 4 scenarios.
- **Linux**: 4 scenarios.
- **Emscripten**: 4 scenarios.
- **Total**: 16 build scenarios.

### CI/CD Pipeline Setup
To implement these in a CI/CD system (e.g., GitHub Actions), create a matrix job that tests all combinations. Here’s an example GitHub Actions workflow:

```yaml
name: Build and Test Leo Runtime

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  build:
    strategy:
      matrix:
        os: [windows-latest, macos-latest, ubuntu-latest, ubuntu-latest]
        shared: [ON, OFF]
        tests: [ON, OFF]
        compiler: [msvc, clang, gcc, emscripten]
        exclude:
          # Exclude non-Emscripten builds on Emscripten runner
          - os: ubuntu-latest
            compiler: msvc
          - os: ubuntu-latest
            compiler: clang
          - os: windows-latest
            compiler: clang
          - os: windows-latest
            compiler: gcc
          - os: windows-latest
            compiler: emscripten
          - os: macos-latest
            compiler: msvc
          - os: macos-latest
            compiler: gcc
          - os: macos-latest
            compiler: emscripten
          # Map ubuntu-latest + emscripten to Emscripten builds
          - os: ubuntu-latest
            compiler: gcc
            shared: OFF
            tests: OFF
        include:
          - os: ubuntu-latest
            compiler: emscripten
            shared: ON
            tests: ON
          - os: ubuntu-latest
            compiler: emscripten
            shared: ON
            tests: OFF
          - os: ubuntu-latest
            compiler: emscripten
            shared: OFF
            tests: ON
          - os: ubuntu-latest
            compiler: emscripten
            shared: OFF
            tests: OFF

    runs-on: ${{ matrix.os }}

    steps:
      - uses: actions/checkout@v4

      - name: Setup Ninja
        if: matrix.os == 'windows-latest'
        uses: seanmiddleditch/gha-setup-ninja@v4

      - name: Setup Emscripten
        if: matrix.compiler == 'emscripten'
        uses: mymindstorm/setup-emsdk@v14
        with:
          version: latest

      - name: Configure CMake
        run: |
          ${{ matrix.compiler == 'emscripten' && 'emcmake' || '' }} cmake -B build -S . \
            -DLEO_BUILD_SHARED=${{ matrix.shared }} \
            -DLEO_BUILD_TESTS=${{ matrix.tests }} \
            ${{ matrix.os == 'windows-latest' && '-G "Ninja"' || '' }}

      - name: Build
        run: cmake --build build
```

### Notes
- **Matrix Strategy**: The matrix tests all combinations of `os`, `shared`, and `tests`, with exclusions to match compilers to platforms (e.g., MSVC on Windows, Clang on macOS, Emscripten on Ubuntu).
- **Emscripten Setup**: Uses `emcmake` for configuration and requires the Emscripten SDK (`setup-emsdk` action).
- **Artifacts**: Optionally, add a step to upload artifacts (e.g., `leo.dll`, `libleo.so`, `leo.js`) for inspection:
  ```yaml
  - name: Upload Artifacts
    uses: actions/upload-artifact@v4
    with:
      name: leo-${{ matrix.os }}-${{ matrix.shared }}-${{ matrix.tests }}
      path: |
        build/leo.*
        build/test_leo_runtime*
  ```
- **Symbol Visibility**: Ensure `include/leo/export.h` defines `LEO_API` correctly, as discussed previously, to handle symbol exports across platforms.
- **Sources**: The `sources.cmake` file (e.g., listing `src/engine.c`) is included, so test scenarios cover all source files. Update `sources.cmake` as new files are added.

### Validation
For each scenario, verify:
- The build completes without errors (`cmake --build build`).
- Expected artifacts are produced (e.g., `leo.dll`, `libleo.a`, `test_leo_runtime`).
- For `LEO_BUILD_TESTS=ON`, ensure the test executable builds and links correctly (you can optionally run `ctest` to verify test execution, but your focus is build success).

This covers all necessary build scenarios to ensure your `leo` runtime builds robustly across platforms and configurations. If you need help setting up a specific CI/CD system or adding Emscripten-specific export flags, let me know!