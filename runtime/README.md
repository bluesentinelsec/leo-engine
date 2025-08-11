# Runtime

## High level C API

```c
// main.c — Leo high-level C API usage example (caller perspective)

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <leo/leo_engine.h>

// ---- User state you want accessible in callbacks ---------------------------
typedef struct AppState {
    int frame_count;
    const char *greeting;
} AppState;

// ---- Callback prototypes (idiomatic C typedefs usually live in the header) --
static bool on_init(void *user);
static void on_update(float dt, void *user);
static void on_render(void *user);
static void on_exit(void *user);

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    // 1) Prepare user data that your game wants to access in callbacks
    AppState state = {
        .frame_count = 0,
        .greeting = "hello from leo!"
    };

    // 2) Configure the engine (stack-allocated config; no heap unless you set heap strings)
    leo_engine_config_t config;
    leo_engine_config_init(&config);     // fill with sensible defaults

    // Required callbacks (lifecycle)
    config.on_init   = on_init;
    config.on_update = on_update;
    config.on_render = on_render;
    config.on_exit   = on_exit;

    // Optional bits (window, timing, assets, etc.) — tweak as needed
    config.window.title        = "Leo Sample";
    config.window.width        = 1280;
    config.window.height       = 720;
    config.window.start_fullscreen = false;

    // Example timing choices: fixed update @ 60Hz, uncapped render with vsync
    config.timing.fixed_hz     = 60.0f;
    config.timing.vsync        = true;
    config.timing.max_fps_cap  = 0;      // 0 = uncapped (renderer may still vsync)

    // If your engine uses a resource pack or entry script, set them here if applicable:
    // config.assets.pack_path    = "resources.leopack";
    // config.script.entry_point  = "scripts/game.lua";

    // 3) Create the engine
    leo_engine_t engine;                 // stack handle
    leo_err err = leo_engine_new(&engine, &config);
    if (err != LEO_OK) {
        fprintf(stderr, "leo_engine_new failed: %s\n", leo_err_str(err));
        return EXIT_FAILURE;
    }

    // 4) Run the main loop — blocks until the game exits or an error occurs
    err = leo_engine_run(&engine, &config, &state);
    if (err != LEO_OK && err != LEO_ERR_SHUTDOWN_REQUESTED) {
        fprintf(stderr, "leo_engine_run failed: %s\n", leo_err_str(err));
        // fall through to cleanup
    }

    // 5) Clean up
    leo_engine_free(&engine);

    // If config owns heap strings (e.g., duplicated title/paths), call this:
    // leo_engine_config_free(&config);

    return (err == LEO_OK || err == LEO_ERR_SHUTDOWN_REQUESTED) ? EXIT_SUCCESS : EXIT_FAILURE;
}

// ---- Callback implementations ----------------------------------------------

static bool on_init(void *user) {
    AppState *s = (AppState*)user;
    s->frame_count = 0;
    // Load textures/sounds, create scenes, etc. Return false to abort startup.
    printf("[init] %s\n", s->greeting);
    return true;
}

static void on_update(float dt, void *user) {
    AppState *s = (AppState*)user;
    (void)dt;
    s->frame_count++;

    // Example: if (leo_input_key_pressed_once(LEO_KEY_ESCAPE)) leo_request_shutdown();
    // Advance simulation, handle input, spawn/despawn, etc.
}

static void on_render(void *user) {
    AppState *s = (AppState*)user;
    // Draw your world/UI here using Leo’s draw APIs.
    // Example (imaginary): leo_draw_text(10, 10, "Frames: %d", s->frame_count);
    (void)s;
}

static void on_exit(void *user) {
    AppState *s = (AppState*)user;
    printf("[exit] total frames: %d\n", s->frame_count);
    // Free any user-managed resources if needed.
}
```
