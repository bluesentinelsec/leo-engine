// lua_bindings.c - minimal Lua bindings for leo engine and image
#include "leo/lua_bindings.h"

#include "leo/engine.h"
#include "leo/image.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

// Userdata for leo_Texture2D
typedef struct {
    leo_Texture2D tex;
    int owns; // 1 if should unload on __gc
} leo__LuaTexture;

static const char *LEO_TEX_META = "leo_texture";

static leo__LuaTexture *push_texture(lua_State *L, leo_Texture2D t, int take_ownership)
{
    leo__LuaTexture *ud = (leo__LuaTexture *)lua_newuserdata(L, sizeof(leo__LuaTexture));
    ud->tex = t;
    ud->owns = take_ownership;
    luaL_getmetatable(L, LEO_TEX_META);
    lua_setmetatable(L, -2);
    return ud;
}

static leo__LuaTexture *check_texture_ud(lua_State *L, int idx)
{
    return (leo__LuaTexture *)luaL_checkudata(L, idx, LEO_TEX_META);
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
    push_texture(L, t, 1);
    return 1;
}

static int l_image_load_from_memory(lua_State *L)
{
    size_t len = 0;
    const char *ext = luaL_checkstring(L, 1);
    const char *buf = luaL_checklstring(L, 2, &len);
    leo_Texture2D t = leo_LoadTextureFromMemory(ext, (const unsigned char *)buf, (int)len);
    push_texture(L, t, 1);
    return 1;
}

static int l_image_load_from_texture(lua_State *L)
{
    leo__LuaTexture *srcud = check_texture_ud(L, 1);
    leo_Texture2D src = srcud->tex;
    leo_Texture2D t = leo_LoadTextureFromTexture(src);
    push_texture(L, t, 1);
    return 1;
}

static int l_image_load_from_pixels(lua_State *L)
{
    size_t len = 0;
    const char *pixels = luaL_checklstring(L, 1, &len);
    int width = (int)luaL_checkinteger(L, 2);
    int height = (int)luaL_checkinteger(L, 3);
    int pitch = (int)luaL_optinteger(L, 4, 0);
    int format = (int)luaL_optinteger(L, 5, LEO_TEXFORMAT_R8G8B8A8);
    if (pitch == 0)
    {
        int bpp = leo_TexFormatBytesPerPixel((leo_TexFormat)format);
        pitch = width * bpp;
    }
    (void)len; // pixel size is implied by pitch and height; driver copies
    leo_Texture2D t = leo_LoadTextureFromPixels(pixels, width, height, pitch, (leo_TexFormat)format);
    push_texture(L, t, 1);
    return 1;
}

static int l_image_is_ready(lua_State *L)
{
    leo__LuaTexture *ud = check_texture_ud(L, 1);
    lua_pushboolean(L, leo_IsTextureReady(ud->tex));
    return 1;
}

static int l_image_unload(lua_State *L)
{
    // We manage lifetime manually: call leo_UnloadTexture and clear fields
    leo__LuaTexture *ud = check_texture_ud(L, 1);
    if (ud->owns) {
        leo_UnloadTexture(&ud->tex);
        ud->owns = 0;
        ud->tex.width = 0;
        ud->tex.height = 0;
        ud->tex._handle = NULL;
    }
    return 0;
}

// Optional GC via metatable on texture tables
static int l_texture_gc(lua_State *L)
{
    leo__LuaTexture *ud = (leo__LuaTexture *)luaL_testudata(L, 1, LEO_TEX_META);
    if (ud && ud->owns) {
        leo_UnloadTexture(&ud->tex);
        ud->owns = 0;
    }
    return 0;
}

static void register_texture_mt(lua_State *L)
{
    if (luaL_newmetatable(L, LEO_TEX_META))
    {
        lua_pushcfunction(L, l_texture_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_pop(L, 1);
}

// helper function must be at file scope for C
static int l_image_bytes_per_pixel(lua_State *L)
{
    int fmt = (int)luaL_checkinteger(L, 1);
    int bpp = leo_TexFormatBytesPerPixel((leo_TexFormat)fmt);
    lua_pushinteger(L, bpp);
    return 1;
}

int leo_LuaOpenBindings(void *vL)
{
    lua_State *L = (lua_State *)vL;
    register_texture_mt(L);

    // No namespace: register global leo_* symbols

    // engine functions
    lua_pushcfunction(L, l_leo_init_window); lua_setglobal(L, "leo_init_window");
    lua_pushcfunction(L, l_leo_close_window); lua_setglobal(L, "leo_close_window");
    lua_pushcfunction(L, l_leo_begin_drawing); lua_setglobal(L, "leo_begin_drawing");
    lua_pushcfunction(L, l_leo_end_drawing); lua_setglobal(L, "leo_end_drawing");
    lua_pushcfunction(L, l_leo_clear_background); lua_setglobal(L, "leo_clear_background");

    // image functions and constants as globals
    lua_pushcfunction(L, l_image_load); lua_setglobal(L, "leo_image_load");
    lua_pushcfunction(L, l_image_load_from_memory); lua_setglobal(L, "leo_image_load_from_memory");
    lua_pushcfunction(L, l_image_load_from_texture); lua_setglobal(L, "leo_image_load_from_texture");
    lua_pushcfunction(L, l_image_load_from_pixels); lua_setglobal(L, "leo_image_load_from_pixels");
    lua_pushcfunction(L, l_image_unload); lua_setglobal(L, "leo_image_unload");
    lua_pushcfunction(L, l_image_is_ready); lua_setglobal(L, "leo_image_is_ready");
    lua_pushcfunction(L, l_image_bytes_per_pixel); lua_setglobal(L, "leo_image_bytes_per_pixel");

    // enum constants
    lua_pushinteger(L, LEO_TEXFORMAT_UNDEFINED); lua_setglobal(L, "leo_TEXFORMAT_UNDEFINED");
    lua_pushinteger(L, LEO_TEXFORMAT_R8G8B8A8); lua_setglobal(L, "leo_TEXFORMAT_R8G8B8A8");
    lua_pushinteger(L, LEO_TEXFORMAT_R8G8B8); lua_setglobal(L, "leo_TEXFORMAT_R8G8B8");
    lua_pushinteger(L, LEO_TEXFORMAT_GRAY8); lua_setglobal(L, "leo_TEXFORMAT_GRAY8");
    lua_pushinteger(L, LEO_TEXFORMAT_GRAY8_ALPHA); lua_setglobal(L, "leo_TEXFORMAT_GRAY8_ALPHA");

    return 0;
}
