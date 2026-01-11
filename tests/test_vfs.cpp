#include "leo/engine_config.h"
#include "leo/vfs.h"
#include <SDL3/SDL.h>
#include <catch2/catch_test_macros.hpp>
#include <physfs.h>

TEST_CASE("VFS mounts resources directory", "[vfs]")
{
    SDL_Init(0);

    engine::Config config = {.argv0 = "test",
                             .resource_path = nullptr,
                             .organization = "bluesentinelsec",
                             .app_name = "leo-engine",
                             .malloc_fn = SDL_malloc,
                             .realloc_fn = SDL_realloc,
                             .free_fn = SDL_free};

    {
        engine::VFS vfs(config);

        REQUIRE(config.resource_path != nullptr);
        REQUIRE(SDL_strcmp(config.resource_path, "resources/") == 0);
    }

    SDL_Quit();
}

TEST_CASE("VFS can read file from mounted resources", "[vfs]")
{
    SDL_Init(0);

    engine::Config config = {.argv0 = "test",
                             .resource_path = nullptr,
                             .organization = "bluesentinelsec",
                             .app_name = "leo-engine",
                             .malloc_fn = SDL_malloc,
                             .realloc_fn = SDL_realloc,
                             .free_fn = SDL_free};

    {
        engine::VFS vfs(config);

        void *data = nullptr;
        size_t size = 0;
        vfs.ReadAll("maps/map.json", &data, &size);
        REQUIRE(data != nullptr);
        REQUIRE(size > 0);
        SDL_free(data);
    }

    SDL_Quit();
}

TEST_CASE("VFS ReadAll throws when file is missing", "[vfs]")
{
    SDL_Init(0);

    engine::Config config = {.argv0 = "test",
                             .resource_path = nullptr,
                             .organization = "bluesentinelsec",
                             .app_name = "leo-engine",
                             .malloc_fn = SDL_malloc,
                             .realloc_fn = SDL_realloc,
                             .free_fn = SDL_free};

    {
        engine::VFS vfs(config);
        void *data = nullptr;
        size_t size = 0;
        REQUIRE_THROWS_AS(vfs.ReadAll("missing.file", &data, &size), std::runtime_error);
    }

    SDL_Quit();
}

TEST_CASE("VFS throws exception when resources not found", "[vfs]")
{
    SDL_Init(0);

    engine::Config config = {.argv0 = "test",
                             .resource_path = "nonexistent/",
                             .organization = "bluesentinelsec",
                             .app_name = "leo-engine",
                             .malloc_fn = SDL_malloc,
                             .realloc_fn = SDL_realloc,
                             .free_fn = SDL_free};

    REQUIRE_THROWS_AS(engine::VFS(config), std::runtime_error);

    SDL_Quit();
}
