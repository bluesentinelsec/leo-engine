/* ==========================================================
 * File: include/leo/game.h
 * High-level managed loop for Leo
 *
 * Goals:
 *  - Get developers straight into actor creation and game logic
 *  - Own the window, timing, update/render sequencing
 *  - Keep it optional and lightweight (thin wrapper on engine + actors)
 * ========================================================== */
#pragma once

#include "leo/actor.h" /* actor system */
#include "leo/color.h"
#include "leo/engine.h" /* timing & drawing primitives */
#include "leo/export.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* Forward decl */
    typedef struct leo_GameContext leo_GameContext;

    /* ----------------------------------------------------------
       Game configuration
       ---------------------------------------------------------- */
    typedef struct leo_GameConfig
    {
        /* Window / device */
        int window_width;         /* e.g., 1280 */
        int window_height;        /* e.g., 720 */
        const char *window_title; /* e.g., "My Game" */
        int target_fps;           /* e.g., 60 (<=0 means no cap) */

        /* Optional logical resolution */
        int logical_width;                    /* <=0 disables logical scaling */
        int logical_height;                   /* <=0 disables logical scaling */
        leo_LogicalPresentation presentation; /* stretch/letterbox/overscan/disabled */
        leo_ScaleMode scale_mode;             /* nearest/linear/pixelart */

        /* Clear color each frame */
        leo_Color clear_color; /* e.g., LEO_BLACK (or any RGBA) */

        /* Start with global pause? */
        bool start_paused;

        /* User payload carried in the runtime context */
        void *user_data;
    } leo_GameConfig;

    /* ----------------------------------------------------------
       Game runtime context passed to callbacks
       ---------------------------------------------------------- */
    struct leo_GameContext
    {
        /* Immutable after start */
        leo_ActorSystem *actors; /* the actor system */
        leo_Actor *root;         /* convenience: system root */
        void *user_data;

        /* Per-frame telemetry (updated before update callback) */
        float dt;        /* seconds last frame (engine delta) */
        double time_sec; /* seconds since window init */
        int64_t frame;   /* incremented every update */

        /* Control flags (you may set these in callbacks) */
        bool request_quit; /* set true to exit after this frame */

        /* Internals reserved for the runtime */
        void *_impl;
    };

    /* ----------------------------------------------------------
       Game callback signatures
       ---------------------------------------------------------- */

    /* Called once after window + actor system are created.
       Return false to abort startup. Create your initial actor tree here. */
    typedef bool (*leo_GameSetupFn)(leo_GameContext *ctx);

    /* Called every frame (before we call actor_system_update).
       Good place for handling input, toggling pause, camera logic, etc. */
    typedef void (*leo_GameUpdateFn)(leo_GameContext *ctx);

    /* Called every frame *after* we render the actor tree.
       Draw UI overlays, debug, etc. (Runs between BeginDrawing/EndDrawing). */
    typedef void (*leo_GameRenderUiFn)(leo_GameContext *ctx);

    /* Called once right before we tear everything down. */
    typedef void (*leo_GameShutdownFn)(leo_GameContext *ctx);

    /* Bundled callbacks */
    typedef struct leo_GameCallbacks
    {
        leo_GameSetupFn on_setup;        /* required */
        leo_GameUpdateFn on_update;      /* optional */
        leo_GameRenderUiFn on_render_ui; /* optional */
        leo_GameShutdownFn on_shutdown;  /* optional */
    } leo_GameCallbacks;

    /* ----------------------------------------------------------
       High-level entry point
       ---------------------------------------------------------- */

    /* Create window, init actor system, run the main loop, destroy all.
       Returns 0 on normal exit, nonzero on early failure. */
    LEO_API int leo_GameRun(const leo_GameConfig *cfg, const leo_GameCallbacks *cb);

    /* ----------------------------------------------------------
       Convenience helpers (thin pass-throughs)
       ---------------------------------------------------------- */

    /* Global pause control (affects effective pause through the tree). */
    LEO_API void leo_GameSetPaused(leo_GameContext *ctx, bool paused);
    LEO_API bool leo_GameIsPaused(const leo_GameContext *ctx);

    /* Request to quit (same as ctx->request_quit = true). */
    LEO_API void leo_GameQuit(leo_GameContext *ctx);

#ifdef __cplusplus
} /* extern "C" */
#endif
