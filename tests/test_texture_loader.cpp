#include "leo/engine_config.h"
#include "leo/texture_loader.h"
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
            .resource_path = nullptr,
            .organization = "bluesentinelsec",
            .app_name = "leo-engine",
            .malloc_fn = SDL_malloc,
            .realloc_fn = SDL_realloc,
            .free_fn = SDL_free};
}

SDL_Window *CreateTestWindow()
{
    return SDL_CreateWindow("TextureLoader Test", 64, 64, SDL_WINDOW_HIDDEN);
}

} // namespace

TEST_CASE("TextureLoader loads background texture from VFS", "[texture_loader]")
{
    SDLVideoGuard sdl;
    engine::Config config = MakeConfig();

    engine::VFS vfs(config);

    SDL_Window *window = CreateTestWindow();
    REQUIRE(window != nullptr);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
    REQUIRE(renderer != nullptr);

    engine::TextureLoader loader(vfs, renderer);
    {
        engine::Texture texture = loader.Load("images/background_1920x1080.png");
        REQUIRE(texture.handle != nullptr);
        REQUIRE(texture.width == 1920);
        REQUIRE(texture.height == 1080);

        float tex_w = 0.0f;
        float tex_h = 0.0f;
        REQUIRE(SDL_GetTextureSize(texture.handle, &tex_w, &tex_h));
        REQUIRE(static_cast<int>(tex_w) == 1920);
        REQUIRE(static_cast<int>(tex_h) == 1080);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

TEST_CASE("TextureLoader loads character texture from VFS", "[texture_loader]")
{
    SDLVideoGuard sdl;
    engine::Config config = MakeConfig();

    engine::VFS vfs(config);

    SDL_Window *window = CreateTestWindow();
    REQUIRE(window != nullptr);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
    REQUIRE(renderer != nullptr);

    engine::TextureLoader loader(vfs, renderer);
    {
        engine::Texture texture = loader.Load("images/character_64x64.png");
        REQUIRE(texture.handle != nullptr);
        REQUIRE(texture.width == 64);
        REQUIRE(texture.height == 64);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

TEST_CASE("TextureLoader throws when file is missing", "[texture_loader]")
{
    SDLVideoGuard sdl;
    engine::Config config = MakeConfig();

    engine::VFS vfs(config);

    SDL_Window *window = CreateTestWindow();
    REQUIRE(window != nullptr);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
    REQUIRE(renderer != nullptr);

    engine::TextureLoader loader(vfs, renderer);
    REQUIRE_THROWS_AS(loader.Load("images/missing.png"), std::runtime_error);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}
