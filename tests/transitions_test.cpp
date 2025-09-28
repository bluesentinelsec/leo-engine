#include "leo/color.h"
#include "leo/transitions.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Transition system basic functionality", "[transitions]")
{
    SECTION("Can start fade-in transition")
    {
        leo_StartFadeIn(1.0f, LEO_BLACK);
        REQUIRE(leo_IsTransitioning() == true);
    }

    SECTION("Can start fade-out transition")
    {
        leo_StartFadeOut(1.0f, LEO_BLACK, nullptr);
        REQUIRE(leo_IsTransitioning() == true);
    }

    SECTION("Transition completes after duration")
    {
        leo_StartFadeIn(0.1f, LEO_BLACK);
        REQUIRE(leo_IsTransitioning() == true);

        // Update for longer than duration
        leo_UpdateTransitions(0.2f);
        REQUIRE(leo_IsTransitioning() == false);
    }
}
