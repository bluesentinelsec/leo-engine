#include "math_utils.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Sum adds two integers", "[math]")
{
    REQUIRE(Sum(2, 3) == 5);
    REQUIRE(Sum(-1, 1) == 0);
    REQUIRE(Sum(0, 0) == 0);
}
