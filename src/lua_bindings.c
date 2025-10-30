// lua_bindings.c - minimal Lua bindings for leo engine and image
#include "leo/lua_bindings.h"

#include "leo/engine.h"
#include "leo/image.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

// Helpers to push simple structs
static void push_texture(lua_State *L, leo_Texture2D t)
{
    lua_createtable(L, 0, 3);
    lua_pushinteger(L, t.width);
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, t.height);
    lua_setfield(L, -2, "height");
    // store handle as lightuserdata for identity; opaque
    lua_pushlightuserdata(L, t._handle);
    lua_setfield(L, -2, "_handle");
}

static leo_Texture2D check_texture(lua_State *L, int idx)
{
    leo_Texture2D t = {0};
    luaL_checktype(L, idx, LUA_TTABLE);
    lua_getfield(L, idx, "width");
    t.width = (int)luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, idx, "height");
    t.height = (int)luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, idx, "_handle");
    if (!lua_islightuserdata(L, -1))
        luaL_error(L, "texture._handle must be lightuserdata");
    t._handle = lua_touserdata(L, -1);
    lua_pop(L, 1);
    return t;
}

// Engine bindings (subset relevant to scripts)
static int l_leo_init_window(lua_State *L)
{
    int w = (int)luaL_checkinteger(L, 1);
    int h = (int)luaL_checkinteger(L, 2);
    const char *title = luaL_optstring(L, 3, "leo");
    lua_pushboolean(L, leo_InitWindow(w, h, title));
    return 1;
}

static int l_leo_close_window(lua_State *L)
{
    (void)L;
    leo_CloseWindow();
    return 0;
}

static int l_leo_begin_drawing(lua_State *L)
{
    (void)L;
    leo_BeginDrawing();
    return 0;
}

static int l_leo_end_drawing(lua_State *L)
{
    (void)L;
    leo_EndDrawing();
    return 0;
}

static int l_leo_clear_background(lua_State *L)
{
    int r = (int)luaL_checkinteger(L, 1);
    int g = (int)luaL_checkinteger(L, 2);
    int b = (int)luaL_checkinteger(L, 3);
    int a = (int)luaL_optinteger(L, 4, 255);
    leo_ClearBackground(r, g, b, a);
    return 0;
}

// Image/texture bindings
static void attach_texture_mt(lua_State *L, int idx); // forward

static int l_image_load(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    leo_Texture2D t = leo_LoadTexture(path);
    push_texture(L, t);
    attach_texture_mt(L, -1);
    return 1;
}

static int l_image_is_ready(lua_State *L)
{
    leo_Texture2D t = check_texture(L, 1);
    lua_pushboolean(L, leo_IsTextureReady(t));
    return 1;
}

static int l_image_unload(lua_State *L)
{
    // We manage lifetime manually: call leo_UnloadTexture and clear fields
    luaL_checktype(L, 1, LUA_TTABLE);
    leo_Texture2D t = check_texture(L, 1);
    leo_UnloadTexture(&t);
    // zero table fields
    lua_pushinteger(L, 0); lua_setfield(L, 1, "width");
    lua_pushinteger(L, 0); lua_setfield(L, 1, "height");
    lua_pushlightuserdata(L, NULL); lua_setfield(L, 1, "_handle");
    return 0;
}

// Optional GC via metatable on texture tables
static int l_texture_gc(lua_State *L)
{
    // upvalue 1 is a lightuserdata tag; not used here
    if (lua_type(L, 1) == LUA_TTABLE)
    {
        // try unload; ignore errors
        lua_pushvalue(L, 1);
        lua_pushcfunction(L, l_image_unload);
        lua_insert(L, -2);
        lua_call(L, 1, 0);
    }
    return 0;
}

static void register_texture_mt(lua_State *L)
{
    if (luaL_newmetatable(L, "leo.texture"))
    {
        lua_pushcfunction(L, l_texture_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_pop(L, 1);
}

static void attach_texture_mt(lua_State *L, int idx)
{
    idx = lua_absindex(L, idx);
    luaL_getmetatable(L, "leo.texture");
    lua_setmetatable(L, idx);
}

int leo_LuaOpenBindings(void *vL)
{
    lua_State *L = (lua_State *)vL;
    register_texture_mt(L);

    // leo table
    lua_newtable(L);

    // engine functions
    lua_pushcfunction(L, l_leo_init_window); lua_setfield(L, -2, "init_window");
    lua_pushcfunction(L, l_leo_close_window); lua_setfield(L, -2, "close_window");
    lua_pushcfunction(L, l_leo_begin_drawing); lua_setfield(L, -2, "begin_drawing");
    lua_pushcfunction(L, l_leo_end_drawing); lua_setfield(L, -2, "end_drawing");
    lua_pushcfunction(L, l_leo_clear_background); lua_setfield(L, -2, "clear_background");

    // image subtable
    lua_createtable(L, 0, 3);
    lua_pushcfunction(L, l_image_load); lua_setfield(L, -2, "load");
    lua_pushcfunction(L, l_image_unload); lua_setfield(L, -2, "unload");
    lua_pushcfunction(L, l_image_is_ready); lua_setfield(L, -2, "is_ready");
    lua_setfield(L, -2, "image");

    // set global leo
    lua_setglobal(L, "leo");
    return 0;
}
