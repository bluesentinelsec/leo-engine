# Leo Engine — Design Document

## 1. Vision and Positioning

Leo Engine is a **code-first game framework** inspired by developer-centric toolkits such as DragonRuby Toolkit, LÖVE (Love2D), and Arcade (Python). The engine prioritizes clarity, determinism, automation, and reproducibility over editor-driven workflows. It is designed explicitly to integrate cleanly with AI-assisted coding workflows, where consistent conventions, predictable behavior, and well-defined contracts are essential.

Leo Engine targets the following platforms:

* Windows
* macOS
* Linux

(Web builds via Emscripten are explicitly out of scope for now.)

The engine is intended to be a **standalone executable runtime**. Game logic and content are treated as data supplied to the engine at launch. Pointing Leo Engine at the correct data should be sufficient to run a game without rebuilding the engine itself.

The primary design goal is to provide a deterministic, testable, automation-friendly runtime suitable for building complete games while remaining approachable and transparent for systems-oriented developers.

## 2. Runtime Model and Data Inputs

Leo Engine treats game logic and content as external data. The engine consumes game content in one of two supported formats:

1. A directory tree containing media assets and scripts.
2. A packed archive conforming to the **leo-packer** format.

The engine provides command-line arguments that allow callers to override:

* The path to the media directory or packed archive.
* The script used as the game’s entry point.

This enables flexible workflows for local development, automated testing, CI pipelines, and distribution.

### 2.1 Game Data Scope

Game data includes:

* Media assets (images, sound effects, music, fonts)
* Level data authored in **Tiled** (Tiled is treated as a first-class content pipeline)
* Shaders
* Lua scripts (once scripting is enabled)

Until Lua integration is complete, game logic will be implemented directly in C++ using engine APIs.

## 3. Platform and Dependency Strategy

Leo Engine has a hard dependency on **SDL3**. SDL3 provides the foundation for:

* Windowing
* Input
* Audio
* Timing
* Logging
* Memory management abstraction

SDL3 must be **statically linked** into the Leo Engine binary to simplify deployment and ensure predictable runtime behavior across platforms.

Additional third-party libraries may be introduced when justified, subject to the following constraints:

* Prefer libraries that are friendly to static linking.
* Avoid heavy transitive dependency chains.
* Favor portability, long-term maintenance, and build reproducibility.

## 4. Build System and Tooling

Leo Engine is implemented in **C++** and built using **CMake**. The CMake build is wrapped by a **Makefile** to simplify common developer workflows, including:

* Building
* Running tests
* Formatting code
* Cleaning build artifacts

The project follows **test-driven development (TDD)** practices and uses **Catch2** for unit testing.

A typical development loop is:

1. Define test cases that clearly express correctness criteria.
2. Implement tests first.
3. Implement production code.
4. Refactor for clarity and maintainability.
5. Run the formatter (`make fmt`).
6. Verify that the build and all tests pass.

Automation, repeatability, and mechanical sympathy are preferred over ad-hoc workflows.

## 5. Coding Conventions and Engineering Principles

Leo Engine favors conservative, explicit C++ with a strong emphasis on predictability, portability, and debuggability.

### 5.1 Error Handling

* The engine follows a **fail-fast philosophy**.
* Initialization and I/O failures are the most common failure modes and must throw exceptions.
* At the top of the call stack, unhandled exceptions must trigger a simple SDL3 message box displaying a clear and informative error message.

### 5.2 Logging

* Use SDL3’s built-in logging system exclusively.
* Supported log levels:

  * DEBUG
  * INFO
  * WARNING
  * ERROR
  * FATAL
* Avoid excessive logging inside hot loops.
* Logging exists to provide situational awareness and postmortem visibility, not continuous telemetry.

The engine will expose SDL3’s log routing mechanism so callers may redirect logs (for example, to files or custom sinks).

### 5.3 C++ Style and Memory

* Favor simple, readable C++ over advanced language features or syntactic sugar.
* Prefer clarity over cleverness.
* Use SDL’s standard library replacements (`<SDL3/SDL_stdinc.h>`) where practical for consistent cross-platform behavior.
* Use SDL memory allocation APIs exclusively so callers may provide custom allocators via `SDL_SetMemoryFunctions()`.
* Avoid C++ features that implicitly allocate outside SDL’s allocator (as much as reasonably possible).
* Prefer RAII for managing dynamic resources.
* Follow Google’s C++ style guide unless explicitly overridden by Leo Engine conventions.

### 5.4 Scripting

* Lua integration is planned but deferred.
* Core engine systems must stabilize before introducing scripting.
* When enabled, Lua will be used to implement game logic, not engine infrastructure.

## 6. Game Loop and Simulation Model

Leo Engine uses a **single-threaded, fixed-timestep simulation** for simplicity and determinism.

### 6.1 Fixed Tick

* The simulation advances in discrete ticks at a fixed rate (target: **60 Hz**).
* All gameplay-relevant state changes occur **only** inside the simulation tick function (e.g., `SimTick()`), never in rendering code or in direct response to raw OS events.
* Rendering may run at any rate and may interpolate for smooth visuals, but must not mutate gameplay state.

### 6.2 Deterministic Core (Single-Player)

Leo Engine guarantees determinism for single-player simulations by constraining the deterministic core.

The deterministic core includes:

* **Input normalization**: SDL input events are converted into exactly one canonical **InputFrame** per tick.
* **Integer / fixed-point gameplay state**: gameplay-critical state uses integers or fixed-point (positions, velocities, timers, cooldowns, collision bounds, etc.).
* **Deterministic RNG**: the engine provides explicit deterministic RNG streams seeded at startup.
* **Stable update order**: entity and system update ordering is stable and explicitly defined.

