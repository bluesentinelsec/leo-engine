// src/lua_game.c
#include "leo/lua_game.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Lua includes
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "leo/io.h"
#include "leo/keyboard.h"
#include "leo/keys.h"

typedef struct
{
    const leo_LuaGameConfig *cfg;
    const leo_LuaGameCallbacks *cb;
    leo_LuaGameContext ctx;
} leo__LuaGameLoopData;

static bool _load_lua_script(lua_State *L, const char *script_path, char **out_content, size_t *out_size)
{
    if (!script_path || !*script_path)
    {
        script_path = "scripts/game.lua";  // Default script
    }
    
    size_t size = 0;
    char *content = (char *)leo_LoadAsset(script_path, &size);
    if (!content)
    {
        fprintf(stderr, "Failed to load Lua script: %s\n", script_path);
        return false;
    }
    
    int result = luaL_loadbuffer(L, content, size, script_path);
    if (result != LUA_OK)
    {
        fprintf(stderr, "Lua compile error: %s\n", lua_tostring(L, -1));
        free(content);
        return false;
    }
    
    result = lua_pcall(L, 0, 0, 0);
    if (result != LUA_OK)
    {
        fprintf(stderr, "Lua execution error: %s\n", lua_tostring(L, -1));
        free(content);
        return false;
    }
    
    if (out_content) *out_content = content;
    else free(content);
    if (out_size) *out_size = size;
    
    return true;
}

static bool _call_lua_function(lua_State *L, const char *func_name)
{
    lua_getglobal(L, func_name);
    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 1);
        return false;  // Function doesn't exist, that's ok
    }
    
    int result = lua_pcall(L, 0, 1, 0);
    if (result != LUA_OK)
    {
        fprintf(stderr, "Lua error in %s: %s\n", func_name, lua_tostring(L, -1));
        lua_pop(L, 1);
        return false;
    }
    
    bool success = true;
    if (lua_isboolean(L, -1))
    {
        success = lua_toboolean(L, -1);
    }
    lua_pop(L, 1);
    return success;
}

static int lua_quit_game(lua_State *L)
{
    // Get the context from Lua registry
    lua_getfield(L, LUA_REGISTRYINDEX, "leo_context");
    leo_LuaGameContext *ctx = (leo_LuaGameContext *)lua_touserdata(L, -1);
    lua_pop(L, 1);
    
    if (ctx) {
        ctx->request_quit = true;
    }
    return 0;
}

static void _register_lua_functions(lua_State *L, leo_LuaGameContext *ctx)
{
    // Store context in registry for access from Lua functions
    lua_pushlightuserdata(L, ctx);
    lua_setfield(L, LUA_REGISTRYINDEX, "leo_context");
    
    // Register quit function
    lua_pushcfunction(L, lua_quit_game);
    lua_setglobal(L, "leo_quit");
}

static void _call_lua_update(lua_State *L, float dt)
{
    lua_getglobal(L, "leo_update");
    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 1);
        return;
    }
    
    lua_pushnumber(L, dt);
    
    int result = lua_pcall(L, 1, 0, 0);
    if (result != LUA_OK)
    {
        fprintf(stderr, "Lua error in leo_update: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

static void leo__LuaGameFrame(void *arg)
{
    leo__LuaGameLoopData *data = (leo__LuaGameLoopData *)arg;
    leo_LuaGameContext *ctx = &data->ctx;
    const leo_LuaGameConfig *cfg = data->cfg;
    lua_State *L = (lua_State *)ctx->_lua_state;

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

#ifdef DEBUG
    // Hot reload on Delete key press (debug builds only)
    if (leo_IsKeyPressed(KEY_DELETE))
    {
        printf("Hot reloading Lua script...\n");
        if (_load_lua_script(L, cfg->script_path, NULL, NULL))
        {
            printf("Script reloaded successfully!\n");
        }
    }
#endif

    _call_lua_update(L, ctx->dt);

    leo_BeginDrawing();
    {
        leo_ClearBackground(cfg->clear_color.r, cfg->clear_color.g, cfg->clear_color.b, cfg->clear_color.a);
        _call_lua_function(L, "leo_render");
    }
    leo_EndDrawing();
}

static const char *leo__nz(const char *s, const char *fallback)
{
    return (s && *s) ? s : fallback;
}

int leo_LuaGameRun(const leo_LuaGameConfig *cfg, const leo_LuaGameCallbacks *cb)
{
    if (!cfg)
    {
        fprintf(stderr, "leo_LuaGameRun: invalid config\n");
        return 1;
    }

    // Initialize Lua state
    lua_State *L = luaL_newstate();
    if (!L)
    {
        fprintf(stderr, "leo_LuaGameRun: failed to create Lua state\n");
        return 1;
    }
    luaL_openlibs(L);
    
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
        lua_close(L);
        return 2;
    }

    // Mount resources directory for VFS access
    if (!leo_MountDirectory("resources", 0))
    {
        fprintf(stderr, "leo_LuaGameRun: failed to mount resources directory\n");
        // Continue anyway - maybe script is elsewhere
    }

    // Load Lua script (after window init so VFS is available)
    if (!_load_lua_script(L, cfg->script_path, NULL, NULL))
    {
        lua_close(L);
        leo_CloseWindow();
        return 1;
    }

    // Initialize context
    leo_LuaGameContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.user_data = cfg->user_data;
    ctx.dt = 0.0f;
    ctx.time_sec = 0.0;
    ctx.frame = 0;
    ctx.request_quit = false;
    ctx._lua_state = L;

    // Register Leo functions with Lua
    _register_lua_functions(L, &ctx);

    if (cfg->target_fps > 0)
    {
        leo_SetTargetFPS(cfg->target_fps);
    }

    // Call Lua init function
    if (!_call_lua_function(L, "leo_init"))
    {
        fprintf(stderr, "leo_LuaGameRun: leo_init failed or returned false\n");
        lua_close(L);
        leo_CloseWindow();
        return 3;
    }

#ifdef __EMSCRIPTEN__
    leo__LuaGameLoopData *data = malloc(sizeof(leo__LuaGameLoopData));
    if (!data)
    {
        fprintf(stderr, "leo_LuaGameRun: malloc failed\n");
        lua_close(L);
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

#ifdef DEBUG
        // Hot reload on Delete key press (debug builds only)
        if (leo_IsKeyPressed(KEY_DELETE))
        {
            printf("Hot reloading Lua script...\n");
            if (_load_lua_script(L, cfg->script_path, NULL, NULL))
            {
                printf("Script reloaded successfully!\n");
            }
        }
#endif

        _call_lua_update(L, ctx.dt);

        leo_BeginDrawing();
        {
            const int r = cfg->clear_color.r;
            const int g = cfg->clear_color.g;
            const int b = cfg->clear_color.b;
            const int a = cfg->clear_color.a;
            leo_ClearBackground(r, g, b, a);

            _call_lua_function(L, "leo_render");
        }
        leo_EndDrawing();
    }

    // Call Lua exit function
    _call_lua_function(L, "leo_exit");

    lua_close(L);
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
