#include <catch2/catch_all.hpp>

#include "leo/engine.h"


TEST_CASE("Can initialize and close game window (happy path)", "[engine]")
{
	leo_InitWindow(10, 20, "test");
	leo_CloseWindow();
}
