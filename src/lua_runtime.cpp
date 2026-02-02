#include "leo/lua_runtime.h"
#include "leo/audio.h"
#include "leo/camera.h"
#include "leo/collision.h"
#include "leo/engine_core.h"
#include "leo/font.h"
#include "leo/gamepad.h"
#include "leo/graphics.h"
#include "leo/keyboard.h"
#include "leo/mouse.h"
#include "leo/texture_loader.h"
#include "leo/tiled_map.h"
#include "leo/vfs.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_stdinc.h>
#include <physfs.h>
#include <lua.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{

constexpr const char *kRuntimeRegistryKey = "leo.runtime";
constexpr const char *kTextureMeta = "leo.texture";
constexpr const char *kFontMeta = "leo.font";
constexpr const char *kSoundMeta = "leo.sound";
constexpr const char *kMusicMeta = "leo.music";
constexpr const char *kKeyboardMeta = "leo.keyboard";
constexpr const char *kMouseMeta = "leo.mouse";
constexpr const char *kGamepadMeta = "leo.gamepad";
constexpr const char *kCameraMeta = "leo.camera";
constexpr const char *kTiledMapMeta = "leo.tiled_map";
constexpr const char *kAnimationMeta = "leo.animation";

struct AnimationFrame
{
    float x;
    float y;
    float w;
    float h;
    float duration;
};

struct LuaTexture
{
    engine::Texture texture;
};

struct LuaFont
{
    engine::Font font;
    int pixel_size;
};

struct LuaSound
{
    engine::Sound sound;
};

struct LuaMusic
{
    engine::Music music;
};

struct LuaKeyboard
{
    engine::KeyboardState state;
};

struct LuaMouse
{
    engine::MouseState state;
};

struct LuaGamepad
{
    engine::GamepadState state;
};

struct LuaCamera
{
    leo::Camera::Camera2D camera;
};

struct LuaTiledMap
{
    engine::TiledMap map;
};

struct LuaAnimation
{
    engine::Texture texture;
    engine::Texture *texture_ptr;
    bool owns_texture;
    int texture_ref;
    std::vector<AnimationFrame> frames;
    size_t frame_index;
    float frame_time;
    bool playing;
    bool looping;
    float speed;
};

engine::LuaRuntime *GetRuntime(lua_State *L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, kRuntimeRegistryKey);
    engine::LuaRuntime *runtime = static_cast<engine::LuaRuntime *>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return runtime;
}

engine::Key ParseKey(const char *name)
{
    if (!name || !*name)
    {
        return engine::Key::Unknown;
    }

    if (SDL_strlen(name) == 1)
    {
        char c = name[0];
        if (c >= 'a' && c <= 'z')
        {
            return static_cast<engine::Key>(static_cast<int>(engine::Key::A) + (c - 'a'));
        }
        if (c >= 'A' && c <= 'Z')
        {
            return static_cast<engine::Key>(static_cast<int>(engine::Key::A) + (c - 'A'));
        }
        if (c >= '0' && c <= '9')
        {
            return static_cast<engine::Key>(static_cast<int>(engine::Key::Num0) + (c - '0'));
        }
    }

    if (SDL_strcasecmp(name, "escape") == 0)
        return engine::Key::Escape;
    if (SDL_strcasecmp(name, "enter") == 0)
        return engine::Key::Enter;
    if (SDL_strcasecmp(name, "space") == 0)
        return engine::Key::Space;
    if (SDL_strcasecmp(name, "tab") == 0)
        return engine::Key::Tab;
    if (SDL_strcasecmp(name, "backspace") == 0)
        return engine::Key::Backspace;
    if (SDL_strcasecmp(name, "delete") == 0)
        return engine::Key::Delete;
    if (SDL_strcasecmp(name, "left") == 0)
        return engine::Key::Left;
    if (SDL_strcasecmp(name, "right") == 0)
        return engine::Key::Right;
    if (SDL_strcasecmp(name, "up") == 0)
        return engine::Key::Up;
    if (SDL_strcasecmp(name, "down") == 0)
        return engine::Key::Down;
    if (SDL_strcasecmp(name, "lshift") == 0)
        return engine::Key::LShift;
    if (SDL_strcasecmp(name, "rshift") == 0)
        return engine::Key::RShift;
    if (SDL_strcasecmp(name, "lctrl") == 0)
        return engine::Key::LCtrl;
    if (SDL_strcasecmp(name, "rctrl") == 0)
        return engine::Key::RCtrl;
    if (SDL_strcasecmp(name, "lalt") == 0)
        return engine::Key::LAlt;
    if (SDL_strcasecmp(name, "ralt") == 0)
        return engine::Key::RAlt;
    if (SDL_strcasecmp(name, "home") == 0)
        return engine::Key::Home;
    if (SDL_strcasecmp(name, "end") == 0)
        return engine::Key::End;
    if (SDL_strcasecmp(name, "pageup") == 0)
        return engine::Key::PageUp;
    if (SDL_strcasecmp(name, "pagedown") == 0)
        return engine::Key::PageDown;
    if (SDL_strcasecmp(name, "f1") == 0)
        return engine::Key::F1;
    if (SDL_strcasecmp(name, "f2") == 0)
        return engine::Key::F2;
    if (SDL_strcasecmp(name, "f3") == 0)
        return engine::Key::F3;
    if (SDL_strcasecmp(name, "f4") == 0)
        return engine::Key::F4;
    if (SDL_strcasecmp(name, "f5") == 0)
        return engine::Key::F5;
    if (SDL_strcasecmp(name, "f6") == 0)
        return engine::Key::F6;
    if (SDL_strcasecmp(name, "f7") == 0)
        return engine::Key::F7;
    if (SDL_strcasecmp(name, "f8") == 0)
        return engine::Key::F8;
    if (SDL_strcasecmp(name, "f9") == 0)
        return engine::Key::F9;
    if (SDL_strcasecmp(name, "f10") == 0)
        return engine::Key::F10;
    if (SDL_strcasecmp(name, "f11") == 0)
        return engine::Key::F11;
    if (SDL_strcasecmp(name, "f12") == 0)
        return engine::Key::F12;

    return engine::Key::Unknown;
}

bool ParseWindowMode(const char *name, engine::WindowMode *out_mode)
{
    if (!name || !*name || !out_mode)
    {
        return false;
    }

    if (SDL_strcasecmp(name, "windowed") == 0 || SDL_strcasecmp(name, "window") == 0)
    {
        *out_mode = engine::WindowMode::Windowed;
        return true;
    }

    if (SDL_strcasecmp(name, "fullscreen") == 0 || SDL_strcasecmp(name, "exclusive") == 0 ||
        SDL_strcasecmp(name, "fullscreenexclusive") == 0)
    {
        *out_mode = engine::WindowMode::Fullscreen;
        return true;
    }

    if (SDL_strcasecmp(name, "borderless") == 0 || SDL_strcasecmp(name, "borderlessfullscreen") == 0 ||
        SDL_strcasecmp(name, "fullscreenborderless") == 0)
    {
        *out_mode = engine::WindowMode::BorderlessFullscreen;
        return true;
    }

    return false;
}

void ResolveWindowSize(const engine::Config *config, int *out_width, int *out_height)
{
    int width = 1280;
    int height = 720;
    if (config)
    {
        if (config->window_width > 0)
        {
            width = config->window_width;
        }
        if (config->window_height > 0)
        {
            height = config->window_height;
        }
    }
    *out_width = width;
    *out_height = height;
}

const SDL_DisplayMode *GetDesktopDisplayMode(SDL_Window *window)
{
    SDL_DisplayID display_id = SDL_GetDisplayForWindow(window);
    if (display_id == 0)
    {
        return nullptr;
    }

    return SDL_GetDesktopDisplayMode(display_id);
}

bool ApplyWindowMode(SDL_Window *window, const engine::Config *config, engine::WindowMode mode)
{
    if (!window)
    {
        SDL_SetError("ApplyWindowMode requires a window");
        return false;
    }

    switch (mode)
    {
    case engine::WindowMode::Windowed: {
        if (!SDL_SetWindowFullscreen(window, false))
        {
            return false;
        }
        SDL_SetWindowFullscreenMode(window, nullptr);
        SDL_SetWindowBordered(window, true);
        SDL_SetWindowResizable(window, true);

        int width = 0;
        int height = 0;
        ResolveWindowSize(config, &width, &height);
        SDL_SetWindowSize(window, width, height);
        break;
    }
    case engine::WindowMode::BorderlessFullscreen: {
        if (!SDL_SetWindowFullscreen(window, false))
        {
            return false;
        }
        if (!SDL_SetWindowFullscreenMode(window, nullptr))
        {
            return false;
        }
        SDL_SetWindowBordered(window, false);
        SDL_SetWindowResizable(window, false);

        const SDL_DisplayMode *desktop_mode = GetDesktopDisplayMode(window);
        if (!desktop_mode)
        {
            return false;
        }
        SDL_SetWindowSize(window, desktop_mode->w, desktop_mode->h);
        SDL_SetWindowPosition(window, 0, 0);
        SDL_RaiseWindow(window);
        break;
    }
    case engine::WindowMode::Fullscreen: {
        const SDL_DisplayMode *desktop_mode = GetDesktopDisplayMode(window);
        if (!desktop_mode)
        {
            return false;
        }
        if (!SDL_SetWindowFullscreenMode(window, desktop_mode))
        {
            return false;
        }
        SDL_SetWindowResizable(window, false);
        if (!SDL_SetWindowFullscreen(window, true))
        {
            return false;
        }
        break;
    }
    }

    return true;
}

engine::MouseButton ParseMouseButton(const char *name)
{
    if (!name || !*name)
    {
        return engine::MouseButton::Unknown;
    }
    if (SDL_strcasecmp(name, "left") == 0)
        return engine::MouseButton::Left;
    if (SDL_strcasecmp(name, "middle") == 0)
        return engine::MouseButton::Middle;
    if (SDL_strcasecmp(name, "right") == 0)
        return engine::MouseButton::Right;
    if (SDL_strcasecmp(name, "x1") == 0)
        return engine::MouseButton::X1;
    if (SDL_strcasecmp(name, "x2") == 0)
        return engine::MouseButton::X2;
    return engine::MouseButton::Unknown;
}

engine::GamepadButton ParseGamepadButton(const char *name)
{
    if (!name || !*name)
    {
        return engine::GamepadButton::Unknown;
    }
    if (SDL_strcasecmp(name, "south") == 0)
        return engine::GamepadButton::South;
    if (SDL_strcasecmp(name, "east") == 0)
        return engine::GamepadButton::East;
    if (SDL_strcasecmp(name, "west") == 0)
        return engine::GamepadButton::West;
    if (SDL_strcasecmp(name, "north") == 0)
        return engine::GamepadButton::North;
    if (SDL_strcasecmp(name, "back") == 0)
        return engine::GamepadButton::Back;
    if (SDL_strcasecmp(name, "guide") == 0)
        return engine::GamepadButton::Guide;
    if (SDL_strcasecmp(name, "start") == 0)
        return engine::GamepadButton::Start;
    if (SDL_strcasecmp(name, "leftstick") == 0)
        return engine::GamepadButton::LeftStick;
    if (SDL_strcasecmp(name, "rightstick") == 0)
        return engine::GamepadButton::RightStick;
    if (SDL_strcasecmp(name, "leftshoulder") == 0)
        return engine::GamepadButton::LeftShoulder;
    if (SDL_strcasecmp(name, "rightshoulder") == 0)
        return engine::GamepadButton::RightShoulder;
    if (SDL_strcasecmp(name, "dpadup") == 0)
        return engine::GamepadButton::DpadUp;
    if (SDL_strcasecmp(name, "dpaddown") == 0)
        return engine::GamepadButton::DpadDown;
    if (SDL_strcasecmp(name, "dpadleft") == 0)
        return engine::GamepadButton::DpadLeft;
    if (SDL_strcasecmp(name, "dpadright") == 0)
        return engine::GamepadButton::DpadRight;
    if (SDL_strcasecmp(name, "misc1") == 0)
        return engine::GamepadButton::Misc1;
    if (SDL_strcasecmp(name, "touchpad") == 0)
        return engine::GamepadButton::Touchpad;
    return engine::GamepadButton::Unknown;
}

engine::GamepadAxis ParseGamepadAxis(const char *name)
{
    if (!name || !*name)
    {
        return engine::GamepadAxis::Count;
    }
    if (SDL_strcasecmp(name, "leftx") == 0)
        return engine::GamepadAxis::LeftX;
    if (SDL_strcasecmp(name, "lefty") == 0)
        return engine::GamepadAxis::LeftY;
    if (SDL_strcasecmp(name, "rightx") == 0)
        return engine::GamepadAxis::RightX;
    if (SDL_strcasecmp(name, "righty") == 0)
        return engine::GamepadAxis::RightY;
    if (SDL_strcasecmp(name, "lefttrigger") == 0)
        return engine::GamepadAxis::LeftTrigger;
    if (SDL_strcasecmp(name, "righttrigger") == 0)
        return engine::GamepadAxis::RightTrigger;
    return engine::GamepadAxis::Count;
}

engine::AxisDirection ParseAxisDirection(const char *name)
{
    if (!name || !*name)
    {
        return engine::AxisDirection::Positive;
    }
    if (SDL_strcasecmp(name, "negative") == 0 || SDL_strcasecmp(name, "neg") == 0)
    {
        return engine::AxisDirection::Negative;
    }
    return engine::AxisDirection::Positive;
}

Uint8 ClampColorComponent(lua_Number value)
{
    int v = static_cast<int>(value);
    if (v < 0)
    {
        return 0;
    }
    if (v > 255)
    {
        return 255;
    }
    return static_cast<Uint8>(v);
}

leo::Graphics::Color ReadColor(lua_State *L, int index)
{
    Uint8 r = ClampColorComponent(luaL_checknumber(L, index));
    Uint8 g = ClampColorComponent(luaL_checknumber(L, index + 1));
    Uint8 b = ClampColorComponent(luaL_checknumber(L, index + 2));
    Uint8 a = ClampColorComponent(luaL_optnumber(L, index + 3, 255));
    return {r, g, b, a};
}

void ReadPointList(lua_State *L, int index, std::vector<SDL_FPoint> *out_points)
{
    luaL_checktype(L, index, LUA_TTABLE);
    size_t len = lua_rawlen(L, index);
    if (len < 6 || (len % 2) != 0)
    {
        luaL_error(L, "Point list must be an even-length table with at least 3 points");
    }

    out_points->clear();
    out_points->reserve(len / 2);

    for (size_t i = 1; i <= len; i += 2)
    {
        lua_rawgeti(L, index, static_cast<int>(i));
        lua_rawgeti(L, index, static_cast<int>(i + 1));
        float x = static_cast<float>(luaL_checknumber(L, -2));
        float y = static_cast<float>(luaL_checknumber(L, -1));
        lua_pop(L, 2);
        out_points->push_back({x, y});
    }
}

SDL_FPoint ReadPointPair(lua_State *L, int index)
{
    float x = static_cast<float>(luaL_checknumber(L, index));
    float y = static_cast<float>(luaL_checknumber(L, index + 1));
    return {x, y};
}

SDL_FRect ReadRect(lua_State *L, int index)
{
    float x = static_cast<float>(luaL_checknumber(L, index));
    float y = static_cast<float>(luaL_checknumber(L, index + 1));
    float w = static_cast<float>(luaL_checknumber(L, index + 2));
    float h = static_cast<float>(luaL_checknumber(L, index + 3));
    return {x, y, w, h};
}


LuaCamera *CheckCamera(lua_State *L, int index)
{
    return static_cast<LuaCamera *>(luaL_checkudata(L, index, kCameraMeta));
}

LuaTiledMap *CheckTiledMap(lua_State *L, int index)
{
    return static_cast<LuaTiledMap *>(luaL_checkudata(L, index, kTiledMapMeta));
}

SDL_FPoint ApplyCameraPoint(const leo::Camera::Camera2D *camera, SDL_FPoint point)
{
    if (!camera)
    {
        return point;
    }
    return leo::Camera::WorldToScreen(*camera, point);
}

float ApplyCameraScale(const leo::Camera::Camera2D *camera, float value)
{
    return camera ? value * camera->zoom : value;
}

float ApplyCameraRotation(const leo::Camera::Camera2D *camera, float angle)
{
    return camera ? angle + camera->rotation : angle;
}

LuaTexture *CheckTexture(lua_State *L, int index)
{
    return static_cast<LuaTexture *>(luaL_checkudata(L, index, kTextureMeta));
}

LuaAnimation *CheckAnimation(lua_State *L, int index)
{
    return static_cast<LuaAnimation *>(luaL_checkudata(L, index, kAnimationMeta));
}

void InitAnimationState(LuaAnimation *ud)
{
    ud->frames.clear();
    ud->frame_index = 0;
    ud->frame_time = 0.0f;
    ud->playing = false;
    ud->looping = true;
    ud->speed = 1.0f;
}

void ApplyAnimationFlags(lua_State *L, LuaAnimation *ud, int looping_index, int playing_index)
{
    if (!lua_isnoneornil(L, looping_index))
    {
        ud->looping = lua_toboolean(L, looping_index);
    }
    if (!lua_isnoneornil(L, playing_index))
    {
        ud->playing = lua_toboolean(L, playing_index);
    }
}

const char *GetTableStringField(lua_State *L, int index, const char *key)
{
    int idx = lua_absindex(L, index);
    lua_getfield(L, idx, key);
    if (lua_isnil(L, -1))
    {
        luaL_error(L, "Animation sheet requires '%s'", key);
        return "";
    }
    const char *value = luaL_checkstring(L, -1);
    lua_pop(L, 1);
    return value;
}

int GetTableIntField(lua_State *L, int index, const char *key)
{
    int idx = lua_absindex(L, index);
    lua_getfield(L, idx, key);
    if (lua_isnil(L, -1))
    {
        luaL_error(L, "Animation sheet requires '%s'", key);
        return 0;
    }
    int value = static_cast<int>(luaL_checkinteger(L, -1));
    lua_pop(L, 1);
    return value;
}

