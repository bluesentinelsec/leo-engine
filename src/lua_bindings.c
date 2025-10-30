#include "leo/lua_bindings.h"

#include "leo/engine.h"
#include "leo/image.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

typedef struct
{
    leo_Texture2D tex;
    int owns; // whether GC should unload
} leo__LuaTexture;

typedef struct
{
    leo_RenderTexture2D rt;
    int owns; // whether GC should unload
} leo__LuaRenderTexture;

static const char *LEO_TEX_META = "leo_texture";
static const char *LEO_RT_META = "leo_render_texture";

static leo__LuaTexture *push_texture(lua_State *L, leo_Texture2D t, int owns)
{
    leo__LuaTexture *ud = (leo__LuaTexture *)lua_newuserdata(L, sizeof(leo__LuaTexture));
    ud->tex = t;
    ud->owns = owns;
    luaL_getmetatable(L, LEO_TEX_META);
    lua_setmetatable(L, -2);
    return ud;
}

static leo__LuaTexture *check_texture_ud(lua_State *L, int idx)
{
    return (leo__LuaTexture *)luaL_checkudata(L, idx, LEO_TEX_META);
}

static leo__LuaRenderTexture *push_render_texture(lua_State *L, leo_RenderTexture2D rt, int owns)
{
    leo__LuaRenderTexture *ud = (leo__LuaRenderTexture *)lua_newuserdata(L, sizeof(leo__LuaRenderTexture));
    ud->rt = rt;
    ud->owns = owns;
    luaL_getmetatable(L, LEO_RT_META);
    lua_setmetatable(L, -2);
    return ud;
}

static leo__LuaRenderTexture *check_render_texture_ud(lua_State *L, int idx)
{
    return (leo__LuaRenderTexture *)luaL_checkudata(L, idx, LEO_RT_META);
}

static int l_texture_gc(lua_State *L)
{
    leo__LuaTexture *ud = (leo__LuaTexture *)luaL_testudata(L, 1, LEO_TEX_META);
    if (ud && ud->owns)
    {
        leo_UnloadTexture(&ud->tex);
        ud->owns = 0;
        ud->tex.width = 0;
        ud->tex.height = 0;
        ud->tex._handle = NULL;
    }
    return 0;
}

