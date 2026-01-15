#include "leo/lua_runtime.h"
#include "leo/engine_core.h"
#include "leo/audio.h"
#include "leo/font.h"
#include "leo/gamepad.h"
#include "leo/keyboard.h"
#include "leo/mouse.h"
#include "leo/texture_loader.h"
#include "leo/vfs.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_stdinc.h>
#include <lua.hpp>
#include <stdexcept>
#include <string>

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

LuaTexture *CheckTexture(lua_State *L, int index)
{
    return static_cast<LuaTexture *>(luaL_checkudata(L, index, kTextureMeta));
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

    SDL_Color color = runtime->GetDrawColor();
    SDL_SetTextureColorMod(ud->texture.handle, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(ud->texture.handle, color.a);

    float w = static_cast<float>(ud->texture.width) * static_cast<float>(sx);
    float h = static_cast<float>(ud->texture.height) * static_cast<float>(sy);
    SDL_FRect dst = {static_cast<float>(x - ox * sx), static_cast<float>(y - oy * sy), w, h};
    SDL_FPoint center = {static_cast<float>(ox * sx), static_cast<float>(oy * sy)};
    constexpr double kRadToDeg = 57.29577951308232;
    double degrees = angle * kRadToDeg;

    SDL_RenderTextureRotated(runtime->GetRenderer(), ud->texture.handle, nullptr, &dst, degrees, &center,
                             SDL_FLIP_NONE);

    SDL_SetTextureColorMod(ud->texture.handle, 255, 255, 255);
    SDL_SetTextureAlphaMod(ud->texture.handle, 255);
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
        new (&ud->font)
            engine::Font(engine::Font::LoadFromVfs(runtime->GetVfs(), runtime->GetRenderer(), path, size));
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
    const char *text = luaL_checkstring(L, 2);
    float x = static_cast<float>(luaL_checknumber(L, 3));
    float y = static_cast<float>(luaL_checknumber(L, 4));
    int size = static_cast<int>(luaL_optinteger(L, 5, ud->pixel_size));

    SDL_Color color = runtime->GetDrawColor();
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
    if (!runtime->GetCurrentFont())
    {
        return luaL_error(L, "leo.font.print requires leo.font.set(font) first");
    }

    const char *text = luaL_checkstring(L, 1);
    float x = static_cast<float>(luaL_checknumber(L, 2));
    float y = static_cast<float>(luaL_checknumber(L, 3));

    engine::Font *font = runtime->GetCurrentFont();
    int pixel_size = runtime->GetCurrentFontSize();
    if (pixel_size <= 0)
    {
        pixel_size = font->GetLineHeight();
    }
    SDL_Color color = runtime->GetDrawColor();
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
    const char *path = luaL_checkstring(L, 1);
    try
    {
        LuaSound *ud = static_cast<LuaSound *>(lua_newuserdata(L, sizeof(LuaSound)));
        new (&ud->sound) engine::Sound(engine::Sound::LoadFromVfs(runtime->GetVfs(), path));
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
    bool looping = lua_toboolean(L, 2);
    ud->sound.SetLooping(looping);
    return 0;
}

int LuaSoundSetVolume(lua_State *L)
{
    LuaSound *ud = CheckSound(L, 1);
    float volume = static_cast<float>(luaL_checknumber(L, 2));
    ud->sound.SetVolume(volume);
    return 0;
}

int LuaSoundSetPitch(lua_State *L)
{
    LuaSound *ud = CheckSound(L, 1);
    float pitch = static_cast<float>(luaL_checknumber(L, 2));
    ud->sound.SetPitch(pitch);
    return 0;
}

int LuaMusicNew(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    const char *path = luaL_checkstring(L, 1);
    try
    {
        LuaMusic *ud = static_cast<LuaMusic *>(lua_newuserdata(L, sizeof(LuaMusic)));
        new (&ud->music) engine::Music(engine::Music::LoadFromVfs(runtime->GetVfs(), path));
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
    bool looping = lua_toboolean(L, 2);
    ud->music.SetLooping(looping);
    return 0;
}

int LuaMusicSetVolume(lua_State *L)
{
    LuaMusic *ud = CheckMusic(L, 1);
    float volume = static_cast<float>(luaL_checknumber(L, 2));
    ud->music.SetVolume(volume);
    return 0;
}

int LuaMusicSetPitch(lua_State *L)
{
    LuaMusic *ud = CheckMusic(L, 1);
    float pitch = static_cast<float>(luaL_checknumber(L, 2));
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

int LuaFsRead(lua_State *L)
{
    engine::LuaRuntime *runtime = GetRuntime(L);
    const char *path = luaL_checkstring(L, 1);
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

    lua_pushvalue(L, -1);
    lua_pushcclosure(L, LuaMouseIndex, 1);
    lua_setfield(L, -3, "__index");
    lua_pop(L, 2);
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
    lua_pushcfunction(L, LuaGraphicsSetColor);
    lua_setfield(L, -2, "setColor");
    lua_pushcfunction(L, LuaGraphicsClear);
    lua_setfield(L, -2, "clear");
    lua_pushcfunction(L, LuaGraphicsGetSize);
    lua_setfield(L, -2, "getSize");
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

void RegisterTime(lua_State *L)
{
    lua_newtable(L);
    lua_pushcfunction(L, LuaTimeTicks);
    lua_setfield(L, -2, "ticks");
    lua_pushcfunction(L, LuaTimeTickDelta);
    lua_setfield(L, -2, "tickDelta");
}

void RegisterFs(lua_State *L)
{
    lua_newtable(L);
    lua_pushcfunction(L, LuaFsRead);
    lua_setfield(L, -2, "read");
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

    RegisterTime(L);
    lua_setfield(L, -2, "time");

    RegisterFs(L);
    lua_setfield(L, -2, "fs");

    lua_pushcfunction(L, LuaQuit);
    lua_setfield(L, -2, "quit");

    lua_setglobal(L, "leo");
}

} // namespace

namespace engine
{

LuaRuntime::LuaRuntime() noexcept
    : L(nullptr), vfs(nullptr), window(nullptr), renderer(nullptr), config(nullptr), tick_index(0), tick_dt(0.0f),
      loaded(false), quit_requested(false), draw_color({255, 255, 255, 255}), window_mode(WindowMode::Windowed),
      current_font_ref(LUA_NOREF), current_font_ptr(nullptr), current_font_size(0)
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
