// leo/lua_game.h
#ifndef LEO_LUA_GAME_H
#define LEO_LUA_GAME_H

#include "leo/color.h"
#include "leo/engine.h"
#include "leo/export.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct leo_LuaGameContext
    {
        void *user_data;
        float dt;
        double time_sec;
        int64_t frame;
        bool request_quit;
        void *_lua_state; // Internal: lua_State pointer
    } leo_LuaGameContext;

    typedef struct leo_LuaGameConfig
    {
        const char *app_name;
        const char *app_version;
        const char *app_identifier;
        const char *window_title;
        int window_width;
        int window_height;
        leo_WindowMode window_mode; // window display mode
        int target_fps;
        
        /* Optional logical resolution */
        int logical_width;                    /* <=0 disables logical scaling */
        int logical_height;                   /* <=0 disables logical scaling */
        leo_LogicalPresentation presentation; /* stretch/letterbox/overscan/disabled */
        leo_ScaleMode scale_mode;             /* nearest/linear/pixelart */
        
        leo_Color clear_color;
        const char *script_path; // Path to Lua script (via VFS)
        void *user_data;
    } leo_LuaGameConfig;

    typedef struct leo_LuaGameCallbacks
    {
        bool (*on_setup)(leo_LuaGameContext *ctx);
        void (*on_update)(leo_LuaGameContext *ctx);
        void (*on_render)(leo_LuaGameContext *ctx);
        void (*on_shutdown)(leo_LuaGameContext *ctx);
    } leo_LuaGameCallbacks;

    LEO_API int leo_LuaGameRun(const leo_LuaGameConfig *cfg, const leo_LuaGameCallbacks *cb);
    LEO_API void leo_LuaGameQuit(leo_LuaGameContext *ctx);

#ifdef __cplusplus
}
#endif

#endif // LEO_LUA_GAME_H
