#include "leo/steam_runtime.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_loadso.h>
#include <SDL3/SDL_log.h>

#include <string>
#include <vector>

namespace
{
std::vector<std::string> SteamLibraryNames()
{
#if defined(_WIN32)
    return {"steam_api64.dll", "steam_api.dll"};
#elif defined(__APPLE__)
    return {"libsteam_api.dylib"};
#else
    return {"libsteam_api.so"};
#endif
}

std::vector<std::string> BuildCandidatePaths()
{
    std::vector<std::string> candidates;
    const std::vector<std::string> names = SteamLibraryNames();

    const char *base_path = SDL_GetBasePath();
    if (base_path && base_path[0] != '\0')
    {
        std::string base = base_path;
        while (!base.empty() && (base.back() == '/' || base.back() == '\\'))
        {
            base.pop_back();
        }
        for (const std::string &name : names)
        {
            candidates.push_back(base + "/" + name);
        }
    }
    for (const std::string &name : names)
    {
        candidates.push_back(name);
    }

    return candidates;
}

bool LoadSymbol(SDL_SharedObject *library, const char *name, SDL_FunctionPointer *out)
{
    SDL_FunctionPointer symbol = SDL_LoadFunction(library, name);
    if (!symbol)
    {
        return false;
    }
    *out = symbol;
    return true;
}

} // namespace

namespace engine
{

SteamRuntime::SteamRuntime()
    : library(nullptr), initialized(false), steam_api_init_legacy(nullptr), steam_api_init_flat(nullptr),
      steam_api_shutdown(nullptr), steam_api_run_callbacks(nullptr), steam_api_steam_user(nullptr),
      steam_api_steam_utils(nullptr), steam_api_user_b_logged_on(nullptr), steam_api_utils_is_overlay_enabled(nullptr),
      steam_api_is_steam_running(nullptr), last_diag_ticks(0), shutdown_requested(false), has_logged_on_state(false),
      has_overlay_enabled_state(false), has_steam_running_state(false), last_logged_on_state(false),
      last_overlay_enabled_state(false), last_steam_running_state(false)
{
}

SteamRuntime::~SteamRuntime()
{
    Shutdown();
}

bool SteamRuntime::Init()
{
    if (initialized)
    {
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "SteamRuntime: already initialized");
        return true;
    }

