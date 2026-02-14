#ifndef LEO_STEAM_RUNTIME_H
#define LEO_STEAM_RUNTIME_H

#include <SDL3/SDL.h>

namespace engine
{

class SteamRuntime
{
  public:
    SteamRuntime();
    ~SteamRuntime();

    bool Init();
    void RunCallbacks();
    void Shutdown();

    bool IsInitialized() const;
    bool ConsumeShutdownRequested();

  private:
    SDL_SharedObject *library;
    bool initialized;

    using SteamAPI_InitLegacyFn = bool (*)();
    using SteamAPI_InitFlatFn = int (*)(char *);
    using SteamAPI_ShutdownFn = void (*)();
    using SteamAPI_RunCallbacksFn = void (*)();
    using SteamAPI_SteamUserFn = void *(*)();
    using SteamAPI_SteamUtilsFn = void *(*)();
    using SteamAPI_ISteamUser_BLoggedOnFn = bool (*)(void *);
    using SteamAPI_ISteamUtils_IsOverlayEnabledFn = bool (*)(void *);
    using SteamAPI_IsSteamRunningFn = bool (*)();

    SteamAPI_InitLegacyFn steam_api_init_legacy;
    SteamAPI_InitFlatFn steam_api_init_flat;
    SteamAPI_ShutdownFn steam_api_shutdown;
    SteamAPI_RunCallbacksFn steam_api_run_callbacks;
    SteamAPI_SteamUserFn steam_api_steam_user;
    SteamAPI_SteamUtilsFn steam_api_steam_utils;
    SteamAPI_ISteamUser_BLoggedOnFn steam_api_user_b_logged_on;
    SteamAPI_ISteamUtils_IsOverlayEnabledFn steam_api_utils_is_overlay_enabled;
    SteamAPI_IsSteamRunningFn steam_api_is_steam_running;

    Uint64 last_diag_ticks;
    bool shutdown_requested;
    bool has_logged_on_state;
    bool has_overlay_enabled_state;
    bool has_steam_running_state;
    bool last_logged_on_state;
    bool last_overlay_enabled_state;
    bool last_steam_running_state;
};

} // namespace engine

#endif // LEO_STEAM_RUNTIME_H
