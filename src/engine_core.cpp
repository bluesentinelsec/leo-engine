#include "leo/engine_core.h"
#include "leo/lua_runtime.h"
#include <memory>
#include <stdexcept>

namespace
{

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

engine::MouseButton MapMouseButton(Uint8 button)
{
    switch (button)
    {
    case SDL_BUTTON_LEFT:
        return engine::MouseButton::Left;
    case SDL_BUTTON_MIDDLE:
        return engine::MouseButton::Middle;
    case SDL_BUTTON_RIGHT:
        return engine::MouseButton::Right;
    case SDL_BUTTON_X1:
        return engine::MouseButton::X1;
    case SDL_BUTTON_X2:
        return engine::MouseButton::X2;
    default:
        return engine::MouseButton::Unknown;
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

SDL_WindowFlags ResolveBaseWindowFlags()
{
    return SDL_WINDOW_RESIZABLE;
}

const SDL_DisplayMode *GetDesktopDisplayMode(SDL_Window *window)
{
    SDL_DisplayID display_id = SDL_GetDisplayForWindow(window);
    if (display_id == 0)
    {
        throw std::runtime_error(SDL_GetError());
    }

    const SDL_DisplayMode *desktop_mode = SDL_GetDesktopDisplayMode(display_id);
    if (!desktop_mode)
    {
        throw std::runtime_error(SDL_GetError());
    }

    return desktop_mode;
}

void ApplyWindowMode(SDL_Window *window, const leo::Engine::Config &config)
{
    if (!window)
    {
        throw std::runtime_error("ApplyWindowMode requires a window");
    }

    switch (config.window_mode)
    {
    case leo::Engine::WindowMode::Windowed: {
        if (!SDL_SetWindowFullscreen(window, false))
        {
            throw std::runtime_error(SDL_GetError());
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
    case leo::Engine::WindowMode::BorderlessFullscreen: {
        if (!SDL_SetWindowFullscreen(window, false))
        {
            throw std::runtime_error(SDL_GetError());
        }

        if (!SDL_SetWindowFullscreenMode(window, nullptr))
        {
            throw std::runtime_error(SDL_GetError());
        }

        SDL_SetWindowBordered(window, false);
        SDL_SetWindowResizable(window, false);

        const SDL_DisplayMode *desktop_mode = GetDesktopDisplayMode(window);
        SDL_SetWindowSize(window, desktop_mode->w, desktop_mode->h);
        SDL_SetWindowPosition(window, 0, 0);
        SDL_RaiseWindow(window);
        break;
    }
    case leo::Engine::WindowMode::Fullscreen: {
        const SDL_DisplayMode *desktop_mode = GetDesktopDisplayMode(window);
        if (!SDL_SetWindowFullscreenMode(window, desktop_mode))
        {
            throw std::runtime_error(SDL_GetError());
        }
        SDL_SetWindowResizable(window, false);
        if (!SDL_SetWindowFullscreen(window, true))
        {
            throw std::runtime_error(SDL_GetError());
        }
        break;
    }
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

Simulation::Simulation(Config &config)
    : config(config), vfs(config), window(nullptr), renderer(nullptr), lua(nullptr)
{
}

Simulation::~Simulation() = default;

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

    window = SDL_CreateWindow(GetWindowTitle(config), width, height, ResolveBaseWindowFlags());
    if (!window)
    {
        SDL_Quit();
        throw std::runtime_error(SDL_GetError());
    }

    SDL_PumpEvents();

    try
    {
        ApplyWindowMode(window, config);
    }
    catch (...)
    {
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw;
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
    float tick_dt = tick_hz > 0 ? 1.0f / static_cast<float>(tick_hz) : 0.0f;
    Uint32 tick_delay_ms = tick_hz > 0 ? 1000 / tick_hz : 0;
    bool throttle = (config.NumFrameTicks == 0);

    SDL_Event event;
    engine::KeyboardState keyboard_state;
    keyboard_state.Reset();
    engine::MouseState mouse_state;
    mouse_state.Reset();
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
        mouse_state.BeginFrame();
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
            else if (event.type == SDL_EVENT_MOUSE_MOTION)
            {
                mouse_state.SetPosition(event.motion.x, event.motion.y);
                mouse_state.AddDelta(event.motion.xrel, event.motion.yrel);
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                engine::MouseButton button = MapMouseButton(event.button.button);
                mouse_state.SetButtonDown(button);
                mouse_state.SetPosition(event.button.x, event.button.y);
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
            {
                engine::MouseButton button = MapMouseButton(event.button.button);
                mouse_state.SetButtonUp(button);
                mouse_state.SetPosition(event.button.x, event.button.y);
            }
            else if (event.type == SDL_EVENT_MOUSE_WHEEL)
            {
                float wheel_x = event.wheel.x;
                float wheel_y = event.wheel.y;
                if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
                {
                    wheel_x = -wheel_x;
                    wheel_y = -wheel_y;
                }
                mouse_state.AddWheel(wheel_x, wheel_y);
                mouse_state.SetPosition(event.wheel.mouse_x, event.wheel.mouse_y);
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
        input.mouse = mouse_state;
        for (int i = 0; i < kMaxGamepads; ++i)
        {
            input.gamepads[i] = g_gamepads[i].state;
        }
        ctx.frame_index = frame_ticks;
        if (lua)
        {
            lua->SetFrameInfo(frame_ticks, tick_dt);
        }
        OnUpdate(ctx, input, tick_dt);
        OnRender(ctx);
        if (lua && lua->WantsQuit())
        {
            running = false;
        }

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
    if (!config.script_path || !*config.script_path)
    {
        return;
    }

    lua = std::make_unique<engine::LuaRuntime>();
    lua->Init(*ctx.vfs, ctx.window, ctx.renderer, config);
    lua->LoadScript(config.script_path);
    lua->CallLoad();
}

void Simulation::OnUpdate(Context &ctx, const InputFrame &input, float dt)
{
    (void)ctx;
    if (lua)
    {
        lua->CallUpdate(dt, input);
    }
}

void Simulation::OnRender(Context &ctx)
{
    SDL_SetRenderDrawColor(ctx.renderer, 0, 0, 0, 255);
    SDL_RenderClear(ctx.renderer);

    if (lua)
    {
        lua->CallDraw();
    }

    SDL_RenderPresent(ctx.renderer);
}

void Simulation::OnExit(Context &ctx)
{
    (void)ctx;
    if (lua)
    {
        lua->CallShutdown();
        lua.reset();
    }
}

} // namespace Engine
} // namespace leo
