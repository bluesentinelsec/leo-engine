// =============================================
// tests/mouse.cpp — Mouse API tests
// =============================================
#include <catch2/catch_all.hpp>

#include "leo/engine.h"
#include "leo/mouse.h"

#include <SDL3/SDL.h>

using Catch::Approx;

struct GfxEnvMouse
{
    GfxEnvMouse() { REQUIRE(leo_InitWindow(320, 180, "mouse-tests")); }
    ~GfxEnvMouse() { leo_CloseWindow(); }
};

static void push_motion_event(float x, float y, float xrel = 0.0f, float yrel = 0.0f)
{
    SDL_Event e;
    SDL_memset(&e, 0, sizeof(e));
    e.type = SDL_EVENT_MOUSE_MOTION;
    e.motion.x = x;
    e.motion.y = y;
    e.motion.xrel = xrel;
    e.motion.yrel = yrel;
    leo_HandleMouseEvent(&e);
}

static void push_wheel_event(float x, float y)
{
    SDL_Event e;
    SDL_memset(&e, 0, sizeof(e));
    e.type = SDL_EVENT_MOUSE_WHEEL;
    e.wheel.x = x;
    e.wheel.y = y;
    leo_HandleMouseEvent(&e);
}

static void push_button_event(bool down, int button)
{
    SDL_Event e;
    SDL_memset(&e, 0, sizeof(e));
    e.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
    e.button.button = (Uint8)button;
    leo_HandleMouseEvent(&e);
}

TEST_CASE_METHOD(GfxEnvMouse, "Mouse: basic position and delta", "[mouse]")
{
    // Frame 0: seed state
    leo_UpdateMouse();

    // Simulate move to (100,100)
    push_motion_event(100.0f, 100.0f);
    // Query immediately (same frame)
    {
        auto p = leo_GetMousePosition();
        CHECK(p.x == Approx(100.0f));
        CHECK(p.y == Approx(100.0f));
        auto d = leo_GetMouseDelta();
        // prev == curr before first update, delta is 0
        CHECK(d.x == Approx(0.0f));
        CHECK(d.y == Approx(0.0f));
    }

    // Advance to next frame
    leo_UpdateMouse();

    // Now move to (110,120) within this new frame
    push_motion_event(110.0f, 120.0f, 10.0f, 20.0f);
    {
        auto p = leo_GetMousePosition();
        CHECK(p.x == Approx(110.0f));
        CHECK(p.y == Approx(120.0f));
        auto d = leo_GetMouseDelta();
        // delta computed from last update's position (100,100) -> (110,120)
        CHECK(d.x == Approx(10.0f));
        CHECK(d.y == Approx(20.0f));
    }

    // Advance again: delta should become 0 without new motion
    leo_UpdateMouse();
    {
        auto d = leo_GetMouseDelta();
        CHECK(d.x == Approx(0.0f));
        CHECK(d.y == Approx(0.0f));
    }
}

TEST_CASE_METHOD(GfxEnvMouse, "Mouse: offset and scale transform", "[mouse]")
{
    // Defaults
    leo_SetMouseOffset(0, 0);
    leo_SetMouseScale(1.0f, 1.0f);
    leo_UpdateMouse();

    // Position raw (200,100)
    push_motion_event(200.0f, 100.0f);
    {
        auto p = leo_GetMousePosition();
        CHECK(p.x == Approx(200.0f));
        CHECK(p.y == Approx(100.0f));
    }

    // Apply offset(-20,+10) and scale(2x, 0.5x)
    leo_SetMouseOffset(-20, +10);
    leo_SetMouseScale(2.0f, 0.5f);

    // Same raw coordinates — transformed result:
    // x' = (200 - (-20)) / 2 = 110, y' = (100 - 10) / 0.5 = 180
    {
        auto p = leo_GetMousePosition();
        CHECK(p.x == Approx(110.0f));
        CHECK(p.y == Approx(180.0f));
    }
}