The following subsystems are intentionally excluded from determinism requirements:

* Rendering
* Audio callback timing and mixing
* Wall-clock timing (outside the tick accumulator)
* File I/O timing

Floating point is permitted only in rendering and non-gameplay subsystems. Gameplay correctness must not depend on floating-point behavior.

### 6.3 Recording and Playback (Demos)

Leo Engine supports attract mode and replay by recording and replaying **per-tick input**, not raw SDL events.

Recordings consist of:

* Header metadata (engine version, game version, tick rate, initial scene/map ID, configuration hash)
* Initial deterministic RNG seed(s)
* A contiguous stream of **InputFrame** records, one per simulation tick

Playback disables live input devices and injects recorded InputFrames into the simulation tick-by-tick.

State snapshots are not recorded in v1. Replay size grows linearly with time and remains small (on the order of a few megabytes per hour).

Future versions may optionally introduce sparse checkpoints to enable rewind and scrubbing without changing the core demo format.

### 6.4 Time Scaling and Pausing

* The simulation tick duration (`tick_dt`) never changes.
* Time scaling affects how quickly the simulation accumulator fills and therefore how many fixed ticks execute per wall-clock second.

Behavior:

* Slow motion: fewer ticks per second
* Fast forward: more ticks per second
* Pause: zero ticks

The engine maintains a set of independent **time-scale contributors** (for example: weapon wheel, cutscene, debug controls). The final `time_scale` is computed as the product of all contributors (or optionally the minimum).

Time-scale contributors must be derived from deterministic game state or input so that replay behavior remains correct.

## 7. Testing Strategy

Leo Engine includes a dedicated **test mode** for automated testing.

In test mode:

* The engine runs the simulation loop for a specified number of ticks.
* One tick is usually sufficient, but tests may request additional ticks when necessary.

Guidance for AI coding agents:

* Do not force headless rendering or complex mocking unless absolutely required.
* Most engine behavior can be validated by executing a small number of simulation ticks.

This approach keeps tests simple, deterministic, and representative of real runtime behavior.

## 8. Developer and Release Modes

Leo Engine supports multiple runtime profiles.

### 8.1 Developer Mode

* Logging level: DEBUG
* Window mode: Windowed
* Intended for rapid iteration and debugging.

### 8.2 Release Mode

* Logging level: INFO
* Window mode: Fullscreen borderless by default
* Intended for shipped builds.

### 8.3 Configuration Overrides

Callers may override window mode, resolution, and related settings via:

* Command-line arguments
* JSON configuration files

Precedence order:

`CLI overrides > configuration file > engine defaults`

## 9. Memory and Logging Policy

### 9.1 Memory Management

* Leo Engine relies exclusively on SDL3 memory allocation APIs.
* The engine exposes SDL’s allocator hook (`SDL_SetMemoryFunctions`) so callers may provide a custom memory manager if desired.
* The engine avoids C++ features that implicitly allocate outside SDL where practical.

No additional memory tracking or diagnostics are required beyond what SDL provides.

### 9.2 Logging

* Leo Engine wraps SDL3 logging and exposes the ability for callers to redirect output using `SDL_SetLogOutputFunction`.
* Callers may route logs to files, consoles, or custom sinks.
* The engine itself does not impose log retention or persistence policies.

## 10. Lua Integration Strategy

Lua will eventually provide game logic scripting.

Design constraints:

* Lua must not interfere with the managed game loop or determinism guarantees.
* Lua APIs must expose everything required for gameplay logic.
* Lua must be able to manipulate engine state (window mode, audio settings, etc.) through controlled APIs.
* Engine infrastructure remains in C++; Lua is used for orchestration and gameplay logic.

Lua integration is deferred until core systems stabilize.

## 11. Versioning and Compatibility

* The Leo Engine executable follows semantic versioning.
* Game data consists of media files, Lua scripts, shaders, and Tiled maps.
* Compatibility expectations are implicit and managed through engine evolution rather than strict schema guarantees.

## 12. Open Questions

The following questions remain intentionally open and should be resolved iteratively.

1. **InputFrame Schema**

   * What is the exact binary layout and versioning strategy for InputFrame?
   * How are analog inputs quantized and dead-zoned?
   TBD

2. **Deterministic RNG Policy**

   * How many RNG streams exist (global vs per-system)?
   * How are seeds persisted in demos and tests?
One seed, one engine RNG stream.
The caller provides a single uint64 seed (via CLI/config/test harness).
Leo initializes its deterministic RNG with that seed.
All gameplay RNG comes from that one RNG instance.

3. **Entity / System Update Ordering**

   * How is stable ordering guaranteed as systems evolve?
Leo Engine does not impose an actor or ECS policy.
Leo Engine guarantees deterministic simulation by requiring that all gameplay state changes occur inside SimTick() and by avoiding hidden iteration over game objects.

4. **Lua API Surface**

   * How is API versioning handled as engine capabilities grow?
   API should follow the open and closed principle.

5. **Asset Pipeline Constraints**

   * How are shader compilation errors surfaced?
   Throw an exception.
   * Are asset validation tools needed?
   No. If an asset can't be loaded/compiled, throw an exception.

6. **Replay Tooling**

   * Should a minimal replay inspector or CLI tool exist?
   No. Keep scope tight.

7. **Save Games vs Demos**

   * Are save files distinct from demo recordings or unified?
    TBD
8. **Threading Future**

   * Are background IO or asset pipelines planned later, and how will determinism boundaries be preserved?
   “wrap SDL async I/O” + “assets loaded in a scene constructor”
