# Engine Core (Caller Experience)

This document describes the caller-facing experience for a minimal game loop
in Leo Engine. It is intentionally focused on how a game is written and run,
not on internal implementation details.

## Goals

- Provide a fixed-timestep simulation loop for determinism.
- Expose a clean place to render, play audio, and query input.
- Keep setup predictable and minimal for asset validation and tests.

## Quick Start (Caller View)

```cpp
#include "leo/engine_core.h"

int main(int argc, char** argv)
{
    leo::Engine::Config config = {
        .argv0 = argv[0],
        .resource_path = nullptr,
        .organization = "bluesentinelsec",
        .app_name = "leo-engine",
        .window_title = "Leo Engine",
        .window_width = 1280,
        .window_height = 720,
        .logical_width = 320,
        .logical_height = 180,
        .window_mode = leo::Engine::WindowMode::Windowed,
        .tick_hz = 60,
        .NumFrameTicks = 0,
        .malloc_fn = SDL_malloc,
        .realloc_fn = SDL_realloc,
        .free_fn = SDL_free
    };

    leo::Engine::Simulation game(config);
    game.Run();
}
```

At the highest level, your game is a `leo::Engine::Simulation` instance. The
simulation orchestrates the loop and invokes its internal lifecycle methods.

## Configuration

`leo::Engine::Simulation` uses `leo::Engine::Config` as a single source of truth:

- `argv0`, `resource_path`, `organization`, `app_name` for VFS setup.
- `window_title`, `window_width`, `window_height` for window creation.
- `logical_width`, `logical_height` for the virtual render resolution.
- `window_mode` for fullscreen/windowed/borderless.
- `tick_hz` for fixed-timestep simulation rate.
- `NumFrameTicks` for how many frame ticks to run (0 means run indefinitely).

### WindowMode

`leo::Engine::WindowMode` selects the display behavior:

- `Windowed`
- `Fullscreen`
- `BorderlessFullscreen`

## Simulation Callbacks

`leo::Engine::Simulation` defines internal lifecycle methods that the loop
invokes:

- `OnInit(Context&)`  
  One-time setup. Load assets from VFS, initialize game state.
- `OnUpdate(Context&, const InputFrame&)`  
  Fixed-timestep simulation step. All gameplay state changes happen here.
- `OnRender(Context&)`  
  Render current state. Rendering must not mutate gameplay state.
- `OnExit(Context&)`  
  Final cleanup.

## Input Model

The engine delivers one `InputFrame` per simulation tick. This is the canonical
input for deterministic gameplay:

- Keyboard/mouse/controller state is normalized into the frame.
- `OnUpdate` receives the current `InputFrame`.
- Rendering may consult input for UI, but gameplay state should be updated only
  in `OnUpdate`.

## Rendering and Audio (Caller Perspective)

- Rendering happens in `OnRender`. This is where sprite draws, font rendering,
  and debug overlays will live.
- Audio playback may be triggered in `OnInit` or `OnUpdate` (for deterministic
  logic), and should avoid heavy work inside the render path.

## Error Handling

Initialization and I/O failures throw exceptions. Unhandled exceptions terminate
the run loop with a clear error message (SDL message box at the top level).

## What This Enables

This minimal loop is enough to:
- Draw sprites and fonts each frame.
- Play audio clips at deterministic simulation events.
- Validate keyboard, mouse, and controller inputs.
- Record deterministic input streams in the future.
