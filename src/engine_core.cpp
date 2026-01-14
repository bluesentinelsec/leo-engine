#include "leo/engine_core.h"
#include "leo/audio.h"
#include "leo/font.h"
#include "leo/texture_loader.h"
#include <stdexcept>
#include <utility>

namespace
{

struct DemoTextures
{
    engine::Texture background;
    engine::Texture character;
    engine::Font font;
    engine::Text fps_text;
    engine::Sound coin_sound;
    engine::Sound ogre_sound;
    engine::Music music;
    bool loaded = false;
    bool font_loaded = false;
    bool audio_loaded = false;
    Uint64 fps_last_ticks = 0;
    Uint32 fps_frame_count = 0;
    Uint32 sfx_ticks = 0;
};

DemoTextures g_demo;

constexpr int kMaxGamepads = 2;

struct GamepadSlot
{
    SDL_Gamepad *handle = nullptr;
    SDL_JoystickID instance = 0;
    engine::GamepadState state = {};
};

GamepadSlot g_gamepads[kMaxGamepads];

engine::Key MapScancode(SDL_Scancode scancode)
{
    switch (scancode)
    {
    case SDL_SCANCODE_A:
        return engine::Key::A;
    case SDL_SCANCODE_B:
        return engine::Key::B;
    case SDL_SCANCODE_C:
        return engine::Key::C;
    case SDL_SCANCODE_D:
        return engine::Key::D;
    case SDL_SCANCODE_E:
        return engine::Key::E;
    case SDL_SCANCODE_F:
        return engine::Key::F;
    case SDL_SCANCODE_G:
        return engine::Key::G;
    case SDL_SCANCODE_H:
        return engine::Key::H;
    case SDL_SCANCODE_I:
        return engine::Key::I;
    case SDL_SCANCODE_J:
        return engine::Key::J;
    case SDL_SCANCODE_K:
        return engine::Key::K;
    case SDL_SCANCODE_L:
        return engine::Key::L;
    case SDL_SCANCODE_M:
        return engine::Key::M;
    case SDL_SCANCODE_N:
        return engine::Key::N;
    case SDL_SCANCODE_O:
        return engine::Key::O;
    case SDL_SCANCODE_P:
        return engine::Key::P;
    case SDL_SCANCODE_Q:
        return engine::Key::Q;
    case SDL_SCANCODE_R:
        return engine::Key::R;
    case SDL_SCANCODE_S:
        return engine::Key::S;
    case SDL_SCANCODE_T:
        return engine::Key::T;
    case SDL_SCANCODE_U:
        return engine::Key::U;
    case SDL_SCANCODE_V:
        return engine::Key::V;
    case SDL_SCANCODE_W:
        return engine::Key::W;
    case SDL_SCANCODE_X:
        return engine::Key::X;
    case SDL_SCANCODE_Y:
        return engine::Key::Y;
    case SDL_SCANCODE_Z:
        return engine::Key::Z;
    case SDL_SCANCODE_0:
        return engine::Key::Num0;
    case SDL_SCANCODE_1:
        return engine::Key::Num1;
    case SDL_SCANCODE_2:
        return engine::Key::Num2;
    case SDL_SCANCODE_3:
        return engine::Key::Num3;
    case SDL_SCANCODE_4:
        return engine::Key::Num4;
    case SDL_SCANCODE_5:
        return engine::Key::Num5;
    case SDL_SCANCODE_6:
        return engine::Key::Num6;
    case SDL_SCANCODE_7:
        return engine::Key::Num7;
    case SDL_SCANCODE_8:
        return engine::Key::Num8;
    case SDL_SCANCODE_9:
        return engine::Key::Num9;
    case SDL_SCANCODE_ESCAPE:
        return engine::Key::Escape;
    case SDL_SCANCODE_RETURN:
        return engine::Key::Enter;
    case SDL_SCANCODE_SPACE:
        return engine::Key::Space;
    case SDL_SCANCODE_TAB:
        return engine::Key::Tab;
    case SDL_SCANCODE_BACKSPACE:
        return engine::Key::Backspace;
    case SDL_SCANCODE_DELETE:
        return engine::Key::Delete;
    case SDL_SCANCODE_LEFT:
        return engine::Key::Left;
    case SDL_SCANCODE_RIGHT:
        return engine::Key::Right;
    case SDL_SCANCODE_UP:
        return engine::Key::Up;
    case SDL_SCANCODE_DOWN:
        return engine::Key::Down;
    case SDL_SCANCODE_LSHIFT:
        return engine::Key::LShift;
    case SDL_SCANCODE_RSHIFT:
        return engine::Key::RShift;
    case SDL_SCANCODE_LCTRL:
        return engine::Key::LCtrl;
    case SDL_SCANCODE_RCTRL:
        return engine::Key::RCtrl;
    case SDL_SCANCODE_LALT:
        return engine::Key::LAlt;
    case SDL_SCANCODE_RALT:
        return engine::Key::RAlt;
    case SDL_SCANCODE_HOME:
        return engine::Key::Home;
    case SDL_SCANCODE_END:
        return engine::Key::End;
    case SDL_SCANCODE_PAGEUP:
        return engine::Key::PageUp;
    case SDL_SCANCODE_PAGEDOWN:
        return engine::Key::PageDown;
    case SDL_SCANCODE_F1:
        return engine::Key::F1;
    case SDL_SCANCODE_F2:
        return engine::Key::F2;
    case SDL_SCANCODE_F3:
        return engine::Key::F3;
    case SDL_SCANCODE_F4:
        return engine::Key::F4;
    case SDL_SCANCODE_F5:
        return engine::Key::F5;
    case SDL_SCANCODE_F6:
        return engine::Key::F6;
    case SDL_SCANCODE_F7:
        return engine::Key::F7;
    case SDL_SCANCODE_F8:
        return engine::Key::F8;
    case SDL_SCANCODE_F9:
        return engine::Key::F9;
    case SDL_SCANCODE_F10:
        return engine::Key::F10;
    case SDL_SCANCODE_F11:
        return engine::Key::F11;
    case SDL_SCANCODE_F12:
        return engine::Key::F12;
    default:
        return engine::Key::Unknown;
    }
}

engine::GamepadButton MapGamepadButton(Uint8 button)
{
    switch (button)
    {
    case SDL_GAMEPAD_BUTTON_SOUTH:
        return engine::GamepadButton::South;
    case SDL_GAMEPAD_BUTTON_EAST:
        return engine::GamepadButton::East;
    case SDL_GAMEPAD_BUTTON_WEST:
        return engine::GamepadButton::West;
    case SDL_GAMEPAD_BUTTON_NORTH:
        return engine::GamepadButton::North;
    case SDL_GAMEPAD_BUTTON_BACK:
        return engine::GamepadButton::Back;
    case SDL_GAMEPAD_BUTTON_GUIDE:
        return engine::GamepadButton::Guide;
    case SDL_GAMEPAD_BUTTON_START:
        return engine::GamepadButton::Start;
    case SDL_GAMEPAD_BUTTON_LEFT_STICK:
        return engine::GamepadButton::LeftStick;
    case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
        return engine::GamepadButton::RightStick;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
        return engine::GamepadButton::LeftShoulder;
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
        return engine::GamepadButton::RightShoulder;
    case SDL_GAMEPAD_BUTTON_DPAD_UP:
        return engine::GamepadButton::DpadUp;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        return engine::GamepadButton::DpadDown;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
        return engine::GamepadButton::DpadLeft;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
        return engine::GamepadButton::DpadRight;
    case SDL_GAMEPAD_BUTTON_MISC1:
        return engine::GamepadButton::Misc1;
    case SDL_GAMEPAD_BUTTON_TOUCHPAD:
        return engine::GamepadButton::Touchpad;
    default:
        return engine::GamepadButton::Unknown;
    }
}

engine::GamepadAxis MapGamepadAxis(Uint8 axis)
{
    switch (axis)
    {
    case SDL_GAMEPAD_AXIS_LEFTX:
        return engine::GamepadAxis::LeftX;
    case SDL_GAMEPAD_AXIS_LEFTY:
        return engine::GamepadAxis::LeftY;
    case SDL_GAMEPAD_AXIS_RIGHTX:
        return engine::GamepadAxis::RightX;
    case SDL_GAMEPAD_AXIS_RIGHTY:
        return engine::GamepadAxis::RightY;
    case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
        return engine::GamepadAxis::LeftTrigger;
    case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
        return engine::GamepadAxis::RightTrigger;
    default:
        return engine::GamepadAxis::Count;
    }
}

float NormalizeAxis(engine::GamepadAxis axis, Sint16 value)
{
    if (axis == engine::GamepadAxis::LeftTrigger || axis == engine::GamepadAxis::RightTrigger)
    {
        float normalized = static_cast<float>(value) / 32767.0f;
        if (normalized < 0.0f)
        {
            return 0.0f;
        }
        if (normalized > 1.0f)
        {
            return 1.0f;
        }
        return normalized;
    }

    if (value < 0)
    {
        return static_cast<float>(value) / 32768.0f;
    }
    return static_cast<float>(value) / 32767.0f;
}

int FindGamepadSlot(SDL_JoystickID instance_id)
{
    for (int i = 0; i < kMaxGamepads; ++i)
    {
        if (g_gamepads[i].handle && g_gamepads[i].instance == instance_id)
        {
            return i;
        }
    }
    return -1;
}

int FindFreeGamepadSlot()
{
    for (int i = 0; i < kMaxGamepads; ++i)
    {
        if (!g_gamepads[i].handle)
        {
            return i;
        }
    }
    return -1;
}

void AttachGamepad(SDL_JoystickID instance_id)
{
    int slot = FindFreeGamepadSlot();
    if (slot < 0)
    {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Gamepad ignored: no free slots");
        return;
    }

    SDL_Gamepad *gamepad = SDL_OpenGamepad(instance_id);
    if (!gamepad)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to open gamepad: %s", SDL_GetError());
        return;
    }

