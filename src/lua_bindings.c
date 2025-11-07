#include "leo/lua_bindings.h"

#include "leo/engine.h"
#include "leo/font.h"
#include "leo/gamepad.h"
#include "leo/image.h"
#include "leo/json.h"
#include "leo/keyboard.h"
#include "leo/keys.h"

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

typedef struct
{
    leo_Font font;
    int owns; // whether GC should unload
} leo__LuaFont;

typedef struct
{
    leo_JsonDoc *doc;
    int owns;
} leo__LuaJsonDoc;

typedef struct
{
    leo_JsonNode node;
} leo__LuaJsonNode;

static const char *LEO_TEX_META = "leo_texture";
static const char *LEO_RT_META = "leo_render_texture";
static const char *LEO_FONT_META = "leo_font";
static const char *LEO_JSON_DOC_META = "leo_json_doc";
static const char *LEO_JSON_NODE_META = "leo_json_node";

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

static leo__LuaFont *push_font(lua_State *L, leo_Font font, int owns)
{
    leo__LuaFont *ud = (leo__LuaFont *)lua_newuserdata(L, sizeof(leo__LuaFont));
    ud->font = font;
    ud->owns = owns;
    luaL_getmetatable(L, LEO_FONT_META);
    lua_setmetatable(L, -2);
    return ud;
}

static leo__LuaFont *check_font_ud(lua_State *L, int idx)
{
    return (leo__LuaFont *)luaL_checkudata(L, idx, LEO_FONT_META);
}

static leo__LuaJsonDoc *push_json_doc(lua_State *L, leo_JsonDoc *doc, int owns)
{
    leo__LuaJsonDoc *ud = (leo__LuaJsonDoc *)lua_newuserdata(L, sizeof(leo__LuaJsonDoc));
    ud->doc = doc;
    ud->owns = owns;
    luaL_getmetatable(L, LEO_JSON_DOC_META);
    lua_setmetatable(L, -2);
    return ud;
}

static leo__LuaJsonDoc *check_json_doc_ud(lua_State *L, int idx)
{
    return (leo__LuaJsonDoc *)luaL_checkudata(L, idx, LEO_JSON_DOC_META);
}

static leo__LuaJsonNode *push_json_node(lua_State *L, leo_JsonNode node)
{
    leo__LuaJsonNode *ud = (leo__LuaJsonNode *)lua_newuserdata(L, sizeof(leo__LuaJsonNode));
    ud->node = node;
    luaL_getmetatable(L, LEO_JSON_NODE_META);
    lua_setmetatable(L, -2);
    return ud;
}

static leo__LuaJsonNode *check_json_node_ud(lua_State *L, int idx)
{
    return (leo__LuaJsonNode *)luaL_checkudata(L, idx, LEO_JSON_NODE_META);
}

static leo_JsonDoc *lua_check_json_doc(lua_State *L, int idx)
{
    leo__LuaJsonDoc *ud = check_json_doc_ud(L, idx);
    luaL_argcheck(L, ud->doc != NULL, idx, "json document has been freed");
    return ud->doc;
}

