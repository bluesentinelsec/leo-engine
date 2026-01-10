#include <catch2/catch_test_macros.hpp>
#include "version.h"
#include <cstring>

TEST_CASE("Version is defined", "[version]") {
    REQUIRE(LEO_ENGINE_VERSION != nullptr);
    REQUIRE(std::strcmp(LEO_ENGINE_VERSION, "0.1.0") == 0);
}
