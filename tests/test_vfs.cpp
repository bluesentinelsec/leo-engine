#include <catch2/catch_test_macros.hpp>
#include <SDL3/SDL.h>
#include <physfs.h>
#include "leo/vfs.h"
#include "leo/engine_config.h"

// Helper to initialize SDL once
struct SDLFixture {
    SDLFixture() { SDL_Init(0); }
    ~SDLFixture() { SDL_Quit(); }
};

TEST_CASE("VFS mounts resources directory", "[vfs]") {
    SDLFixture sdl;
    
    engine::Config config = {
        .argv0 = "test",
        .resource_path = nullptr,
        .organization = "bluesentinelsec",
        .app_name = "leo-engine",
        .malloc_fn = SDL_malloc,
        .realloc_fn = SDL_realloc,
        .free_fn = SDL_free
    };
    
    engine::VFS vfs(config);
    
    REQUIRE(config.resource_path != nullptr);
    // Accept either "resources/" or "../resources/" depending on working directory
    bool valid_path = (SDL_strcmp(config.resource_path, "resources/") == 0) ||
                      (SDL_strcmp(config.resource_path, "../resources/") == 0);
    REQUIRE(valid_path);
}

TEST_CASE("VFS can read file from mounted resources", "[vfs]") {
    SDLFixture sdl;
    
    engine::Config config = {
        .argv0 = "test",
        .resource_path = nullptr,
        .organization = "bluesentinelsec",
        .app_name = "leo-engine",
        .malloc_fn = SDL_malloc,
        .realloc_fn = SDL_realloc,
        .free_fn = SDL_free
    };
    
    engine::VFS vfs(config);
    
    // Try to read a file from resources
    PHYSFS_File* file = PHYSFS_openRead("maps/map.json");
    REQUIRE(file != nullptr);
    PHYSFS_close(file);
}

TEST_CASE("VFS throws exception when resources not found", "[vfs]") {
    SDLFixture sdl;
    
    engine::Config config = {
        .argv0 = "test",
        .resource_path = "nonexistent/",
        .organization = "bluesentinelsec",
        .app_name = "leo-engine",
        .malloc_fn = SDL_malloc,
        .realloc_fn = SDL_realloc,
        .free_fn = SDL_free
    };
    
    REQUIRE_THROWS_AS(engine::VFS(config), std::runtime_error);
}