int GetTableIntFieldOpt(lua_State *L, int index, const char *key, int default_value)
{
    int idx = lua_absindex(L, index);
    lua_getfield(L, idx, key);
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        return default_value;
    }
    int value = static_cast<int>(luaL_checkinteger(L, -1));
    lua_pop(L, 1);
    return value;
}

float GetTableFloatField(lua_State *L, int index, const char *key)
{
    int idx = lua_absindex(L, index);
    lua_getfield(L, idx, key);
    if (lua_isnil(L, -1))
    {
        luaL_error(L, "Animation sheet requires '%s'", key);
        return 0.0f;
    }
    float value = static_cast<float>(luaL_checknumber(L, -1));
    lua_pop(L, 1);
    return value;
}

float GetTableFloatFieldOpt(lua_State *L, int index, const char *key, float default_value)
{
    int idx = lua_absindex(L, index);
    lua_getfield(L, idx, key);
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        return default_value;
    }
    float value = static_cast<float>(luaL_checknumber(L, -1));
    lua_pop(L, 1);
    return value;
}

bool GetTableBoolFieldOpt(lua_State *L, int index, const char *key, bool default_value)
{
    int idx = lua_absindex(L, index);
    lua_getfield(L, idx, key);
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        return default_value;
    }
    if (!lua_isboolean(L, -1))
    {
        luaL_error(L, "Animation sheet field '%s' must be boolean", key);
        return default_value;
    }
    bool value = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return value;
}

bool GetTableBoolFieldReq(lua_State *L, int index, const char *key, const char *context)
{
    int idx = lua_absindex(L, index);
    lua_getfield(L, idx, key);
    if (lua_isnil(L, -1))
    {
        luaL_error(L, "%s requires '%s'", context, key);
        return false;
    }
    if (!lua_isboolean(L, -1))
    {
        luaL_error(L, "%s field '%s' must be boolean", context, key);
        return false;
    }
    bool value = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return value;
}

bool TableHasField(lua_State *L, int index, const char *key)
{
    int idx = lua_absindex(L, index);
    lua_getfield(L, idx, key);
    bool has_field = !lua_isnil(L, -1);
    lua_pop(L, 1);
    return has_field;
}

float GetTableNumberFieldReq(lua_State *L, int index, const char *key, const char *context)
{
    int idx = lua_absindex(L, index);
    lua_getfield(L, idx, key);
    if (lua_isnil(L, -1))
    {
        luaL_error(L, "%s requires '%s'", context, key);
        return 0.0f;
    }
    float value = static_cast<float>(luaL_checknumber(L, -1));
    lua_pop(L, 1);
    return value;
}

SDL_FPoint GetTablePointField(lua_State *L, int index, const char *key, const char *context)
{
    int idx = lua_absindex(L, index);
    lua_getfield(L, idx, key);
    if (lua_isnil(L, -1))
    {
        luaL_error(L, "%s requires '%s'", context, key);
        return {0.0f, 0.0f};
    }
    luaL_checktype(L, -1, LUA_TTABLE);
    SDL_FPoint point = {GetTableNumberFieldReq(L, -1, "x", context),
                        GetTableNumberFieldReq(L, -1, "y", context)};
    lua_pop(L, 1);
    return point;
}

SDL_FRect GetTableRectField(lua_State *L, int index, const char *key, const char *context)
{
    int idx = lua_absindex(L, index);
    lua_getfield(L, idx, key);
    if (lua_isnil(L, -1))
    {
        luaL_error(L, "%s requires '%s'", context, key);
        return {0.0f, 0.0f, 0.0f, 0.0f};
    }
    luaL_checktype(L, -1, LUA_TTABLE);
    SDL_FRect rect = {GetTableNumberFieldReq(L, -1, "x", context), GetTableNumberFieldReq(L, -1, "y", context),
                      GetTableNumberFieldReq(L, -1, "w", context), GetTableNumberFieldReq(L, -1, "h", context)};
    lua_pop(L, 1);
    return rect;
}

void ReadPointListField(lua_State *L, int index, const char *key, std::vector<SDL_FPoint> *out_points,
                        const char *context)
{
    int idx = lua_absindex(L, index);
    lua_getfield(L, idx, key);
    if (lua_isnil(L, -1))
    {
        luaL_error(L, "%s requires '%s'", context, key);
        lua_pop(L, 1);
        return;
    }
    ReadPointList(L, lua_gettop(L), out_points);
    lua_pop(L, 1);
}

const char *GetTableStringFieldReq(lua_State *L, int index, const char *key, const char *context)
{
    int idx = lua_absindex(L, index);
    lua_getfield(L, idx, key);
    if (lua_isnil(L, -1))
    {
        luaL_error(L, "%s requires '%s'", context, key);
        return "";
    }
    const char *value = luaL_checkstring(L, -1);
    lua_pop(L, 1);
    return value;
}

leo::Graphics::Color ReadColorTable(lua_State *L, int index, const char *context)
{
    float r = GetTableNumberFieldReq(L, index, "r", context);
    float g = GetTableNumberFieldReq(L, index, "g", context);
    float b = GetTableNumberFieldReq(L, index, "b", context);

    int idx = lua_absindex(L, index);
    lua_getfield(L, idx, "a");
    float a = 255.0f;
    if (!lua_isnil(L, -1))
    {
        a = static_cast<float>(luaL_checknumber(L, -1));
    }
    lua_pop(L, 1);

    return {ClampColorComponent(r), ClampColorComponent(g), ClampColorComponent(b), ClampColorComponent(a)};
}

bool ReadColorTableOptional(lua_State *L, int index, const char *context, SDL_Color *out)
{
    int idx = lua_absindex(L, index);
    lua_getfield(L, idx, "r");
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        return false;
    }
    int r = static_cast<int>(luaL_checkinteger(L, -1));
    lua_pop(L, 1);
    int g = static_cast<int>(GetTableNumberFieldReq(L, index, "g", context));
    int b = static_cast<int>(GetTableNumberFieldReq(L, index, "b", context));
    float a = GetTableFloatFieldOpt(L, index, "a", 255.0f);
    *out = {ClampColorComponent(r), ClampColorComponent(g), ClampColorComponent(b), ClampColorComponent(a)};
    return true;
}

bool ReadColorArgsOptional(lua_State *L, int index, SDL_Color *out)
{
    if (lua_isnoneornil(L, index))
    {
        return false;
    }
    int r = static_cast<int>(luaL_checkinteger(L, index));
    int g = static_cast<int>(luaL_checkinteger(L, index + 1));
    int b = static_cast<int>(luaL_checkinteger(L, index + 2));
    int a = static_cast<int>(luaL_optinteger(L, index + 3, 255));
    *out = {ClampColorComponent(r), ClampColorComponent(g), ClampColorComponent(b), ClampColorComponent(a)};
    return true;
}

LuaFont *CheckFont(lua_State *L, int index)
{
    return static_cast<LuaFont *>(luaL_checkudata(L, index, kFontMeta));
}

LuaSound *CheckSound(lua_State *L, int index)
{
    return static_cast<LuaSound *>(luaL_checkudata(L, index, kSoundMeta));
}

LuaMusic *CheckMusic(lua_State *L, int index)
{
    return static_cast<LuaMusic *>(luaL_checkudata(L, index, kMusicMeta));
}

LuaKeyboard *CheckKeyboard(lua_State *L, int index)
{
    return static_cast<LuaKeyboard *>(luaL_checkudata(L, index, kKeyboardMeta));
}

LuaMouse *CheckMouse(lua_State *L, int index)
{
    return static_cast<LuaMouse *>(luaL_checkudata(L, index, kMouseMeta));
}

LuaGamepad *CheckGamepad(lua_State *L, int index)
{
    return static_cast<LuaGamepad *>(luaL_checkudata(L, index, kGamepadMeta));
}

int LuaGraphicsNewImage(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    const char *path = luaL_checkstring(L, 1);

    try
    {
        engine::TextureLoader loader(runtime->GetVfs(), runtime->GetRenderer());
        LuaTexture *ud = static_cast<LuaTexture *>(lua_newuserdata(L, sizeof(LuaTexture)));
        new (&ud->texture) engine::Texture(loader.Load(path));
        luaL_getmetatable(L, kTextureMeta);
        lua_setmetatable(L, -2);
        return 1;
    }
    catch (const std::exception &e)
    {
        return luaL_error(L, "%s", e.what());
    }
}