    g_gamepads[slot].handle = gamepad;
    g_gamepads[slot].instance = instance_id;
    g_gamepads[slot].state.Reset();
    g_gamepads[slot].state.SetConnected(true);
}

void DetachGamepad(SDL_JoystickID instance_id)
{
    int slot = FindGamepadSlot(instance_id);
    if (slot < 0)
    {
        return;
    }

    if (g_gamepads[slot].handle)
    {
        SDL_CloseGamepad(g_gamepads[slot].handle);
    }

    g_gamepads[slot].handle = nullptr;
    g_gamepads[slot].instance = 0;
    g_gamepads[slot].state.Reset();
}

void CloseGamepads()
{
    for (int i = 0; i < kMaxGamepads; ++i)
    {
        if (g_gamepads[i].handle)
        {
            SDL_CloseGamepad(g_gamepads[i].handle);
            g_gamepads[i].handle = nullptr;
        }
        g_gamepads[i].instance = 0;
        g_gamepads[i].state.Reset();
    }
}

const char *GetWindowTitle(const leo::Engine::Config &config)
{
    return config.window_title ? config.window_title : "Leo Engine";
}

void ResolveWindowSize(const leo::Engine::Config &config, int *out_width, int *out_height)
{
    int width = config.window_width > 0 ? config.window_width : 1280;
    int height = config.window_height > 0 ? config.window_height : 720;
    *out_width = width;
    *out_height = height;
}

