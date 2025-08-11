# Leo Engine — C Conventions Guide

This document defines the coding style and API conventions for the Leo Engine runtime C API.

---

## 1. Language & Tooling

* **C Standard:** C17
* **Formatting:** `clang-format` using provided `.clang-format` (Microsoft style)
* **Compiler Flags:**

  ```
  -Wall
  -Wextra
  -Wpedantic
  ```
* **Supported Compilers:**

  * MSVC / CL.exe (Windows)
  * Clang (macOS)
  * GCC (Debian, Fedora)
* **Web Target:**

  * Emscripten supported on macOS and Linux
  * Web builds packaged via a Dockerfile

---

## 2. Files & Project Layout

* **File Naming:** `snake_case` for C (`image.c`, `image.h`, `snake_case.h`)
* **One Module per Pair:**

  * Each feature has a `.c` and `.h` file (e.g., `leo_image.c`, `leo_image.h`)
  * Each module also has a corresponding unit test file (`leo_image_test.c`)
* **Public vs Private Headers:**

  * Public API: `include/leo/*.h`
  * Internal headers: `src/*.h`
* **Umbrella Header:** Provide a single header that includes all public headers for consumers
* **Header Guards:** Use `#pragma once`
* **Documentation:** Document headers with Doxygen markup

---

## 3. Includes & Dependencies

* **Include Order:**

  1. This module’s header
  2. Project public headers
  3. Third-party headers (SDL, stb, miniaudio)
  4. C stdlib
  5. This module’s private headers
* **Angle vs Quotes:**

  * Project headers in quotes `"leo/..."`
  * Third-party in angle brackets `<...>`
* **Public API SDL Policy:**

  * No SDL types in public headers (use opaque handles instead)
* **Philosophy:** Favor readability and maintainability over marginal compile time gains

---

## 4. Naming & API Surface

* **Namespace:** All public symbols prefixed with `leo_`
* **Types:** Opaque handles, e.g.:

  ```c
  typedef struct leo_image* leo_image;
  ```
* **Enums:** Lowercase with underscores (`leo_blend_mode`)
* **Constants/Macros:** Prefer `static const` over macros
* **Functions:** Verb-noun naming, e.g.:

  ```
  leo_image_load
  leo_image_unload
  leo_draw_sprite
  leo_audio_play_music
  ```
* **Booleans:** Use `<stdbool.h>` `bool`

---

## 5. Errors, Diagnostics & Contracts

* **Error Model:**

  * Return `leo_err` (enum) from most functions
  * `LEO_OK == 0`
  * Store details in `leo_last_error()` (thread-local)
* **Fatal Errors:** Engine runtime should return fatal errors gracefully to caller
* **Assertions:** `LEO_ASSERT()` enabled in Debug builds only
* **Logging:** Levels: `DEBUG`, `INFO`, `WARN`, `ERROR` — sink configurable via callback
* **Nullability:** Annotate with comments: `/*out*/`, `/*nullable*/`, `/*notnull*/`
* **Ownership:**

  * Document ownership per API
  * Standardize release as `leo_*_unload()`
* **Thread-safety:** Mark functions with `/* main-thread only */` or `/* thread-safe */` in headers

---

## 6. Memory & Allocation

* **Allocators:** Global settable via:

  ```c
  leo_set_allocators(malloc, realloc, free);
  ```

  (must be called before init)
* **Zero-init Policy:** All created resources are zero-initialized
* **Failure Returns:** Handle constructors return `NULL` on failure and set `leo_last_error()`
* **Sizes:** Use `size_t` for sizes/lengths; `uint32_t` only for file formats/ABI

---

## 7. Data & ABI

* **Endianness:** Little-endian on disk; convert at load
* **Struct Packing:** No packed structs in public headers — use explicit serialization
* **Versioning:** `leo_version.h` provides:

  ```c
  #define LEO_VERSION_MAJOR
  #define LEO_VERSION_MINOR
  #define LEO_VERSION_PATCH
  const char* leo_version_string(void);
  ```

---

## 8. Rendering & Math Conventions

* **Coordinate Space:** Top-left origin `(0,0)`

  * Integers for positions/sizes
  * Floats where subpixel precision matters (scales, angles)
* **Color:**

  ```c
  typedef struct { uint8_t r, g, b, a; } leo_color;
  ```
* **Rectangles:**

  * `leo_recti` for int
  * `leo_rectf` for float
* **Transforms:** Keep sprite API simple initially; add minimal 2D transforms later

---

## 9. Header Documentation Style

* **Doxygen:**

  * `///` for brief comments
  * `/** ... */` for details
  * Use `@param`, `@return`, `@note`, `@thread`
* **Examples:** Include small code examples for key APIs (load, draw, text)

---

## 10. Error Codes (Detail)

* **Layout:** Group by subsystem:

  * `LEO_E_IMAGE_*`
  * `LEO_E_AUDIO_*`
  * `LEO_E_VFS_*`
* **Strings:** Provide:

  ```c
  const char* leo_err_str(leo_err);
  ```

---

## 11. Tests & Examples

* **Self-include Test:** Each public header compiles standalone
* **Unit Test Naming:**

  ```
  tests/test_vfs_pack.c
  tests/test_image_load.c
  ```

---

