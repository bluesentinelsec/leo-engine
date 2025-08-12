#include <catch2/catch_all.hpp>
extern "C" {
#include "leo/engine.h"
}

TEST_CASE("leo_sum adds two ints", "[runtime]") {
    REQUIRE(leo_sum(2, 3) == 5);
    REQUIRE(leo_sum(-1, 1) == 0);
    REQUIRE(leo_sum(0, 0) == 0);
}