Uint32 ResolveTickHz(const leo::Engine::Config &config)
{
    return config.tick_hz > 0 ? static_cast<Uint32>(config.tick_hz) : 60;
}

SDL_WindowFlags ResolveWindowFlags(leo::Engine::WindowMode mode)
{
    switch (mode)
    {
    case leo::Engine::WindowMode::Fullscreen:
        return SDL_WINDOW_FULLSCREEN;
    case leo::Engine::WindowMode::BorderlessFullscreen:
        return static_cast<SDL_WindowFlags>(SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS);
    case leo::Engine::WindowMode::Windowed:
    default:
        return 0;
    }
}

void ConfigureLogicalPresentation(const leo::Engine::Config &config, SDL_Renderer *renderer)
{
    if (config.logical_width <= 0 || config.logical_height <= 0)
    {
        return;
    }

    if (!SDL_SetRenderLogicalPresentation(renderer, config.logical_width, config.logical_height,
                                          SDL_LOGICAL_PRESENTATION_LETTERBOX))
    {
        throw std::runtime_error("Failed to set logical presentation");
    }
}

} // namespace

namespace leo
{
namespace Engine
{

Simulation::Simulation(Config &config) : config(config), vfs(config), window(nullptr), renderer(nullptr)
{
}

int Simulation::Run()
{

    if (!SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_HAPTIC | SDL_INIT_GAMEPAD))
    {
        throw std::runtime_error(SDL_GetError());
    }

    SDL_SetGamepadEventsEnabled(true);

    int width = 0;
    int height = 0;
    ResolveWindowSize(config, &width, &height);

    window = SDL_CreateWindow(GetWindowTitle(config), width, height, ResolveWindowFlags(config.window_mode));
    if (!window)
    {
        SDL_Quit();
        throw std::runtime_error(SDL_GetError());
    }

    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer)
    {
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw std::runtime_error(SDL_GetError());
    }

