// ==========================================================
// File: tests/touch_test.cpp
// Unit tests for leo/touch.h touch and gesture API
// ==========================================================

#include <catch2/catch_all.hpp>

extern "C"
{
#include "leo/engine.h"
#include "leo/error.h"
#include "leo/touch.h"
}

// Helper: reset engine state between tests
static void resetEngineState()
{
    leo_CloseWindow();
    leo_ClearError();
}

TEST_CASE("Touch API basic functionality", "[touch]")
{
    resetEngineState();
    REQUIRE(leo_InitWindow(800, 600, "Touch Test"));

    SECTION("Touch detection functions return false when no touches")
    {
        REQUIRE(leo_IsTouchDown(0) == false);
        REQUIRE(leo_IsTouchPressed(0) == false);
        REQUIRE(leo_IsTouchReleased(0) == false);
        REQUIRE(leo_GetTouchPointCount() == 0);
    }

    SECTION("Touch position functions return zero when no touches")
    {
        leo_Vector2Touch pos = leo_GetTouchPosition(0);
        REQUIRE(pos.x == 0.0f);
        REQUIRE(pos.y == 0.0f);
        REQUIRE(leo_GetTouchX(0) == 0);
        REQUIRE(leo_GetTouchY(0) == 0);
    }

    SECTION("Touch point ID functions work with no touches")
    {
        REQUIRE(leo_GetTouchPointId(0) == -1);
    }

    resetEngineState();
}

TEST_CASE("Gesture API basic functionality", "[touch][gestures]")
{
    resetEngineState();
    REQUIRE(leo_InitWindow(800, 600, "Gesture Test"));

    SECTION("Gesture detection returns none when no gestures")
    {
        REQUIRE(leo_IsGestureDetected(LEO_GESTURE_TAP) == false);
        REQUIRE(leo_IsGestureDetected(LEO_GESTURE_DRAG) == false);
        REQUIRE(leo_GetGestureDetected() == LEO_GESTURE_NONE);
    }

    SECTION("Gesture properties return zero when no gestures")
    {
        REQUIRE(leo_GetGestureHoldDuration() == 0.0f);

        leo_Vector2Touch drag = leo_GetGestureDragVector();
        REQUIRE(drag.x == 0.0f);
        REQUIRE(drag.y == 0.0f);

        REQUIRE(leo_GetGestureDragAngle() == 0.0f);

        leo_Vector2Touch pinch = leo_GetGesturePinchVector();
        REQUIRE(pinch.x == 0.0f);
        REQUIRE(pinch.y == 0.0f);

        REQUIRE(leo_GetGesturePinchAngle() == 0.0f);
    }

    SECTION("Gesture configuration works")
    {
        leo_SetGesturesEnabled(LEO_GESTURE_TAP | LEO_GESTURE_DRAG);
        // No way to query current settings, but function should not crash
    }

    resetEngineState();
}

TEST_CASE("Touch gesture constants are correct", "[touch]")
{
    REQUIRE(LEO_GESTURE_NONE == 0);
    REQUIRE(LEO_GESTURE_TAP == 1);
    REQUIRE(LEO_GESTURE_DOUBLETAP == 2);
    REQUIRE(LEO_GESTURE_HOLD == 4);
    REQUIRE(LEO_GESTURE_DRAG == 8);
    REQUIRE(LEO_GESTURE_SWIPE_RIGHT == 16);
    REQUIRE(LEO_GESTURE_SWIPE_LEFT == 32);
    REQUIRE(LEO_GESTURE_SWIPE_UP == 64);
    REQUIRE(LEO_GESTURE_SWIPE_DOWN == 128);
    REQUIRE(LEO_GESTURE_PINCH_IN == 256);
    REQUIRE(LEO_GESTURE_PINCH_OUT == 512);
}

TEST_CASE("Touch API handles invalid indices gracefully", "[touch]")
{
    resetEngineState();
    REQUIRE(leo_InitWindow(800, 600, "Touch Test"));

    SECTION("Negative touch indices")
    {
        REQUIRE(leo_IsTouchDown(-1) == false);
        REQUIRE(leo_IsTouchPressed(-1) == false);
        REQUIRE(leo_IsTouchReleased(-1) == false);

        leo_Vector2Touch pos = leo_GetTouchPosition(-1);
        REQUIRE(pos.x == 0.0f);
        REQUIRE(pos.y == 0.0f);

        REQUIRE(leo_GetTouchX(-1) == 0);
        REQUIRE(leo_GetTouchY(-1) == 0);
    }

    SECTION("Large touch indices")
    {
        REQUIRE(leo_IsTouchDown(999) == false);
        REQUIRE(leo_IsTouchPressed(999) == false);
        REQUIRE(leo_IsTouchReleased(999) == false);

        leo_Vector2Touch pos = leo_GetTouchPosition(999);
        REQUIRE(pos.x == 0.0f);
        REQUIRE(pos.y == 0.0f);

        REQUIRE(leo_GetTouchX(999) == 0);
        REQUIRE(leo_GetTouchY(999) == 0);
    }

    resetEngineState();
}