    const std::vector<std::string> candidates = BuildCandidatePaths();
    for (const std::string &path : candidates)
    {
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "SteamRuntime: trying to load '%s'", path.c_str());
        library = SDL_LoadObject(path.c_str());
        if (!library)
        {
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "SteamRuntime: load failed for '%s': %s", path.c_str(),
                         SDL_GetError());
            continue;
        }

        steam_api_init_legacy = reinterpret_cast<SteamAPI_InitLegacyFn>(SDL_LoadFunction(library, "SteamAPI_Init"));
        steam_api_init_flat = reinterpret_cast<SteamAPI_InitFlatFn>(SDL_LoadFunction(library, "SteamAPI_InitFlat"));
        steam_api_shutdown =
            reinterpret_cast<SteamAPI_ShutdownFn>(SDL_LoadFunction(library, "SteamAPI_Shutdown"));
        steam_api_run_callbacks =
            reinterpret_cast<SteamAPI_RunCallbacksFn>(SDL_LoadFunction(library, "SteamAPI_RunCallbacks"));

        if ((!steam_api_init_legacy && !steam_api_init_flat) || !steam_api_shutdown || !steam_api_run_callbacks)
        {
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
                         "SteamRuntime: required symbol missing in '%s': %s",
                         path.c_str(), SDL_GetError());
            SDL_UnloadObject(library);
            library = nullptr;
            steam_api_init_legacy = nullptr;
            steam_api_init_flat = nullptr;
            steam_api_shutdown = nullptr;
            steam_api_run_callbacks = nullptr;
            continue;
        }

        bool init_ok = false;
        if (steam_api_init_legacy)
        {
            init_ok = steam_api_init_legacy();
            if (!init_ok)
            {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "SteamRuntime: SteamAPI_Init failed for '%s'", path.c_str());
            }
        }
        else
        {
            constexpr int kSteamApiInitOk = 0;
            char err_msg[1024];
            err_msg[0] = '\0';
            int init_result = steam_api_init_flat(err_msg);
            init_ok = (init_result == kSteamApiInitOk);
            if (!init_ok)
            {
                if (err_msg[0] != '\0')
                {
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                                "SteamRuntime: SteamAPI_InitFlat failed for '%s' (code=%d): %s", path.c_str(),
                                init_result, err_msg);
                }
                else
                {
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                                "SteamRuntime: SteamAPI_InitFlat failed for '%s' (code=%d)", path.c_str(),
                                init_result);
                }
            }
        }

        if (!init_ok)
        {
            SDL_UnloadObject(library);
            library = nullptr;
            steam_api_init_legacy = nullptr;
            steam_api_init_flat = nullptr;
            steam_api_shutdown = nullptr;
            steam_api_run_callbacks = nullptr;
            return false;
        }

        initialized = true;
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "SteamRuntime: Steam enabled");
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "SteamRuntime: initialized from '%s'", path.c_str());

        SDL_FunctionPointer symbol = nullptr;
        if (LoadSymbol(library, "SteamAPI_SteamUser_v023", &symbol))
        {
            steam_api_steam_user = reinterpret_cast<SteamAPI_SteamUserFn>(symbol);
        }
        else
        {
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "SteamRuntime: diagnostics symbol missing: SteamAPI_SteamUser_v023");
        }

        symbol = nullptr;
        if (LoadSymbol(library, "SteamAPI_SteamUtils_v010", &symbol))
        {
            steam_api_steam_utils = reinterpret_cast<SteamAPI_SteamUtilsFn>(symbol);
        }
        else
        {
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
                         "SteamRuntime: diagnostics symbol missing: SteamAPI_SteamUtils_v010");
        }

        symbol = nullptr;
        if (LoadSymbol(library, "SteamAPI_ISteamUser_BLoggedOn", &symbol))
        {
            steam_api_user_b_logged_on = reinterpret_cast<SteamAPI_ISteamUser_BLoggedOnFn>(symbol);
        }
        else
        {
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
                         "SteamRuntime: diagnostics symbol missing: SteamAPI_ISteamUser_BLoggedOn");
        }

        symbol = nullptr;
        if (LoadSymbol(library, "SteamAPI_ISteamUtils_IsOverlayEnabled", &symbol))
        {
            steam_api_utils_is_overlay_enabled =
                reinterpret_cast<SteamAPI_ISteamUtils_IsOverlayEnabledFn>(symbol);
        }
        else
        {
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
                         "SteamRuntime: diagnostics symbol missing: SteamAPI_ISteamUtils_IsOverlayEnabled");
        }

        symbol = nullptr;
        if (LoadSymbol(library, "SteamAPI_IsSteamRunning", &symbol))
        {
            steam_api_is_steam_running = reinterpret_cast<SteamAPI_IsSteamRunningFn>(symbol);
        }

        void *steam_user = nullptr;
        if (steam_api_steam_user)
        {
            steam_user = steam_api_steam_user();
        }
        if (steam_user && steam_api_user_b_logged_on)
        {
            last_logged_on_state = steam_api_user_b_logged_on(steam_user);
            has_logged_on_state = true;
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "SteamRuntime: Steam user logged on: %s",
                        last_logged_on_state ? "yes" : "no");
        }

        void *steam_utils = nullptr;
        if (steam_api_steam_utils)
        {
            steam_utils = steam_api_steam_utils();
        }
        if (steam_utils && steam_api_utils_is_overlay_enabled)
        {
            last_overlay_enabled_state = steam_api_utils_is_overlay_enabled(steam_utils);
            has_overlay_enabled_state = true;
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "SteamRuntime: Steam overlay enabled: %s",
                        last_overlay_enabled_state ? "yes" : "no");
        }

        if (steam_api_is_steam_running)
        {
            last_steam_running_state = steam_api_is_steam_running();
            has_steam_running_state = true;
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "SteamRuntime: Steam client running: %s",
                        last_steam_running_state ? "yes" : "no");
        }

        last_diag_ticks = SDL_GetTicks();
        return true;
    }

    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "SteamRuntime: SDK not found, continuing without Steam");
    return false;
}