TEST_CASE_METHOD(GfxEnvMouse, "Mouse: wheel scalar & vector", "[mouse]")
{
    leo_UpdateMouse();

    // Push a wheel event Y=+1
    push_wheel_event(0.0f, 1.0f);

    // Read in the same frame (UpdateMouse() clears per-frame wheel to 0)
    {
        auto v = leo_GetMouseWheelMoveV();
        CHECK(v.x == Approx(0.0f));
        CHECK(v.y == Approx(1.0f));
        CHECK(leo_GetMouseWheelMove() == Approx(1.0f));
    }

    // Advance: should clear to zero
    leo_UpdateMouse();
    {
        auto v = leo_GetMouseWheelMoveV();
        CHECK(v.x == Approx(0.0f));
        CHECK(v.y == Approx(0.0f));
        CHECK(leo_GetMouseWheelMove() == Approx(0.0f));
    }

    // Dominant axis selection: X wins if larger magnitude
    push_wheel_event(2.0f, 1.0f);
    CHECK(leo_GetMouseWheelMove() == Approx(2.0f));
}

TEST_CASE_METHOD(GfxEnvMouse, "Mouse: button states (skip if no mouse)", "[mouse][buttons]")
{
    // If the platform reports no mouse, don’t fail—just skip these assertions.
    if (!SDL_HasMouse()) {
        WARN("SDL reports no mouse attached; skipping button edge tests.");
        SUCCEED();
        return;
    }

    // Frame 0: clear
    leo_UpdateMouse();

    // Generate a DOWN event for left button and check immediate pressed/down
    push_button_event(true, LEO_MOUSE_BUTTON_LEFT);
    {
        // Immediately after event (same frame, before Update):
        CHECK(leo_IsMouseButtonDown(LEO_MOUSE_BUTTON_LEFT));
        CHECK(leo_IsMouseButtonPressed(LEO_MOUSE_BUTTON_LEFT));   // curr=1, prev=0
        CHECK_FALSE(leo_IsMouseButtonReleased(LEO_MOUSE_BUTTON_LEFT));
    }

    // Advance a frame: pressed edge should be gone but down stays true
    leo_UpdateMouse();
    {
        CHECK(leo_IsMouseButtonDown(LEO_MOUSE_BUTTON_LEFT));
        CHECK_FALSE(leo_IsMouseButtonPressed(LEO_MOUSE_BUTTON_LEFT));
        CHECK_FALSE(leo_IsMouseButtonReleased(LEO_MOUSE_BUTTON_LEFT));
    }

    // Now send UP event and validate release edge immediately
    push_button_event(false, LEO_MOUSE_BUTTON_LEFT);
    {
        CHECK_FALSE(leo_IsMouseButtonDown(LEO_MOUSE_BUTTON_LEFT));
        CHECK(leo_IsMouseButtonReleased(LEO_MOUSE_BUTTON_LEFT));  // curr=0, prev=1
        CHECK_FALSE(leo_IsMouseButtonPressed(LEO_MOUSE_BUTTON_LEFT));
    }

    // Advance: edges clear
    leo_UpdateMouse();
    {
        CHECK_FALSE(leo_IsMouseButtonDown(LEO_MOUSE_BUTTON_LEFT));
        CHECK_FALSE(leo_IsMouseButtonPressed(LEO_MOUSE_BUTTON_LEFT));
        CHECK_FALSE(leo_IsMouseButtonReleased(LEO_MOUSE_BUTTON_LEFT));
    }
}

TEST_CASE_METHOD(GfxEnvMouse, "Mouse: SetMousePosition", "[mouse]")
{
    // This calls SDL_WarpMouseInWindow; behavior may be platform-limited on some CI
    // We only assert it doesn't crash and updates cached position.
    leo_UpdateMouse();

    // Use neutral offset/scale
    leo_SetMouseOffset(0, 0);
    leo_SetMouseScale(1.0f, 1.0f);

    // Set and verify cached getters (no hard failure if platform doesn't move the OS cursor)
    leo_SetMousePosition(42, 84);
    auto p = leo_GetMousePosition();
    CHECK(p.x == Approx(42.0f));
    CHECK(p.y == Approx(84.0f));
}