static int l_render_texture_gc(lua_State *L)
{
    leo__LuaRenderTexture *ud = (leo__LuaRenderTexture *)luaL_testudata(L, 1, LEO_RT_META);
    if (ud && ud->owns)
    {
        leo_UnloadRenderTexture(ud->rt);
        ud->owns = 0;
        ud->rt.texture.width = 0;
        ud->rt.texture.height = 0;
        ud->rt.texture._handle = NULL;
        ud->rt.width = 0;
        ud->rt.height = 0;
        ud->rt._rt_handle = NULL;
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

static void register_render_texture_mt(lua_State *L)
{
    if (luaL_newmetatable(L, LEO_RT_META))
    {
        lua_pushcfunction(L, l_render_texture_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_pop(L, 1);
}

static float lua_get_field_float(lua_State *L, int index, const char *name, float def)
{
    float value = def;
    lua_getfield(L, index, name);
    if (!lua_isnil(L, -1))
        value = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);
    return value;
}

static void lua_camera_from_table(lua_State *L, int index, leo_Camera2D *cam)
{
    cam->target.x = lua_get_field_float(L, index, "target_x", 0.0f);
    cam->target.y = lua_get_field_float(L, index, "target_y", 0.0f);
    cam->offset.x = lua_get_field_float(L, index, "offset_x", 0.0f);
    cam->offset.y = lua_get_field_float(L, index, "offset_y", 0.0f);
    cam->rotation = lua_get_field_float(L, index, "rotation", 0.0f);
    cam->zoom = lua_get_field_float(L, index, "zoom", 1.0f);
}

static void lua_camera_from_numbers(lua_State *L, int index, leo_Camera2D *cam)
{
    cam->target.x = (float)luaL_checknumber(L, index);
    cam->target.y = (float)luaL_checknumber(L, index + 1);
    cam->offset.x = (float)luaL_checknumber(L, index + 2);
    cam->offset.y = (float)luaL_checknumber(L, index + 3);
    cam->rotation = (float)luaL_checknumber(L, index + 4);
    cam->zoom = (float)luaL_checknumber(L, index + 5);
}

static int lua_try_camera(lua_State *L, int index, leo_Camera2D *cam)
{
    if (lua_isnoneornil(L, index))
        return 0;
    if (lua_istable(L, index))
    {
        lua_camera_from_table(L, index, cam);
        return 1;
    }
    lua_camera_from_numbers(L, index, cam);
    return 6;
}

static void lua_push_camera_table(lua_State *L, const leo_Camera2D *cam)
{
    lua_createtable(L, 0, 6);
    lua_pushnumber(L, cam->target.x);
    lua_setfield(L, -2, "target_x");
    lua_pushnumber(L, cam->target.y);
    lua_setfield(L, -2, "target_y");
    lua_pushnumber(L, cam->offset.x);
    lua_setfield(L, -2, "offset_x");
    lua_pushnumber(L, cam->offset.y);
    lua_setfield(L, -2, "offset_y");
    lua_pushnumber(L, cam->rotation);
    lua_setfield(L, -2, "rotation");
    lua_pushnumber(L, cam->zoom);
    lua_setfield(L, -2, "zoom");
}

static void lua_push_vector2(lua_State *L, leo_Vector2 v)
{
    lua_pushnumber(L, v.x);
    lua_pushnumber(L, v.y);
}

// -----------------------------------------------------------------------------
// Engine window/system bindings
// -----------------------------------------------------------------------------
static int l_leo_init_window(lua_State *L)
{
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);
    const char *title = luaL_optstring(L, 3, "leo");
    lua_pushboolean(L, leo_InitWindow(width, height, title));
    return 1;
}

static int l_leo_close_window(lua_State *L)
{
    (void)L;
    leo_CloseWindow();
    return 0;
}

static int l_leo_get_window(lua_State *L)
{
    void *win = leo_GetWindow();
    if (win)
        lua_pushlightuserdata(L, win);
    else
        lua_pushnil(L);
    return 1;
}

static int l_leo_get_renderer(lua_State *L)
{
    void *renderer = leo_GetRenderer();
    if (renderer)
        lua_pushlightuserdata(L, renderer);
    else
        lua_pushnil(L);
    return 1;
}

static int l_leo_set_fullscreen(lua_State *L)
{
    int enabled = lua_toboolean(L, 1);
    lua_pushboolean(L, leo_SetFullscreen(enabled != 0));
    return 1;
}

static int l_leo_set_window_mode(lua_State *L)
{
    int mode = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, leo_SetWindowMode((leo_WindowMode)mode));
    return 1;
}

static int l_leo_get_window_mode(lua_State *L)
{
    lua_pushinteger(L, leo_GetWindowMode());
    return 1;
}

static int l_leo_window_should_close(lua_State *L)
{
    (void)L;
    lua_pushboolean(L, leo_WindowShouldClose());
    return 1;
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

// -----------------------------------------------------------------------------
// Timing / frame info
// -----------------------------------------------------------------------------
static int l_leo_set_target_fps(lua_State *L)
{
    int fps = (int)luaL_checkinteger(L, 1);
    leo_SetTargetFPS(fps);
    return 0;
}

static int l_leo_get_frame_time(lua_State *L)
{
    lua_pushnumber(L, leo_GetFrameTime());
    return 1;
}

static int l_leo_get_time(lua_State *L)
{
    lua_pushnumber(L, leo_GetTime());
    return 1;
}

static int l_leo_get_fps(lua_State *L)
{
    lua_pushinteger(L, leo_GetFPS());
    return 1;
}

// -----------------------------------------------------------------------------
// Screen info
// -----------------------------------------------------------------------------
static int l_leo_get_screen_width(lua_State *L)
{
    lua_pushinteger(L, leo_GetScreenWidth());
    return 1;
}

static int l_leo_get_screen_height(lua_State *L)
{
    lua_pushinteger(L, leo_GetScreenHeight());
    return 1;
}

// -----------------------------------------------------------------------------
// Camera bindings
// -----------------------------------------------------------------------------
static int l_leo_begin_mode2d(lua_State *L)
{
    leo_Camera2D cam;
    if (lua_istable(L, 1))
        lua_camera_from_table(L, 1, &cam);
    else
        lua_camera_from_numbers(L, 1, &cam);
    leo_BeginMode2D(cam);
    return 0;
}

static int l_leo_end_mode2d(lua_State *L)
{
    (void)L;
    leo_EndMode2D();
    return 0;
}

static int l_leo_is_camera_active(lua_State *L)
{
    (void)L;
    lua_pushboolean(L, leo_IsCameraActive());
    return 1;
}

static int l_leo_get_world_to_screen2d(lua_State *L)
{
    leo_Vector2 position;
    position.x = (float)luaL_checknumber(L, 1);
    position.y = (float)luaL_checknumber(L, 2);

    leo_Camera2D cam;
    if (lua_try_camera(L, 3, &cam) == 0)
        cam = leo_GetCurrentCamera2D();

    lua_push_vector2(L, leo_GetWorldToScreen2D(position, cam));
    return 2;
}

static int l_leo_get_screen_to_world2d(lua_State *L)
{
    leo_Vector2 position;
    position.x = (float)luaL_checknumber(L, 1);
    position.y = (float)luaL_checknumber(L, 2);

    leo_Camera2D cam;
    if (lua_try_camera(L, 3, &cam) == 0)
        cam = leo_GetCurrentCamera2D();

    lua_push_vector2(L, leo_GetScreenToWorld2D(position, cam));
    return 2;
}

static int l_leo_get_current_camera2d(lua_State *L)
{
    leo_Camera2D cam = leo_GetCurrentCamera2D();
    lua_push_camera_table(L, &cam);
    return 1;
}

// -----------------------------------------------------------------------------
// Render texture bindings
// -----------------------------------------------------------------------------
static int l_leo_load_render_texture(lua_State *L)
{
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);
    leo_RenderTexture2D rt = leo_LoadRenderTexture(width, height);
    push_render_texture(L, rt, 1);
    return 1;
}

static int l_leo_unload_render_texture(lua_State *L)
{
    leo__LuaRenderTexture *ud = check_render_texture_ud(L, 1);
    if (ud->owns)
    {
        leo_UnloadRenderTexture(ud->rt);
        ud->owns = 0;
    }
    ud->rt.texture.width = 0;
    ud->rt.texture.height = 0;
    ud->rt.texture._handle = NULL;
    ud->rt.width = 0;
    ud->rt.height = 0;
    ud->rt._rt_handle = NULL;
    return 0;
}

static int l_leo_begin_texture_mode(lua_State *L)
{
    leo__LuaRenderTexture *ud = check_render_texture_ud(L, 1);
    leo_BeginTextureMode(ud->rt);
    return 0;
}

static int l_leo_end_texture_mode(lua_State *L)
{
    (void)L;
    leo_EndTextureMode();
    return 0;
}

static int l_leo_render_texture_get_texture(lua_State *L)
{
    leo__LuaRenderTexture *ud = check_render_texture_ud(L, 1);
    push_texture(L, ud->rt.texture, 0);
    return 1;
}

static int l_leo_render_texture_get_size(lua_State *L)
{
    leo__LuaRenderTexture *ud = check_render_texture_ud(L, 1);
    lua_pushinteger(L, ud->rt.width);
    lua_pushinteger(L, ud->rt.height);
    return 2;
}

// -----------------------------------------------------------------------------
// Image / texture bindings
// -----------------------------------------------------------------------------
static int l_image_load(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    leo_Texture2D tex = leo_LoadTexture(path);
    push_texture(L, tex, 1);
    return 1;
}

static int l_image_load_from_memory(lua_State *L)
{
    size_t len = 0;
    const char *ext = luaL_checkstring(L, 1);
    const char *data = luaL_checklstring(L, 2, &len);
    leo_Texture2D tex = leo_LoadTextureFromMemory(ext, (const unsigned char *)data, (int)len);
    push_texture(L, tex, 1);
    return 1;
}

static int l_image_load_from_texture(lua_State *L)
{
    leo__LuaTexture *src = check_texture_ud(L, 1);
    leo_Texture2D tex = leo_LoadTextureFromTexture(src->tex);
    push_texture(L, tex, 1);
    return 1;
}

static int l_image_load_from_pixels(lua_State *L)
{
    size_t len = 0;
    const void *pixels = luaL_checklstring(L, 1, &len);
    int width = (int)luaL_checkinteger(L, 2);
    int height = (int)luaL_checkinteger(L, 3);
    int pitch = (int)luaL_optinteger(L, 4, 0);
    int format = (int)luaL_optinteger(L, 5, LEO_TEXFORMAT_R8G8B8A8);
    if (pitch == 0)
    {
        int bpp = leo_TexFormatBytesPerPixel((leo_TexFormat)format);
        pitch = width * bpp;
    }
    (void)len;
    leo_Texture2D tex = leo_LoadTextureFromPixels(pixels, width, height, pitch, (leo_TexFormat)format);
    push_texture(L, tex, 1);
    return 1;
}

static int l_image_is_ready(lua_State *L)
{
    leo__LuaTexture *tex = check_texture_ud(L, 1);
    lua_pushboolean(L, leo_IsTextureReady(tex->tex));
    return 1;
}

static int l_image_unload(lua_State *L)
{
    leo__LuaTexture *tex = check_texture_ud(L, 1);
    if (tex->owns)
    {
        leo_UnloadTexture(&tex->tex);
        tex->owns = 0;
    }
    tex->tex.width = 0;
    tex->tex.height = 0;
    tex->tex._handle = NULL;
    return 0;
}

static int l_image_bytes_per_pixel(lua_State *L)
{
    int format = (int)luaL_checkinteger(L, 1);
    lua_pushinteger(L, leo_TexFormatBytesPerPixel((leo_TexFormat)format));
    return 1;
}

static int l_leo_texture_get_size(lua_State *L)
{
    leo__LuaTexture *tex = check_texture_ud(L, 1);
    lua_pushinteger(L, tex->tex.width);
    lua_pushinteger(L, tex->tex.height);
    return 2;
}

// -----------------------------------------------------------------------------
// Texture drawing bindings
// -----------------------------------------------------------------------------
static int l_leo_draw_texture_rec(lua_State *L)
{
    leo__LuaTexture *tex = check_texture_ud(L, 1);
    leo_Rectangle src;
    src.x = (float)luaL_checknumber(L, 2);
    src.y = (float)luaL_checknumber(L, 3);
    src.width = (float)luaL_checknumber(L, 4);
    src.height = (float)luaL_checknumber(L, 5);
    leo_Vector2 pos;
    pos.x = (float)luaL_checknumber(L, 6);
    pos.y = (float)luaL_checknumber(L, 7);
    int r = (int)luaL_optinteger(L, 8, 255);
    int g = (int)luaL_optinteger(L, 9, 255);
    int b = (int)luaL_optinteger(L, 10, 255);
    int a = (int)luaL_optinteger(L, 11, 255);
    leo_Color tint = {(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a};
    leo_DrawTextureRec(tex->tex, src, pos, tint);
    return 0;
}

static int l_leo_draw_texture_pro(lua_State *L)
{
    leo__LuaTexture *tex = check_texture_ud(L, 1);
    leo_Rectangle src;
    src.x = (float)luaL_checknumber(L, 2);
    src.y = (float)luaL_checknumber(L, 3);
    src.width = (float)luaL_checknumber(L, 4);
    src.height = (float)luaL_checknumber(L, 5);
    leo_Rectangle dest;
    dest.x = (float)luaL_checknumber(L, 6);
    dest.y = (float)luaL_checknumber(L, 7);
    dest.width = (float)luaL_checknumber(L, 8);
    dest.height = (float)luaL_checknumber(L, 9);
    leo_Vector2 origin;
    origin.x = (float)luaL_checknumber(L, 10);
    origin.y = (float)luaL_checknumber(L, 11);
    float rotation = (float)luaL_optnumber(L, 12, 0.0);
    int r = (int)luaL_optinteger(L, 13, 255);
    int g = (int)luaL_optinteger(L, 14, 255);
    int b = (int)luaL_optinteger(L, 15, 255);
    int a = (int)luaL_optinteger(L, 16, 255);
    leo_Color tint = {(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a};
    leo_DrawTexturePro(tex->tex, src, dest, origin, rotation, tint);
    return 0;
}

// -----------------------------------------------------------------------------
// Logical resolution
// -----------------------------------------------------------------------------
static int l_leo_set_logical_resolution(lua_State *L)
{
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);
    int presentation = (int)luaL_checkinteger(L, 3);
    int scale = (int)luaL_checkinteger(L, 4);
    lua_pushboolean(L, leo_SetLogicalResolution(width, height,
                                                (leo_LogicalPresentation)presentation,
                                                (leo_ScaleMode)scale));
    return 1;
}

int leo_LuaOpenBindings(void *vL)
{
    lua_State *L = (lua_State *)vL;

    register_texture_mt(L);
    register_render_texture_mt(L);

    static const luaL_Reg leo_funcs[] = {
        {"leo_init_window", l_leo_init_window},
        {"leo_close_window", l_leo_close_window},
        {"leo_get_window", l_leo_get_window},
        {"leo_get_renderer", l_leo_get_renderer},
        {"leo_set_fullscreen", l_leo_set_fullscreen},
        {"leo_set_window_mode", l_leo_set_window_mode},
        {"leo_get_window_mode", l_leo_get_window_mode},
        {"leo_window_should_close", l_leo_window_should_close},
        {"leo_begin_drawing", l_leo_begin_drawing},
        {"leo_end_drawing", l_leo_end_drawing},
        {"leo_clear_background", l_leo_clear_background},
        {"leo_set_target_fps", l_leo_set_target_fps},
        {"leo_get_frame_time", l_leo_get_frame_time},
        {"leo_get_time", l_leo_get_time},
        {"leo_get_fps", l_leo_get_fps},
        {"leo_get_screen_width", l_leo_get_screen_width},
        {"leo_get_screen_height", l_leo_get_screen_height},
        {"leo_begin_mode2d", l_leo_begin_mode2d},
        {"leo_end_mode2d", l_leo_end_mode2d},
        {"leo_is_camera_active", l_leo_is_camera_active},
        {"leo_get_world_to_screen2d", l_leo_get_world_to_screen2d},
        {"leo_get_screen_to_world2d", l_leo_get_screen_to_world2d},
        {"leo_get_current_camera2d", l_leo_get_current_camera2d},
        {"leo_load_render_texture", l_leo_load_render_texture},
        {"leo_unload_render_texture", l_leo_unload_render_texture},
        {"leo_begin_texture_mode", l_leo_begin_texture_mode},
        {"leo_end_texture_mode", l_leo_end_texture_mode},
        {"leo_render_texture_get_texture", l_leo_render_texture_get_texture},
        {"leo_render_texture_get_size", l_leo_render_texture_get_size},
        {"leo_image_load", l_image_load},
        {"leo_image_load_from_memory", l_image_load_from_memory},
        {"leo_image_load_from_texture", l_image_load_from_texture},
        {"leo_image_load_from_pixels", l_image_load_from_pixels},
        {"leo_image_is_ready", l_image_is_ready},
        {"leo_image_unload", l_image_unload},
        {"leo_image_bytes_per_pixel", l_image_bytes_per_pixel},
        {"leo_texture_get_size", l_leo_texture_get_size},
        {"leo_draw_texture_rec", l_leo_draw_texture_rec},
        {"leo_draw_texture_pro", l_leo_draw_texture_pro},
        {"leo_set_logical_resolution", l_leo_set_logical_resolution},
        {NULL, NULL}
    };

    for (const luaL_Reg *reg = leo_funcs; reg->name; ++reg)
    {
        lua_pushcfunction(L, reg->func);
        lua_setglobal(L, reg->name);
    }

    // Constants for texture formats
    lua_pushinteger(L, LEO_TEXFORMAT_UNDEFINED); lua_setglobal(L, "leo_TEXFORMAT_UNDEFINED");
    lua_pushinteger(L, LEO_TEXFORMAT_R8G8B8A8); lua_setglobal(L, "leo_TEXFORMAT_R8G8B8A8");
    lua_pushinteger(L, LEO_TEXFORMAT_R8G8B8); lua_setglobal(L, "leo_TEXFORMAT_R8G8B8");
    lua_pushinteger(L, LEO_TEXFORMAT_GRAY8); lua_setglobal(L, "leo_TEXFORMAT_GRAY8");
    lua_pushinteger(L, LEO_TEXFORMAT_GRAY8_ALPHA); lua_setglobal(L, "leo_TEXFORMAT_GRAY8_ALPHA");

    // Window mode constants
    lua_pushinteger(L, LEO_WINDOW_MODE_WINDOWED); lua_setglobal(L, "leo_WINDOW_MODE_WINDOWED");
    lua_pushinteger(L, LEO_WINDOW_MODE_BORDERLESS_FULLSCREEN); lua_setglobal(L, "leo_WINDOW_MODE_BORDERLESS_FULLSCREEN");
    lua_pushinteger(L, LEO_WINDOW_MODE_FULLSCREEN_EXCLUSIVE); lua_setglobal(L, "leo_WINDOW_MODE_FULLSCREEN_EXCLUSIVE");

    // Logical presentation constants
    lua_pushinteger(L, LEO_LOGICAL_PRESENTATION_DISABLED); lua_setglobal(L, "leo_LOGICAL_PRESENTATION_DISABLED");
    lua_pushinteger(L, LEO_LOGICAL_PRESENTATION_STRETCH); lua_setglobal(L, "leo_LOGICAL_PRESENTATION_STRETCH");
    lua_pushinteger(L, LEO_LOGICAL_PRESENTATION_LETTERBOX); lua_setglobal(L, "leo_LOGICAL_PRESENTATION_LETTERBOX");
    lua_pushinteger(L, LEO_LOGICAL_PRESENTATION_OVERSCAN); lua_setglobal(L, "leo_LOGICAL_PRESENTATION_OVERSCAN");

    // Scale mode constants
    lua_pushinteger(L, LEO_SCALE_NEAREST); lua_setglobal(L, "leo_SCALE_NEAREST");
    lua_pushinteger(L, LEO_SCALE_LINEAR); lua_setglobal(L, "leo_SCALE_LINEAR");
    lua_pushinteger(L, LEO_SCALE_PIXELART); lua_setglobal(L, "leo_SCALE_PIXELART");

    return 0;
}
