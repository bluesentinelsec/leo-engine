#include "leo/gamepad.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("GamepadState tracks button transitions", "[gamepad]")
{
    engine::GamepadState state;
    state.Reset();
    state.SetConnected(true);

    state.BeginFrame();
    state.SetButtonDown(engine::GamepadButton::South);
    REQUIRE(state.IsButtonDown(engine::GamepadButton::South));
    REQUIRE(state.IsButtonPressed(engine::GamepadButton::South));
    REQUIRE_FALSE(state.IsButtonReleased(engine::GamepadButton::South));

    state.BeginFrame();
    REQUIRE(state.IsButtonDown(engine::GamepadButton::South));
    REQUIRE_FALSE(state.IsButtonPressed(engine::GamepadButton::South));

    state.SetButtonUp(engine::GamepadButton::South);
    REQUIRE_FALSE(state.IsButtonDown(engine::GamepadButton::South));
    REQUIRE(state.IsButtonReleased(engine::GamepadButton::South));
}

TEST_CASE("GamepadState axis helpers detect threshold transitions", "[gamepad]")
{
    engine::GamepadState state;
    state.Reset();
    state.SetConnected(true);

    state.BeginFrame();
    state.SetAxis(engine::GamepadAxis::LeftX, 0.2f);
    REQUIRE_FALSE(state.IsAxisDown(engine::GamepadAxis::LeftX, 0.5f, engine::AxisDirection::Positive));

    state.BeginFrame();
    state.SetAxis(engine::GamepadAxis::LeftX, 0.7f);
    REQUIRE(state.IsAxisDown(engine::GamepadAxis::LeftX, 0.5f, engine::AxisDirection::Positive));
    REQUIRE(state.IsAxisPressed(engine::GamepadAxis::LeftX, 0.5f, engine::AxisDirection::Positive));

    state.BeginFrame();
    state.SetAxis(engine::GamepadAxis::LeftX, 0.6f);
    REQUIRE_FALSE(state.IsAxisPressed(engine::GamepadAxis::LeftX, 0.5f, engine::AxisDirection::Positive));

    state.BeginFrame();
    state.SetAxis(engine::GamepadAxis::LeftX, 0.2f);
    REQUIRE(state.IsAxisReleased(engine::GamepadAxis::LeftX, 0.5f, engine::AxisDirection::Positive));
}

TEST_CASE("GamepadState axis helpers support negative direction", "[gamepad]")
{
    engine::GamepadState state;
    state.Reset();
    state.SetConnected(true);

    state.BeginFrame();
    state.SetAxis(engine::GamepadAxis::LeftY, -0.6f);
    REQUIRE(state.IsAxisDown(engine::GamepadAxis::LeftY, 0.5f, engine::AxisDirection::Negative));
    REQUIRE(state.IsAxisPressed(engine::GamepadAxis::LeftY, 0.5f, engine::AxisDirection::Negative));
}
