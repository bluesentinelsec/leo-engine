#include "leo/engine_config.h"
#include "leo/font.h"
#include "leo/vfs.h"
#include <SDL3/SDL.h>
#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

namespace
{

struct SDLVideoGuard
{
    SDLVideoGuard()
    {
        SDL_Init(SDL_INIT_VIDEO);
    }

    ~SDLVideoGuard()
    {
        SDL_Quit();
    }
};

engine::Config MakeConfig()
{
    return {.argv0 = "test",
            .resource_path = ".",
            .script_path = nullptr,
            .organization = "bluesentinelsec",
            .app_name = "leo-engine",
            .malloc_fn = SDL_malloc,
            .realloc_fn = SDL_realloc,
            .free_fn = SDL_free};
}

SDL_Window *CreateTestWindow()
{
    return SDL_CreateWindow("Font Test", 64, 64, SDL_WINDOW_HIDDEN);
}

} // namespace

TEST_CASE("Font loads from VFS and reports line height", "[font]")
{
    SDLVideoGuard sdl;
    engine::Config config = MakeConfig();
    engine::VFS vfs(config);

    SDL_Window *window = CreateTestWindow();
    REQUIRE(window != nullptr);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
    REQUIRE(renderer != nullptr);

    engine::Font font = engine::Font::LoadFromVfs(vfs, renderer, "resources/font/font.ttf", 24);
    REQUIRE(font.IsReady());
    REQUIRE(font.GetLineHeight() > 0);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

TEST_CASE("Text can update string and draw", "[font]")
{
    SDLVideoGuard sdl;
    engine::Config config = MakeConfig();
    engine::VFS vfs(config);

    SDL_Window *window = CreateTestWindow();
    REQUIRE(window != nullptr);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
    REQUIRE(renderer != nullptr);

    engine::Font font = engine::Font::LoadFromVfs(vfs, renderer, "resources/font/font.ttf", 24);
    engine::TextDesc desc = {
        .font = &font, .text = "FPS: 60", .pixel_size = 24, .position = {0.0f, 0.0f}, .color = {255, 255, 255, 255}};
    engine::Text text(desc);
    text.SetString("FPS: 120");
    text.Draw(renderer);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

TEST_CASE("Font loader throws when file is missing", "[font]")
{
    SDLVideoGuard sdl;
    engine::Config config = MakeConfig();
    engine::VFS vfs(config);

    SDL_Window *window = CreateTestWindow();
    REQUIRE(window != nullptr);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
    REQUIRE(renderer != nullptr);

    REQUIRE_THROWS_AS(engine::Font::LoadFromVfs(vfs, renderer, "resources/font/missing.ttf", 24),
                      std::runtime_error);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}
