#include "leo/engine_core.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Simulation runs requested frame ticks", "[engine_core]")
{
    engine::Config config = {.argv0 = "test",
                             .resource_path = nullptr,
                             .organization = "bluesentinelsec",
                             .app_name = "leo-engine",
                             .window_title = "Leo Engine Test",
                             .window_width = 64,
                             .window_height = 64,
                             .logical_width = 0,
                             .logical_height = 0,
                             .window_mode = engine::WindowMode::Windowed,
                             .tick_hz = 60,
                             .NumFrameTicks = 1,
                             .malloc_fn = SDL_malloc,
                             .realloc_fn = SDL_realloc,
                             .free_fn = SDL_free};

    leo::Engine::Simulation game(config);
    REQUIRE(game.Run() == 0);
}
