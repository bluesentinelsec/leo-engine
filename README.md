# **Leo Engine Design Document (Draft)**

## **1. Vision & Goals**

Leo Engine aims to make **game programming feel good** — fast, minimal, and developer-friendly.
It focuses on **retro 2D games**, supporting 8-bit era virtual resolutions and **pixel-perfect rendering**.

**Core Goals:**

* Minimal dependencies: **SDL3** is the only large dependency; all others are static-linked.
* Tight scope: Only support **Windows**, **macOS**, **Linux**, and **Web** (Emscripten).
* Clean **C API** that’s friendly to language bindings.
* Provide both **low-level primitives** and **high-level convenience functions**.
* Fast bootstrap-to-release workflow via a **CLI project manager**.

---

## **2. Dependencies**

**Primary Dependency**

* **SDL3** — windowing, input, rendering, audio device setup, timers.

**Static-linked Libraries**

* **miniaudio** — sound and music.
* **stb\_image** — PNG/JPG image loading.
* **stb\_truetype** — font rendering.
* **getopt** — CLI argument parsing.
* **Lua** — scripting.

---

## **3. Target Audience & Use Cases**

* **Indie developers** making retro-style games.
* **Hobbyists** who want a minimal learning curve.
* **Professional developers** who want a small, portable runtime.

Goal:

Get a sprite on the screen in a few commands:
```
pip install leo-engine
leo-engine new
leo-engine run
```

Publish instantly for any supported platform:
```
leo-engine publish --platform windows-amd64
```

Should feel CI/CD friendly.
---

## **4. Architecture Overview**

### **4.1 Engine Runtime**

* Distributed as a shared/static library.
* All subsystems included:
  **Windowing**, **rendering**, **input devices**, **sprites**, **collisions**, **camera**, **views**, **timers**, **threads**, **filesystem abstraction**, **sound**.
* Font/text rendering baked in (stb\_truetype) with batching.
* Simple particle system helper.
* Basic tilemap rendering (Tiled JSON format).
* Event system for decoupled communication.
* Virtual filesystem (loose files or `.pack`).

### **4.2 Front End**

* Standalone EXE statically linking the runtime.
* Lua bindings for scripting.
* Integrates with the CLI project manager. Note: this means if you are a bindings user, you are on your own with packaging.
* Lua-side helper modules for convenience.

---

## **5. API Philosophy**

* **Naming:** `leo_verb_noun()` with simple native C types.
* **Error handling:** C-style error codes + `leo_last_error()` for details.
* **Threading:** Main-thread-only for SDL calls; background jobs via worker pool.
* **Coordinates:** Origin at **top-left**, units in pixels.
* **Colors:** RGBA, 0–255 range.
* **Handles over structs** for ABI stability.

---

## **6. Rendering & Resolution**

* We provide a low level API that lets you render however you wish
* We also provide a high level API with particular rendering behavior
* Virtual resolution system with **pixel-perfect scaling**.
* Integer scaling and letterboxing favored by high-level API.
* Sprite batching options for performance.
* Blend modes: None, Alpha, Additive, Multiply.
* Camera and view system with parallax support, and support for split screen multiplayer games.

---

## **7. Input System**

* Low-level: raw keyboard, mouse, and gamepad.
* High-level: action binding system (optional).
* Relies on SDL’s gamepad database, with override capability.

---

## **8. Audio**

* You can load and play any audio formats supported by miniaudio
* We provide simple functions; we assume you are not an audio engineer
* If you have sophisticated audio requirements, I suggest using a different library.

---

## **9. Game Loop & Timing**

* Low level API so caller can make whatever game loop they like
* High level API where game loop is managed
* Managed loop: fixed-timestep update + variable render.
* Pause behavior customizable per game.
* Deterministic PRNG for replays.

---

## **10. Tiled Integration**

* Runtime: JSON parser.
* Front end: High-level Tiled map loader (JSON only; minimal compression support at start).

---

## **11. Resource System / VFS**

* Search order:

  * Debug: Loose dir → pack file.
  * Release: Pack file → loose dir.
* **Pack format:** LeoPack v1 — indexed for O(1) access.
* **Compression:** Per-file (sdefl).
* **Encryption:** Optional XOR cipher.
* Hot reload for Lua, images, font, shapes, and audio.

---

## **12. Lua Scripting API**

* Low-level: thin C bindings.
* High-level: Lua-idiomatic helpers.
* High-level lifecycle: `leo.init()`, `leo.update()`, `leo.render()`, `leo.exit()`.
* Pretty stack traces; SDL3 logging integration.

---

## **13. CLI Project Manager**

**Commands**

1. `new` — Bootstrap a new project.
2. `run` — Run project with hot reload.
3. `build` — Debug build for collaboration.
4. `release` — Package for distribution.
5. `profile` - Report profile info, and RAM consumption.
6. `demo` - Would be neat if this could play engine samples


**Features**

* Downloads correct runtime for target platform.
* Caches runtimes locally.
* Supports version pinning and checksum verification.
* Customized branding (icons, program metadata)

Debugger and IDE support would be nice... need to look into it
It would also be cool to one day support publishing to itch.io, Steam, and other stores.

---

## **14. Platform Support & CI**

* Initial:

  * Windows 11 (x86\_64)
  * macOS (arm64)
  * Linux (x86\_64)
* ABI stability promised via semantic versioning.
* Distribute via GitHub Releases with checksums.

---

## **15. Performance & Memory**

* User-supplied allocators supported.
* Explicit resource unload functions.
* Profiling hooks for frame time.
* Also need visibility into consumed RAM.

---

## **16. Extensibility**

* Extend via Lua API.
* Source can be forked for custom builds.
* Native C module injection of interest.

---

## **17. Documentation & Samples**

* C API documented with Doxygen.
* Cheat sheets for C and Lua APIs.
* Tiny, focused sample projects (sprite, tilemap, collision, audio, text, camera, particles).
* Unit tests double as examples.

---


