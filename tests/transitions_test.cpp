#include "leo/actor.h"
#include "leo/color.h"
#include "leo/transitions.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Transition system basic functionality", "[transitions]")
{
    leo_ActorSystem *sys = leo_actor_system_create();
    leo_Actor *root = leo_actor_system_root(sys);

    SECTION("Can create fade transition")
    {
        leo_TransitionDesc desc = {};
        desc.type = LEO_TRANSITION_FADE_IN;
        desc.duration = 1.0f;
        desc.color = LEO_BLACK;
        desc.on_complete = nullptr;
        desc.user_data = nullptr;

        leo_Actor *transition = leo_transition_start(root, &desc);
        REQUIRE(transition != nullptr);
    }

    SECTION("Can create circle in transition")
    {
        leo_TransitionDesc desc = {};
        desc.type = LEO_TRANSITION_CIRCLE_IN;
        desc.duration = 0.5f;
        desc.color = LEO_WHITE;
        desc.on_complete = nullptr;
        desc.user_data = nullptr;

        leo_Actor *transition = leo_transition_start(root, &desc);
        REQUIRE(transition != nullptr);
    }

    SECTION("Can create circle out transition")
    {
        leo_TransitionDesc desc = {};
        desc.type = LEO_TRANSITION_CIRCLE_OUT;
        desc.duration = 2.0f;
        desc.color = LEO_RED;
        desc.on_complete = nullptr;
        desc.user_data = nullptr;

        leo_Actor *transition = leo_transition_start(root, &desc);
        REQUIRE(transition != nullptr);
    }

    leo_actor_system_destroy(sys);
}

TEST_CASE("Transition completion callback", "[transitions]")
{
    leo_ActorSystem *sys = leo_actor_system_create();
    leo_Actor *root = leo_actor_system_root(sys);

    bool callback_called = false;
    auto callback = [](void *user_data) {
        bool *flag = static_cast<bool *>(user_data);
        *flag = true;
    };

    leo_TransitionDesc desc = {};
    desc.type = LEO_TRANSITION_FADE_OUT;
    desc.duration = 0.1f;
    desc.color = LEO_BLACK;
    desc.on_complete = callback;
    desc.user_data = &callback_called;

    leo_Actor *transition = leo_transition_start(root, &desc);
    REQUIRE(transition != nullptr);

    // Run for longer than duration to trigger completion
    for (int i = 0; i < 5; i++)
    {
        leo_actor_system_update(sys, 0.05f);
    }

    REQUIRE(callback_called == true);

    leo_actor_system_destroy(sys);
}

TEST_CASE("Transition progress over time", "[transitions]")
{
    leo_ActorSystem *sys = leo_actor_system_create();
    leo_Actor *root = leo_actor_system_root(sys);

    leo_TransitionDesc desc = {};
    desc.type = LEO_TRANSITION_FADE_IN;
    desc.duration = 1.0f;
    desc.color = LEO_BLACK;
    desc.on_complete = nullptr;
    desc.user_data = nullptr;

    leo_Actor *transition = leo_transition_start(root, &desc);
    REQUIRE(transition != nullptr);

    // Test that transition progresses over time
    // (We'll need to expose progress getter for this test)
    leo_actor_system_update(sys, 0.5f);
    // At this point, transition should be 50% complete

    leo_actor_system_destroy(sys);
}
