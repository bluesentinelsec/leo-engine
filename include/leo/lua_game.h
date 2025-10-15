// leo/lua_game.h
#ifndef LEO_LUA_GAME_H
#define LEO_LUA_GAME_H

#include <stdint.h>
#include <stdbool.h>
#include "leo/color.h"
#include "leo/engine.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct leo_LuaGameContext
{
    void *user_data;
    float dt;
    double time_sec;
    int64_t frame;
    bool request_quit;
    void *_lua_state;  // Internal: lua_State pointer
} leo_LuaGameContext;

typedef struct leo_LuaGameConfig
{
    const char *app_name;
    const char *app_version;
    const char *app_identifier;
    const char *window_title;
    int window_width;
    int window_height;
    int target_fps;
    leo_Color clear_color;
    const char *script_path;  // Path to Lua script (via VFS)
    void *user_data;
} leo_LuaGameConfig;

typedef struct leo_LuaGameCallbacks
{
    bool (*on_setup)(leo_LuaGameContext *ctx);
    void (*on_update)(leo_LuaGameContext *ctx);
    void (*on_render)(leo_LuaGameContext *ctx);
    void (*on_shutdown)(leo_LuaGameContext *ctx);
} leo_LuaGameCallbacks;

int leo_LuaGameRun(const leo_LuaGameConfig *cfg, const leo_LuaGameCallbacks *cb);
void leo_LuaGameQuit(leo_LuaGameContext *ctx);

#ifdef __cplusplus
}
#endif

#endif // LEO_LUA_GAME_H
