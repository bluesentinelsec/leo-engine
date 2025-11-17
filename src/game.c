// src/game.c
#include "leo/game.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_stdinc.h>
#include <stdio.h>

typedef struct
{
    const leo_GameConfig *cfg;
    const leo_GameCallbacks *cb;
    leo_GameContext ctx;
} leo__GameLoopData;

static void leo__GameFrame(void *arg)
{
    leo__GameLoopData *data = (leo__GameLoopData *)arg;
    leo_GameContext *ctx = &data->ctx;
    const leo_GameConfig *cfg = data->cfg;
    const leo_GameCallbacks *cb = data->cb;

    if (ctx->request_quit || leo_WindowShouldClose())
    {
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#endif
        return;
    }

    ctx->dt = leo_GetFrameTime();
    ctx->time_sec = leo_GetTime();
    ctx->frame++;

    if (cb->on_update)
        cb->on_update(ctx);

    leo_actor_system_update(ctx->actors, ctx->dt);

    leo_BeginDrawing();
    {
        leo_ClearBackground(cfg->clear_color.r, cfg->clear_color.g, cfg->clear_color.b, cfg->clear_color.a);
        leo_actor_system_render(ctx->actors);
        if (cb->on_render_ui)
            cb->on_render_ui(ctx);
    }
    leo_EndDrawing();
}

/* Small helper: safe title */
static const char *leo__nz(const char *s, const char *fallback)
{
    return (s && *s) ? s : fallback;
}

int leo_GameRun(const leo_GameConfig *cfg, const leo_GameCallbacks *cb)
{
    if (!cfg || !cb || !cb->on_setup)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "leo_GameRun: invalid arguments (cfg/cb/null or missing on_setup)");
        return 1;
    }

    /* ----- App metadata ----- */
    const char *app_name = (cfg->app_name && *cfg->app_name) ? cfg->app_name : "Game";
    const char *app_version = (cfg->app_version && *cfg->app_version) ? cfg->app_version : "1.0.0";
    const char *app_identifier =
        (cfg->app_identifier && *cfg->app_identifier) ? cfg->app_identifier : "com.leo-engine.game";

    SDL_SetAppMetadata(app_name, app_version, app_identifier);

    /* ----- Window & renderer init ----- */
    const int win_w = (cfg->window_width > 0) ? cfg->window_width : 1280;
    const int win_h = (cfg->window_height > 0) ? cfg->window_height : 720;
    const char *title = leo__nz(cfg->window_title, "Leo Game");

    if (!leo_InitWindow(win_w, win_h, title))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "leo_GameRun: leo_InitWindow(%d,%d,\"%s\") failed", win_w, win_h, title);
        return 2;
    }

    /* Set window mode */
    if (!leo_SetWindowMode(cfg->window_mode))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "leo_GameRun: leo_SetWindowMode failed; continuing in windowed mode");
    }

    if (cfg->target_fps > 0)
    {
        leo_SetTargetFPS(cfg->target_fps);
    }

    if (cfg->logical_width > 0 && cfg->logical_height > 0)
    {
        if (!leo_SetLogicalResolution(cfg->logical_width, cfg->logical_height, cfg->presentation, cfg->scale_mode))
        {
            /* Non-fatal: keep running without logical scaling */
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "leo_GameRun: leo_SetLogicalResolution failed; continuing without logical scaling");
        }
    }

    /* ----- Actor system ----- */
    leo_ActorSystem *actors = leo_actor_system_create();
    if (!actors)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "leo_GameRun: leo_actor_system_create failed");
        leo_CloseWindow();
        return 3;
    }
    if (cfg->start_paused)
    {
        leo_actor_system_set_paused(actors, true);
    }

    leo_Actor *root = leo_actor_system_root(actors);

    /* ----- Game context ----- */
    leo_GameContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.actors = actors;
    ctx.root = root;
    ctx.user_data = cfg->user_data;
    ctx.dt = 0.0f;
    ctx.time_sec = 0.0;
    ctx.frame = 0;
    ctx.request_quit = false;
    ctx._impl = (void *)actors; /* reserved; may be useful later */

    /* ----- User setup ----- */
    if (!cb->on_setup(&ctx))
    {
        /* Setup rejected; clean up and exit gracefully */
        leo_actor_system_destroy(actors);
        leo_CloseWindow();
        return 4;
    }

    /* ----- Main loop ----- */
#ifdef __EMSCRIPTEN__
    /* Package up loop state for emscripten */
    leo__GameLoopData *data = malloc(sizeof(leo__GameLoopData));
    if (!data)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "leo_GameRun: malloc failed");
        leo_actor_system_destroy(actors);
        leo_CloseWindow();
        return 5;
    }
    data->cfg = cfg;
    data->cb = cb;
    data->ctx = ctx; /* copy initial context */

    emscripten_set_main_loop_arg(leo__GameFrame, data, (cfg->target_fps > 0) ? cfg->target_fps : 0,
                                 1 /* simulate infinite loop */
    );

    /* NOTE: under emscripten, cleanup must happen inside emscripten_cancel_main_loop
       callback, so we don’t reach here unless canceled. */
#else
    for (;;)
    {
        /* Exit checks first (user or OS) */
        if (ctx.request_quit || leo_WindowShouldClose())
        {
            break;
        }

        /* Timing snapshot for this frame */
        ctx.dt = leo_GetFrameTime();
        ctx.time_sec = leo_GetTime();
        ctx.frame++;

        /* Optional user pre-update */
        if (cb->on_update)
        {
            cb->on_update(&ctx);
        }

        /* Update actor tree */
        leo_actor_system_update(actors, ctx.dt);

        /* Render pass */
        leo_BeginDrawing();
        {
            const int r = cfg->clear_color.r;
            const int g = cfg->clear_color.g;
            const int b = cfg->clear_color.b;
            const int a = cfg->clear_color.a;
            leo_ClearBackground(r, g, b, a);

            leo_actor_system_render(actors);

            if (cb->on_render_ui)
            {
                cb->on_render_ui(&ctx);
            }
        }
        leo_EndDrawing();
    }

    /* ----- Shutdown callback ----- */
    if (cb->on_shutdown)
    {
        cb->on_shutdown(&ctx);
    }

    /* ----- Teardown ----- */
    leo_actor_system_destroy(actors);
    leo_CloseWindow();
#endif

    return 0;
}

/* ----------------------------------------------------------
   Convenience helpers
   ---------------------------------------------------------- */

void leo_GameSetPaused(leo_GameContext *ctx, bool paused)
{
    if (!ctx || !ctx->actors)
        return;
    leo_actor_system_set_paused(ctx->actors, paused);
}

bool leo_GameIsPaused(const leo_GameContext *ctx)
{
    if (!ctx || !ctx->actors)
        return false;
    return leo_actor_system_is_paused(ctx->actors);
}

void leo_GameQuit(leo_GameContext *ctx)
{
    if (!ctx)
        return;
    ctx->request_quit = true;
}