int LuaGraphicsDraw(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    LuaTexture *ud = CheckTexture(L, 1);
    double x = luaL_checknumber(L, 2);
    double y = luaL_checknumber(L, 3);
    double angle = luaL_optnumber(L, 4, 0.0);
    double sx = luaL_optnumber(L, 5, 1.0);
    double sy = luaL_optnumber(L, 6, 1.0);
    double ox = luaL_optnumber(L, 7, 0.0);
    double oy = luaL_optnumber(L, 8, 0.0);

    if (!ud->texture.handle)
    {
        return 0;
    }

    const leo::Camera::Camera2D *camera = runtime->GetActiveCamera();
    SDL_FPoint screen = ApplyCameraPoint(camera, {static_cast<float>(x), static_cast<float>(y)});
    float zoom = camera ? camera->zoom : 1.0f;
    double render_angle = ApplyCameraRotation(camera, static_cast<float>(angle));

    SDL_Color color = runtime->GetDrawColor();
    SDL_SetTextureColorMod(ud->texture.handle, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(ud->texture.handle, color.a);

    float render_sx = static_cast<float>(sx) * zoom;
    float render_sy = static_cast<float>(sy) * zoom;
    float w = static_cast<float>(ud->texture.width) * render_sx;
    float h = static_cast<float>(ud->texture.height) * render_sy;
    SDL_FRect dst = {screen.x - static_cast<float>(ox * render_sx), screen.y - static_cast<float>(oy * render_sy), w,
                     h};
    SDL_FPoint center = {static_cast<float>(ox * render_sx), static_cast<float>(oy * render_sy)};
    constexpr double kRadToDeg = 57.29577951308232;
    double degrees = render_angle * kRadToDeg;

    SDL_RenderTextureRotated(runtime->GetRenderer(), ud->texture.handle, nullptr, &dst, degrees, &center,
                             SDL_FLIP_NONE);

    SDL_SetTextureColorMod(ud->texture.handle, 255, 255, 255);
    SDL_SetTextureAlphaMod(ud->texture.handle, 255);
    return 0;
}

int LuaGraphicsDrawEx(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    LuaTexture *ud = CheckTexture(L, 1);
    double src_x = luaL_checknumber(L, 2);
    double src_y = luaL_checknumber(L, 3);
    double src_w = luaL_checknumber(L, 4);
    double src_h = luaL_checknumber(L, 5);
    double x = luaL_checknumber(L, 6);
    double y = luaL_checknumber(L, 7);
    double angle = luaL_optnumber(L, 8, 0.0);
    double sx = luaL_optnumber(L, 9, 1.0);
    double sy = luaL_optnumber(L, 10, 1.0);
    double ox = luaL_optnumber(L, 11, 0.0);
    double oy = luaL_optnumber(L, 12, 0.0);
    bool flip_x = lua_toboolean(L, 13);
    bool flip_y = lua_toboolean(L, 14);

    if (!ud->texture.handle || src_w <= 0.0 || src_h <= 0.0)
    {
        return 0;
    }

    const leo::Camera::Camera2D *camera = runtime->GetActiveCamera();
    SDL_FPoint screen = ApplyCameraPoint(camera, {static_cast<float>(x), static_cast<float>(y)});
    float zoom = camera ? camera->zoom : 1.0f;
    double render_angle = ApplyCameraRotation(camera, static_cast<float>(angle));

    SDL_Color color = runtime->GetDrawColor();
    int color_index = 15;
    if (!lua_isnoneornil(L, color_index))
    {
        auto clamp = [](int value) -> Uint8 {
            if (value < 0)
                return 0;
            if (value > 255)
                return 255;
            return static_cast<Uint8>(value);
        };
        int r = static_cast<int>(luaL_checkinteger(L, color_index));
        int g = static_cast<int>(luaL_checkinteger(L, color_index + 1));
        int b = static_cast<int>(luaL_checkinteger(L, color_index + 2));
        int a = static_cast<int>(luaL_optinteger(L, color_index + 3, 255));
        color = {clamp(r), clamp(g), clamp(b), clamp(a)};
    }

    SDL_SetTextureColorMod(ud->texture.handle, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(ud->texture.handle, color.a);

    float render_sx = static_cast<float>(sx) * zoom;
    float render_sy = static_cast<float>(sy) * zoom;
    float w = static_cast<float>(src_w) * render_sx;
    float h = static_cast<float>(src_h) * render_sy;
    SDL_FRect src = {static_cast<float>(src_x), static_cast<float>(src_y), static_cast<float>(src_w),
                     static_cast<float>(src_h)};
    SDL_FRect dst = {screen.x - static_cast<float>(ox * render_sx), screen.y - static_cast<float>(oy * render_sy), w,
                     h};
    SDL_FPoint center = {static_cast<float>(ox * render_sx), static_cast<float>(oy * render_sy)};
    constexpr double kRadToDeg = 57.29577951308232;
    double degrees = render_angle * kRadToDeg;

    SDL_FlipMode flip = SDL_FLIP_NONE;
    if (flip_x)
    {
        flip = static_cast<SDL_FlipMode>(flip | SDL_FLIP_HORIZONTAL);
    }
    if (flip_y)
    {
        flip = static_cast<SDL_FlipMode>(flip | SDL_FLIP_VERTICAL);
    }

    SDL_RenderTextureRotated(runtime->GetRenderer(), ud->texture.handle, &src, &dst, degrees, &center, flip);

    SDL_SetTextureColorMod(ud->texture.handle, 255, 255, 255);
    SDL_SetTextureAlphaMod(ud->texture.handle, 255);
    return 0;
}

int LuaAnimationNew(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    const char *path = luaL_checkstring(L, 1);

    try
    {
        engine::TextureLoader loader(runtime->GetVfs(), runtime->GetRenderer());
        LuaAnimation *ud = static_cast<LuaAnimation *>(lua_newuserdata(L, sizeof(LuaAnimation)));
        new (ud) LuaAnimation{};
        ud->texture = loader.Load(path);
        ud->texture_ptr = &ud->texture;
        ud->owns_texture = true;
        ud->texture_ref = LUA_NOREF;
        InitAnimationState(ud);
        ApplyAnimationFlags(L, ud, 2, 3);
        luaL_getmetatable(L, kAnimationMeta);
        lua_setmetatable(L, -2);
        return 1;
    }
    catch (const std::exception &e)
    {
        return luaL_error(L, "%s", e.what());
    }
}

int LuaAnimationNewFromTexture(lua_State *L)
{
    LuaTexture *tex = CheckTexture(L, 1);
    LuaAnimation *ud = static_cast<LuaAnimation *>(lua_newuserdata(L, sizeof(LuaAnimation)));
    new (ud) LuaAnimation{};
    ud->texture_ptr = &tex->texture;
    ud->owns_texture = false;
    ud->texture_ref = LUA_NOREF;
    InitAnimationState(ud);
    ApplyAnimationFlags(L, ud, 2, 3);

    lua_pushvalue(L, 1);
    ud->texture_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    luaL_getmetatable(L, kAnimationMeta);
    lua_setmetatable(L, -2);
    return 1;
}

int LuaAnimationNewSheet(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    const char *path = nullptr;
    int frame_w = 0;
    int frame_h = 0;
    int frame_count = 0;
    float frame_time = 0.0f;
    bool looping = true;
    bool playing = false;

    if (lua_istable(L, 1))
    {
        path = GetTableStringField(L, 1, "path");
        frame_w = GetTableIntField(L, 1, "frame_w");
        frame_h = GetTableIntField(L, 1, "frame_h");
        frame_count = GetTableIntField(L, 1, "frame_count");
        frame_time = GetTableFloatField(L, 1, "frame_time");
        looping = GetTableBoolFieldOpt(L, 1, "looping", true);
        playing = GetTableBoolFieldOpt(L, 1, "playing", false);
    }
    else
    {
        path = luaL_checkstring(L, 1);
        frame_w = static_cast<int>(luaL_checkinteger(L, 2));
        frame_h = static_cast<int>(luaL_checkinteger(L, 3));
        frame_count = static_cast<int>(luaL_checkinteger(L, 4));
        frame_time = static_cast<float>(luaL_checknumber(L, 5));
    }

    if (frame_w <= 0 || frame_h <= 0 || frame_count <= 0 || frame_time <= 0.0f)
    {
        return luaL_error(L, "Animation sheets require positive frame size, count, and time");
    }

    try
    {
        engine::TextureLoader loader(runtime->GetVfs(), runtime->GetRenderer());
        LuaAnimation *ud = static_cast<LuaAnimation *>(lua_newuserdata(L, sizeof(LuaAnimation)));
        new (ud) LuaAnimation{};
        ud->texture = loader.Load(path);
        ud->texture_ptr = &ud->texture;
        ud->owns_texture = true;
        ud->texture_ref = LUA_NOREF;
        InitAnimationState(ud);
        ud->frames.reserve(static_cast<size_t>(frame_count));
        for (int i = 0; i < frame_count; ++i)
        {
            float x = static_cast<float>(i * frame_w);
            ud->frames.push_back({x, 0.0f, static_cast<float>(frame_w), static_cast<float>(frame_h), frame_time});
        }
        if (lua_istable(L, 1))
        {
            ud->looping = looping;
            ud->playing = playing;
        }
        else
        {
            ApplyAnimationFlags(L, ud, 6, 7);
        }
        luaL_getmetatable(L, kAnimationMeta);
        lua_setmetatable(L, -2);
        return 1;
    }
    catch (const std::exception &e)
    {
        return luaL_error(L, "%s", e.what());
    }
}

int LuaAnimationNewSheetEx(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    const char *path = nullptr;
    int frame_w = 0;
    int frame_h = 0;
    int frame_count = 0;
    float frame_time = 0.0f;
    int start_x = 0;
    int start_y = 0;
    int pad_x = 0;
    int pad_y = 0;
    int columns = 0;
    bool looping = true;
    bool playing = false;

    if (lua_istable(L, 1))
    {
        path = GetTableStringField(L, 1, "path");
        frame_w = GetTableIntField(L, 1, "frame_w");
        frame_h = GetTableIntField(L, 1, "frame_h");
        frame_count = GetTableIntField(L, 1, "frame_count");
        frame_time = GetTableFloatField(L, 1, "frame_time");
        start_x = GetTableIntFieldOpt(L, 1, "start_x", 0);
        start_y = GetTableIntFieldOpt(L, 1, "start_y", 0);
        pad_x = GetTableIntFieldOpt(L, 1, "pad_x", 0);
        pad_y = GetTableIntFieldOpt(L, 1, "pad_y", 0);
        columns = GetTableIntFieldOpt(L, 1, "columns", 0);
        looping = GetTableBoolFieldOpt(L, 1, "looping", true);
        playing = GetTableBoolFieldOpt(L, 1, "playing", false);
    }
    else
    {
        path = luaL_checkstring(L, 1);
        frame_w = static_cast<int>(luaL_checkinteger(L, 2));
        frame_h = static_cast<int>(luaL_checkinteger(L, 3));
        frame_count = static_cast<int>(luaL_checkinteger(L, 4));
        frame_time = static_cast<float>(luaL_checknumber(L, 5));
        start_x = static_cast<int>(luaL_optinteger(L, 6, 0));
        start_y = static_cast<int>(luaL_optinteger(L, 7, 0));
        pad_x = static_cast<int>(luaL_optinteger(L, 8, 0));
        pad_y = static_cast<int>(luaL_optinteger(L, 9, 0));
        columns = static_cast<int>(luaL_optinteger(L, 10, 0));
    }

    if (frame_w <= 0 || frame_h <= 0 || frame_count <= 0 || frame_time <= 0.0f)
    {
        return luaL_error(L, "Animation sheets require positive frame size, count, and time");
    }

    if (start_x < 0 || start_y < 0 || pad_x < 0 || pad_y < 0)
    {
        return luaL_error(L, "Animation sheet offsets and padding must be non-negative");
    }

    try
    {
        engine::TextureLoader loader(runtime->GetVfs(), runtime->GetRenderer());
        LuaAnimation *ud = static_cast<LuaAnimation *>(lua_newuserdata(L, sizeof(LuaAnimation)));
        new (ud) LuaAnimation{};
        ud->texture = loader.Load(path);
        ud->texture_ptr = &ud->texture;
        ud->owns_texture = true;
        ud->texture_ref = LUA_NOREF;
        InitAnimationState(ud);

        const int stride_x = frame_w + pad_x;
        const int stride_y = frame_h + pad_y;
        if (stride_x <= 0 || stride_y <= 0)
        {
            return luaL_error(L, "Animation sheet frame stride must be positive");
        }

        if (columns <= 0)
        {
            int available_w = ud->texture.width - start_x;
            columns = available_w > 0 ? (available_w + pad_x) / stride_x : 0;
        }

        if (columns <= 0)
        {
            return luaL_error(L, "Animation sheet column count must be positive");
        }

        ud->frames.reserve(static_cast<size_t>(frame_count));
        for (int i = 0; i < frame_count; ++i)
        {
            int col = i % columns;
            int row = i / columns;
            float x = static_cast<float>(start_x + col * stride_x);
            float y = static_cast<float>(start_y + row * stride_y);

            if (x + frame_w > ud->texture.width || y + frame_h > ud->texture.height)
            {
                return luaL_error(L, "Animation sheet frame exceeds texture bounds");
            }

            ud->frames.push_back({x, y, static_cast<float>(frame_w), static_cast<float>(frame_h), frame_time});
        }

        if (lua_istable(L, 1))
        {
            ud->looping = looping;
            ud->playing = playing;
        }
        else
        {
            ApplyAnimationFlags(L, ud, 11, 12);
        }
        luaL_getmetatable(L, kAnimationMeta);
        lua_setmetatable(L, -2);
        return 1;
    }
    catch (const std::exception &e)
    {
        return luaL_error(L, "%s", e.what());
    }
}

int LuaAnimationAddFrame(lua_State *L)
{
    LuaAnimation *ud = CheckAnimation(L, 1);
    float x = static_cast<float>(luaL_checknumber(L, 2));
    float y = static_cast<float>(luaL_checknumber(L, 3));
    float w = static_cast<float>(luaL_checknumber(L, 4));
    float h = static_cast<float>(luaL_checknumber(L, 5));
    float duration = static_cast<float>(luaL_checknumber(L, 6));

    if (w <= 0.0f || h <= 0.0f || duration <= 0.0f)
    {
        return luaL_error(L, "Animation frames require positive width, height, and duration");
    }

    ud->frames.push_back({x, y, w, h, duration});
    return 0;
}

int LuaAnimationPlay(lua_State *L)
{
    LuaAnimation *ud = CheckAnimation(L, 1);
    ud->playing = true;
    return 0;
}

int LuaAnimationPause(lua_State *L)
{
    LuaAnimation *ud = CheckAnimation(L, 1);
    ud->playing = false;
    return 0;
}

int LuaAnimationResume(lua_State *L)
{
    LuaAnimation *ud = CheckAnimation(L, 1);
    ud->playing = true;
    return 0;
}

int LuaAnimationRestart(lua_State *L)
{
    LuaAnimation *ud = CheckAnimation(L, 1);
    ud->frame_index = 0;
    ud->frame_time = 0.0f;
    ud->playing = true;
    return 0;
}

int LuaAnimationIsPlaying(lua_State *L)
{
    LuaAnimation *ud = CheckAnimation(L, 1);
    lua_pushboolean(L, ud->playing);
    return 1;
}

int LuaAnimationSetLooping(lua_State *L)
{
    LuaAnimation *ud = CheckAnimation(L, 1);
    ud->looping = lua_toboolean(L, 2);
    return 0;
}

int LuaAnimationSetSpeed(lua_State *L)
{
    LuaAnimation *ud = CheckAnimation(L, 1);
    float speed = static_cast<float>(luaL_checknumber(L, 2));
    ud->speed = speed > 0.0f ? speed : 0.0f;
    return 0;
}

int LuaAnimationUpdate(lua_State *L)
{
    LuaAnimation *ud = CheckAnimation(L, 1);
    float dt = static_cast<float>(luaL_checknumber(L, 2));

    if (!ud->playing || ud->frames.empty() || ud->speed <= 0.0f)
    {
        return 0;
    }

    float scaled = dt * ud->speed;
    ud->frame_time += scaled;

    while (ud->frame_time >= ud->frames[ud->frame_index].duration)
    {
        ud->frame_time -= ud->frames[ud->frame_index].duration;
        ud->frame_index++;
        if (ud->frame_index >= ud->frames.size())
        {
            if (ud->looping)
            {
                ud->frame_index = 0;
            }
            else
            {
                ud->frame_index = ud->frames.size() - 1;
                ud->playing = false;
                break;
            }
        }
    }

    return 0;
}

int LuaAnimationDraw(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    LuaAnimation *ud = CheckAnimation(L, 1);
    double x = 0.0;
    double y = 0.0;
    double angle = 0.0;
    double sx = 1.0;
    double sy = 1.0;
    double ox = 0.0;
    double oy = 0.0;
    bool flip_x = false;
    bool flip_y = false;
    bool has_color_override = false;
    SDL_Color override_color = {255, 255, 255, 255};

    if (lua_istable(L, 2))
    {
        x = GetTableFloatField(L, 2, "x");
        y = GetTableFloatField(L, 2, "y");
        angle = GetTableFloatFieldOpt(L, 2, "angle", 0.0f);
        sx = GetTableFloatFieldOpt(L, 2, "sx", 1.0f);
        sy = GetTableFloatFieldOpt(L, 2, "sy", 1.0f);
        ox = GetTableFloatFieldOpt(L, 2, "ox", 0.0f);
        oy = GetTableFloatFieldOpt(L, 2, "oy", 0.0f);
        flip_x = GetTableBoolFieldOpt(L, 2, "flipX", false);
        flip_y = GetTableBoolFieldOpt(L, 2, "flipY", false);

        int idx = lua_absindex(L, 2);
        lua_getfield(L, idx, "r");
        if (!lua_isnil(L, -1))
        {
            has_color_override = true;
            int r = static_cast<int>(luaL_checkinteger(L, -1));
            lua_pop(L, 1);
            int g = GetTableIntField(L, 2, "g");
            int b = GetTableIntField(L, 2, "b");
            int a = GetTableIntFieldOpt(L, 2, "a", 255);
            auto clamp = [](int value) -> Uint8 {
                if (value < 0)
                    return 0;
                if (value > 255)
                    return 255;
                return static_cast<Uint8>(value);
            };
            override_color = {clamp(r), clamp(g), clamp(b), clamp(a)};
        }
        else
        {
            lua_pop(L, 1);
        }
    }
    else
    {
        x = luaL_checknumber(L, 2);
        y = luaL_checknumber(L, 3);
        angle = luaL_optnumber(L, 4, 0.0);
        sx = luaL_optnumber(L, 5, 1.0);
        sy = luaL_optnumber(L, 6, 1.0);
        ox = luaL_optnumber(L, 7, 0.0);
        oy = luaL_optnumber(L, 8, 0.0);
        flip_x = lua_toboolean(L, 9);
        flip_y = lua_toboolean(L, 10);
    }

    if (!ud->texture_ptr || !ud->texture_ptr->handle || ud->frames.empty())
    {
        return 0;
    }

    const AnimationFrame &frame = ud->frames[ud->frame_index];

    const leo::Camera::Camera2D *camera = runtime->GetActiveCamera();
    SDL_FPoint screen = ApplyCameraPoint(camera, {static_cast<float>(x), static_cast<float>(y)});
    float zoom = camera ? camera->zoom : 1.0f;
    double render_angle = ApplyCameraRotation(camera, static_cast<float>(angle));

    SDL_Color color = runtime->GetDrawColor();
    if (has_color_override)
    {
        color = override_color;
    }
    else
    {
        int color_index = 11;
        if (!lua_isnoneornil(L, color_index))
        {
            auto clamp = [](int value) -> Uint8 {
                if (value < 0)
                    return 0;
                if (value > 255)
                    return 255;
                return static_cast<Uint8>(value);
            };
            int r = static_cast<int>(luaL_checkinteger(L, color_index));
            int g = static_cast<int>(luaL_checkinteger(L, color_index + 1));
            int b = static_cast<int>(luaL_checkinteger(L, color_index + 2));
            int a = static_cast<int>(luaL_optinteger(L, color_index + 3, 255));
            color = {clamp(r), clamp(g), clamp(b), clamp(a)};
        }
    }

    SDL_SetTextureColorMod(ud->texture_ptr->handle, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(ud->texture_ptr->handle, color.a);

    float render_sx = static_cast<float>(sx) * zoom;
    float render_sy = static_cast<float>(sy) * zoom;
    float w = frame.w * render_sx;
    float h = frame.h * render_sy;
    SDL_FRect src = {frame.x, frame.y, frame.w, frame.h};
    SDL_FRect dst = {screen.x - static_cast<float>(ox * render_sx), screen.y - static_cast<float>(oy * render_sy), w,
                     h};
    SDL_FPoint center = {static_cast<float>(ox * render_sx), static_cast<float>(oy * render_sy)};
    constexpr double kRadToDeg = 57.29577951308232;
    double degrees = render_angle * kRadToDeg;

    SDL_FlipMode flip = SDL_FLIP_NONE;
    if (flip_x)
    {
        flip = static_cast<SDL_FlipMode>(flip | SDL_FLIP_HORIZONTAL);
    }
    if (flip_y)
    {
        flip = static_cast<SDL_FlipMode>(flip | SDL_FLIP_VERTICAL);
    }

    SDL_RenderTextureRotated(runtime->GetRenderer(), ud->texture_ptr->handle, &src, &dst, degrees, &center, flip);

    SDL_SetTextureColorMod(ud->texture_ptr->handle, 255, 255, 255);
    SDL_SetTextureAlphaMod(ud->texture_ptr->handle, 255);
    return 0;
}

int LuaAnimationGc(lua_State *L)
{
    LuaAnimation *ud = CheckAnimation(L, 1);
    if (ud->owns_texture)
    {
        ud->texture.Reset();
    }
    std::vector<AnimationFrame>().swap(ud->frames);
    if (ud->texture_ref != LUA_NOREF)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, ud->texture_ref);
        ud->texture_ref = LUA_NOREF;
    }
    return 0;
}

int LuaGraphicsSetColor(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    int r = static_cast<int>(luaL_checkinteger(L, 1));
    int g = static_cast<int>(luaL_checkinteger(L, 2));
    int b = static_cast<int>(luaL_checkinteger(L, 3));
    int a = static_cast<int>(luaL_optinteger(L, 4, 255));

    auto clamp = [](int value) -> Uint8 {
        if (value < 0)
            return 0;
        if (value > 255)
            return 255;
        return static_cast<Uint8>(value);
    };

    runtime->SetDrawColor({clamp(r), clamp(g), clamp(b), clamp(a)});
    return 0;
}

int LuaGraphicsClear(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    int r = static_cast<int>(luaL_optinteger(L, 1, 0));
    int g = static_cast<int>(luaL_optinteger(L, 2, 0));
    int b = static_cast<int>(luaL_optinteger(L, 3, 0));
    int a = static_cast<int>(luaL_optinteger(L, 4, 255));

    SDL_SetRenderDrawColor(runtime->GetRenderer(), r, g, b, a);
    SDL_RenderClear(runtime->GetRenderer());
    return 0;
}

int LuaGraphicsGetSize(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    int width = 0;
    int height = 0;

    const engine::Config *config = runtime->GetConfig();
    if (config && config->logical_width > 0 && config->logical_height > 0)
    {
        width = config->logical_width;
        height = config->logical_height;
    }
    else if (runtime->GetWindow())
    {
        SDL_GetWindowSize(runtime->GetWindow(), &width, &height);
    }

    lua_pushinteger(L, width);
    lua_pushinteger(L, height);
    return 2;
}

int LuaGraphicsBeginViewport(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    int x = static_cast<int>(luaL_checkinteger(L, 1));
    int y = static_cast<int>(luaL_checkinteger(L, 2));
    int w = static_cast<int>(luaL_checkinteger(L, 3));
    int h = static_cast<int>(luaL_checkinteger(L, 4));
    if (w <= 0 || h <= 0)
    {
        return luaL_error(L, "leo.graphics.beginViewport requires positive width and height");
    }
    SDL_Rect viewport = {x, y, w, h};
    SDL_SetRenderViewport(runtime->GetRenderer(), &viewport);
    return 0;
}

int LuaGraphicsEndViewport(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    SDL_SetRenderViewport(runtime->GetRenderer(), nullptr);
    return 0;
}

int LuaGraphicsDrawGrid(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    float step = 64.0f;
    leo::Graphics::Color color = {255, 255, 255, 255};

    if (lua_istable(L, 1))
    {
        x = GetTableNumberFieldReq(L, 1, "x", "drawGrid");
        y = GetTableNumberFieldReq(L, 1, "y", "drawGrid");
        w = GetTableNumberFieldReq(L, 1, "w", "drawGrid");
        h = GetTableNumberFieldReq(L, 1, "h", "drawGrid");
        step = GetTableNumberFieldReq(L, 1, "step", "drawGrid");
        color = ReadColorTable(L, 1, "drawGrid");
    }
    else
    {
        x = static_cast<float>(luaL_checknumber(L, 1));
        y = static_cast<float>(luaL_checknumber(L, 2));
        w = static_cast<float>(luaL_checknumber(L, 3));
        h = static_cast<float>(luaL_checknumber(L, 4));
        step = static_cast<float>(luaL_checknumber(L, 5));
        color = ReadColor(L, 6);
    }

    if (w <= 0.0f || h <= 0.0f)
    {
        return 0;
    }
    if (step <= 0.0f)
    {
        return luaL_error(L, "leo.graphics.drawGrid requires positive step");
    }

    const leo::Camera::Camera2D *camera = runtime->GetActiveCamera();
    for (float gx = x; gx <= x + w; gx += step)
    {
        SDL_FPoint p1 = ApplyCameraPoint(camera, {gx, y});
        SDL_FPoint p2 = ApplyCameraPoint(camera, {gx, y + h});
        leo::Graphics::DrawLine(runtime->GetRenderer(), p1.x, p1.y, p2.x, p2.y, color);
    }
    for (float gy = y; gy <= y + h; gy += step)
    {
        SDL_FPoint p1 = ApplyCameraPoint(camera, {x, gy});
        SDL_FPoint p2 = ApplyCameraPoint(camera, {x + w, gy});
        leo::Graphics::DrawLine(runtime->GetRenderer(), p1.x, p1.y, p2.x, p2.y, color);
    }

    return 0;
}

int LuaWindowSetSize(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    SDL_Window *window = runtime->GetWindow();
    if (!window)
    {
        return luaL_error(L, "leo.window.setSize requires an active window");
    }

    int width = static_cast<int>(luaL_checkinteger(L, 1));
    int height = static_cast<int>(luaL_checkinteger(L, 2));
    if (width <= 0 || height <= 0)
    {
        return luaL_error(L, "leo.window.setSize requires positive dimensions");
    }

    SDL_SetWindowSize(window, width, height);
    return 0;
}

int LuaWindowSetMode(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    SDL_Window *window = runtime->GetWindow();
    if (!window)
    {
        return luaL_error(L, "leo.window.setMode requires an active window");
    }

    const char *mode_name = luaL_checkstring(L, 1);
    engine::WindowMode mode = engine::WindowMode::Windowed;
    if (!ParseWindowMode(mode_name, &mode))
    {
        return luaL_error(L, "Unknown window mode: %s", mode_name);
    }

    if (mode == runtime->GetWindowMode())
    {
        return 0;
    }

    const engine::Config *config = runtime->GetConfig();
    if (!ApplyWindowMode(window, config, mode))
    {
        const char *error = SDL_GetError();
        return luaL_error(L, "%s", (error && *error) ? error : "Failed to change window mode");
    }

    runtime->SetWindowMode(mode);
    return 0;
}

int LuaWindowGetSize(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    SDL_Window *window = runtime->GetWindow();
    int width = 0;
    int height = 0;
    if (window)
    {
        SDL_GetWindowSize(window, &width, &height);
    }

    lua_pushinteger(L, width);
    lua_pushinteger(L, height);
    return 2;
}

int LuaGraphicsDrawPixel(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    SDL_FPoint point{};
    leo::Graphics::Color color{};

    if (lua_istable(L, 1))
    {
        point = {GetTableNumberFieldReq(L, 1, "x", "drawPixel"), GetTableNumberFieldReq(L, 1, "y", "drawPixel")};
        color = ReadColorTable(L, 1, "drawPixel");
    }
    else
    {
        point = ReadPointPair(L, 1);
        color = ReadColor(L, 3);
    }
    const leo::Camera::Camera2D *camera = runtime->GetActiveCamera();
    SDL_FPoint screen = ApplyCameraPoint(camera, point);
    leo::Graphics::DrawPixel(runtime->GetRenderer(), screen.x, screen.y, color);
    return 0;
}

int LuaGraphicsDrawLine(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    SDL_FPoint p1{};
    SDL_FPoint p2{};
    leo::Graphics::Color color{};

    if (lua_istable(L, 1))
    {
        p1 = {GetTableNumberFieldReq(L, 1, "x1", "drawLine"), GetTableNumberFieldReq(L, 1, "y1", "drawLine")};
        p2 = {GetTableNumberFieldReq(L, 1, "x2", "drawLine"), GetTableNumberFieldReq(L, 1, "y2", "drawLine")};
        color = ReadColorTable(L, 1, "drawLine");
    }
    else
    {
        p1 = ReadPointPair(L, 1);
        p2 = ReadPointPair(L, 3);
        color = ReadColor(L, 5);
    }
    const leo::Camera::Camera2D *camera = runtime->GetActiveCamera();
    SDL_FPoint s1 = ApplyCameraPoint(camera, p1);
    SDL_FPoint s2 = ApplyCameraPoint(camera, p2);
    leo::Graphics::DrawLine(runtime->GetRenderer(), s1.x, s1.y, s2.x, s2.y, color);
    return 0;
}

int LuaGraphicsDrawCircleFilled(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    SDL_FPoint center{};
    float radius = 0.0f;
    leo::Graphics::Color color{};

    if (lua_istable(L, 1))
    {
        center = {GetTableNumberFieldReq(L, 1, "x", "drawCircleFilled"),
                  GetTableNumberFieldReq(L, 1, "y", "drawCircleFilled")};
        radius = GetTableNumberFieldReq(L, 1, "radius", "drawCircleFilled");
        color = ReadColorTable(L, 1, "drawCircleFilled");
    }
    else
    {
        center = ReadPointPair(L, 1);
        radius = static_cast<float>(luaL_checknumber(L, 3));
        color = ReadColor(L, 4);
    }
    const leo::Camera::Camera2D *camera = runtime->GetActiveCamera();
    SDL_FPoint screen = ApplyCameraPoint(camera, center);
    float scaled_radius = ApplyCameraScale(camera, radius);
    leo::Graphics::DrawCircleFilled(runtime->GetRenderer(), screen.x, screen.y, scaled_radius, color);
    return 0;
}

int LuaGraphicsDrawCircleOutline(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    SDL_FPoint center{};
    float radius = 0.0f;
    leo::Graphics::Color color{};

    if (lua_istable(L, 1))
    {
        center = {GetTableNumberFieldReq(L, 1, "x", "drawCircleOutline"),
                  GetTableNumberFieldReq(L, 1, "y", "drawCircleOutline")};
        radius = GetTableNumberFieldReq(L, 1, "radius", "drawCircleOutline");
        color = ReadColorTable(L, 1, "drawCircleOutline");
    }
    else
    {
        center = ReadPointPair(L, 1);
        radius = static_cast<float>(luaL_checknumber(L, 3));
        color = ReadColor(L, 4);
    }
    const leo::Camera::Camera2D *camera = runtime->GetActiveCamera();
    SDL_FPoint screen = ApplyCameraPoint(camera, center);
    float scaled_radius = ApplyCameraScale(camera, radius);
    leo::Graphics::DrawCircleOutline(runtime->GetRenderer(), screen.x, screen.y, scaled_radius, color);
    return 0;
}

int LuaGraphicsDrawRectangleFilled(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    SDL_FRect rect{};
    leo::Graphics::Color color{};

    if (lua_istable(L, 1))
    {
        rect = {GetTableNumberFieldReq(L, 1, "x", "drawRectangleFilled"),
                GetTableNumberFieldReq(L, 1, "y", "drawRectangleFilled"),
                GetTableNumberFieldReq(L, 1, "w", "drawRectangleFilled"),
                GetTableNumberFieldReq(L, 1, "h", "drawRectangleFilled")};
        color = ReadColorTable(L, 1, "drawRectangleFilled");
    }
    else
    {
        rect = ReadRect(L, 1);
        color = ReadColor(L, 5);
    }
    const leo::Camera::Camera2D *camera = runtime->GetActiveCamera();
    if (camera && camera->rotation != 0.0f)
    {
        SDL_FPoint corners[4] = {
            {rect.x, rect.y}, {rect.x + rect.w, rect.y}, {rect.x + rect.w, rect.y + rect.h}, {rect.x, rect.y + rect.h}};
        for (SDL_FPoint &corner : corners)
        {
            corner = ApplyCameraPoint(camera, corner);
        }
        leo::Graphics::DrawPolyFilled(runtime->GetRenderer(), corners, 4, color);
        return 0;
    }

    SDL_FPoint screen = ApplyCameraPoint(camera, {rect.x, rect.y});
    float w = ApplyCameraScale(camera, rect.w);
    float h = ApplyCameraScale(camera, rect.h);
    leo::Graphics::DrawRectangleFilled(runtime->GetRenderer(), screen.x, screen.y, w, h, color);
    return 0;
}

int LuaGraphicsDrawRectangleOutline(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    SDL_FRect rect{};
    leo::Graphics::Color color{};

    if (lua_istable(L, 1))
    {
        rect = {GetTableNumberFieldReq(L, 1, "x", "drawRectangleOutline"),
                GetTableNumberFieldReq(L, 1, "y", "drawRectangleOutline"),
                GetTableNumberFieldReq(L, 1, "w", "drawRectangleOutline"),
                GetTableNumberFieldReq(L, 1, "h", "drawRectangleOutline")};
        color = ReadColorTable(L, 1, "drawRectangleOutline");
    }
    else
    {
        rect = ReadRect(L, 1);
        color = ReadColor(L, 5);
    }
    const leo::Camera::Camera2D *camera = runtime->GetActiveCamera();
    if (camera && camera->rotation != 0.0f)
    {
        SDL_FPoint corners[4] = {
            {rect.x, rect.y}, {rect.x + rect.w, rect.y}, {rect.x + rect.w, rect.y + rect.h}, {rect.x, rect.y + rect.h}};
        for (SDL_FPoint &corner : corners)
        {
            corner = ApplyCameraPoint(camera, corner);
        }
        leo::Graphics::DrawPolyOutline(runtime->GetRenderer(), corners, 4, color);
        return 0;
    }

    SDL_FPoint screen = ApplyCameraPoint(camera, {rect.x, rect.y});
    float w = ApplyCameraScale(camera, rect.w);
    float h = ApplyCameraScale(camera, rect.h);
    leo::Graphics::DrawRectangleOutline(runtime->GetRenderer(), screen.x, screen.y, w, h, color);
    return 0;
}

int LuaGraphicsDrawRectangleRoundedFilled(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    SDL_FRect rect{};
    float radius = 0.0f;
    leo::Graphics::Color color{};

    if (lua_istable(L, 1))
    {
        rect = {GetTableNumberFieldReq(L, 1, "x", "drawRectangleRoundedFilled"),
                GetTableNumberFieldReq(L, 1, "y", "drawRectangleRoundedFilled"),
                GetTableNumberFieldReq(L, 1, "w", "drawRectangleRoundedFilled"),
                GetTableNumberFieldReq(L, 1, "h", "drawRectangleRoundedFilled")};
        radius = GetTableNumberFieldReq(L, 1, "radius", "drawRectangleRoundedFilled");
        color = ReadColorTable(L, 1, "drawRectangleRoundedFilled");
    }
    else
    {
        rect = ReadRect(L, 1);
        radius = static_cast<float>(luaL_checknumber(L, 5));
        color = ReadColor(L, 6);
    }
    const leo::Camera::Camera2D *camera = runtime->GetActiveCamera();
    if (camera && camera->rotation != 0.0f)
    {
        SDL_FPoint corners[4] = {
            {rect.x, rect.y}, {rect.x + rect.w, rect.y}, {rect.x + rect.w, rect.y + rect.h}, {rect.x, rect.y + rect.h}};
        for (SDL_FPoint &corner : corners)
        {
            corner = ApplyCameraPoint(camera, corner);
        }
        leo::Graphics::DrawPolyFilled(runtime->GetRenderer(), corners, 4, color);
        return 0;
    }

    SDL_FPoint screen = ApplyCameraPoint(camera, {rect.x, rect.y});
    float w = ApplyCameraScale(camera, rect.w);
    float h = ApplyCameraScale(camera, rect.h);
    float scaled_radius = ApplyCameraScale(camera, radius);
    leo::Graphics::DrawRectangleRoundedFilled(runtime->GetRenderer(), screen.x, screen.y, w, h, scaled_radius, color);
    return 0;
}

int LuaGraphicsDrawRectangleRoundedOutline(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    SDL_FRect rect{};
    float radius = 0.0f;
    leo::Graphics::Color color{};

    if (lua_istable(L, 1))
    {
        rect = {GetTableNumberFieldReq(L, 1, "x", "drawRectangleRoundedOutline"),
                GetTableNumberFieldReq(L, 1, "y", "drawRectangleRoundedOutline"),
                GetTableNumberFieldReq(L, 1, "w", "drawRectangleRoundedOutline"),
                GetTableNumberFieldReq(L, 1, "h", "drawRectangleRoundedOutline")};
        radius = GetTableNumberFieldReq(L, 1, "radius", "drawRectangleRoundedOutline");
        color = ReadColorTable(L, 1, "drawRectangleRoundedOutline");
    }
    else
    {
        rect = ReadRect(L, 1);
        radius = static_cast<float>(luaL_checknumber(L, 5));
        color = ReadColor(L, 6);
    }
    const leo::Camera::Camera2D *camera = runtime->GetActiveCamera();
    if (camera && camera->rotation != 0.0f)
    {
        SDL_FPoint corners[4] = {
            {rect.x, rect.y}, {rect.x + rect.w, rect.y}, {rect.x + rect.w, rect.y + rect.h}, {rect.x, rect.y + rect.h}};
        for (SDL_FPoint &corner : corners)
        {
            corner = ApplyCameraPoint(camera, corner);
        }
        leo::Graphics::DrawPolyOutline(runtime->GetRenderer(), corners, 4, color);
        return 0;
    }

    SDL_FPoint screen = ApplyCameraPoint(camera, {rect.x, rect.y});
    float w = ApplyCameraScale(camera, rect.w);
    float h = ApplyCameraScale(camera, rect.h);
    float scaled_radius = ApplyCameraScale(camera, radius);
    leo::Graphics::DrawRectangleRoundedOutline(runtime->GetRenderer(), screen.x, screen.y, w, h, scaled_radius, color);
    return 0;
}

int LuaGraphicsDrawTriangleFilled(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    SDL_FPoint a{};
    SDL_FPoint b{};
    SDL_FPoint c{};
    leo::Graphics::Color color{};

    if (lua_istable(L, 1))
    {
        a = {GetTableNumberFieldReq(L, 1, "x1", "drawTriangleFilled"),
             GetTableNumberFieldReq(L, 1, "y1", "drawTriangleFilled")};
        b = {GetTableNumberFieldReq(L, 1, "x2", "drawTriangleFilled"),
             GetTableNumberFieldReq(L, 1, "y2", "drawTriangleFilled")};
        c = {GetTableNumberFieldReq(L, 1, "x3", "drawTriangleFilled"),
             GetTableNumberFieldReq(L, 1, "y3", "drawTriangleFilled")};
        color = ReadColorTable(L, 1, "drawTriangleFilled");
    }
    else
    {
        a = ReadPointPair(L, 1);
        b = ReadPointPair(L, 3);
        c = ReadPointPair(L, 5);
        color = ReadColor(L, 7);
    }
    const leo::Camera::Camera2D *camera = runtime->GetActiveCamera();
    a = ApplyCameraPoint(camera, a);
    b = ApplyCameraPoint(camera, b);
    c = ApplyCameraPoint(camera, c);
    leo::Graphics::DrawTriangleFilled(runtime->GetRenderer(), a, b, c, color);
    return 0;
}

int LuaGraphicsDrawTriangleOutline(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    SDL_FPoint a{};
    SDL_FPoint b{};
    SDL_FPoint c{};
    leo::Graphics::Color color{};

    if (lua_istable(L, 1))
    {
        a = {GetTableNumberFieldReq(L, 1, "x1", "drawTriangleOutline"),
             GetTableNumberFieldReq(L, 1, "y1", "drawTriangleOutline")};
        b = {GetTableNumberFieldReq(L, 1, "x2", "drawTriangleOutline"),
             GetTableNumberFieldReq(L, 1, "y2", "drawTriangleOutline")};
        c = {GetTableNumberFieldReq(L, 1, "x3", "drawTriangleOutline"),
             GetTableNumberFieldReq(L, 1, "y3", "drawTriangleOutline")};
        color = ReadColorTable(L, 1, "drawTriangleOutline");
    }
    else
    {
        a = ReadPointPair(L, 1);
        b = ReadPointPair(L, 3);
        c = ReadPointPair(L, 5);
        color = ReadColor(L, 7);
    }
    const leo::Camera::Camera2D *camera = runtime->GetActiveCamera();
    a = ApplyCameraPoint(camera, a);
    b = ApplyCameraPoint(camera, b);
    c = ApplyCameraPoint(camera, c);
    leo::Graphics::DrawTriangleOutline(runtime->GetRenderer(), a, b, c, color);
    return 0;
}

int LuaGraphicsDrawPolyFilled(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    std::vector<SDL_FPoint> points;
    leo::Graphics::Color color{};

    if (lua_istable(L, 1) && TableHasField(L, 1, "points"))
    {
        int idx = lua_absindex(L, 1);
        lua_getfield(L, idx, "points");
        ReadPointList(L, lua_gettop(L), &points);
        lua_pop(L, 1);
        color = ReadColorTable(L, 1, "drawPolyFilled");
    }
    else
    {
        ReadPointList(L, 1, &points);
        color = ReadColor(L, 2);
    }
    const leo::Camera::Camera2D *camera = runtime->GetActiveCamera();
    if (camera)
    {
        for (SDL_FPoint &point : points)
        {
            point = ApplyCameraPoint(camera, point);
        }
    }
    leo::Graphics::DrawPolyFilled(runtime->GetRenderer(), points.data(), static_cast<int>(points.size()), color);
    return 0;
}

int LuaGraphicsDrawPolyOutline(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    std::vector<SDL_FPoint> points;
    leo::Graphics::Color color{};

    if (lua_istable(L, 1) && TableHasField(L, 1, "points"))
    {
        int idx = lua_absindex(L, 1);
        lua_getfield(L, idx, "points");
        ReadPointList(L, lua_gettop(L), &points);
        lua_pop(L, 1);
        color = ReadColorTable(L, 1, "drawPolyOutline");
    }
    else
    {
        ReadPointList(L, 1, &points);
        color = ReadColor(L, 2);
    }
    const leo::Camera::Camera2D *camera = runtime->GetActiveCamera();
    if (camera)
    {
        for (SDL_FPoint &point : points)
        {
            point = ApplyCameraPoint(camera, point);
        }
    }
    leo::Graphics::DrawPolyOutline(runtime->GetRenderer(), points.data(), static_cast<int>(points.size()), color);
    return 0;
}

int LuaCollisionCheckRecs(lua_State *L)
{
    SDL_FRect a = {};
    SDL_FRect b = {};
    if (lua_istable(L, 1))
    {
        a = GetTableRectField(L, 1, "a", "collision.checkRecs");
        b = GetTableRectField(L, 1, "b", "collision.checkRecs");
    }
    else
    {
        a = ReadRect(L, 1);
        b = ReadRect(L, 5);
    }
    lua_pushboolean(L, leo::Collision::CheckCollisionRecs(a, b));
    return 1;
}

int LuaCollisionCheckCircles(lua_State *L)
{
    SDL_FPoint c1 = {};
    SDL_FPoint c2 = {};
    float r1 = 0.0f;
    float r2 = 0.0f;
    if (lua_istable(L, 1))
    {
        c1 = GetTablePointField(L, 1, "c1", "collision.checkCircles");
        c2 = GetTablePointField(L, 1, "c2", "collision.checkCircles");
        r1 = GetTableNumberFieldReq(L, 1, "r1", "collision.checkCircles");
        r2 = GetTableNumberFieldReq(L, 1, "r2", "collision.checkCircles");
    }
    else
    {
        c1 = ReadPointPair(L, 1);
        r1 = static_cast<float>(luaL_checknumber(L, 3));
        c2 = ReadPointPair(L, 4);
        r2 = static_cast<float>(luaL_checknumber(L, 6));
    }
    lua_pushboolean(L, leo::Collision::CheckCollisionCircles(c1, r1, c2, r2));
    return 1;
}

int LuaCollisionCheckCircleRec(lua_State *L)
{
    SDL_FPoint center = {};
    float radius = 0.0f;
    SDL_FRect rec = {};
    if (lua_istable(L, 1))
    {
        center = GetTablePointField(L, 1, "center", "collision.checkCircleRec");
        radius = GetTableNumberFieldReq(L, 1, "radius", "collision.checkCircleRec");
        rec = GetTableRectField(L, 1, "rect", "collision.checkCircleRec");
    }
    else
    {
        center = ReadPointPair(L, 1);
        radius = static_cast<float>(luaL_checknumber(L, 3));
        rec = ReadRect(L, 4);
    }
    lua_pushboolean(L, leo::Collision::CheckCollisionCircleRec(center, radius, rec));
    return 1;
}

int LuaCollisionCheckCircleLine(lua_State *L)
{
    SDL_FPoint center = {};
    float radius = 0.0f;
    SDL_FPoint p1 = {};
    SDL_FPoint p2 = {};
    if (lua_istable(L, 1))
    {
        center = GetTablePointField(L, 1, "center", "collision.checkCircleLine");
        radius = GetTableNumberFieldReq(L, 1, "radius", "collision.checkCircleLine");
        p1 = GetTablePointField(L, 1, "p1", "collision.checkCircleLine");
        p2 = GetTablePointField(L, 1, "p2", "collision.checkCircleLine");
    }
    else
    {
        center = ReadPointPair(L, 1);
        radius = static_cast<float>(luaL_checknumber(L, 3));
        p1 = ReadPointPair(L, 4);
        p2 = ReadPointPair(L, 6);
    }
    lua_pushboolean(L, leo::Collision::CheckCollisionCircleLine(center, radius, p1, p2));
    return 1;
}

int LuaCollisionCheckPointRec(lua_State *L)
{
    SDL_FPoint point = {};
    SDL_FRect rec = {};
    if (lua_istable(L, 1))
    {
        point = GetTablePointField(L, 1, "point", "collision.checkPointRec");
        rec = GetTableRectField(L, 1, "rect", "collision.checkPointRec");
    }
    else
    {
        point = ReadPointPair(L, 1);
        rec = ReadRect(L, 3);
    }
    lua_pushboolean(L, leo::Collision::CheckCollisionPointRec(point, rec));
    return 1;
}

int LuaCollisionCheckPointCircle(lua_State *L)
{
    SDL_FPoint point = {};
    SDL_FPoint center = {};
    float radius = 0.0f;
    if (lua_istable(L, 1))
    {
        point = GetTablePointField(L, 1, "point", "collision.checkPointCircle");
        center = GetTablePointField(L, 1, "center", "collision.checkPointCircle");
        radius = GetTableNumberFieldReq(L, 1, "radius", "collision.checkPointCircle");
    }
    else
    {
        point = ReadPointPair(L, 1);
        center = ReadPointPair(L, 3);
        radius = static_cast<float>(luaL_checknumber(L, 5));
    }
    lua_pushboolean(L, leo::Collision::CheckCollisionPointCircle(point, center, radius));
    return 1;
}

int LuaCollisionCheckPointTriangle(lua_State *L)
{
    SDL_FPoint point = {};
    SDL_FPoint p1 = {};
    SDL_FPoint p2 = {};
    SDL_FPoint p3 = {};
    if (lua_istable(L, 1))
    {
        point = GetTablePointField(L, 1, "point", "collision.checkPointTriangle");
        p1 = GetTablePointField(L, 1, "p1", "collision.checkPointTriangle");
        p2 = GetTablePointField(L, 1, "p2", "collision.checkPointTriangle");
        p3 = GetTablePointField(L, 1, "p3", "collision.checkPointTriangle");
    }
    else
    {
        point = ReadPointPair(L, 1);
        p1 = ReadPointPair(L, 3);
        p2 = ReadPointPair(L, 5);
        p3 = ReadPointPair(L, 7);
    }
    lua_pushboolean(L, leo::Collision::CheckCollisionPointTriangle(point, p1, p2, p3));
    return 1;
}

int LuaCollisionCheckPointLine(lua_State *L)
{
    SDL_FPoint point = {};
    SDL_FPoint p1 = {};
    SDL_FPoint p2 = {};
    if (lua_istable(L, 1))
    {
        point = GetTablePointField(L, 1, "point", "collision.checkPointLine");
        p1 = GetTablePointField(L, 1, "p1", "collision.checkPointLine");
        p2 = GetTablePointField(L, 1, "p2", "collision.checkPointLine");
    }
    else
    {
        point = ReadPointPair(L, 1);
        p1 = ReadPointPair(L, 3);
        p2 = ReadPointPair(L, 5);
    }
    lua_pushboolean(L, leo::Collision::CheckCollisionPointLine(point, p1, p2));
    return 1;
}

int LuaCollisionCheckPointPoly(lua_State *L)
{
    SDL_FPoint point = {};
    std::vector<SDL_FPoint> points;
    if (lua_istable(L, 1))
    {
        point = GetTablePointField(L, 1, "point", "collision.checkPointPoly");
        ReadPointListField(L, 1, "points", &points, "collision.checkPointPoly");
    }
    else
    {
        point = ReadPointPair(L, 1);
        ReadPointList(L, 3, &points);
    }
    lua_pushboolean(L, leo::Collision::CheckCollisionPointPoly(point, points.data(), static_cast<int>(points.size())));
    return 1;
}

int LuaCollisionCheckLines(lua_State *L)
{
    SDL_FPoint p1 = {};
    SDL_FPoint p2 = {};
    SDL_FPoint p3 = {};
    SDL_FPoint p4 = {};
    if (lua_istable(L, 1))
    {
        p1 = GetTablePointField(L, 1, "p1", "collision.checkLines");
        p2 = GetTablePointField(L, 1, "p2", "collision.checkLines");
        p3 = GetTablePointField(L, 1, "p3", "collision.checkLines");
        p4 = GetTablePointField(L, 1, "p4", "collision.checkLines");
    }
    else
    {
        p1 = ReadPointPair(L, 1);
        p2 = ReadPointPair(L, 3);
        p3 = ReadPointPair(L, 5);
        p4 = ReadPointPair(L, 7);
    }
    lua_pushboolean(L, leo::Collision::CheckCollisionLines(p1, p2, p3, p4));
    return 1;
}

int LuaCameraNew(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    const engine::Config *config = runtime->GetConfig();
    LuaCamera *ud = static_cast<LuaCamera *>(lua_newuserdata(L, sizeof(LuaCamera)));
    if (config)
    {
        ud->camera = leo::Camera::CreateDefault(*config);
    }
    else
    {
        ud->camera = leo::Camera::CreateDefault(0.0f, 0.0f);
    }

    if (lua_istable(L, 1))
    {
        int idx = lua_absindex(L, 1);

        lua_getfield(L, idx, "position");
        if (!lua_isnil(L, -1))
        {
            luaL_checktype(L, -1, LUA_TTABLE);
            float x = GetTableNumberFieldReq(L, -1, "x", "camera.new position");
            float y = GetTableNumberFieldReq(L, -1, "y", "camera.new position");
            ud->camera.position = {x, y};
        }
        lua_pop(L, 1);

        lua_getfield(L, idx, "target");
        if (!lua_isnil(L, -1))
        {
            luaL_checktype(L, -1, LUA_TTABLE);
            float x = GetTableNumberFieldReq(L, -1, "x", "camera.new target");
            float y = GetTableNumberFieldReq(L, -1, "y", "camera.new target");
            ud->camera.target = {x, y};
        }
        lua_pop(L, 1);

        lua_getfield(L, idx, "offset");
        if (!lua_isnil(L, -1))
        {
            luaL_checktype(L, -1, LUA_TTABLE);
            float x = GetTableNumberFieldReq(L, -1, "x", "camera.new offset");
            float y = GetTableNumberFieldReq(L, -1, "y", "camera.new offset");
            ud->camera.offset = {x, y};
        }
        lua_pop(L, 1);

        lua_getfield(L, idx, "zoom");
        if (!lua_isnil(L, -1))
        {
            float zoom = static_cast<float>(luaL_checknumber(L, -1));
            if (zoom > 0.0f)
            {
                ud->camera.zoom = zoom;
            }
        }
        lua_pop(L, 1);

        lua_getfield(L, idx, "rotation");
        if (!lua_isnil(L, -1))
        {
            ud->camera.rotation = static_cast<float>(luaL_checknumber(L, -1));
        }
        lua_pop(L, 1);

        lua_getfield(L, idx, "deadzone");
        if (!lua_isnil(L, -1))
        {
            luaL_checktype(L, -1, LUA_TTABLE);
            float w = GetTableNumberFieldReq(L, -1, "w", "camera.new deadzone");
            float h = GetTableNumberFieldReq(L, -1, "h", "camera.new deadzone");
            ud->camera.deadzone = {0.0f, 0.0f, w, h};
            ud->camera.use_deadzone = (w > 0.0f && h > 0.0f);
        }
        lua_pop(L, 1);

        lua_getfield(L, idx, "smooth_time");
        if (!lua_isnil(L, -1))
        {
            float seconds = static_cast<float>(luaL_checknumber(L, -1));
            ud->camera.smooth_time = seconds < 0.0f ? 0.0f : seconds;
        }
        lua_pop(L, 1);

        lua_getfield(L, idx, "bounds");
        if (!lua_isnil(L, -1))
        {
            luaL_checktype(L, -1, LUA_TTABLE);
            float x = GetTableNumberFieldReq(L, -1, "x", "camera.new bounds");
            float y = GetTableNumberFieldReq(L, -1, "y", "camera.new bounds");
            float w = GetTableNumberFieldReq(L, -1, "w", "camera.new bounds");
            float h = GetTableNumberFieldReq(L, -1, "h", "camera.new bounds");
            ud->camera.bounds = {x, y, w, h};
        }
        lua_pop(L, 1);

        lua_getfield(L, idx, "clamp");
        if (!lua_isnil(L, -1))
        {
            ud->camera.clamp_to_bounds = lua_toboolean(L, -1);
        }
        lua_pop(L, 1);
    }

    luaL_getmetatable(L, kCameraMeta);
    lua_setmetatable(L, -2);
    return 1;
}

int LuaCameraSetTarget(lua_State *L)
{
    LuaCamera *ud = CheckCamera(L, 1);
    ud->camera.target = ReadPointPair(L, 2);
    return 0;
}

int LuaCameraSetPosition(lua_State *L)
{
    LuaCamera *ud = CheckCamera(L, 1);
    ud->camera.position = ReadPointPair(L, 2);
    return 0;
}

int LuaCameraSetOffset(lua_State *L)
{
    LuaCamera *ud = CheckCamera(L, 1);
    ud->camera.offset = ReadPointPair(L, 2);
    return 0;
}

int LuaCameraSetDeadzone(lua_State *L)
{
    LuaCamera *ud = CheckCamera(L, 1);
    float w = static_cast<float>(luaL_checknumber(L, 2));
    float h = static_cast<float>(luaL_checknumber(L, 3));
    ud->camera.deadzone = {0.0f, 0.0f, w, h};
    ud->camera.use_deadzone = (w > 0.0f && h > 0.0f);
    return 0;
}

int LuaCameraSetSmoothTime(lua_State *L)
{
    LuaCamera *ud = CheckCamera(L, 1);
    float seconds = static_cast<float>(luaL_checknumber(L, 2));
    ud->camera.smooth_time = seconds < 0.0f ? 0.0f : seconds;
    return 0;
}

int LuaCameraSetBounds(lua_State *L)
{
    LuaCamera *ud = CheckCamera(L, 1);
    ud->camera.bounds = ReadRect(L, 2);
    return 0;
}

int LuaCameraSetClamp(lua_State *L)
{
    LuaCamera *ud = CheckCamera(L, 1);
    ud->camera.clamp_to_bounds = lua_toboolean(L, 2);
    return 0;
}

int LuaCameraSetZoom(lua_State *L)
{
    LuaCamera *ud = CheckCamera(L, 1);
    float zoom = static_cast<float>(luaL_checknumber(L, 2));
    ud->camera.zoom = zoom > 0.0f ? zoom : ud->camera.zoom;
    return 0;
}

int LuaCameraSetRotation(lua_State *L)
{
    LuaCamera *ud = CheckCamera(L, 1);
    ud->camera.rotation = static_cast<float>(luaL_checknumber(L, 2));
    return 0;
}

int LuaCameraUpdate(lua_State *L)
{
    LuaCamera *ud = CheckCamera(L, 1);
    float dt = static_cast<float>(luaL_checknumber(L, 2));
    if (lua_istable(L, 3))
    {
        int idx = lua_absindex(L, 3);

        lua_getfield(L, idx, "target");
        if (!lua_isnil(L, -1))
        {
            luaL_checktype(L, -1, LUA_TTABLE);
            float x = GetTableNumberFieldReq(L, -1, "x", "camera.update target");
            float y = GetTableNumberFieldReq(L, -1, "y", "camera.update target");
            ud->camera.target = {x, y};
        }
        lua_pop(L, 1);

        lua_getfield(L, idx, "zoom");
        if (!lua_isnil(L, -1))
        {
            float zoom = static_cast<float>(luaL_checknumber(L, -1));
            if (zoom > 0.0f)
            {
                ud->camera.zoom = zoom;
            }
        }
        lua_pop(L, 1);

        lua_getfield(L, idx, "rotation");
        if (!lua_isnil(L, -1))
        {
            ud->camera.rotation = static_cast<float>(luaL_checknumber(L, -1));
        }
        lua_pop(L, 1);
    }
    leo::Camera::Update(ud->camera, dt);
    return 0;
}

int LuaCameraWorldToScreen(lua_State *L)
{
    LuaCamera *ud = CheckCamera(L, 1);
    SDL_FPoint world = ReadPointPair(L, 2);
    SDL_FPoint screen = leo::Camera::WorldToScreen(ud->camera, world);
    lua_pushnumber(L, screen.x);
    lua_pushnumber(L, screen.y);
    return 2;
}

int LuaCameraScreenToWorld(lua_State *L)
{
    LuaCamera *ud = CheckCamera(L, 1);
    SDL_FPoint screen = ReadPointPair(L, 2);
    SDL_FPoint world = leo::Camera::ScreenToWorld(ud->camera, screen);
    lua_pushnumber(L, world.x);
    lua_pushnumber(L, world.y);
    return 2;
}

int LuaGraphicsBeginCamera(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    LuaCamera *ud = CheckCamera(L, 1);
    runtime->SetActiveCamera(&ud->camera);
    return 0;
}

int LuaGraphicsEndCamera(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    runtime->SetActiveCamera(nullptr);
    return 0;
}

int LuaTiledLoad(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    const char *path = nullptr;
    if (lua_istable(L, 1))
    {
        path = GetTableStringFieldReq(L, 1, "path", "leo.tiled.load");
    }
    else
    {
        path = luaL_checkstring(L, 1);
    }

    try
    {
        LuaTiledMap *ud = static_cast<LuaTiledMap *>(lua_newuserdata(L, sizeof(LuaTiledMap)));
        engine::TiledMap loaded = engine::TiledMap::LoadFromVfs(runtime->GetVfs(), runtime->GetRenderer(), path);
        new (&ud->map) engine::TiledMap(std::move(loaded));
        luaL_getmetatable(L, kTiledMapMeta);
        lua_setmetatable(L, -2);
        return 1;
    }
    catch (const std::exception &e)
    {
        return luaL_error(L, "%s", e.what());
    }
}

int LuaTiledMapGc(lua_State *L)
{
    LuaTiledMap *ud = CheckTiledMap(L, 1);
    ud->map.Reset();
    return 0;
}

int LuaTiledMapDraw(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    LuaTiledMap *ud = CheckTiledMap(L, 1);
    float x = 0.0f;
    float y = 0.0f;
    if (lua_istable(L, 2))
    {
        x = GetTableFloatFieldOpt(L, 2, "x", 0.0f);
        y = GetTableFloatFieldOpt(L, 2, "y", 0.0f);
    }
    else
    {
        x = static_cast<float>(luaL_optnumber(L, 2, 0.0));
        y = static_cast<float>(luaL_optnumber(L, 3, 0.0));
    }
    ud->map.Draw(runtime->GetRenderer(), x, y, runtime->GetActiveCamera());
    return 0;
}

int LuaTiledMapDrawLayer(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    LuaTiledMap *ud = CheckTiledMap(L, 1);
    int layer_index = 0;
    float x = 0.0f;
    float y = 0.0f;
    if (lua_istable(L, 2))
    {
        layer_index =
            static_cast<int>(GetTableNumberFieldReq(L, 2, "layer", "map.drawLayer")) - 1;
        x = GetTableFloatFieldOpt(L, 2, "x", 0.0f);
        y = GetTableFloatFieldOpt(L, 2, "y", 0.0f);
    }
    else
    {
        layer_index = static_cast<int>(luaL_checkinteger(L, 2)) - 1;
        x = static_cast<float>(luaL_optnumber(L, 3, 0.0));
        y = static_cast<float>(luaL_optnumber(L, 4, 0.0));
    }
    ud->map.DrawLayer(runtime->GetRenderer(), layer_index, x, y, runtime->GetActiveCamera());
    return 0;
}

int LuaTiledMapGetSize(lua_State *L)
{
    LuaTiledMap *ud = CheckTiledMap(L, 1);
    lua_pushinteger(L, ud->map.GetWidth());
    lua_pushinteger(L, ud->map.GetHeight());
    return 2;
}

int LuaTiledMapGetTileSize(lua_State *L)
{
    LuaTiledMap *ud = CheckTiledMap(L, 1);
    lua_pushinteger(L, ud->map.GetTileWidth());
    lua_pushinteger(L, ud->map.GetTileHeight());
    return 2;
}

int LuaTiledMapGetPixelSize(lua_State *L)
{
    LuaTiledMap *ud = CheckTiledMap(L, 1);
    SDL_FRect bounds = ud->map.GetPixelBounds();
    lua_pushinteger(L, static_cast<lua_Integer>(bounds.w));
    lua_pushinteger(L, static_cast<lua_Integer>(bounds.h));
    return 2;
}

int LuaTiledMapGetLayerCount(lua_State *L)
{
    LuaTiledMap *ud = CheckTiledMap(L, 1);
    lua_pushinteger(L, ud->map.GetLayerCount());
    return 1;
}

int LuaTiledMapGetLayerName(lua_State *L)
{
    LuaTiledMap *ud = CheckTiledMap(L, 1);
    int layer_index = static_cast<int>(luaL_checkinteger(L, 2)) - 1;
    const char *name = ud->map.GetLayerName(layer_index);
    if (!name)
    {
        lua_pushnil(L);
        return 1;
    }
    lua_pushstring(L, name);
    return 1;
}

int LuaTextureGc(lua_State *L)
{
    LuaTexture *ud = CheckTexture(L, 1);
    ud->texture.Reset();
    return 0;
}

int LuaTextureGetSize(lua_State *L)
{
    LuaTexture *ud = CheckTexture(L, 1);
    lua_pushinteger(L, ud->texture.width);
    lua_pushinteger(L, ud->texture.height);
    return 2;
}

int LuaFontNew(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    const char *path = luaL_checkstring(L, 1);
    int size = static_cast<int>(luaL_checkinteger(L, 2));

    try
    {
        LuaFont *ud = static_cast<LuaFont *>(lua_newuserdata(L, sizeof(LuaFont)));
        new (&ud->font) engine::Font(engine::Font::LoadFromVfs(runtime->GetVfs(), runtime->GetRenderer(), path, size));
        ud->pixel_size = size;
        luaL_getmetatable(L, kFontMeta);
        lua_setmetatable(L, -2);
        return 1;
    }
    catch (const std::exception &e)
    {
        return luaL_error(L, "%s", e.what());
    }
}

int LuaFontGc(lua_State *L)
{
    LuaFont *ud = CheckFont(L, 1);
    ud->font.Reset();
    return 0;
}

int LuaFontGetLineHeight(lua_State *L)
{
    LuaFont *ud = CheckFont(L, 1);
    lua_pushinteger(L, ud->font.GetLineHeight());
    return 1;
}

int LuaFontPrint(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    LuaFont *ud = CheckFont(L, 1);
    const char *text = nullptr;
    float x = 0.0f;
    float y = 0.0f;
    int size = ud->pixel_size;
    SDL_Color color = runtime->GetDrawColor();

    if (lua_istable(L, 2))
    {
        text = GetTableStringFieldReq(L, 2, "text", "font.print");
        x = GetTableNumberFieldReq(L, 2, "x", "font.print");
        y = GetTableNumberFieldReq(L, 2, "y", "font.print");
        size = static_cast<int>(GetTableFloatFieldOpt(L, 2, "size", static_cast<float>(ud->pixel_size)));
        ReadColorTableOptional(L, 2, "font.print", &color);
    }
    else
    {
        text = luaL_checkstring(L, 2);
        x = static_cast<float>(luaL_checknumber(L, 3));
        y = static_cast<float>(luaL_checknumber(L, 4));
        size = static_cast<int>(luaL_optinteger(L, 5, ud->pixel_size));
        ReadColorArgsOptional(L, 6, &color);
    }

    engine::TextDesc desc = {
        .font = &ud->font,
        .text = text,
        .pixel_size = size,
        .position = {x, y},
        .color = {color.r, color.g, color.b, color.a},
    };
    engine::Text text_obj(desc);
    text_obj.Draw(runtime->GetRenderer());
    return 0;
}

int LuaFontSet(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    LuaFont *ud = CheckFont(L, 1);

    runtime->ClearCurrentFontRef(L);
    lua_pushvalue(L, 1);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    runtime->SetCurrentFont(&ud->font, ud->pixel_size, ref);
    return 0;
}

int LuaFontPrintCurrent(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    SDL_Color color = runtime->GetDrawColor();
    LuaFont *ud = nullptr;
    engine::Font *font = nullptr;
    const char *text = nullptr;
    float x = 0.0f;
    float y = 0.0f;
    int pixel_size = 0;

    if (lua_istable(L, 1))
    {
        lua_getfield(L, 1, "font");
        if (!lua_isnil(L, -1))
        {
            ud = CheckFont(L, -1);
        }
        lua_pop(L, 1);

        text = GetTableStringFieldReq(L, 1, "text", "leo.font.print");
        x = GetTableNumberFieldReq(L, 1, "x", "leo.font.print");
        y = GetTableNumberFieldReq(L, 1, "y", "leo.font.print");
        if (ud)
        {
            pixel_size = static_cast<int>(GetTableFloatFieldOpt(L, 1, "size", static_cast<float>(ud->pixel_size)));
        }
        else
        {
            pixel_size = static_cast<int>(
                GetTableFloatFieldOpt(L, 1, "size", static_cast<float>(runtime->GetCurrentFontSize())));
        }
        ReadColorTableOptional(L, 1, "leo.font.print", &color);
    }
    else if (luaL_testudata(L, 1, kFontMeta))
    {
        ud = CheckFont(L, 1);
        text = luaL_checkstring(L, 2);
        x = static_cast<float>(luaL_checknumber(L, 3));
        y = static_cast<float>(luaL_checknumber(L, 4));
        pixel_size = static_cast<int>(luaL_optinteger(L, 5, ud->pixel_size));
        ReadColorArgsOptional(L, 6, &color);
    }
    else
    {
        text = luaL_checkstring(L, 1);
        x = static_cast<float>(luaL_checknumber(L, 2));
        y = static_cast<float>(luaL_checknumber(L, 3));
        ReadColorArgsOptional(L, 4, &color);
    }

    if (ud)
    {
        font = &ud->font;
    }
    else
    {
        font = runtime->GetCurrentFont();
    }

    if (!font)
    {
        return luaL_error(L, "leo.font.print requires leo.font.set(font) first");
    }

    if (pixel_size <= 0)
    {
        pixel_size = font->GetLineHeight();
    }
    engine::TextDesc desc = {
        .font = font,
        .text = text,
        .pixel_size = pixel_size,
        .position = {x, y},
        .color = {color.r, color.g, color.b, color.a},
    };
    engine::Text text_obj(desc);
    text_obj.Draw(runtime->GetRenderer());
    return 0;
}

int LuaSoundNew(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    const char *path = nullptr;
    bool apply_looping = false;
    bool looping = false;
    bool apply_volume = false;
    float volume = 0.0f;
    bool apply_pitch = false;
    float pitch = 0.0f;
    bool auto_play = false;

    if (lua_istable(L, 1))
    {
        path = GetTableStringFieldReq(L, 1, "path", "leo.audio.newSound");
        if (TableHasField(L, 1, "looping"))
        {
            looping = GetTableBoolFieldReq(L, 1, "looping", "leo.audio.newSound");
            apply_looping = true;
        }
        if (TableHasField(L, 1, "volume"))
        {
            volume = GetTableFloatFieldOpt(L, 1, "volume", 0.0f);
            apply_volume = true;
        }
        if (TableHasField(L, 1, "pitch"))
        {
            pitch = GetTableFloatFieldOpt(L, 1, "pitch", 0.0f);
            apply_pitch = true;
        }
        auto_play = GetTableBoolFieldOpt(L, 1, "play", false) || GetTableBoolFieldOpt(L, 1, "playing", false);
    }
    else
    {
        path = luaL_checkstring(L, 1);
    }
    try
    {
        LuaSound *ud = static_cast<LuaSound *>(lua_newuserdata(L, sizeof(LuaSound)));
        new (&ud->sound) engine::Sound(engine::Sound::LoadFromVfs(runtime->GetVfs(), path));
        if (apply_looping)
        {
            ud->sound.SetLooping(looping);
        }
        if (apply_volume)
        {
            ud->sound.SetVolume(volume);
        }
        if (apply_pitch)
        {
            ud->sound.SetPitch(pitch);
        }
        if (auto_play)
        {
            ud->sound.Play();
        }
        luaL_getmetatable(L, kSoundMeta);
        lua_setmetatable(L, -2);
        return 1;
    }
    catch (const std::exception &e)
    {
        return luaL_error(L, "%s", e.what());
    }
}

int LuaSoundGc(lua_State *L)
{
    LuaSound *ud = CheckSound(L, 1);
    ud->sound.Reset();
    return 0;
}

int LuaSoundPlay(lua_State *L)
{
    LuaSound *ud = CheckSound(L, 1);
    ud->sound.Play();
    return 0;
}

int LuaSoundPause(lua_State *L)
{
    LuaSound *ud = CheckSound(L, 1);
    ud->sound.Pause();
    return 0;
}

int LuaSoundStop(lua_State *L)
{
    LuaSound *ud = CheckSound(L, 1);
    ud->sound.Stop();
    return 0;
}

int LuaSoundIsPlaying(lua_State *L)
{
    LuaSound *ud = CheckSound(L, 1);
    lua_pushboolean(L, ud->sound.IsPlaying());
    return 1;
}

int LuaSoundSetLooping(lua_State *L)
{
    LuaSound *ud = CheckSound(L, 1);
    bool looping = false;
    if (lua_istable(L, 2))
    {
        looping = GetTableBoolFieldReq(L, 2, "looping", "sound.setLooping");
    }
    else
    {
        looping = lua_toboolean(L, 2);
    }
    ud->sound.SetLooping(looping);
    return 0;
}

int LuaSoundSetVolume(lua_State *L)
{
    LuaSound *ud = CheckSound(L, 1);
    float volume = 0.0f;
    if (lua_istable(L, 2))
    {
        volume = GetTableNumberFieldReq(L, 2, "volume", "sound.setVolume");
    }
    else
    {
        volume = static_cast<float>(luaL_checknumber(L, 2));
    }
    ud->sound.SetVolume(volume);
    return 0;
}

int LuaSoundSetPitch(lua_State *L)
{
    LuaSound *ud = CheckSound(L, 1);
    float pitch = 0.0f;
    if (lua_istable(L, 2))
    {
        pitch = GetTableNumberFieldReq(L, 2, "pitch", "sound.setPitch");
    }
    else
    {
        pitch = static_cast<float>(luaL_checknumber(L, 2));
    }
    ud->sound.SetPitch(pitch);
    return 0;
}

int LuaMusicNew(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    const char *path = nullptr;
    bool apply_looping = false;
    bool looping = false;
    bool apply_volume = false;
    float volume = 0.0f;
    bool apply_pitch = false;
    float pitch = 0.0f;
    bool auto_play = false;

    if (lua_istable(L, 1))
    {
        path = GetTableStringFieldReq(L, 1, "path", "leo.audio.newMusic");
        if (TableHasField(L, 1, "looping"))
        {
            looping = GetTableBoolFieldReq(L, 1, "looping", "leo.audio.newMusic");
            apply_looping = true;
        }
        if (TableHasField(L, 1, "volume"))
        {
            volume = GetTableFloatFieldOpt(L, 1, "volume", 0.0f);
            apply_volume = true;
        }
        if (TableHasField(L, 1, "pitch"))
        {
            pitch = GetTableFloatFieldOpt(L, 1, "pitch", 0.0f);
            apply_pitch = true;
        }
        auto_play = GetTableBoolFieldOpt(L, 1, "play", false) || GetTableBoolFieldOpt(L, 1, "playing", false);
    }
    else
    {
        path = luaL_checkstring(L, 1);
    }
    try
    {
        LuaMusic *ud = static_cast<LuaMusic *>(lua_newuserdata(L, sizeof(LuaMusic)));
        new (&ud->music) engine::Music(engine::Music::LoadFromVfs(runtime->GetVfs(), path));
        if (apply_looping)
        {
            ud->music.SetLooping(looping);
        }
        if (apply_volume)
        {
            ud->music.SetVolume(volume);
        }
        if (apply_pitch)
        {
            ud->music.SetPitch(pitch);
        }
        if (auto_play)
        {
            ud->music.Play();
        }
        luaL_getmetatable(L, kMusicMeta);
        lua_setmetatable(L, -2);
        return 1;
    }
    catch (const std::exception &e)
    {
        return luaL_error(L, "%s", e.what());
    }
}

int LuaMusicGc(lua_State *L)
{
    LuaMusic *ud = CheckMusic(L, 1);
    ud->music.Reset();
    return 0;
}

int LuaMusicPlay(lua_State *L)
{
    LuaMusic *ud = CheckMusic(L, 1);
    ud->music.Play();
    return 0;
}

int LuaMusicPause(lua_State *L)
{
    LuaMusic *ud = CheckMusic(L, 1);
    ud->music.Pause();
    return 0;
}

int LuaMusicStop(lua_State *L)
{
    LuaMusic *ud = CheckMusic(L, 1);
    ud->music.Stop();
    return 0;
}

int LuaMusicIsPlaying(lua_State *L)
{
    LuaMusic *ud = CheckMusic(L, 1);
    lua_pushboolean(L, ud->music.IsPlaying());
    return 1;
}

int LuaMusicSetLooping(lua_State *L)
{
    LuaMusic *ud = CheckMusic(L, 1);
    bool looping = false;
    if (lua_istable(L, 2))
    {
        looping = GetTableBoolFieldReq(L, 2, "looping", "music.setLooping");
    }
    else
    {
        looping = lua_toboolean(L, 2);
    }
    ud->music.SetLooping(looping);
    return 0;
}

int LuaMusicSetVolume(lua_State *L)
{
    LuaMusic *ud = CheckMusic(L, 1);
    float volume = 0.0f;
    if (lua_istable(L, 2))
    {
        volume = GetTableNumberFieldReq(L, 2, "volume", "music.setVolume");
    }
    else
    {
        volume = static_cast<float>(luaL_checknumber(L, 2));
    }
    ud->music.SetVolume(volume);
    return 0;
}

int LuaMusicSetPitch(lua_State *L)
{
    LuaMusic *ud = CheckMusic(L, 1);
    float pitch = 0.0f;
    if (lua_istable(L, 2))
    {
        pitch = GetTableNumberFieldReq(L, 2, "pitch", "music.setPitch");
    }
    else
    {
        pitch = static_cast<float>(luaL_checknumber(L, 2));
    }
    ud->music.SetPitch(pitch);
    return 0;
}

int LuaKeyboardIsDown(lua_State *L)
{
    LuaKeyboard *ud = CheckKeyboard(L, 1);
    const char *name = luaL_checkstring(L, 2);
    engine::Key key = ParseKey(name);
    lua_pushboolean(L, ud->state.IsDown(key));
    return 1;
}

int LuaKeyboardIsPressed(lua_State *L)
{
    LuaKeyboard *ud = CheckKeyboard(L, 1);
    const char *name = luaL_checkstring(L, 2);
    engine::Key key = ParseKey(name);
    lua_pushboolean(L, ud->state.IsPressed(key));
    return 1;
}

int LuaKeyboardIsReleased(lua_State *L)
{
    LuaKeyboard *ud = CheckKeyboard(L, 1);
    const char *name = luaL_checkstring(L, 2);
    engine::Key key = ParseKey(name);
    lua_pushboolean(L, ud->state.IsReleased(key));
    return 1;
}

int LuaMouseIsDown(lua_State *L)
{
    LuaMouse *ud = CheckMouse(L, 1);
    const char *name = luaL_checkstring(L, 2);
    engine::MouseButton button = ParseMouseButton(name);
    lua_pushboolean(L, ud->state.IsButtonDown(button));
    return 1;
}

int LuaMouseIsPressed(lua_State *L)
{
    LuaMouse *ud = CheckMouse(L, 1);
    const char *name = luaL_checkstring(L, 2);
    engine::MouseButton button = ParseMouseButton(name);
    lua_pushboolean(L, ud->state.IsButtonPressed(button));
    return 1;
}

int LuaMouseIsReleased(lua_State *L)
{
    LuaMouse *ud = CheckMouse(L, 1);
    const char *name = luaL_checkstring(L, 2);
    engine::MouseButton button = ParseMouseButton(name);
    lua_pushboolean(L, ud->state.IsButtonReleased(button));
    return 1;
}

int LuaMouseSetCursorVisible(lua_State *L)
{
    int arg_index = 1;
    if (!lua_isboolean(L, 1))
    {
        if (lua_isboolean(L, 2))
        {
            arg_index = 2;
        }
        else
        {
            return luaL_error(L, "setCursorVisible requires a boolean");
        }
    }
    bool visible = lua_toboolean(L, arg_index);
    if (visible)
    {
        SDL_ShowCursor();
    }
    else
    {
        SDL_HideCursor();
    }
    return 0;
}

int LuaMouseIndex(lua_State *L)
{
    LuaMouse *ud = CheckMouse(L, 1);
    const char *key = luaL_checkstring(L, 2);
    if (SDL_strcmp(key, "x") == 0)
    {
        lua_pushnumber(L, ud->state.GetX());
        return 1;
    }
    if (SDL_strcmp(key, "y") == 0)
    {
        lua_pushnumber(L, ud->state.GetY());
        return 1;
    }
    if (SDL_strcmp(key, "dx") == 0)
    {
        lua_pushnumber(L, ud->state.GetDeltaX());
        return 1;
    }
    if (SDL_strcmp(key, "dy") == 0)
    {
        lua_pushnumber(L, ud->state.GetDeltaY());
        return 1;
    }
    if (SDL_strcmp(key, "wheelX") == 0)
    {
        lua_pushnumber(L, ud->state.GetWheelX());
        return 1;
    }
    if (SDL_strcmp(key, "wheelY") == 0)
    {
        lua_pushnumber(L, ud->state.GetWheelY());
        return 1;
    }

    lua_pushvalue(L, lua_upvalueindex(1));
    lua_getfield(L, -1, key);
    return 1;
}

int LuaGamepadIsConnected(lua_State *L)
{
    LuaGamepad *ud = CheckGamepad(L, 1);
    lua_pushboolean(L, ud->state.IsConnected());
    return 1;
}

int LuaGamepadIsDown(lua_State *L)
{
    LuaGamepad *ud = CheckGamepad(L, 1);
    const char *name = luaL_checkstring(L, 2);
    engine::GamepadButton button = ParseGamepadButton(name);
    lua_pushboolean(L, ud->state.IsButtonDown(button));
    return 1;
}

int LuaGamepadIsPressed(lua_State *L)
{
    LuaGamepad *ud = CheckGamepad(L, 1);
    const char *name = luaL_checkstring(L, 2);
    engine::GamepadButton button = ParseGamepadButton(name);
    lua_pushboolean(L, ud->state.IsButtonPressed(button));
    return 1;
}

int LuaGamepadIsReleased(lua_State *L)
{
    LuaGamepad *ud = CheckGamepad(L, 1);
    const char *name = luaL_checkstring(L, 2);
    engine::GamepadButton button = ParseGamepadButton(name);
    lua_pushboolean(L, ud->state.IsButtonReleased(button));
    return 1;
}

int LuaGamepadAxis(lua_State *L)
{
    LuaGamepad *ud = CheckGamepad(L, 1);
    const char *name = luaL_checkstring(L, 2);
    engine::GamepadAxis axis = ParseGamepadAxis(name);
    lua_pushnumber(L, ud->state.GetAxis(axis));
    return 1;
}

int LuaGamepadAxisDown(lua_State *L)
{
    LuaGamepad *ud = CheckGamepad(L, 1);
    const char *axis_name = luaL_checkstring(L, 2);
    float threshold = static_cast<float>(luaL_checknumber(L, 3));
    const char *dir_name = luaL_checkstring(L, 4);
    engine::GamepadAxis axis = ParseGamepadAxis(axis_name);
    engine::AxisDirection dir = ParseAxisDirection(dir_name);
    lua_pushboolean(L, ud->state.IsAxisDown(axis, threshold, dir));
    return 1;
}

int LuaGamepadAxisPressed(lua_State *L)
{
    LuaGamepad *ud = CheckGamepad(L, 1);
    const char *axis_name = luaL_checkstring(L, 2);
    float threshold = static_cast<float>(luaL_checknumber(L, 3));
    const char *dir_name = luaL_checkstring(L, 4);
    engine::GamepadAxis axis = ParseGamepadAxis(axis_name);
    engine::AxisDirection dir = ParseAxisDirection(dir_name);
    lua_pushboolean(L, ud->state.IsAxisPressed(axis, threshold, dir));
    return 1;
}

int LuaGamepadAxisReleased(lua_State *L)
{
    LuaGamepad *ud = CheckGamepad(L, 1);
    const char *axis_name = luaL_checkstring(L, 2);
    float threshold = static_cast<float>(luaL_checknumber(L, 3));
    const char *dir_name = luaL_checkstring(L, 4);
    engine::GamepadAxis axis = ParseGamepadAxis(axis_name);
    engine::AxisDirection dir = ParseAxisDirection(dir_name);
    lua_pushboolean(L, ud->state.IsAxisReleased(axis, threshold, dir));
    return 1;
}

int LuaLogDebug(lua_State *L)
{
    const char *message = luaL_checkstring(L, 1);
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "%s", message);
    return 0;
}

