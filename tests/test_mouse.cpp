#include "leo/mouse.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("MouseState tracks button transitions", "[mouse]")
{
    engine::MouseState state;
    state.Reset();

    state.BeginFrame();
    state.SetButtonDown(engine::MouseButton::Left);
    REQUIRE(state.IsButtonDown(engine::MouseButton::Left));
    REQUIRE(state.IsButtonPressed(engine::MouseButton::Left));
    REQUIRE_FALSE(state.IsButtonReleased(engine::MouseButton::Left));

    state.BeginFrame();
    REQUIRE(state.IsButtonDown(engine::MouseButton::Left));
    REQUIRE_FALSE(state.IsButtonPressed(engine::MouseButton::Left));

    state.SetButtonUp(engine::MouseButton::Left);
    REQUIRE_FALSE(state.IsButtonDown(engine::MouseButton::Left));
    REQUIRE(state.IsButtonReleased(engine::MouseButton::Left));
}

TEST_CASE("MouseState accumulates motion and wheel per frame", "[mouse]")
{
    engine::MouseState state;
    state.Reset();

    state.BeginFrame();
    state.SetPosition(10.0f, 12.0f);
    state.AddDelta(3.0f, -2.0f);
    state.AddWheel(0.0f, 1.0f);

    REQUIRE(state.GetX() == 10.0f);
    REQUIRE(state.GetY() == 12.0f);
    REQUIRE(state.GetDeltaX() == 3.0f);
    REQUIRE(state.GetDeltaY() == -2.0f);
    REQUIRE(state.GetWheelY() == 1.0f);

    state.BeginFrame();
    REQUIRE(state.GetDeltaX() == 0.0f);
    REQUIRE(state.GetDeltaY() == 0.0f);
    REQUIRE(state.GetWheelY() == 0.0f);
}
