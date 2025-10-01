// ==========================================================
// File: tests/window_mode_test.cpp
// Unit tests for window mode functionality
// Tests windowed, borderless fullscreen, and fullscreen exclusive modes
// ==========================================================

#include <catch2/catch_all.hpp>

extern "C"
{
#include "leo/engine.h"
#include "leo/game.h"
#include "leo/error.h"
}

// Helper: reset engine state between tests
static void resetEngineState()
{
    leo_CloseWindow();
    leo_ClearError();
}

TEST_CASE("Window mode enum values are correct", "[window_mode]")
{
    REQUIRE(LEO_WINDOW_MODE_WINDOWED == 0);
    REQUIRE(LEO_WINDOW_MODE_BORDERLESS_FULLSCREEN == 1);
    REQUIRE(LEO_WINDOW_MODE_FULLSCREEN_EXCLUSIVE == 2);
}

TEST_CASE("leo_SetWindowMode and leo_GetWindowMode work correctly", "[window_mode]")
{
    resetEngineState();

    SECTION("Functions fail when called before window initialization")
    {
        REQUIRE(leo_SetWindowMode(LEO_WINDOW_MODE_WINDOWED) == false);
        REQUIRE(leo_GetError() != std::string(""));
        leo_ClearError();
    }

    SECTION("Window starts in windowed mode")
    {
        REQUIRE(leo_InitWindow(800, 600, "Window Mode Test"));
        REQUIRE(leo_GetWindowMode() == LEO_WINDOW_MODE_WINDOWED);
        resetEngineState();
    }

    SECTION("Can switch to borderless fullscreen")
    {
        REQUIRE(leo_InitWindow(800, 600, "Window Mode Test"));
        REQUIRE(leo_SetWindowMode(LEO_WINDOW_MODE_BORDERLESS_FULLSCREEN) == true);
        REQUIRE(leo_GetWindowMode() == LEO_WINDOW_MODE_BORDERLESS_FULLSCREEN);
        resetEngineState();
    }

    SECTION("Can switch to fullscreen exclusive")
    {
        REQUIRE(leo_InitWindow(800, 600, "Window Mode Test"));
        REQUIRE(leo_SetWindowMode(LEO_WINDOW_MODE_FULLSCREEN_EXCLUSIVE) == true);
        REQUIRE(leo_GetWindowMode() == LEO_WINDOW_MODE_FULLSCREEN_EXCLUSIVE);
        resetEngineState();
    }

    SECTION("Can switch back to windowed from fullscreen")
    {
        REQUIRE(leo_InitWindow(800, 600, "Window Mode Test"));
        REQUIRE(leo_SetWindowMode(LEO_WINDOW_MODE_FULLSCREEN_EXCLUSIVE) == true);
        REQUIRE(leo_SetWindowMode(LEO_WINDOW_MODE_WINDOWED) == true);
        REQUIRE(leo_GetWindowMode() == LEO_WINDOW_MODE_WINDOWED);
        resetEngineState();
    }

    SECTION("Setting same mode twice is idempotent")
    {
        REQUIRE(leo_InitWindow(800, 600, "Window Mode Test"));
        REQUIRE(leo_SetWindowMode(LEO_WINDOW_MODE_BORDERLESS_FULLSCREEN) == true);
        REQUIRE(leo_SetWindowMode(LEO_WINDOW_MODE_BORDERLESS_FULLSCREEN) == true);
        REQUIRE(leo_GetWindowMode() == LEO_WINDOW_MODE_BORDERLESS_FULLSCREEN);
        resetEngineState();
    }
}

// Test callback state for game integration tests
struct WindowModeTestState
{
    bool setup_called = false;
    bool update_called = false;
    leo_WindowMode expected_mode = LEO_WINDOW_MODE_WINDOWED;
    leo_WindowMode actual_mode = LEO_WINDOW_MODE_WINDOWED;
};

static bool window_mode_setup(leo_GameContext *ctx)
{
    WindowModeTestState *state = (WindowModeTestState *)ctx->user_data;
    state->setup_called = true;
    state->actual_mode = leo_GetWindowMode();
    return true;
}

static void window_mode_update(leo_GameContext *ctx)
{
    WindowModeTestState *state = (WindowModeTestState *)ctx->user_data;
    state->update_called = true;
    leo_GameQuit(ctx); // Exit immediately
}

TEST_CASE("leo_GameRun respects window_mode in config", "[window_mode][game]")
{
    SECTION("Windowed mode")
    {
        WindowModeTestState state;
        state.expected_mode = LEO_WINDOW_MODE_WINDOWED;

        leo_GameConfig cfg{};
        cfg.window_width = 320;
        cfg.window_height = 240;
        cfg.window_title = "Window Mode Test";
        cfg.window_mode = LEO_WINDOW_MODE_WINDOWED;
        cfg.target_fps = 0;
        cfg.logical_width = 0;
        cfg.logical_height = 0;
        cfg.clear_color = {0, 0, 0, 255};
        cfg.start_paused = false;
        cfg.user_data = &state;

        leo_GameCallbacks cbs{};
        cbs.on_setup = window_mode_setup;
        cbs.on_update = window_mode_update;

        int result = leo_GameRun(&cfg, &cbs);
        REQUIRE(result == 0);
        REQUIRE(state.setup_called == true);
        REQUIRE(state.update_called == true);
        REQUIRE(state.actual_mode == LEO_WINDOW_MODE_WINDOWED);
    }

    SECTION("Borderless fullscreen mode")
    {
        WindowModeTestState state;
        state.expected_mode = LEO_WINDOW_MODE_BORDERLESS_FULLSCREEN;

        leo_GameConfig cfg{};
        cfg.window_width = 320;
        cfg.window_height = 240;
        cfg.window_title = "Window Mode Test";
        cfg.window_mode = LEO_WINDOW_MODE_BORDERLESS_FULLSCREEN;
        cfg.target_fps = 0;
        cfg.logical_width = 0;
        cfg.logical_height = 0;
        cfg.clear_color = {0, 0, 0, 255};
        cfg.start_paused = false;
        cfg.user_data = &state;

        leo_GameCallbacks cbs{};
        cbs.on_setup = window_mode_setup;
        cbs.on_update = window_mode_update;

        int result = leo_GameRun(&cfg, &cbs);
        REQUIRE(result == 0);
        REQUIRE(state.setup_called == true);
        REQUIRE(state.update_called == true);
        REQUIRE(state.actual_mode == LEO_WINDOW_MODE_BORDERLESS_FULLSCREEN);
    }

    SECTION("Fullscreen exclusive mode")
    {
        WindowModeTestState state;
        state.expected_mode = LEO_WINDOW_MODE_FULLSCREEN_EXCLUSIVE;

        leo_GameConfig cfg{};
        cfg.window_width = 320;
        cfg.window_height = 240;
        cfg.window_title = "Window Mode Test";
        cfg.window_mode = LEO_WINDOW_MODE_FULLSCREEN_EXCLUSIVE;
        cfg.target_fps = 0;
        cfg.logical_width = 0;
        cfg.logical_height = 0;
        cfg.clear_color = {0, 0, 0, 255};
        cfg.start_paused = false;
        cfg.user_data = &state;

        leo_GameCallbacks cbs{};
        cbs.on_setup = window_mode_setup;
        cbs.on_update = window_mode_update;

        int result = leo_GameRun(&cfg, &cbs);
        REQUIRE(result == 0);
        REQUIRE(state.setup_called == true);
        REQUIRE(state.update_called == true);
        REQUIRE(state.actual_mode == LEO_WINDOW_MODE_FULLSCREEN_EXCLUSIVE);
    }
}
