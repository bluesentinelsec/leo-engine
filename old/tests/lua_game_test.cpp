// tests/lua_game_test.cpp
#include <catch2/catch_all.hpp>

extern "C"
{
#include "leo/color.h"
#include "leo/engine.h"
#include "leo/error.h"
#include "leo/lua_game.h"
}

// Helper: reset engine state between tests
static void resetSDLState()
{
    leo_CloseWindow();
    leo_ClearError();
}

TEST_CASE("leo_LuaGameRun: loads and executes Lua script", "[lua_game]")
{
    resetSDLState();

    leo_LuaGameConfig cfg{};
    cfg.window_width = 320;
    cfg.window_height = 180;
    cfg.window_title = "lua game test";
    cfg.target_fps = 0;
    cfg.clear_color = LEO_BLACK;
    cfg.script_path = "scripts/game.lua";
    cfg.user_data = nullptr;

    leo_LuaGameCallbacks cbs{};

    // This should load scripts/game.lua and call leo_init()
    // The script should return true from leo_init() to succeed
    int rc = leo_LuaGameRun(&cfg, &cbs);

    // Should succeed if script loads and leo_init() returns true
    REQUIRE(rc == 0);

    resetSDLState();
}

TEST_CASE("leo_LuaGameRun: handles missing script gracefully", "[lua_game]")
{
    resetSDLState();

    leo_LuaGameConfig cfg{};
    cfg.window_width = 320;
    cfg.window_height = 180;
    cfg.window_title = "lua game test";
    cfg.target_fps = 0;
    cfg.clear_color = LEO_BLACK;
    cfg.script_path = "nonexistent/script.lua";
    cfg.user_data = nullptr;

    leo_LuaGameCallbacks cbs{};

    int rc = leo_LuaGameRun(&cfg, &cbs);

    // Should fail gracefully when script doesn't exist
    REQUIRE(rc != 0);

    resetSDLState();
}
