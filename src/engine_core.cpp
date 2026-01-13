#include "leo/engine_core.h"
#include "leo/texture_loader.h"
#include <stdexcept>
#include <utility>

namespace
{

struct DemoTextures
{
    engine::Texture background;
    engine::Texture character;
    bool loaded = false;
};

DemoTextures g_demo;

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
    while (running)
    {
        Uint32 frame_start = SDL_GetTicks();
        InputFrame input = {};
        input.quit_requested = false;
        input.frame_index = frame_ticks;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                input.quit_requested = true;
                running = false;
            }
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
}

void Simulation::OnUpdate(Context &ctx, const InputFrame &input)
{
    (void)ctx;
    (void)input;
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

    SDL_RenderPresent(ctx.renderer);
}

void Simulation::OnExit(Context &ctx)
{
    (void)ctx;
    g_demo.background.Reset();
    g_demo.character.Reset();
    g_demo.loaded = false;
}

} // namespace Engine
} // namespace leo