int LuaLogInfo(lua_State *L)
{
    const char *message = luaL_checkstring(L, 1);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s", message);
    return 0;
}

int LuaLogWarn(lua_State *L)
{
    const char *message = luaL_checkstring(L, 1);
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s", message);
    return 0;
}

int LuaLogError(lua_State *L)
{
    const char *message = luaL_checkstring(L, 1);
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", message);
    return 0;
}

int LuaLogFatal(lua_State *L)
{
    const char *message = luaL_checkstring(L, 1);
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s", message);
    return 0;
}

int LuaTimeTicks(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    lua_pushinteger(L, runtime->GetTickIndex());
    return 1;
}

int LuaTimeTickDelta(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    lua_pushnumber(L, runtime->GetTickDt());
    return 1;
}

int LuaTimeNow(lua_State *L)
{
    Uint64 counter = SDL_GetPerformanceCounter();
    Uint64 freq = SDL_GetPerformanceFrequency();
    double seconds = freq > 0 ? static_cast<double>(counter) / static_cast<double>(freq) : 0.0;
    lua_pushnumber(L, seconds);
    return 1;
}

int LuaFsRead(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    const char *path = nullptr;
    if (lua_istable(L, 1))
    {
        path = GetTableStringFieldReq(L, 1, "path", "leo.fs.read");
    }
    else
    {
        path = luaL_checkstring(L, 1);
    }
    void *data = nullptr;
    size_t size = 0;

    try
    {
        runtime->GetVfs().ReadAll(path, &data, &size);
    }
    catch (const std::exception &e)
    {
        if (data)
        {
            SDL_free(data);
        }
        return luaL_error(L, "%s", e.what());
    }

    lua_pushlstring(L, static_cast<const char *>(data), size);
    SDL_free(data);
    return 1;
}