    ConfigureLogicalPresentation(config, renderer);

    Context ctx = {};
    ctx.window = window;
    ctx.renderer = renderer;
    ctx.vfs = &vfs;
    ctx.config = &config;
    ctx.frame_index = 0;

    OnInit(ctx);

    bool running = true;
    Uint32 frame_ticks = 0;
    Uint32 tick_hz = ResolveTickHz(config);
    Uint32 tick_delay_ms = tick_hz > 0 ? 1000 / tick_hz : 0;
    bool throttle = (config.NumFrameTicks == 0);

    SDL_Event event;
    engine::KeyboardState keyboard_state;
    keyboard_state.Reset();
    for (int i = 0; i < kMaxGamepads; ++i)
    {
        g_gamepads[i].handle = nullptr;
        g_gamepads[i].instance = 0;
        g_gamepads[i].state.Reset();
    }
    while (running)
    {
        Uint32 frame_start = SDL_GetTicks();
        InputFrame input = {};
        input.quit_requested = false;
        input.frame_index = frame_ticks;
        keyboard_state.BeginFrame();
        for (int i = 0; i < kMaxGamepads; ++i)
        {
            g_gamepads[i].state.BeginFrame();
        }

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                input.quit_requested = true;
                running = false;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN)
            {
                engine::Key key = MapScancode(event.key.scancode);
                keyboard_state.SetKeyDown(key);
            }
            else if (event.type == SDL_EVENT_KEY_UP)
            {
                engine::Key key = MapScancode(event.key.scancode);
                keyboard_state.SetKeyUp(key);
            }
            else if (event.type == SDL_EVENT_GAMEPAD_ADDED)
            {
                AttachGamepad(event.gdevice.which);
            }
            else if (event.type == SDL_EVENT_GAMEPAD_REMOVED)
            {
                DetachGamepad(event.gdevice.which);
            }
            else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
            {
                int slot = FindGamepadSlot(event.gbutton.which);
                if (slot >= 0)
                {
                    engine::GamepadButton button = MapGamepadButton(event.gbutton.button);
                    g_gamepads[slot].state.SetButtonDown(button);
                }
            }
            else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_UP)
            {
                int slot = FindGamepadSlot(event.gbutton.which);
                if (slot >= 0)
                {
                    engine::GamepadButton button = MapGamepadButton(event.gbutton.button);
                    g_gamepads[slot].state.SetButtonUp(button);
                }
            }
            else if (event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION)
            {
                int slot = FindGamepadSlot(event.gaxis.which);
                if (slot >= 0)
                {
                    engine::GamepadAxis axis = MapGamepadAxis(event.gaxis.axis);
                    if (axis != engine::GamepadAxis::Count)
                    {
                        float value = NormalizeAxis(axis, event.gaxis.value);
                        g_gamepads[slot].state.SetAxis(axis, value);
                    }
                }
            }
        }

        input.keyboard = keyboard_state;
        for (int i = 0; i < kMaxGamepads; ++i)
        {
            input.gamepads[i] = g_gamepads[i].state;
        }
        ctx.frame_index = frame_ticks;
        OnUpdate(ctx, input);
        OnRender(ctx);

        frame_ticks++;
        if (config.NumFrameTicks > 0 && frame_ticks >= config.NumFrameTicks)
        {
            running = false;
        }

        if (throttle && tick_delay_ms > 0)
        {
            Uint32 frame_ms = SDL_GetTicks() - frame_start;
            if (frame_ms < tick_delay_ms)
            {
                SDL_Delay(tick_delay_ms - frame_ms);
            }
        }
    }

    OnExit(ctx);

    CloseGamepads();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    renderer = nullptr;
    window = nullptr;

    return 0;
}