void SteamRuntime::RunCallbacks()
{
    if (initialized && steam_api_run_callbacks)
    {
        steam_api_run_callbacks();

        const Uint64 now = SDL_GetTicks();
        if (now - last_diag_ticks < 1000)
        {
            return;
        }
        last_diag_ticks = now;

        void *steam_user = nullptr;
        if (steam_api_steam_user)
        {
            steam_user = steam_api_steam_user();
        }
        if (steam_user && steam_api_user_b_logged_on)
        {
            const bool logged_on = steam_api_user_b_logged_on(steam_user);
            if (!has_logged_on_state || logged_on != last_logged_on_state)
            {
                has_logged_on_state = true;
                last_logged_on_state = logged_on;
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "SteamRuntime: Steam user logged on: %s",
                            logged_on ? "yes" : "no");
            }
            if (!logged_on)
            {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "SteamRuntime: Steam user logged off; exiting");
                shutdown_requested = true;
            }
        }

        void *steam_utils = nullptr;
        if (steam_api_steam_utils)
        {
            steam_utils = steam_api_steam_utils();
        }
        if (steam_utils && steam_api_utils_is_overlay_enabled)
        {
            const bool overlay_enabled = steam_api_utils_is_overlay_enabled(steam_utils);
            if (!has_overlay_enabled_state || overlay_enabled != last_overlay_enabled_state)
            {
                has_overlay_enabled_state = true;
                last_overlay_enabled_state = overlay_enabled;
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "SteamRuntime: Steam overlay enabled: %s",
                            overlay_enabled ? "yes" : "no");
            }
        }

        if (steam_api_is_steam_running)
        {
            const bool steam_running = steam_api_is_steam_running();
            if (!has_steam_running_state || steam_running != last_steam_running_state)
            {
                has_steam_running_state = true;
                last_steam_running_state = steam_running;
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "SteamRuntime: Steam client running: %s",
                            steam_running ? "yes" : "no");
            }
            if (!steam_running)
            {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "SteamRuntime: Steam client not running; exiting");
                shutdown_requested = true;
            }
        }
    }
}

void SteamRuntime::Shutdown()
{
    if (initialized && steam_api_shutdown)
    {
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "SteamRuntime: shutting down Steam API");
        steam_api_shutdown();
    }

    if (library)
    {
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "SteamRuntime: unloading Steam SDK library");
        SDL_UnloadObject(library);
    }

    library = nullptr;
    initialized = false;
    steam_api_init_legacy = nullptr;
    steam_api_init_flat = nullptr;
    steam_api_shutdown = nullptr;
    steam_api_run_callbacks = nullptr;
    steam_api_steam_user = nullptr;
    steam_api_steam_utils = nullptr;
    steam_api_user_b_logged_on = nullptr;
    steam_api_utils_is_overlay_enabled = nullptr;
    steam_api_is_steam_running = nullptr;
    last_diag_ticks = 0;
    shutdown_requested = false;
    has_logged_on_state = false;
    has_overlay_enabled_state = false;
    has_steam_running_state = false;
    last_logged_on_state = false;
    last_overlay_enabled_state = false;
    last_steam_running_state = false;
}

bool SteamRuntime::IsInitialized() const
{
    return initialized;
}

bool SteamRuntime::ConsumeShutdownRequested()
{
    if (!shutdown_requested)
    {
        return false;
    }
    shutdown_requested = false;
    return true;
}

} // namespace engine