int LuaFsReadWriteDir(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    const char *path = nullptr;
    if (lua_istable(L, 1))
    {
        path = GetTableStringFieldReq(L, 1, "path", "leo.fs.readWriteDir");
    }
    else
    {
        path = luaL_checkstring(L, 1);
    }

    void *data = nullptr;
    size_t size = 0;

    try
    {
        runtime->GetVfs().ReadAllWriteDir(path, &data, &size);
    }
    catch (const std::exception &e)
    {
        if (data)
        {
            SDL_free(data);
        }
        return luaL_error(L, "%s", e.what());
    }

    lua_pushlstring(L, static_cast<const char *>(data), size);
    SDL_free(data);
    return 1;
}

int LuaFsWrite(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    const char *path = nullptr;
    const char *data = nullptr;
    size_t size = 0;

    if (lua_istable(L, 1))
    {
        path = GetTableStringFieldReq(L, 1, "path", "leo.fs.write");
        int idx = lua_absindex(L, 1);
        lua_getfield(L, idx, "data");
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1);
            return luaL_error(L, "leo.fs.write requires 'data'");
        }
        data = luaL_checklstring(L, -1, &size);
        lua_pop(L, 1);
    }
    else
    {
        path = luaL_checkstring(L, 1);
        data = luaL_checklstring(L, 2, &size);
    }

    try
    {
        runtime->GetVfs().WriteAll(path, data, size);
    }
    catch (const std::exception &e)
    {
        return luaL_error(L, "%s", e.what());
    }

    return 0;
}