void Simulation::OnInit(Context &ctx)
{
    engine::TextureLoader loader(*ctx.vfs, ctx.renderer);
    engine::Texture background = loader.Load("images/background_1920x1080.png");
    engine::Texture character = loader.Load("images/character_64x64.png");
    g_demo.background = std::move(background);
    g_demo.character = std::move(character);
    g_demo.loaded = true;

    engine::Font font = engine::Font::LoadFromVfs(*ctx.vfs, ctx.renderer, "font/font.ttf", 24);
    g_demo.font = std::move(font);
    engine::TextDesc fps_desc = {
        .font = &g_demo.font,
        .text = "FPS: 0",
        .pixel_size = 24,
        .position = {16.0f, 16.0f},
        .color = {255, 255, 255, 255},
    };
    g_demo.fps_text = engine::Text(fps_desc);
    g_demo.font_loaded = true;
    g_demo.fps_last_ticks = SDL_GetTicks();
    g_demo.fps_frame_count = 0;

    g_demo.coin_sound = engine::Sound::LoadFromVfs(*ctx.vfs, "sound/coin.wav");
    g_demo.ogre_sound = engine::Sound::LoadFromVfs(*ctx.vfs, "sound/ogre3.wav");
    g_demo.music = engine::Music::LoadFromVfs(*ctx.vfs, "music/music.wav");
    g_demo.music.SetLooping(true);
    g_demo.music.Play();
    g_demo.coin_sound.Play();
    g_demo.audio_loaded = true;
    g_demo.sfx_ticks = 0;
}

void Simulation::OnUpdate(Context &ctx, const InputFrame &input)
{
    (void)ctx;
    (void)input;

    if (g_demo.audio_loaded)
    {
        g_demo.sfx_ticks++;
        if (g_demo.sfx_ticks % 120 == 0)
        {
            g_demo.coin_sound.Play();
        }
        if (g_demo.sfx_ticks % 300 == 0)
        {
            g_demo.ogre_sound.Play();
        }
    }
}

void Simulation::OnRender(Context &ctx)
{
    SDL_SetRenderDrawColor(ctx.renderer, 0, 0, 0, 255);
    SDL_RenderClear(ctx.renderer);

    if (g_demo.loaded)
    {
        float target_w = 0.0f;
        float target_h = 0.0f;
        if (ctx.config && ctx.config->logical_width > 0)
        {
            target_w = static_cast<float>(ctx.config->logical_width);
        }
        if (ctx.config && ctx.config->logical_height > 0)
        {
            target_h = static_cast<float>(ctx.config->logical_height);
        }
        if (target_w <= 0.0f)
        {
            target_w = g_demo.background.width > 0 ? static_cast<float>(g_demo.background.width)
                                                   : static_cast<float>(g_demo.character.width);
        }
        if (target_h <= 0.0f)
        {
            target_h = g_demo.background.height > 0 ? static_cast<float>(g_demo.background.height)
                                                    : static_cast<float>(g_demo.character.height);
        }

        if (g_demo.background.handle)
        {
            SDL_FRect dst = {0.0f, 0.0f, target_w, target_h};
            SDL_RenderTextureRotated(ctx.renderer, g_demo.background.handle, nullptr, &dst, 0.0, nullptr,
                                     SDL_FLIP_NONE);
        }

        if (g_demo.character.handle)
        {
            SDL_FRect src = {0.0f, 0.0f, static_cast<float>(g_demo.character.width),
                             static_cast<float>(g_demo.character.height)};
            SDL_FRect dst = {(target_w - src.w) * 0.5f, (target_h - src.h) * 0.5f, src.w, src.h};
            SDL_FPoint center = {dst.w * 0.5f, dst.h * 0.5f};
            double angle = static_cast<double>(ctx.frame_index % 360);
            SDL_RenderTextureRotated(ctx.renderer, g_demo.character.handle, &src, &dst, angle, &center, SDL_FLIP_NONE);
        }
    }

    if (g_demo.font_loaded)
    {
        g_demo.fps_frame_count++;
        Uint64 now = SDL_GetTicks();
        Uint64 elapsed = now - g_demo.fps_last_ticks;
        if (elapsed >= 1000)
        {
            double fps = (static_cast<double>(g_demo.fps_frame_count) * 1000.0) / static_cast<double>(elapsed);
            char buffer[32];
            SDL_snprintf(buffer, sizeof(buffer), "FPS: %.1f", fps);
            g_demo.fps_text.SetString(buffer);
            g_demo.fps_frame_count = 0;
            g_demo.fps_last_ticks = now;
        }
        g_demo.fps_text.Draw(ctx.renderer);
    }

    SDL_RenderPresent(ctx.renderer);
}

void Simulation::OnExit(Context &ctx)
{
    (void)ctx;
    g_demo.background.Reset();
    g_demo.character.Reset();
    g_demo.loaded = false;
    g_demo.fps_text = engine::Text();
    g_demo.font.Reset();
    g_demo.font_loaded = false;
    g_demo.music.Reset();
    g_demo.coin_sound.Reset();
    g_demo.ogre_sound.Reset();
    g_demo.audio_loaded = false;
}

} // namespace Engine
} // namespace leo
