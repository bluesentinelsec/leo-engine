# Leo Engine — Emscripten (Web) Build Guide

This guide explains how to build the **Leo Engine Runtime** for the web (WASM) using **Emscripten**, and how downstream apps can **consume** it via CMake.

---

## 1) Requirements

* **Emscripten SDK** (emsdk)
  [https://emscripten.org/docs/getting\_started/downloads.html](https://emscripten.org/docs/getting_started/downloads.html)
* **CMake** ≥ 3.25
* A recent **Clang/LLVM** (comes with emsdk)
* (Optional) **Docker** / **Podman**

---

## 2) Building Leo (library only)

You can build locally with emsdk or inside a container.

### Option A — Local build with emsdk

```bash
# 1) Get emsdk and activate it (once per shell)
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh   # IMPORTANT: sets PATH, EM_CONFIG, etc.

# 2) Configure Leo with emcmake (tests OFF by default in the example)
cd /path/to/leo
emcmake cmake -S . -B build-wasm \
  -DCMAKE_BUILD_TYPE=Release \
  -DLEO_BUILD_SHARED=OFF \
  -DLEO_BUILD_TESTS=OFF

# 3) Build
cmake --build build-wasm -j
```

**Artifacts:** you’ll get a static library (e.g. `build-wasm/libleo.a`) and headers under `include/`.
This is what **consumers** should link against from their Web app.

> ℹ️ Under Emscripten you won’t build a `.html` automatically unless your **app** provides one. Leo is a library—consumers produce the final `.html/.js/.wasm`.

---

### Option B — Build with Docker (provided Dockerfile)

We provide a simple container that installs emsdk, configures the project, and builds:

```bash
# Build the image
docker build . -t dev:latest

# Run it and expose port 8000 (useful if your app emits an index.html later)
docker run -it -p 8000:8000 dev:latest
```

The Dockerfile does:

* Install Debian deps, `emsdk`, and environment
* `emcmake cmake` with `-DLEO_BUILD_SHARED=OFF -DLEO_BUILD_TESTS=OFF`
* `cmake --build .`
* Lists artifacts, then serves `/leo` via `python3 -m http.server 8000`

> Note: As Leo is a library, serving is only meaningful once you add a **client app** that generates `.html/.js/.wasm`.

---

## 3) SDL3 dependency (two ways)

Your CMake supports **either**:

* **Find SDL3** (preferred on Web when using Emscripten’s built-in SDL):

  * Consumers pass `-sUSE_SDL=3` in their app’s link options.
* **Vendor SDL3** (fallback):

  * Set `-DLEO_VENDOR_SDL=ON`. Your CMake will FetchContent SDL and provide `SDL3::SDL3`.

If SDL3 isn’t found and vendoring is OFF, CMake will stop with:

```
SDL3::SDL3 not found. Set LEO_VENDOR_SDL=ON or provide SDL3.
```

---

## 4) Consuming Leo in a Web app (CMake)

Below is a minimal **consumer** example that links to `Leo::Runtime` and uses **Emscripten’s SDL**:

**App layout**

```
mygame/
  CMakeLists.txt
  main.c
```

**`main.c`**

```c
#include "leo/engine.h"
#include "leo/graphics.h"
#include "leo/color.h"

int main(void) {
    if (!leo_InitWindow(800, 600, "My Web Game")) return 1;
    leo_SetTargetFPS(60);

    while (!leo_WindowShouldClose()) {
        leo_BeginDrawing();
        leo_ClearBackground(32, 32, 48, 255);
        leo_DrawCircle(400, 300, 50, (leo_Color){255, 128, 0, 255});
        leo_EndDrawing();
    }
    leo_CloseWindow();
    return 0;
}
```

**`CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.25)
project(mygame LANGUAGES C CXX)

include(FetchContent)

# Bring Leo
FetchContent_Declare(leo
  GIT_REPOSITORY https://github.com/bluesentinelsec/leo-engine.git
  GIT_TAG main)
FetchContent_MakeAvailable(leo)

add_executable(mygame main.c)
target_link_libraries(mygame PRIVATE Leo::Runtime)

# Tell Emscripten to use its SDL3 and grow memory if needed
target_link_options(mygame PRIVATE
  "-sUSE_SDL=3"
  "-sALLOW_MEMORY_GROWTH=1"
  "-sEXIT_RUNTIME=1"
)
```

**Build & run**

```bash
# From mygame/
source /path/to/emsdk/emsdk_env.sh
emcmake cmake -S . -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm -j

# Outputs typically:
#   build-wasm/mygame.js
#   build-wasm/mygame.wasm
# Serve them with any static server:
cd build-wasm
python3 -m http.server 8000
# Open http://localhost:8000/mygame.html if generated or use your own shell html
```

> If you want Emscripten to emit an HTML shell, add:
>
> ```cmake
> target_link_options(mygame PRIVATE "--shell-file" "${CMAKE_SOURCE_DIR}/shell.html")
> ```
>
> or tell CMake/emscripten to produce `.html`:
>
> ```cmake
> set_target_properties(mygame PROPERTIES SUFFIX ".html")
> ```

---

## 5) (Optional) Running unit tests on Web

Your CMake can build tests for Node when `LEO_BUILD_TESTS=ON`:

```bash
source ./emsdk/emsdk_env.sh
emcmake cmake -S . -B build-wasm \
  -DCMAKE_BUILD_TYPE=Release \
  -DLEO_BUILD_SHARED=OFF \
  -DLEO_BUILD_TESTS=ON
cmake --build build-wasm -j

# Run Catch2 tests under Node (graphics/window tests should be filtered out)
node build-wasm/test_leo_runtime.js "~[engine] ~[graphics] ~[draw] ~[loop]"
```

(Your test target is linked with `-sENVIRONMENT=node -sEXIT_RUNTIME=1`. If you later want full SDL window tests in a browser, use `emrun` + headless Chrome instead.)

---

## 6) Troubleshooting

* **`EM_ASM does not work in -std=c* modes`**
  Your CMake already enables GNU extensions for Web (`CMAKE_C_EXTENSIONS/CXX_EXTENSIONS ON`). If you set flags manually, ensure `-std=gnu11` / `-std=gnu++17`.

* **`SDL3::SDL3 not found`**
  Either:

  * turn on vendoring: `-DLEO_VENDOR_SDL=ON`, **or**
  * let your app use Emscripten SDL: add `-sUSE_SDL=3` to your **app’s** link options.

* **Shared libs on Web**
  `LEO_BUILD_SHARED` is forced OFF under Emscripten. You’ll get `libleo.a` (static) which is correct for WASM.

---