int LuaFsGetWriteDir(lua_State *L)
{
    const char *dir = PHYSFS_getWriteDir();
    if (!dir || !*dir)
    {
        lua_pushnil(L);
        return 1;
    }
    lua_pushstring(L, dir);
    return 1;
}

int LuaFsListWriteDir(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    const char *path = "";

    if (lua_istable(L, 1))
    {
        if (TableHasField(L, 1, "path"))
        {
            path = GetTableStringFieldReq(L, 1, "path", "leo.fs.listWriteDir");
        }
    }
    else if (!lua_isnoneornil(L, 1))
    {
        path = luaL_checkstring(L, 1);
    }

    char **entries = nullptr;
    try
    {
        runtime->GetVfs().ListWriteDir(path, &entries);
    }
    catch (const std::exception &e)
    {
        return luaL_error(L, "%s", e.what());
    }

    lua_newtable(L);
    if (entries)
    {
        for (int i = 0; entries[i]; ++i)
        {
            lua_pushstring(L, entries[i]);
            lua_rawseti(L, -2, i + 1);
        }
        runtime->GetVfs().FreeList(entries);
    }

    return 1;
}

int LuaFsListWriteDirFiles(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    char **entries = nullptr;
    try
    {
        runtime->GetVfs().ListWriteDirFiles(&entries);
    }
    catch (const std::exception &e)
    {
        return luaL_error(L, "%s", e.what());
    }

    lua_newtable(L);
    if (entries)
    {
        for (int i = 0; entries[i]; ++i)
        {
            lua_pushstring(L, entries[i]);
            lua_rawseti(L, -2, i + 1);
        }
        runtime->GetVfs().FreeList(entries);
    }

    return 1;
}

