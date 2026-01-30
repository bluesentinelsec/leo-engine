#include <CLI11.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <lua.hpp>
#include <physfs.h>
#include <stb_image.h>
#include <stb_truetype.h>
#include <tmxlite/Map.hpp>

#include <filesystem>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <cctype>

#include "leo/engine_core.h"

#include "version.h"

namespace
{
struct ResourceConfig
{
    std::string mount_path;
    std::string prefix;
};

const char *LogPriorityLabel(SDL_LogPriority priority)
{
    switch (priority)
    {
    case SDL_LOG_PRIORITY_VERBOSE:
        return "VERBOSE";
    case SDL_LOG_PRIORITY_DEBUG:
        return "DEBUG";
    case SDL_LOG_PRIORITY_INFO:
        return "INFO";
    case SDL_LOG_PRIORITY_WARN:
        return "WARN";
    case SDL_LOG_PRIORITY_ERROR:
        return "ERROR";
    case SDL_LOG_PRIORITY_CRITICAL:
        return "FATAL";
    default:
        return "INFO";
    }
}

void SDLCALL LogOutput(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
    (void)userdata;
    (void)category;

    using clock = std::chrono::system_clock;
    auto now = clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t tt = clock::to_time_t(now);
    std::tm local_time{};
#if defined(_WIN32)
    localtime_s(&local_time, &tt);
#else
    localtime_r(&tt, &local_time);
#endif

    char time_buf[32];
    std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &local_time);
    std::fprintf(stderr, "%s.%03d [%s] %s\n", time_buf, static_cast<int>(ms.count()),
                 LogPriorityLabel(priority), message ? message : "");
}

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

std::string ToLowerAscii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}
} // namespace

int main(int argc, char *argv[])
{
    CLI::App app{"Leo Engine"};
    bool show_version = false;
    std::string resource_path_arg;
    std::string script_path_arg;
    std::string organization = "bluesentinelsec";
    std::string app_name = "leo-engine";
    std::string window_title = "Leo Engine";
    int window_width = 1280;
    int window_height = 720;
    int logical_width = 1280;
    int logical_height = 720;
    std::string window_mode_str = "borderless-fullscreen";
    int tick_hz = 60;
    int num_frame_ticks = 0;
    std::string log_level = "info";
    app.add_flag("--version", show_version, "Show version information");
    app.add_option("-r,--resources,--resource", resource_path_arg, "Resource directory/archive to mount");
    app.add_option("-s,--script", script_path_arg, "Lua script path (VFS path)");
    app.add_option("--organization,--org", organization, "Organization for write dir");
    app.add_option("--app-name", app_name, "Application name for write dir");
    app.add_option("--title", window_title, "Window title");
    app.add_option("--window-width", window_width, "Window width in pixels");
    app.add_option("--window-height", window_height, "Window height in pixels");
    app.add_option("--logical-width", logical_width, "Logical render width");
    app.add_option("--logical-height", logical_height, "Logical render height");
    app.add_option("--window-mode", window_mode_str,
                   "Window mode: windowed, fullscreen, borderless-fullscreen");
    app.add_option("--tick-hz", tick_hz, "Fixed update tick rate");
    app.add_option("--frame-ticks,--num-frame-ticks", num_frame_ticks, "Number of frame ticks (0 = run until exit)");
    app.add_option("--log-level", log_level, "Log level: verbose, debug, info, warn, error, fatal");

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
        SDL_SetLogOutputFunction(LogOutput, nullptr);

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

        std::string window_mode_normalized = ToLowerAscii(window_mode_str);
        leo::Engine::WindowMode window_mode = leo::Engine::WindowMode::BorderlessFullscreen;
        if (window_mode_normalized == "windowed")
        {
            window_mode = leo::Engine::WindowMode::Windowed;
        }
        else if (window_mode_normalized == "fullscreen")
        {
            window_mode = leo::Engine::WindowMode::Fullscreen;
        }
        else if (window_mode_normalized == "borderless-fullscreen" ||
                 window_mode_normalized == "borderless_fullscreen" || window_mode_normalized == "borderlessfullscreen")
        {
            window_mode = leo::Engine::WindowMode::BorderlessFullscreen;
        }
        else
        {
            throw std::runtime_error("Invalid window mode: " + window_mode_str);
        }

        std::string log_level_normalized = ToLowerAscii(log_level);
        SDL_LogPriority log_priority = SDL_LOG_PRIORITY_INFO;
        if (log_level_normalized == "verbose")
        {
            log_priority = SDL_LOG_PRIORITY_VERBOSE;
        }
        else if (log_level_normalized == "debug")
        {
            log_priority = SDL_LOG_PRIORITY_DEBUG;
        }
        else if (log_level_normalized == "info")
        {
            log_priority = SDL_LOG_PRIORITY_INFO;
        }
        else if (log_level_normalized == "warn" || log_level_normalized == "warning")
        {
            log_priority = SDL_LOG_PRIORITY_WARN;
        }
        else if (log_level_normalized == "error")
        {
            log_priority = SDL_LOG_PRIORITY_ERROR;
        }
        else if (log_level_normalized == "fatal" || log_level_normalized == "critical")
        {
            log_priority = SDL_LOG_PRIORITY_CRITICAL;
        }
        else
        {
            throw std::runtime_error("Invalid log level: " + log_level);
        }

        if (tick_hz <= 0)
        {
            throw std::runtime_error("tick-hz must be positive");
        }
        if (num_frame_ticks < 0)
        {
            throw std::runtime_error("frame-ticks must be >= 0");
        }

        leo::Engine::Config config = {.argv0 = argv[0],
                                      .resource_path = resource_path.empty() ? nullptr : resource_path.c_str(),
                                      .script_path = script_path.c_str(),
                                      .organization = organization.c_str(),
                                      .app_name = app_name.c_str(),
                                      .window_title = window_title.c_str(),
                                      .window_width = window_width,
                                      .window_height = window_height,
                                      .logical_width = logical_width,
                                      .logical_height = logical_height,
                                      .window_mode = window_mode,
                                      .tick_hz = tick_hz,
                                      .NumFrameTicks = static_cast<Uint32>(num_frame_ticks),
                                      .malloc_fn = SDL_malloc,
                                      .realloc_fn = SDL_realloc,
                                      .free_fn = SDL_free};

        SDL_SetLogPriority(SDL_LOG_CATEGORY_APPLICATION, log_priority);

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
