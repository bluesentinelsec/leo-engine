#include <CLI11.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <lua.hpp>
#include <physfs.h>
#include <stb_image.h>
#include <stb_truetype.h>
#include <tmxlite/Map.hpp>

#include <filesystem>
#include <iostream>
#include <string>

#include "leo/engine_core.h"

#include "version.h"

namespace
{
struct ResourceConfig
{
    std::string mount_path;
    std::string prefix;
};

ResourceConfig ResolveResourceConfig(const std::string &resource_arg)
{
    ResourceConfig result{resource_arg, ""};
    if (resource_arg.empty())
    {
        return result;
    }

    std::filesystem::path resource_fs(resource_arg);
    std::error_code ec;
    if (std::filesystem::is_directory(resource_fs, ec))
    {
        std::filesystem::path normalized = resource_fs.lexically_normal();
        std::string base = normalized.filename().string();
        if (base.empty())
        {
            base = normalized.parent_path().filename().string();
        }

        std::filesystem::path parent = normalized.parent_path();
        if (parent.empty())
        {
            parent = ".";
        }

        result.mount_path = parent.string();
        result.prefix = base;
        return result;
    }

    std::string stem = resource_fs.stem().string();
    if (!stem.empty())
    {
        result.prefix = stem;
    }
    return result;
}
} // namespace

int main(int argc, char *argv[])
{
    CLI::App app{"Leo Engine"};
    bool show_version = false;
    std::string resource_path_arg;
    std::string script_path_arg;
    app.add_flag("--version", show_version, "Show version information");
    app.add_option("-r,--resources,--resource", resource_path_arg, "Resource directory/archive to mount");
    app.add_option("-s,--script", script_path_arg, "Lua script path (VFS path)");

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
        ResourceConfig resource_config = ResolveResourceConfig(resource_path_arg);
        std::string resource_path = resource_config.mount_path;
        std::string script_path = "resources/scripts/game.lua";
        if (!resource_config.prefix.empty())
        {
            script_path = resource_config.prefix + "/scripts/game.lua";
        }
        if (!script_path_arg.empty())
        {
            script_path = script_path_arg;
        }

        leo::Engine::Config config = {.argv0 = argv[0],
                                      .resource_path = resource_path.empty() ? nullptr : resource_path.c_str(),
                                      .script_path = script_path.c_str(),
                                      .organization = "bluesentinelsec",
                                      .app_name = "leo-engine",
                                      .window_title = "Leo Engine",
                                      .window_width = 1280,
                                      .window_height = 720,
                                      .logical_width = 1920,
                                      .logical_height = 1080,
                                      .window_mode = leo::Engine::WindowMode::BorderlessFullscreen,
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