int LuaFsDeleteFile(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    const char *path = nullptr;
    if (lua_istable(L, 1))
    {
        path = GetTableStringFieldReq(L, 1, "path", "leo.fs.deleteFile");
    }
    else
    {
        path = luaL_checkstring(L, 1);
    }

    try
    {
        runtime->GetVfs().DeleteFile(path);
    }
    catch (const std::exception &e)
    {
        return luaL_error(L, "%s", e.what());
    }
    return 0;
}

int LuaFsDeleteDir(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    const char *path = nullptr;
    if (lua_istable(L, 1))
    {
        path = GetTableStringFieldReq(L, 1, "path", "leo.fs.deleteDir");
    }
    else
    {
        path = luaL_checkstring(L, 1);
    }

    try
    {
        runtime->GetVfs().DeleteDirRecursive(path);
    }
    catch (const std::exception &e)
    {
        return luaL_error(L, "%s", e.what());
    }
    return 0;
}

int LuaQuit(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    runtime->RequestQuit();
    return 0;
}

void RegisterTextureMeta(lua_State *L)
{
    luaL_newmetatable(L, kTextureMeta);
    lua_pushcfunction(L, LuaTextureGc);
    lua_setfield(L, -2, "__gc");

    lua_newtable(L);
    lua_pushcfunction(L, LuaTextureGetSize);
    lua_setfield(L, -2, "getSize");
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

void RegisterTiledMapMeta(lua_State *L)
{
    luaL_newmetatable(L, kTiledMapMeta);
    lua_pushcfunction(L, LuaTiledMapGc);
    lua_setfield(L, -2, "__gc");

    lua_newtable(L);
    lua_pushcfunction(L, LuaTiledMapDraw);
    lua_setfield(L, -2, "draw");
    lua_pushcfunction(L, LuaTiledMapDrawLayer);
    lua_setfield(L, -2, "drawLayer");
    lua_pushcfunction(L, LuaTiledMapGetSize);
    lua_setfield(L, -2, "getSize");
    lua_pushcfunction(L, LuaTiledMapGetTileSize);
    lua_setfield(L, -2, "getTileSize");
    lua_pushcfunction(L, LuaTiledMapGetPixelSize);
    lua_setfield(L, -2, "getPixelSize");
    lua_pushcfunction(L, LuaTiledMapGetLayerCount);
    lua_setfield(L, -2, "getLayerCount");
    lua_pushcfunction(L, LuaTiledMapGetLayerName);
    lua_setfield(L, -2, "getLayerName");
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

void RegisterAnimationMeta(lua_State *L)
{
    luaL_newmetatable(L, kAnimationMeta);
    lua_pushcfunction(L, LuaAnimationGc);
    lua_setfield(L, -2, "__gc");

    lua_newtable(L);
    lua_pushcfunction(L, LuaAnimationAddFrame);
    lua_setfield(L, -2, "addFrame");
    lua_pushcfunction(L, LuaAnimationPlay);
    lua_setfield(L, -2, "play");
    lua_pushcfunction(L, LuaAnimationPause);
    lua_setfield(L, -2, "pause");
    lua_pushcfunction(L, LuaAnimationResume);
    lua_setfield(L, -2, "resume");
    lua_pushcfunction(L, LuaAnimationRestart);
    lua_setfield(L, -2, "restart");
    lua_pushcfunction(L, LuaAnimationIsPlaying);
    lua_setfield(L, -2, "isPlaying");
    lua_pushcfunction(L, LuaAnimationSetLooping);
    lua_setfield(L, -2, "setLooping");
    lua_pushcfunction(L, LuaAnimationSetSpeed);
    lua_setfield(L, -2, "setSpeed");
    lua_pushcfunction(L, LuaAnimationUpdate);
    lua_setfield(L, -2, "update");
    lua_pushcfunction(L, LuaAnimationDraw);
    lua_setfield(L, -2, "draw");
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

void RegisterFontMeta(lua_State *L)
{
    luaL_newmetatable(L, kFontMeta);
    lua_pushcfunction(L, LuaFontGc);
    lua_setfield(L, -2, "__gc");

    lua_newtable(L);
    lua_pushcfunction(L, LuaFontPrint);
    lua_setfield(L, -2, "print");
    lua_pushcfunction(L, LuaFontGetLineHeight);
    lua_setfield(L, -2, "getLineHeight");
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

void RegisterSoundMeta(lua_State *L)
{
    luaL_newmetatable(L, kSoundMeta);
    lua_pushcfunction(L, LuaSoundGc);
    lua_setfield(L, -2, "__gc");

    lua_newtable(L);
    lua_pushcfunction(L, LuaSoundPlay);
    lua_setfield(L, -2, "play");
    lua_pushcfunction(L, LuaSoundPause);
    lua_setfield(L, -2, "pause");
    lua_pushcfunction(L, LuaSoundStop);
    lua_setfield(L, -2, "stop");
    lua_pushcfunction(L, LuaSoundIsPlaying);
    lua_setfield(L, -2, "isPlaying");
    lua_pushcfunction(L, LuaSoundSetLooping);
    lua_setfield(L, -2, "setLooping");
    lua_pushcfunction(L, LuaSoundSetVolume);
    lua_setfield(L, -2, "setVolume");
    lua_pushcfunction(L, LuaSoundSetPitch);
    lua_setfield(L, -2, "setPitch");
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

void RegisterMusicMeta(lua_State *L)
{
    luaL_newmetatable(L, kMusicMeta);
    lua_pushcfunction(L, LuaMusicGc);
    lua_setfield(L, -2, "__gc");

    lua_newtable(L);
    lua_pushcfunction(L, LuaMusicPlay);
    lua_setfield(L, -2, "play");
    lua_pushcfunction(L, LuaMusicPause);
    lua_setfield(L, -2, "pause");
    lua_pushcfunction(L, LuaMusicStop);
    lua_setfield(L, -2, "stop");
    lua_pushcfunction(L, LuaMusicIsPlaying);
    lua_setfield(L, -2, "isPlaying");
    lua_pushcfunction(L, LuaMusicSetLooping);
    lua_setfield(L, -2, "setLooping");
    lua_pushcfunction(L, LuaMusicSetVolume);
    lua_setfield(L, -2, "setVolume");
    lua_pushcfunction(L, LuaMusicSetPitch);
    lua_setfield(L, -2, "setPitch");
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

void RegisterKeyboardMeta(lua_State *L)
{
    luaL_newmetatable(L, kKeyboardMeta);
    lua_newtable(L);
    lua_pushcfunction(L, LuaKeyboardIsDown);
    lua_setfield(L, -2, "isDown");
    lua_pushcfunction(L, LuaKeyboardIsPressed);
    lua_setfield(L, -2, "isPressed");
    lua_pushcfunction(L, LuaKeyboardIsReleased);
    lua_setfield(L, -2, "isReleased");
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

void RegisterMouseMeta(lua_State *L)
{
    luaL_newmetatable(L, kMouseMeta);

    lua_newtable(L);
    lua_pushcfunction(L, LuaMouseIsDown);
    lua_setfield(L, -2, "isDown");
    lua_pushcfunction(L, LuaMouseIsPressed);
    lua_setfield(L, -2, "isPressed");
    lua_pushcfunction(L, LuaMouseIsReleased);
    lua_setfield(L, -2, "isReleased");
    lua_pushcfunction(L, LuaMouseSetCursorVisible);
    lua_setfield(L, -2, "setCursorVisible");

    lua_pushvalue(L, -1);
    lua_pushcclosure(L, LuaMouseIndex, 1);
    lua_setfield(L, -3, "__index");
    lua_pop(L, 2);
}

void RegisterMouseModule(lua_State *L)
{
    lua_newtable(L);
    lua_pushcfunction(L, LuaMouseSetCursorVisible);
    lua_setfield(L, -2, "setCursorVisible");
}

void RegisterGamepadMeta(lua_State *L)
{
    luaL_newmetatable(L, kGamepadMeta);
    lua_newtable(L);
    lua_pushcfunction(L, LuaGamepadIsConnected);
    lua_setfield(L, -2, "isConnected");
    lua_pushcfunction(L, LuaGamepadIsDown);
    lua_setfield(L, -2, "isDown");
    lua_pushcfunction(L, LuaGamepadIsPressed);
    lua_setfield(L, -2, "isPressed");
    lua_pushcfunction(L, LuaGamepadIsReleased);
    lua_setfield(L, -2, "isReleased");
    lua_pushcfunction(L, LuaGamepadAxis);
    lua_setfield(L, -2, "axis");
    lua_pushcfunction(L, LuaGamepadAxisDown);
    lua_setfield(L, -2, "isAxisDown");
    lua_pushcfunction(L, LuaGamepadAxisPressed);
    lua_setfield(L, -2, "isAxisPressed");
    lua_pushcfunction(L, LuaGamepadAxisReleased);
    lua_setfield(L, -2, "isAxisReleased");
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

void RegisterGraphics(lua_State *L)
{
    lua_newtable(L);
    lua_pushcfunction(L, LuaGraphicsNewImage);
    lua_setfield(L, -2, "newImage");
    lua_pushcfunction(L, LuaGraphicsDraw);
    lua_setfield(L, -2, "draw");
    lua_pushcfunction(L, LuaGraphicsDrawEx);
    lua_setfield(L, -2, "drawEx");
    lua_pushcfunction(L, LuaGraphicsSetColor);
    lua_setfield(L, -2, "setColor");
    lua_pushcfunction(L, LuaGraphicsClear);
    lua_setfield(L, -2, "clear");
    lua_pushcfunction(L, LuaGraphicsGetSize);
    lua_setfield(L, -2, "getSize");
    lua_pushcfunction(L, LuaGraphicsBeginViewport);
    lua_setfield(L, -2, "beginViewport");
    lua_pushcfunction(L, LuaGraphicsEndViewport);
    lua_setfield(L, -2, "endViewport");
    lua_pushcfunction(L, LuaGraphicsDrawGrid);
    lua_setfield(L, -2, "drawGrid");
    lua_pushcfunction(L, LuaGraphicsDrawPixel);
    lua_setfield(L, -2, "drawPixel");
    lua_pushcfunction(L, LuaGraphicsDrawLine);
    lua_setfield(L, -2, "drawLine");
    lua_pushcfunction(L, LuaGraphicsDrawCircleFilled);
    lua_setfield(L, -2, "drawCircleFilled");
    lua_pushcfunction(L, LuaGraphicsDrawCircleOutline);
    lua_setfield(L, -2, "drawCircleOutline");
    lua_pushcfunction(L, LuaGraphicsDrawRectangleFilled);
    lua_setfield(L, -2, "drawRectangleFilled");
    lua_pushcfunction(L, LuaGraphicsDrawRectangleOutline);
    lua_setfield(L, -2, "drawRectangleOutline");
    lua_pushcfunction(L, LuaGraphicsDrawRectangleRoundedFilled);
    lua_setfield(L, -2, "drawRectangleRoundedFilled");
    lua_pushcfunction(L, LuaGraphicsDrawRectangleRoundedOutline);
    lua_setfield(L, -2, "drawRectangleRoundedOutline");
    lua_pushcfunction(L, LuaGraphicsDrawTriangleFilled);
    lua_setfield(L, -2, "drawTriangleFilled");
    lua_pushcfunction(L, LuaGraphicsDrawTriangleOutline);
    lua_setfield(L, -2, "drawTriangleOutline");
    lua_pushcfunction(L, LuaGraphicsDrawPolyFilled);
    lua_setfield(L, -2, "drawPolyFilled");
    lua_pushcfunction(L, LuaGraphicsDrawPolyOutline);
    lua_setfield(L, -2, "drawPolyOutline");
    lua_pushcfunction(L, LuaGraphicsBeginCamera);
    lua_setfield(L, -2, "beginCamera");
    lua_pushcfunction(L, LuaGraphicsEndCamera);
    lua_setfield(L, -2, "endCamera");
}

void RegisterWindow(lua_State *L)
{
    lua_newtable(L);
    lua_pushcfunction(L, LuaWindowSetSize);
    lua_setfield(L, -2, "setSize");
    lua_pushcfunction(L, LuaWindowSetMode);
    lua_setfield(L, -2, "setMode");
    lua_pushcfunction(L, LuaWindowGetSize);
    lua_setfield(L, -2, "getSize");
}

void RegisterFont(lua_State *L)
{
    lua_newtable(L);
    lua_pushcfunction(L, LuaFontNew);
    lua_setfield(L, -2, "new");
    lua_pushcfunction(L, LuaFontSet);
    lua_setfield(L, -2, "set");
    lua_pushcfunction(L, LuaFontPrintCurrent);
    lua_setfield(L, -2, "print");
}

void RegisterAudio(lua_State *L)
{
    lua_newtable(L);
    lua_pushcfunction(L, LuaSoundNew);
    lua_setfield(L, -2, "newSound");
    lua_pushcfunction(L, LuaMusicNew);
    lua_setfield(L, -2, "newMusic");
}

void RegisterAnimation(lua_State *L)
{
    lua_newtable(L);
    lua_pushcfunction(L, LuaAnimationNew);
    lua_setfield(L, -2, "new");
    lua_pushcfunction(L, LuaAnimationNewFromTexture);
    lua_setfield(L, -2, "newFromTexture");
    lua_pushcfunction(L, LuaAnimationNewSheet);
    lua_setfield(L, -2, "newSheet");
    lua_pushcfunction(L, LuaAnimationNewSheetEx);
    lua_setfield(L, -2, "newSheetEx");
}

void RegisterLog(lua_State *L)
{
    lua_newtable(L);
    lua_pushcfunction(L, LuaLogDebug);
    lua_setfield(L, -2, "debug");
    lua_pushcfunction(L, LuaLogInfo);
    lua_setfield(L, -2, "info");
    lua_pushcfunction(L, LuaLogWarn);
    lua_setfield(L, -2, "warn");
    lua_pushcfunction(L, LuaLogError);
    lua_setfield(L, -2, "error");
    lua_pushcfunction(L, LuaLogFatal);
    lua_setfield(L, -2, "fatal");
}

int LuaMathClamp(lua_State *L)
{
    float value = static_cast<float>(luaL_checknumber(L, 1));
    float min_value = static_cast<float>(luaL_checknumber(L, 2));
    float max_value = static_cast<float>(luaL_checknumber(L, 3));
    if (min_value > max_value)
    {
        std::swap(min_value, max_value);
    }
    float clamped = value < min_value ? min_value : (value > max_value ? max_value : value);
    lua_pushnumber(L, clamped);
    return 1;
}

int LuaMathClamp01(lua_State *L)
{
    float value = static_cast<float>(luaL_checknumber(L, 1));
    float clamped = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
    lua_pushnumber(L, clamped);
    return 1;
}

void RegisterMath(lua_State *L)
{
    lua_newtable(L);
    lua_pushcfunction(L, LuaMathClamp);
    lua_setfield(L, -2, "clamp");
    lua_pushcfunction(L, LuaMathClamp01);
    lua_setfield(L, -2, "clamp01");
}

void RegisterTime(lua_State *L)
{
    lua_newtable(L);
    lua_pushcfunction(L, LuaTimeTicks);
    lua_setfield(L, -2, "ticks");
    lua_pushcfunction(L, LuaTimeTickDelta);
    lua_setfield(L, -2, "tickDelta");
    lua_pushcfunction(L, LuaTimeNow);
    lua_setfield(L, -2, "now");
}

void RegisterFs(lua_State *L)
{
    lua_newtable(L);
    lua_pushcfunction(L, LuaFsRead);
    lua_setfield(L, -2, "read");
    lua_pushcfunction(L, LuaFsReadWriteDir);
    lua_setfield(L, -2, "readWriteDir");
    lua_pushcfunction(L, LuaFsWrite);
    lua_setfield(L, -2, "write");
    lua_pushcfunction(L, LuaFsGetWriteDir);
    lua_setfield(L, -2, "getWriteDir");
    lua_pushcfunction(L, LuaFsListWriteDir);
    lua_setfield(L, -2, "listWriteDir");
    lua_pushcfunction(L, LuaFsListWriteDirFiles);
    lua_setfield(L, -2, "listWriteDirFiles");
    lua_pushcfunction(L, LuaFsDeleteFile);
    lua_setfield(L, -2, "deleteFile");
    lua_pushcfunction(L, LuaFsDeleteDir);
    lua_setfield(L, -2, "deleteDir");
}

void RegisterTiled(lua_State *L)
{
    lua_newtable(L);
    lua_pushcfunction(L, LuaTiledLoad);
    lua_setfield(L, -2, "load");
}

void RegisterCamera(lua_State *L)
{
    luaL_newmetatable(L, kCameraMeta);
    lua_newtable(L);
    lua_pushcfunction(L, LuaCameraSetTarget);
    lua_setfield(L, -2, "setTarget");
    lua_pushcfunction(L, LuaCameraSetPosition);
    lua_setfield(L, -2, "setPosition");
    lua_pushcfunction(L, LuaCameraSetOffset);
    lua_setfield(L, -2, "setOffset");
    lua_pushcfunction(L, LuaCameraSetDeadzone);
    lua_setfield(L, -2, "setDeadzone");
    lua_pushcfunction(L, LuaCameraSetSmoothTime);
    lua_setfield(L, -2, "setSmoothTime");
    lua_pushcfunction(L, LuaCameraSetBounds);
    lua_setfield(L, -2, "setBounds");
    lua_pushcfunction(L, LuaCameraSetClamp);
    lua_setfield(L, -2, "setClamp");
    lua_pushcfunction(L, LuaCameraSetZoom);
    lua_setfield(L, -2, "setZoom");
    lua_pushcfunction(L, LuaCameraSetRotation);
    lua_setfield(L, -2, "setRotation");
    lua_pushcfunction(L, LuaCameraUpdate);
    lua_setfield(L, -2, "update");
    lua_pushcfunction(L, LuaCameraWorldToScreen);
    lua_setfield(L, -2, "worldToScreen");
    lua_pushcfunction(L, LuaCameraScreenToWorld);
    lua_setfield(L, -2, "screenToWorld");
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    lua_newtable(L);
    lua_pushcfunction(L, LuaCameraNew);
    lua_setfield(L, -2, "new");
}

void RegisterCollision(lua_State *L)
{
    lua_newtable(L);
    lua_pushcfunction(L, LuaCollisionCheckRecs);
    lua_setfield(L, -2, "checkRecs");
    lua_pushcfunction(L, LuaCollisionCheckCircles);
    lua_setfield(L, -2, "checkCircles");
    lua_pushcfunction(L, LuaCollisionCheckCircleRec);
    lua_setfield(L, -2, "checkCircleRec");
    lua_pushcfunction(L, LuaCollisionCheckCircleLine);
    lua_setfield(L, -2, "checkCircleLine");
    lua_pushcfunction(L, LuaCollisionCheckPointRec);
    lua_setfield(L, -2, "checkPointRec");
    lua_pushcfunction(L, LuaCollisionCheckPointCircle);
    lua_setfield(L, -2, "checkPointCircle");
    lua_pushcfunction(L, LuaCollisionCheckPointTriangle);
    lua_setfield(L, -2, "checkPointTriangle");
    lua_pushcfunction(L, LuaCollisionCheckPointLine);
    lua_setfield(L, -2, "checkPointLine");
    lua_pushcfunction(L, LuaCollisionCheckPointPoly);
    lua_setfield(L, -2, "checkPointPoly");
    lua_pushcfunction(L, LuaCollisionCheckLines);
    lua_setfield(L, -2, "checkLines");
}

void PushKeyboard(lua_State *L, const engine::KeyboardState &state)
{
    LuaKeyboard *ud = static_cast<LuaKeyboard *>(lua_newuserdata(L, sizeof(LuaKeyboard)));
    ud->state = state;
    luaL_getmetatable(L, kKeyboardMeta);
    lua_setmetatable(L, -2);
}

void PushMouse(lua_State *L, const engine::MouseState &state)
{
    LuaMouse *ud = static_cast<LuaMouse *>(lua_newuserdata(L, sizeof(LuaMouse)));
    ud->state = state;
    luaL_getmetatable(L, kMouseMeta);
    lua_setmetatable(L, -2);
}

void PushGamepad(lua_State *L, const engine::GamepadState &state)
{
    LuaGamepad *ud = static_cast<LuaGamepad *>(lua_newuserdata(L, sizeof(LuaGamepad)));
    ud->state = state;
    luaL_getmetatable(L, kGamepadMeta);
    lua_setmetatable(L, -2);
}

void PushInputFrame(lua_State *L, const leo::Engine::InputFrame &input)
{
    lua_newtable(L);
    lua_pushboolean(L, input.quit_requested);
    lua_setfield(L, -2, "quit");
    lua_pushinteger(L, input.frame_index);
    lua_setfield(L, -2, "frame");

    PushKeyboard(L, input.keyboard);
    lua_setfield(L, -2, "keyboard");

    PushMouse(L, input.mouse);
    lua_setfield(L, -2, "mouse");

    lua_newtable(L);
    for (int i = 0; i < 2; ++i)
    {
        PushGamepad(L, input.gamepads[i]);
        lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "gamepads");
}

void RegisterLeo(lua_State *L)
{
    RegisterTextureMeta(L);
    RegisterTiledMapMeta(L);
    RegisterAnimationMeta(L);
    RegisterFontMeta(L);
    RegisterSoundMeta(L);
    RegisterMusicMeta(L);
    RegisterKeyboardMeta(L);
    RegisterMouseMeta(L);
    RegisterGamepadMeta(L);

    lua_newtable(L);

    RegisterGraphics(L);
    lua_setfield(L, -2, "graphics");

    RegisterWindow(L);
    lua_setfield(L, -2, "window");

    RegisterFont(L);
    lua_setfield(L, -2, "font");

    RegisterAudio(L);
    lua_setfield(L, -2, "audio");

    RegisterAnimation(L);
    lua_setfield(L, -2, "animation");

    RegisterLog(L);
    lua_setfield(L, -2, "log");

    RegisterMouseModule(L);
    lua_setfield(L, -2, "mouse");

    RegisterMath(L);
    lua_setfield(L, -2, "math");

    RegisterTime(L);
    lua_setfield(L, -2, "time");

    RegisterFs(L);
    lua_setfield(L, -2, "fs");

    RegisterTiled(L);
    lua_setfield(L, -2, "tiled");

    RegisterCamera(L);
    lua_setfield(L, -2, "camera");

    RegisterCollision(L);
    lua_setfield(L, -2, "collision");

    lua_pushcfunction(L, LuaQuit);
    lua_setfield(L, -2, "quit");

    lua_setglobal(L, "leo");
}

} // namespace

namespace engine
{

LuaRuntime::LuaRuntime() noexcept
    : L(nullptr), vfs(nullptr), window(nullptr), renderer(nullptr), config(nullptr), tick_index(0), tick_dt(0.0f),
      loaded(false), quit_requested(false), draw_color({255, 255, 255, 255}), active_camera(nullptr),
      window_mode(WindowMode::Windowed), current_font_ref(LUA_NOREF), current_font_ptr(nullptr), current_font_size(0)
{
}

LuaRuntime::~LuaRuntime()
{
    if (L)
    {
        lua_close(L);
        L = nullptr;
    }
}

void LuaRuntime::Init(VFS &vfs_ref, SDL_Window *window_ref, SDL_Renderer *renderer_ref, const engine::Config &cfg)
{
    vfs = &vfs_ref;
    window = window_ref;
    renderer = renderer_ref;
    config = &cfg;
    window_mode = cfg.window_mode;

    L = luaL_newstate();
    if (!L)
    {
        throw std::runtime_error("Failed to create Lua state");
    }

    luaL_openlibs(L);

    lua_pushlightuserdata(L, this);
    lua_setfield(L, LUA_REGISTRYINDEX, kRuntimeRegistryKey);

    RegisterLeo(L);
}

void LuaRuntime::LoadScript(const char *vfs_path)
{
    if (!vfs_path || !*vfs_path)
    {
        throw std::runtime_error("LuaRuntime requires a script path");
    }

    void *data = nullptr;
    size_t size = 0;
    vfs->ReadAll(vfs_path, &data, &size);
    if (!data || size == 0)
    {
        if (data)
        {
            SDL_free(data);
        }
        throw std::runtime_error("LuaRuntime received empty script buffer");
    }

    std::string chunk_name = std::string("@") + vfs_path;
    int load_result = luaL_loadbuffer(L, static_cast<const char *>(data), size, chunk_name.c_str());
    SDL_free(data);
    if (load_result != LUA_OK)
    {
        std::string error = lua_tostring(L, -1);
        lua_pop(L, 1);
        throw std::runtime_error(error);
    }

    if (lua_pcall(L, 0, 0, 0) != LUA_OK)
    {
        std::string error = lua_tostring(L, -1);
        lua_pop(L, 1);
        throw std::runtime_error(error);
    }

    loaded = true;
}

void LuaRuntime::SetFrameInfo(Uint32 tick, float dt)
{
    tick_index = tick;
    tick_dt = dt;
}

void LuaRuntime::CallLoad()
{
    if (!loaded)
    {
        return;
    }

    lua_getglobal(L, "leo");
    lua_getfield(L, -1, "load");
    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 2);
        return;
    }

    lua_remove(L, -2);
    if (lua_pcall(L, 0, 0, 0) != LUA_OK)
    {
        std::string error = lua_tostring(L, -1);
        lua_pop(L, 1);
        throw std::runtime_error(error);
    }
}

void LuaRuntime::CallUpdate(float dt, const ::leo::Engine::InputFrame &input)
{
    if (!loaded)
    {
        return;
    }

    lua_getglobal(L, "leo");
    lua_getfield(L, -1, "update");
    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 2);
        return;
    }

    lua_remove(L, -2);
    lua_pushnumber(L, dt);
    PushInputFrame(L, input);
    if (lua_pcall(L, 2, 0, 0) != LUA_OK)
    {
        std::string error = lua_tostring(L, -1);
        lua_pop(L, 1);
        throw std::runtime_error(error);
    }
}

void LuaRuntime::CallDraw()
{
    if (!loaded)
    {
        return;
    }

    lua_getglobal(L, "leo");
    lua_getfield(L, -1, "draw");
    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 2);
        return;
    }

    lua_remove(L, -2);
    if (lua_pcall(L, 0, 0, 0) != LUA_OK)
    {
        std::string error = lua_tostring(L, -1);
        lua_pop(L, 1);
        throw std::runtime_error(error);
    }
}

void LuaRuntime::CallShutdown()
{
    if (!loaded)
    {
        return;
    }

    lua_getglobal(L, "leo");
    lua_getfield(L, -1, "shutdown");
    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 2);
        return;
    }

    lua_remove(L, -2);
    if (lua_pcall(L, 0, 0, 0) != LUA_OK)
    {
        std::string error = lua_tostring(L, -1);
        lua_pop(L, 1);
        throw std::runtime_error(error);
    }
}

