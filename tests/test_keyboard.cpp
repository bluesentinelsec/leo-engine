#include "leo/keyboard.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("KeyboardState tracks pressed and released transitions", "[keyboard]")
{
    engine::KeyboardState state;
    state.Reset();

    state.BeginFrame();
    state.SetKeyDown(engine::Key::Space);
    REQUIRE(state.IsDown(engine::Key::Space));
    REQUIRE(state.IsPressed(engine::Key::Space));
    REQUIRE_FALSE(state.IsReleased(engine::Key::Space));

    state.BeginFrame();
    REQUIRE(state.IsDown(engine::Key::Space));
    REQUIRE_FALSE(state.IsPressed(engine::Key::Space));
    REQUIRE_FALSE(state.IsReleased(engine::Key::Space));

    state.SetKeyUp(engine::Key::Space);
    REQUIRE_FALSE(state.IsDown(engine::Key::Space));
    REQUIRE(state.IsReleased(engine::Key::Space));

    state.BeginFrame();
    REQUIRE_FALSE(state.IsPressed(engine::Key::Space));
    REQUIRE_FALSE(state.IsReleased(engine::Key::Space));
}

TEST_CASE("KeyboardState ignores unknown key transitions", "[keyboard]")
{
    engine::KeyboardState state;
    state.Reset();
    state.BeginFrame();

    state.SetKeyDown(engine::Key::Unknown);
    REQUIRE_FALSE(state.IsDown(engine::Key::Unknown));
    REQUIRE_FALSE(state.IsPressed(engine::Key::Unknown));
    REQUIRE_FALSE(state.IsReleased(engine::Key::Unknown));
    REQUIRE(state.IsUp(engine::Key::Unknown));
}