static leo_JsonNode lua_check_json_node(lua_State *L, int idx)
{
    leo__LuaJsonNode *ud = check_json_node_ud(L, idx);
    return ud->node;
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

static int l_font_gc(lua_State *L)
{
    leo__LuaFont *ud = (leo__LuaFont *)luaL_testudata(L, 1, LEO_FONT_META);
    if (ud && ud->owns)
    {
        leo_UnloadFont(&ud->font);
        ud->owns = 0;
        ud->font._atlas = NULL;
        ud->font._glyphs = NULL;
        ud->font.baseSize = 0;
        ud->font.lineHeight = 0;
        ud->font.glyphCount = 0;
        ud->font._atlasW = 0;
        ud->font._atlasH = 0;
    }
    return 0;
}

static int l_json_doc_gc(lua_State *L)
{
    leo__LuaJsonDoc *ud = (leo__LuaJsonDoc *)luaL_testudata(L, 1, LEO_JSON_DOC_META);
    if (ud && ud->owns && ud->doc)
    {
        leo_json_free(ud->doc);
        ud->doc = NULL;
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

static void register_font_mt(lua_State *L)
{
    if (luaL_newmetatable(L, LEO_FONT_META))
    {
        lua_pushcfunction(L, l_font_gc);
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

static void register_json_doc_mt(lua_State *L)
{
    if (luaL_newmetatable(L, LEO_JSON_DOC_META))
    {
        lua_pushcfunction(L, l_json_doc_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_pop(L, 1);
}

static void register_json_node_mt(lua_State *L)
{
    if (luaL_newmetatable(L, LEO_JSON_NODE_META))
    {
        lua_pushstring(L, "leo_json_node");
        lua_setfield(L, -2, "__name");
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
// Keyboard input
// -----------------------------------------------------------------------------
static int l_leo_is_key_pressed(lua_State *L)
{
    int key = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, leo_IsKeyPressed(key));
    return 1;
}

static int l_leo_is_key_pressed_repeat(lua_State *L)
{
    int key = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, leo_IsKeyPressedRepeat(key));
    return 1;
}

static int l_leo_is_key_down(lua_State *L)
{
    int key = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, leo_IsKeyDown(key));
    return 1;
}

static int l_leo_is_key_released(lua_State *L)
{
    int key = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, leo_IsKeyReleased(key));
    return 1;
}

static int l_leo_is_key_up(lua_State *L)
{
    int key = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, leo_IsKeyUp(key));
    return 1;
}

static int l_leo_get_key_pressed(lua_State *L)
{
    lua_pushinteger(L, leo_GetKeyPressed());
    return 1;
}

static int l_leo_get_char_pressed(lua_State *L)
{
    lua_pushinteger(L, leo_GetCharPressed());
    return 1;
}

static int l_leo_set_exit_key(lua_State *L)
{
    int key = (int)luaL_checkinteger(L, 1);
    leo_SetExitKey(key);
    return 0;
}

static int l_leo_update_keyboard(lua_State *L)
{
    (void)L;
    leo_UpdateKeyboard();
    return 0;
}

static int l_leo_is_exit_key_pressed(lua_State *L)
{
    lua_pushboolean(L, leo_IsExitKeyPressed());
    return 1;
}

static int l_leo_cleanup_keyboard(lua_State *L)
{
    (void)L;
    leo_CleanupKeyboard();
    return 0;
}

// -----------------------------------------------------------------------------
// Gamepad input
// -----------------------------------------------------------------------------
static int l_leo_init_gamepads(lua_State *L)
{
    (void)L;
    leo_InitGamepads();
    return 0;
}

static int l_leo_update_gamepads(lua_State *L)
{
    (void)L;
    leo_UpdateGamepads();
    return 0;
}

static int l_leo_handle_gamepad_event(lua_State *L)
{
    luaL_argcheck(L, lua_islightuserdata(L, 1), 1, "expected lightuserdata event");
    void *evt = lua_touserdata(L, 1);
    luaL_argcheck(L, evt != NULL, 1, "event pointer must not be NULL");
    leo_HandleGamepadEvent(evt);
    return 0;
}

static int l_leo_shutdown_gamepads(lua_State *L)
{
    (void)L;
    leo_ShutdownGamepads();
    return 0;
}

static int l_leo_is_gamepad_available(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, leo_IsGamepadAvailable(gamepad));
    return 1;
}

static int l_leo_get_gamepad_name(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    const char *name = leo_GetGamepadName(gamepad);
    if (name)
        lua_pushstring(L, name);
    else
        lua_pushnil(L);
    return 1;
}

static int l_leo_is_gamepad_button_pressed(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int button = (int)luaL_checkinteger(L, 2);
    lua_pushboolean(L, leo_IsGamepadButtonPressed(gamepad, button));
    return 1;
}

static int l_leo_is_gamepad_button_down(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int button = (int)luaL_checkinteger(L, 2);
    lua_pushboolean(L, leo_IsGamepadButtonDown(gamepad, button));
    return 1;
}

static int l_leo_is_gamepad_button_released(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int button = (int)luaL_checkinteger(L, 2);
    lua_pushboolean(L, leo_IsGamepadButtonReleased(gamepad, button));
    return 1;
}

static int l_leo_is_gamepad_button_up(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int button = (int)luaL_checkinteger(L, 2);
    lua_pushboolean(L, leo_IsGamepadButtonUp(gamepad, button));
    return 1;
}

static int l_leo_get_gamepad_button_pressed(lua_State *L)
{
    lua_pushinteger(L, leo_GetGamepadButtonPressed());
    return 1;
}

static int l_leo_get_gamepad_axis_count(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    lua_pushinteger(L, leo_GetGamepadAxisCount(gamepad));
    return 1;
}

static int l_leo_get_gamepad_axis_movement(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int axis = (int)luaL_checkinteger(L, 2);
    lua_pushnumber(L, leo_GetGamepadAxisMovement(gamepad, axis));
    return 1;
}

static int l_leo_set_gamepad_vibration(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    float left = (float)luaL_checknumber(L, 2);
    float right = (float)luaL_checknumber(L, 3);
    float duration = (float)luaL_checknumber(L, 4);
    lua_pushboolean(L, leo_SetGamepadVibration(gamepad, left, right, duration));
    return 1;
}

static int l_leo_set_gamepad_axis_deadzone(lua_State *L)
{
    float deadzone = (float)luaL_checknumber(L, 1);
    leo_SetGamepadAxisDeadzone(deadzone);
    return 0;
}

static int l_leo_set_gamepad_stick_threshold(lua_State *L)
{
    float press_threshold = (float)luaL_checknumber(L, 1);
    float release_threshold = (float)luaL_checknumber(L, 2);
    leo_SetGamepadStickThreshold(press_threshold, release_threshold);
    return 0;
}

static int l_leo_get_gamepad_stick(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int stick = (int)luaL_checkinteger(L, 2);
    leo_Vector2 v = leo_GetGamepadStick(gamepad, stick);
    lua_push_vector2(L, v);
    return 2;
}

static int l_leo_is_gamepad_stick_pressed(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int stick = (int)luaL_checkinteger(L, 2);
    int dir = (int)luaL_checkinteger(L, 3);
    lua_pushboolean(L, leo_IsGamepadStickPressed(gamepad, stick, dir));
    return 1;
}

static int l_leo_is_gamepad_stick_down(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int stick = (int)luaL_checkinteger(L, 2);
    int dir = (int)luaL_checkinteger(L, 3);
    lua_pushboolean(L, leo_IsGamepadStickDown(gamepad, stick, dir));
    return 1;
}

static int l_leo_is_gamepad_stick_released(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int stick = (int)luaL_checkinteger(L, 2);
    int dir = (int)luaL_checkinteger(L, 3);
    lua_pushboolean(L, leo_IsGamepadStickReleased(gamepad, stick, dir));
    return 1;
}

static int l_leo_is_gamepad_stick_up(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int stick = (int)luaL_checkinteger(L, 2);
    int dir = (int)luaL_checkinteger(L, 3);
    lua_pushboolean(L, leo_IsGamepadStickUp(gamepad, stick, dir));
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
// Font bindings
// -----------------------------------------------------------------------------
static int l_leo_load_font(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    int pixel_size = (int)luaL_checkinteger(L, 2);
    leo_Font font = leo_LoadFont(path, pixel_size);
    push_font(L, font, 1);
    return 1;
}

static int l_leo_load_font_from_memory(lua_State *L)
{
    const char *file_type = NULL;
    size_t len = 0;
    const unsigned char *data = NULL;
    int pixel_size = 0;
    int top = lua_gettop(L);
    if (top >= 3)
    {
        if (!lua_isnoneornil(L, 1))
            file_type = luaL_checkstring(L, 1);
        data = (const unsigned char *)luaL_checklstring(L, 2, &len);
        pixel_size = (int)luaL_checkinteger(L, 3);
    }
    else
    {
        data = (const unsigned char *)luaL_checklstring(L, 1, &len);
        pixel_size = (int)luaL_checkinteger(L, 2);
    }
    leo_Font font = leo_LoadFontFromMemory(file_type, data, (int)len, pixel_size);
    push_font(L, font, 1);
    return 1;
}

static int l_leo_unload_font(lua_State *L)
{
    leo__LuaFont *ud = check_font_ud(L, 1);
    if (ud->owns)
    {
        leo_UnloadFont(&ud->font);
        ud->owns = 0;
    }
    ud->font._atlas = NULL;
    ud->font._glyphs = NULL;
    ud->font.baseSize = 0;
    ud->font.lineHeight = 0;
    ud->font.glyphCount = 0;
    ud->font._atlasW = 0;
    ud->font._atlasH = 0;
    return 0;
}

static int l_leo_is_font_ready(lua_State *L)
{
    leo__LuaFont *ud = check_font_ud(L, 1);
    lua_pushboolean(L, leo_IsFontReady(ud->font));
    return 1;
}

static int l_leo_set_default_font(lua_State *L)
{
    if (lua_isnoneornil(L, 1))
    {
        leo_Font empty = {0};
        leo_SetDefaultFont(empty);
    }
    else
    {
        leo__LuaFont *ud = check_font_ud(L, 1);
        leo_SetDefaultFont(ud->font);
    }
    return 0;
}

static int l_leo_get_default_font(lua_State *L)
{
    leo_Font font = leo_GetDefaultFont();
    if (!leo_IsFontReady(font))
    {
        lua_pushnil(L);
        return 1;
    }
    push_font(L, font, 0);
    return 1;
}

static inline leo_Color lua_check_color(lua_State *L, int idx)
{
    int r = (int)luaL_checkinteger(L, idx);
    int g = (int)luaL_checkinteger(L, idx + 1);
    int b = (int)luaL_checkinteger(L, idx + 2);
    int a = (int)luaL_optinteger(L, idx + 3, 255);
    leo_Color col = {(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a};
    return col;
}

static int l_leo_draw_fps(lua_State *L)
{
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    leo_DrawFPS(x, y);
    return 0;
}

static int l_leo_draw_text(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    int x = (int)luaL_checkinteger(L, 2);
    int y = (int)luaL_checkinteger(L, 3);
    int font_size = (int)luaL_checkinteger(L, 4);
    leo_Color tint = lua_check_color(L, 5);
    leo_DrawText(text, x, y, font_size, tint);
    return 0;
}

static int l_leo_draw_text_ex(lua_State *L)
{
    leo__LuaFont *ud = check_font_ud(L, 1);
    const char *text = luaL_checkstring(L, 2);
    leo_Vector2 pos;
    pos.x = (float)luaL_checknumber(L, 3);
    pos.y = (float)luaL_checknumber(L, 4);
    float font_size = (float)luaL_checknumber(L, 5);
    float spacing = (float)luaL_checknumber(L, 6);
    leo_Color tint = lua_check_color(L, 7);
    leo_DrawTextEx(ud->font, text, pos, font_size, spacing, tint);
    return 0;
}

static int l_leo_draw_text_pro(lua_State *L)
{
    leo__LuaFont *ud = check_font_ud(L, 1);
    const char *text = luaL_checkstring(L, 2);
    leo_Vector2 pos;
    pos.x = (float)luaL_checknumber(L, 3);
    pos.y = (float)luaL_checknumber(L, 4);
    leo_Vector2 origin;
    origin.x = (float)luaL_checknumber(L, 5);
    origin.y = (float)luaL_checknumber(L, 6);
    float rotation = (float)luaL_checknumber(L, 7);
    float font_size = (float)luaL_checknumber(L, 8);
    float spacing = (float)luaL_checknumber(L, 9);
    leo_Color tint = lua_check_color(L, 10);
    leo_DrawTextPro(ud->font, text, pos, origin, rotation, font_size, spacing, tint);
    return 0;
}

static int l_leo_measure_text_ex(lua_State *L)
{
    leo__LuaFont *ud = check_font_ud(L, 1);
    const char *text = luaL_checkstring(L, 2);
    float font_size = (float)luaL_checknumber(L, 3);
    float spacing = (float)luaL_checknumber(L, 4);
    leo_Vector2 dims = leo_MeasureTextEx(ud->font, text, font_size, spacing);
    lua_pushnumber(L, dims.x);
    lua_pushnumber(L, dims.y);
    return 2;
}

static int l_leo_measure_text(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    int font_size = (int)luaL_checkinteger(L, 2);
    lua_pushinteger(L, leo_MeasureText(text, font_size));
    return 1;
}

static int l_leo_get_font_line_height(lua_State *L)
{
    leo__LuaFont *ud = check_font_ud(L, 1);
    float font_size = (float)luaL_checknumber(L, 2);
    lua_pushinteger(L, leo_GetFontLineHeight(ud->font, font_size));
    return 1;
}

static int l_leo_get_font_base_size(lua_State *L)
{
    leo__LuaFont *ud = check_font_ud(L, 1);
    lua_pushinteger(L, leo_GetFontBaseSize(ud->font));
    return 1;
}

// -----------------------------------------------------------------------------
// JSON bindings
// -----------------------------------------------------------------------------
static int l_leo_json_parse(lua_State *L)
{
    size_t len = 0;
    const char *data = luaL_checklstring(L, 1, &len);
    const char *err = NULL;
    leo_JsonDoc *doc = leo_json_parse(data, len, &err);
    if (!doc)
    {
        lua_pushnil(L);
        lua_pushstring(L, err ? err : "leo_json_parse failed");
        return 2;
    }
    push_json_doc(L, doc, 1);
    return 1;
}

static int l_leo_json_load(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    const char *err = NULL;
    leo_JsonDoc *doc = leo_json_load(path, &err);
    if (!doc)
    {
        lua_pushnil(L);
        lua_pushstring(L, err ? err : "leo_json_load failed");
        return 2;
    }
    push_json_doc(L, doc, 1);
    return 1;
}

static int l_leo_json_free(lua_State *L)
{
    leo__LuaJsonDoc *ud = check_json_doc_ud(L, 1);
    if (ud->doc)
    {
        leo_json_free(ud->doc);
        ud->doc = NULL;
    }
    ud->owns = 0;
    return 0;
}

static int l_leo_json_root(lua_State *L)
{
    leo_JsonDoc *doc = lua_check_json_doc(L, 1);
    push_json_node(L, leo_json_root(doc));
    return 1;
}

static int l_leo_json_is_null(lua_State *L)
{
    lua_pushboolean(L, leo_json_is_null(lua_check_json_node(L, 1)));
    return 1;
}

static int l_leo_json_is_object(lua_State *L)
{
    lua_pushboolean(L, leo_json_is_object(lua_check_json_node(L, 1)));
    return 1;
}

static int l_leo_json_is_array(lua_State *L)
{
    lua_pushboolean(L, leo_json_is_array(lua_check_json_node(L, 1)));
    return 1;
}

static int l_leo_json_is_string(lua_State *L)
{
    lua_pushboolean(L, leo_json_is_string(lua_check_json_node(L, 1)));
    return 1;
}

static int l_leo_json_is_number(lua_State *L)
{
    lua_pushboolean(L, leo_json_is_number(lua_check_json_node(L, 1)));
    return 1;
}

static int l_leo_json_is_bool(lua_State *L)
{
    lua_pushboolean(L, leo_json_is_bool(lua_check_json_node(L, 1)));
    return 1;
}

static int l_leo_json_obj_get(lua_State *L)
{
    leo_JsonNode obj = lua_check_json_node(L, 1);
    const char *key = luaL_checkstring(L, 2);
    push_json_node(L, leo_json_obj_get(obj, key));
    return 1;
}

static int l_leo_json_arr_size(lua_State *L)
{
    size_t size = leo_json_arr_size(lua_check_json_node(L, 1));
    lua_pushinteger(L, (lua_Integer)size);
    return 1;
}

static int l_leo_json_arr_get(lua_State *L)
{
    leo_JsonNode arr = lua_check_json_node(L, 1);
    size_t index = (size_t)luaL_checkinteger(L, 2);
    push_json_node(L, leo_json_arr_get(arr, index));
    return 1;
}

static int l_leo_json_get_string(lua_State *L)
{
    leo_JsonNode obj = lua_check_json_node(L, 1);
    const char *key = luaL_checkstring(L, 2);
    const char *out = NULL;
    bool ok = leo_json_get_string(obj, key, &out) && out;
    lua_pushboolean(L, ok);
    if (ok)
        lua_pushstring(L, out);
    else
        lua_pushnil(L);
    return 2;
}

static int l_leo_json_get_int(lua_State *L)
{
    leo_JsonNode obj = lua_check_json_node(L, 1);
    const char *key = luaL_checkstring(L, 2);
    int out = 0;
    bool ok = leo_json_get_int(obj, key, &out);
    lua_pushboolean(L, ok);
    if (ok)
        lua_pushinteger(L, out);
    else
        lua_pushnil(L);
    return 2;
}

static int l_leo_json_get_double(lua_State *L)
{
    leo_JsonNode obj = lua_check_json_node(L, 1);
    const char *key = luaL_checkstring(L, 2);
    double out = 0.0;
    bool ok = leo_json_get_double(obj, key, &out);
    lua_pushboolean(L, ok);
    if (ok)
        lua_pushnumber(L, out);
    else
        lua_pushnil(L);
    return 2;
}

static int l_leo_json_get_bool(lua_State *L)
{
    leo_JsonNode obj = lua_check_json_node(L, 1);
    const char *key = luaL_checkstring(L, 2);
    bool out = false;
    bool ok = leo_json_get_bool(obj, key, &out);
    lua_pushboolean(L, ok);
    if (ok)
        lua_pushboolean(L, out);
    else
        lua_pushnil(L);
    return 2;
}

static int l_leo_json_as_string(lua_State *L)
{
    const char *s = leo_json_as_string(lua_check_json_node(L, 1));
    if (s)
        lua_pushstring(L, s);
    else
        lua_pushnil(L);
    return 1;
}

static int l_leo_json_as_int(lua_State *L)
{
    lua_pushinteger(L, leo_json_as_int(lua_check_json_node(L, 1)));
    return 1;
}

static int l_leo_json_as_double(lua_State *L)
{
    lua_pushnumber(L, leo_json_as_double(lua_check_json_node(L, 1)));
    return 1;
}

static int l_leo_json_as_bool(lua_State *L)
{
    lua_pushboolean(L, leo_json_as_bool(lua_check_json_node(L, 1)));
    return 1;
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
    register_font_mt(L);
    register_json_doc_mt(L);
    register_json_node_mt(L);

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
        {"leo_is_key_pressed", l_leo_is_key_pressed},
        {"leo_is_key_pressed_repeat", l_leo_is_key_pressed_repeat},
        {"leo_is_key_down", l_leo_is_key_down},
        {"leo_is_key_released", l_leo_is_key_released},
        {"leo_is_key_up", l_leo_is_key_up},
        {"leo_get_key_pressed", l_leo_get_key_pressed},
        {"leo_get_char_pressed", l_leo_get_char_pressed},
        {"leo_set_exit_key", l_leo_set_exit_key},
        {"leo_update_keyboard", l_leo_update_keyboard},
        {"leo_is_exit_key_pressed", l_leo_is_exit_key_pressed},
        {"leo_cleanup_keyboard", l_leo_cleanup_keyboard},
        {"leo_init_gamepads", l_leo_init_gamepads},
        {"leo_update_gamepads", l_leo_update_gamepads},
        {"leo_handle_gamepad_event", l_leo_handle_gamepad_event},
        {"leo_shutdown_gamepads", l_leo_shutdown_gamepads},
        {"leo_is_gamepad_available", l_leo_is_gamepad_available},
        {"leo_get_gamepad_name", l_leo_get_gamepad_name},
        {"leo_is_gamepad_button_pressed", l_leo_is_gamepad_button_pressed},
        {"leo_is_gamepad_button_down", l_leo_is_gamepad_button_down},
        {"leo_is_gamepad_button_released", l_leo_is_gamepad_button_released},
        {"leo_is_gamepad_button_up", l_leo_is_gamepad_button_up},
        {"leo_get_gamepad_button_pressed", l_leo_get_gamepad_button_pressed},
        {"leo_get_gamepad_axis_count", l_leo_get_gamepad_axis_count},
        {"leo_get_gamepad_axis_movement", l_leo_get_gamepad_axis_movement},
        {"leo_set_gamepad_vibration", l_leo_set_gamepad_vibration},
        {"leo_set_gamepad_axis_deadzone", l_leo_set_gamepad_axis_deadzone},
        {"leo_set_gamepad_stick_threshold", l_leo_set_gamepad_stick_threshold},
        {"leo_get_gamepad_stick", l_leo_get_gamepad_stick},
        {"leo_is_gamepad_stick_pressed", l_leo_is_gamepad_stick_pressed},
        {"leo_is_gamepad_stick_down", l_leo_is_gamepad_stick_down},
        {"leo_is_gamepad_stick_released", l_leo_is_gamepad_stick_released},
        {"leo_is_gamepad_stick_up", l_leo_is_gamepad_stick_up},
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
        {"leo_load_font", l_leo_load_font},
        {"leo_load_font_from_memory", l_leo_load_font_from_memory},
        {"leo_unload_font", l_leo_unload_font},
        {"leo_is_font_ready", l_leo_is_font_ready},
        {"leo_set_default_font", l_leo_set_default_font},
        {"leo_get_default_font", l_leo_get_default_font},
        {"leo_draw_fps", l_leo_draw_fps},
        {"leo_draw_text", l_leo_draw_text},
        {"leo_draw_text_ex", l_leo_draw_text_ex},
        {"leo_draw_text_pro", l_leo_draw_text_pro},
        {"leo_measure_text_ex", l_leo_measure_text_ex},
        {"leo_measure_text", l_leo_measure_text},
        {"leo_get_font_line_height", l_leo_get_font_line_height},
        {"leo_get_font_base_size", l_leo_get_font_base_size},
        {"leo_json_parse", l_leo_json_parse},
        {"leo_json_load", l_leo_json_load},
        {"leo_json_free", l_leo_json_free},
        {"leo_json_root", l_leo_json_root},
        {"leo_json_is_null", l_leo_json_is_null},
        {"leo_json_is_object", l_leo_json_is_object},
        {"leo_json_is_array", l_leo_json_is_array},
        {"leo_json_is_string", l_leo_json_is_string},
        {"leo_json_is_number", l_leo_json_is_number},
        {"leo_json_is_bool", l_leo_json_is_bool},
        {"leo_json_obj_get", l_leo_json_obj_get},
        {"leo_json_arr_size", l_leo_json_arr_size},
        {"leo_json_arr_get", l_leo_json_arr_get},
        {"leo_json_get_string", l_leo_json_get_string},
        {"leo_json_get_int", l_leo_json_get_int},
        {"leo_json_get_double", l_leo_json_get_double},
        {"leo_json_get_bool", l_leo_json_get_bool},
        {"leo_json_as_string", l_leo_json_as_string},
        {"leo_json_as_int", l_leo_json_as_int},
        {"leo_json_as_double", l_leo_json_as_double},
        {"leo_json_as_bool", l_leo_json_as_bool},
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

#define LEO_SET_KEY_CONST(name)            \
    do                                     \
    {                                      \
        lua_pushinteger(L, name);          \
        lua_setglobal(L, "leo_" #name);    \
    } while (0)

    // Keyboard key constants
    LEO_SET_KEY_CONST(KEY_UNKNOWN);
    LEO_SET_KEY_CONST(KEY_A);
    LEO_SET_KEY_CONST(KEY_B);
    LEO_SET_KEY_CONST(KEY_C);
    LEO_SET_KEY_CONST(KEY_D);
    LEO_SET_KEY_CONST(KEY_E);
    LEO_SET_KEY_CONST(KEY_F);
    LEO_SET_KEY_CONST(KEY_G);
    LEO_SET_KEY_CONST(KEY_H);
    LEO_SET_KEY_CONST(KEY_I);
    LEO_SET_KEY_CONST(KEY_J);
    LEO_SET_KEY_CONST(KEY_K);
    LEO_SET_KEY_CONST(KEY_L);
    LEO_SET_KEY_CONST(KEY_M);
    LEO_SET_KEY_CONST(KEY_N);
    LEO_SET_KEY_CONST(KEY_O);
    LEO_SET_KEY_CONST(KEY_P);
    LEO_SET_KEY_CONST(KEY_Q);
    LEO_SET_KEY_CONST(KEY_R);
    LEO_SET_KEY_CONST(KEY_S);
    LEO_SET_KEY_CONST(KEY_T);
    LEO_SET_KEY_CONST(KEY_U);
    LEO_SET_KEY_CONST(KEY_V);
    LEO_SET_KEY_CONST(KEY_W);
    LEO_SET_KEY_CONST(KEY_X);
    LEO_SET_KEY_CONST(KEY_Y);
    LEO_SET_KEY_CONST(KEY_Z);
    LEO_SET_KEY_CONST(KEY_0);
    LEO_SET_KEY_CONST(KEY_1);
    LEO_SET_KEY_CONST(KEY_2);
    LEO_SET_KEY_CONST(KEY_3);
    LEO_SET_KEY_CONST(KEY_4);
    LEO_SET_KEY_CONST(KEY_5);
    LEO_SET_KEY_CONST(KEY_6);
    LEO_SET_KEY_CONST(KEY_7);
    LEO_SET_KEY_CONST(KEY_8);
    LEO_SET_KEY_CONST(KEY_9);
    LEO_SET_KEY_CONST(KEY_RETURN);
    LEO_SET_KEY_CONST(KEY_ESCAPE);
    LEO_SET_KEY_CONST(KEY_BACKSPACE);
    LEO_SET_KEY_CONST(KEY_TAB);
    LEO_SET_KEY_CONST(KEY_SPACE);
    LEO_SET_KEY_CONST(KEY_LEFT);
    LEO_SET_KEY_CONST(KEY_RIGHT);
    LEO_SET_KEY_CONST(KEY_UP);
    LEO_SET_KEY_CONST(KEY_DOWN);
    LEO_SET_KEY_CONST(KEY_LCTRL);
    LEO_SET_KEY_CONST(KEY_LSHIFT);
    LEO_SET_KEY_CONST(KEY_LALT);
    LEO_SET_KEY_CONST(KEY_RCTRL);
    LEO_SET_KEY_CONST(KEY_RSHIFT);
    LEO_SET_KEY_CONST(KEY_RALT);
    LEO_SET_KEY_CONST(KEY_F1);
    LEO_SET_KEY_CONST(KEY_F2);
    LEO_SET_KEY_CONST(KEY_F3);
    LEO_SET_KEY_CONST(KEY_F4);
    LEO_SET_KEY_CONST(KEY_F5);
    LEO_SET_KEY_CONST(KEY_F6);
    LEO_SET_KEY_CONST(KEY_F7);
    LEO_SET_KEY_CONST(KEY_F8);
    LEO_SET_KEY_CONST(KEY_F9);
    LEO_SET_KEY_CONST(KEY_F10);
    LEO_SET_KEY_CONST(KEY_F11);
    LEO_SET_KEY_CONST(KEY_F12);
    LEO_SET_KEY_CONST(KEY_MINUS);
    LEO_SET_KEY_CONST(KEY_EQUALS);
    LEO_SET_KEY_CONST(KEY_LEFTBRACKET);
    LEO_SET_KEY_CONST(KEY_RIGHTBRACKET);
    LEO_SET_KEY_CONST(KEY_BACKSLASH);
    LEO_SET_KEY_CONST(KEY_SEMICOLON);
    LEO_SET_KEY_CONST(KEY_APOSTROPHE);
    LEO_SET_KEY_CONST(KEY_GRAVE);
    LEO_SET_KEY_CONST(KEY_COMMA);
    LEO_SET_KEY_CONST(KEY_PERIOD);
    LEO_SET_KEY_CONST(KEY_SLASH);
    LEO_SET_KEY_CONST(KEY_INSERT);
    LEO_SET_KEY_CONST(KEY_DELETE);
    LEO_SET_KEY_CONST(KEY_HOME);
    LEO_SET_KEY_CONST(KEY_END);
    LEO_SET_KEY_CONST(KEY_PAGEUP);
    LEO_SET_KEY_CONST(KEY_PAGEDOWN);
    LEO_SET_KEY_CONST(KEY_F13);
    LEO_SET_KEY_CONST(KEY_F14);
    LEO_SET_KEY_CONST(KEY_F15);
    LEO_SET_KEY_CONST(KEY_F16);
    LEO_SET_KEY_CONST(KEY_F17);
    LEO_SET_KEY_CONST(KEY_F18);
    LEO_SET_KEY_CONST(KEY_F19);
    LEO_SET_KEY_CONST(KEY_F20);
    LEO_SET_KEY_CONST(KEY_F21);
    LEO_SET_KEY_CONST(KEY_F22);
    LEO_SET_KEY_CONST(KEY_F23);
    LEO_SET_KEY_CONST(KEY_F24);
    LEO_SET_KEY_CONST(KEY_KP_0);
    LEO_SET_KEY_CONST(KEY_KP_1);
    LEO_SET_KEY_CONST(KEY_KP_2);
    LEO_SET_KEY_CONST(KEY_KP_3);
    LEO_SET_KEY_CONST(KEY_KP_4);
    LEO_SET_KEY_CONST(KEY_KP_5);
    LEO_SET_KEY_CONST(KEY_KP_6);
    LEO_SET_KEY_CONST(KEY_KP_7);
    LEO_SET_KEY_CONST(KEY_KP_8);
    LEO_SET_KEY_CONST(KEY_KP_9);
    LEO_SET_KEY_CONST(KEY_KP_PLUS);
    LEO_SET_KEY_CONST(KEY_KP_MINUS);
    LEO_SET_KEY_CONST(KEY_KP_MULTIPLY);
    LEO_SET_KEY_CONST(KEY_KP_DIVIDE);
    LEO_SET_KEY_CONST(KEY_KP_ENTER);
    LEO_SET_KEY_CONST(KEY_KP_PERIOD);
    LEO_SET_KEY_CONST(KEY_KP_EQUALS);
    LEO_SET_KEY_CONST(KEY_PRINTSCREEN);
    LEO_SET_KEY_CONST(KEY_SCROLLLOCK);
    LEO_SET_KEY_CONST(KEY_PAUSE);
    LEO_SET_KEY_CONST(KEY_MENU);
    LEO_SET_KEY_CONST(KEY_VOLUMEUP);
    LEO_SET_KEY_CONST(KEY_VOLUMEDOWN);
    LEO_SET_KEY_CONST(KEY_MUTE);
    LEO_SET_KEY_CONST(KEY_AUDIONEXT);
    LEO_SET_KEY_CONST(KEY_AUDIOPREV);
    LEO_SET_KEY_CONST(KEY_AUDIOSTOP);
    LEO_SET_KEY_CONST(KEY_AUDIOPLAY);
    LEO_SET_KEY_CONST(KEY_INTERNATIONAL1);
    LEO_SET_KEY_CONST(KEY_INTERNATIONAL2);
    LEO_SET_KEY_CONST(KEY_INTERNATIONAL3);
    LEO_SET_KEY_CONST(KEY_INTERNATIONAL4);
    LEO_SET_KEY_CONST(KEY_INTERNATIONAL5);
    LEO_SET_KEY_CONST(KEY_INTERNATIONAL6);
    LEO_SET_KEY_CONST(KEY_INTERNATIONAL7);
    LEO_SET_KEY_CONST(KEY_INTERNATIONAL8);
    LEO_SET_KEY_CONST(KEY_INTERNATIONAL9);
    LEO_SET_KEY_CONST(KEY_LANG1);
    LEO_SET_KEY_CONST(KEY_LANG2);
    LEO_SET_KEY_CONST(KEY_LANG3);
    LEO_SET_KEY_CONST(KEY_LANG4);
    LEO_SET_KEY_CONST(KEY_LANG5);
    LEO_SET_KEY_CONST(KEY_LANG6);
    LEO_SET_KEY_CONST(KEY_LANG7);
    LEO_SET_KEY_CONST(KEY_LANG8);
    LEO_SET_KEY_CONST(KEY_LANG9);
    LEO_SET_KEY_CONST(KEY_CAPSLOCK);
    LEO_SET_KEY_CONST(KEY_NUMLOCKCLEAR);
    LEO_SET_KEY_CONST(KEY_SYSREQ);
    LEO_SET_KEY_CONST(KEY_APPLICATION);
    LEO_SET_KEY_CONST(KEY_POWER);
    LEO_SET_KEY_CONST(KEY_SLEEP);
    LEO_SET_KEY_CONST(KEY_AC_SEARCH);
    LEO_SET_KEY_CONST(KEY_AC_HOME);
    LEO_SET_KEY_CONST(KEY_AC_BACK);
    LEO_SET_KEY_CONST(KEY_AC_FORWARD);
    LEO_SET_KEY_CONST(KEY_AC_STOP);

#undef LEO_SET_KEY_CONST

#define LEO_SET_GAMEPAD_CONST(value, label)    \
    do                                         \
    {                                          \
        lua_pushinteger(L, value);             \
        lua_setglobal(L, "leo_" label);        \
    } while (0)

    // Gamepad limits
    LEO_SET_GAMEPAD_CONST(LEO_MAX_GAMEPADS, "MAX_GAMEPADS");

    // Gamepad button constants
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_UNKNOWN, "GAMEPAD_BUTTON_UNKNOWN");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_FACE_DOWN, "GAMEPAD_BUTTON_FACE_DOWN");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_FACE_RIGHT, "GAMEPAD_BUTTON_FACE_RIGHT");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_FACE_LEFT, "GAMEPAD_BUTTON_FACE_LEFT");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_FACE_UP, "GAMEPAD_BUTTON_FACE_UP");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_LEFT_BUMPER, "GAMEPAD_BUTTON_LEFT_BUMPER");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_RIGHT_BUMPER, "GAMEPAD_BUTTON_RIGHT_BUMPER");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_BACK, "GAMEPAD_BUTTON_BACK");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_GUIDE, "GAMEPAD_BUTTON_GUIDE");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_START, "GAMEPAD_BUTTON_START");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_LEFT_STICK, "GAMEPAD_BUTTON_LEFT_STICK");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_RIGHT_STICK, "GAMEPAD_BUTTON_RIGHT_STICK");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_DPAD_UP, "GAMEPAD_BUTTON_DPAD_UP");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_DPAD_RIGHT, "GAMEPAD_BUTTON_DPAD_RIGHT");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_DPAD_DOWN, "GAMEPAD_BUTTON_DPAD_DOWN");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_DPAD_LEFT, "GAMEPAD_BUTTON_DPAD_LEFT");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_COUNT, "GAMEPAD_BUTTON_COUNT");

    // Gamepad axis constants
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_AXIS_LEFT_X, "GAMEPAD_AXIS_LEFT_X");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_AXIS_LEFT_Y, "GAMEPAD_AXIS_LEFT_Y");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_AXIS_RIGHT_X, "GAMEPAD_AXIS_RIGHT_X");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_AXIS_RIGHT_Y, "GAMEPAD_AXIS_RIGHT_Y");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_AXIS_LEFT_TRIGGER, "GAMEPAD_AXIS_LEFT_TRIGGER");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_AXIS_RIGHT_TRIGGER, "GAMEPAD_AXIS_RIGHT_TRIGGER");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_AXIS_COUNT, "GAMEPAD_AXIS_COUNT");

    // Gamepad stick and direction constants
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_STICK_LEFT, "GAMEPAD_STICK_LEFT");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_STICK_RIGHT, "GAMEPAD_STICK_RIGHT");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_DIR_LEFT, "GAMEPAD_DIR_LEFT");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_DIR_RIGHT, "GAMEPAD_DIR_RIGHT");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_DIR_UP, "GAMEPAD_DIR_UP");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_DIR_DOWN, "GAMEPAD_DIR_DOWN");

#undef LEO_SET_GAMEPAD_CONST

    return 0;
}
