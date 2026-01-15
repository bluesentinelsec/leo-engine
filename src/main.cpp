#include <CLI11.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <lua.hpp>
#include <physfs.h>
#include <stb_image.h>
#include <stb_truetype.h>
#include <tmxlite/Map.hpp>

#include <iostream>

#include "leo/engine_core.h"

#include "version.h"

int main(int argc, char *argv[])
{
    CLI::App app{"Leo Engine"};
    bool show_version = false;
    app.add_flag("--version", show_version, "Show version information");

    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::ParseError &e)
    {
        return app.exit(e);
    }

    if (show_version)
    {
        std::cout << LEO_ENGINE_VERSION << std::endl;
        return 0;
    }

    try
    {
        leo::Engine::Config config = {.argv0 = argv[0],
                                      .resource_path = nullptr,
                                      .script_path = "scripts/game.lua",
                                      .organization = "bluesentinelsec",
                                      .app_name = "leo-engine",
                                      .window_title = "Leo Engine",
                                      .window_width = 1280,
                                      .window_height = 720,
                                      .logical_width = 1920,
                                      .logical_height = 1080,
                                      .window_mode = leo::Engine::WindowMode::Windowed,
                                      .tick_hz = 60,
                                      .NumFrameTicks = 0,
                                      .malloc_fn = SDL_malloc,
                                      .realloc_fn = SDL_realloc,
                                      .free_fn = SDL_free};

        leo::Engine::Simulation game(config);
        return game.Run();
    }
    catch (const std::exception &e)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", e.what());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Leo Engine Error", e.what(), nullptr);
        return 1;
    }
    catch (...)
    {
        const char *message = "Unknown error";
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", message);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Leo Engine Error", message, nullptr);
        return 1;
    }
}
