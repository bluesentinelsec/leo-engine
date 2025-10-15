// src/lua_game.c
#include "leo/lua_game.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

// Lua includes
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

typedef struct
{
    const leo_LuaGameConfig *cfg;
    const leo_LuaGameCallbacks *cb;
    leo_LuaGameContext ctx;
} leo__LuaGameLoopData;

static void leo__LuaGameFrame(void *arg)
{
    leo__LuaGameLoopData *data = (leo__LuaGameLoopData *)arg;
    leo_LuaGameContext *ctx = &data->ctx;
    const leo_LuaGameConfig *cfg = data->cfg;
    const leo_LuaGameCallbacks *cb = data->cb;

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

    leo_BeginDrawing();
    {
        leo_ClearBackground(cfg->clear_color.r, cfg->clear_color.g, cfg->clear_color.b, cfg->clear_color.a);
        if (cb->on_render)
            cb->on_render(ctx);
    }
    leo_EndDrawing();
}

static const char *leo__nz(const char *s, const char *fallback)
{
    return (s && *s) ? s : fallback;
}

int leo_LuaGameRun(const leo_LuaGameConfig *cfg, const leo_LuaGameCallbacks *cb)
{
    if (!cfg || !cb || !cb->on_setup)
    {
        fprintf(stderr, "leo_LuaGameRun: invalid arguments\n");
        return 1;
    }

    // Initialize Lua state to prove it works
    lua_State *L = luaL_newstate();
    if (!L)
    {
        fprintf(stderr, "leo_LuaGameRun: failed to create Lua state\n");
        return 1;
    }
    luaL_openlibs(L);
    
    // Test Lua with simple expression
    int lua_result = luaL_dostring(L, "return 2 + 2");
    if (lua_result != LUA_OK)
    {
        fprintf(stderr, "leo_LuaGameRun: Lua test failed\n");
        lua_close(L);
        return 1;
    }
    
    // Clean up Lua state
    lua_close(L);

    const char *app_name = (cfg->app_name && *cfg->app_name) ? cfg->app_name : "Lua Game";
    const char *app_version = (cfg->app_version && *cfg->app_version) ? cfg->app_version : "1.0.0";
    const char *app_identifier = (cfg->app_identifier && *cfg->app_identifier) ? cfg->app_identifier : "com.leo-engine.lua-game";

    SDL_SetAppMetadata(app_name, app_version, app_identifier);

    const int win_w = (cfg->window_width > 0) ? cfg->window_width : 1280;
    const int win_h = (cfg->window_height > 0) ? cfg->window_height : 720;
    const char *title = leo__nz(cfg->window_title, "Leo Lua Game");

    if (!leo_InitWindow(win_w, win_h, title))
    {
        fprintf(stderr, "leo_LuaGameRun: leo_InitWindow failed\n");
        return 2;
    }

    if (cfg->target_fps > 0)
    {
        leo_SetTargetFPS(cfg->target_fps);
    }

    leo_LuaGameContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.user_data = cfg->user_data;
    ctx.dt = 0.0f;
    ctx.time_sec = 0.0;
    ctx.frame = 0;
    ctx.request_quit = false;

    if (!cb->on_setup(&ctx))
    {
        leo_CloseWindow();
        return 3;
    }

#ifdef __EMSCRIPTEN__
    leo__LuaGameLoopData *data = malloc(sizeof(leo__LuaGameLoopData));
    if (!data)
    {
        fprintf(stderr, "leo_LuaGameRun: malloc failed\n");
        leo_CloseWindow();
        return 4;
    }
    data->cfg = cfg;
    data->cb = cb;
    data->ctx = ctx;

    emscripten_set_main_loop_arg(leo__LuaGameFrame, data, (cfg->target_fps > 0) ? cfg->target_fps : 0, 1);
#else
    for (;;)
    {
        if (ctx.request_quit || leo_WindowShouldClose())
        {
            break;
        }

        ctx.dt = leo_GetFrameTime();
        ctx.time_sec = leo_GetTime();
        ctx.frame++;

        if (cb->on_update)
        {
            cb->on_update(&ctx);
        }

        leo_BeginDrawing();
        {
            const int r = cfg->clear_color.r;
            const int g = cfg->clear_color.g;
            const int b = cfg->clear_color.b;
            const int a = cfg->clear_color.a;
            leo_ClearBackground(r, g, b, a);

            if (cb->on_render)
            {
                cb->on_render(&ctx);
            }
        }
        leo_EndDrawing();
    }

    if (cb->on_shutdown)
    {
        cb->on_shutdown(&ctx);
    }

    leo_CloseWindow();
#endif

    return 0;
}

void leo_LuaGameQuit(leo_LuaGameContext *ctx)
{
    if (!ctx)
        return;
    ctx->request_quit = true;
}
