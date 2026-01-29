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
bool StartsWithResourcesPrefix(const std::string &path)
{
    return path.rfind("resources/", 0) == 0 || path.rfind("resources\\", 0) == 0;
}

bool IsDotDotPath(const std::filesystem::path &path)
{
    std::string text = path.generic_string();
    return text.rfind("..", 0) == 0;
}

std::string NormalizeScriptPath(const std::string &script_arg, std::string &resource_path)
{
    if (script_arg.empty())
    {
        return script_arg;
    }

    std::string script_path = script_arg;

    if (script_path.rfind("./", 0) == 0)
    {
        script_path = script_path.substr(2);
    }

    if (StartsWithResourcesPrefix(script_path) &&
        (resource_path.empty() || resource_path == "resources" || resource_path == "resources/"))
    {
        if (resource_path.empty())
        {
            resource_path = "resources";
        }
        script_path = script_path.substr(std::string("resources/").size());
        return script_path;
    }

    if (!resource_path.empty())
    {
        std::filesystem::path root(resource_path);
        std::filesystem::path rel = std::filesystem::path(script_path).lexically_relative(root);
        if (!rel.empty() && !IsDotDotPath(rel))
        {
            return rel.generic_string();
        }
        return script_path;
    }

    std::filesystem::path script_fs(script_path);
    if (script_fs.is_absolute())
    {
        std::filesystem::path inferred_root;
        std::filesystem::path cursor;
        bool found_resources_root = false;

        for (const auto &part : script_fs)
        {
            cursor /= part;
            if (part == "resources")
            {
                inferred_root = cursor;
                found_resources_root = true;
                break;
            }
        }

        if (found_resources_root)
        {
            resource_path = inferred_root.string();
            std::filesystem::path rel = script_fs.lexically_relative(inferred_root);
            if (!rel.empty() && !IsDotDotPath(rel))
            {
                script_path = rel.generic_string();
            }
            return script_path;
        }

        std::filesystem::path parent = script_fs.parent_path();
        if (!parent.empty())
        {
            resource_path = parent.string();
            script_path = script_fs.filename().string();
        }
    }

    return script_path;
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
    app.add_option("-s,--script", script_path_arg, "Lua script path (VFS or filesystem path)");

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
        std::string resource_path = resource_path_arg;
        std::string script_path = "scripts/game.lua";
        if (!script_path_arg.empty())
        {
            script_path = NormalizeScriptPath(script_path_arg, resource_path);
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