bool LuaRuntime::WantsQuit() const noexcept
{
    return quit_requested;
}

void LuaRuntime::ClearQuitRequest() noexcept
{
    quit_requested = false;
}

bool LuaRuntime::IsLoaded() const noexcept
{
    return loaded;
}

void LuaRuntime::RequestQuit() noexcept
{
    quit_requested = true;
}

WindowMode LuaRuntime::GetWindowMode() const noexcept
{
    return window_mode;
}

void LuaRuntime::SetWindowMode(WindowMode mode) noexcept
{
    window_mode = mode;
}

const ::leo::Camera::Camera2D *LuaRuntime::GetActiveCamera() const noexcept
{
    return active_camera;
}

void LuaRuntime::SetActiveCamera(const ::leo::Camera::Camera2D *camera) noexcept
{
    active_camera = camera;
}

VFS &LuaRuntime::GetVfs() const
{
    return *vfs;
}

SDL_Window *LuaRuntime::GetWindow() const noexcept
{
    return window;
}

SDL_Renderer *LuaRuntime::GetRenderer() const noexcept
{
    return renderer;
}

const engine::Config *LuaRuntime::GetConfig() const noexcept
{
    return config;
}

Uint32 LuaRuntime::GetTickIndex() const noexcept
{
    return tick_index;
}

float LuaRuntime::GetTickDt() const noexcept
{
    return tick_dt;
}

SDL_Color LuaRuntime::GetDrawColor() const noexcept
{
    return draw_color;
}

void LuaRuntime::SetDrawColor(const SDL_Color &color) noexcept
{
    draw_color = color;
}

engine::Font *LuaRuntime::GetCurrentFont() const noexcept
{
    return current_font_ptr;
}

int LuaRuntime::GetCurrentFontSize() const noexcept
{
    return current_font_size;
}

int LuaRuntime::GetCurrentFontRef() const noexcept
{
    return current_font_ref;
}

void LuaRuntime::SetCurrentFont(engine::Font *font, int pixel_size, int ref) noexcept
{
    current_font_ptr = font;
    current_font_size = pixel_size;
    current_font_ref = ref;
}

void LuaRuntime::ClearCurrentFontRef(lua_State *state)
{
    if (current_font_ref != LUA_NOREF)
    {
        luaL_unref(state, LUA_REGISTRYINDEX, current_font_ref);
    }
    current_font_ref = LUA_NOREF;
    current_font_ptr = nullptr;
    current_font_size = 0;
}

} // namespace engine
