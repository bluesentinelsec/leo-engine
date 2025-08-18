#include <catch2/catch_all.hpp>

#include "leo/error.h"
#include <string.h>

TEST_CASE("Error handling basics", "[error]")
{
	SECTION("Initial state is empty")
	{
		leo_ClearError(); // Ensure clean start
		REQUIRE(strcmp(leo_GetError(), "") == 0);
	}

	SECTION("Set and get simple error")
	{
		leo_SetError("Test error");
		REQUIRE(strcmp(leo_GetError(), "Test error") == 0);
	}

	SECTION("Set error with formatting")
	{
		leo_SetError("Error code: %d, message: %s", 42, "formatted");
		REQUIRE(strcmp(leo_GetError(), "Error code: 42, message: formatted") == 0);
	}

	SECTION("Clear error resets to empty")
	{
		leo_SetError("To be cleared");
		leo_ClearError();
		REQUIRE(strcmp(leo_GetError(), "") == 0);
	}

	SECTION("Overlong error is truncated")
	{
		char long_fmt[8192];
		memset(long_fmt, 'A', sizeof(long_fmt) - 1);
		long_fmt[sizeof(long_fmt) - 1] = '\0';
		leo_SetError("%s", long_fmt);
		REQUIRE(strlen(leo_GetError()) < 8192); // Buffer limits it
		REQUIRE(strlen(leo_GetError()) == 4095); // Assuming 4096-1 for null
	}

	SECTION("Multiple sets override previous")
	{
		leo_SetError("First error");
		leo_SetError("Second error");
		REQUIRE(strcmp(leo_GetError(), "Second error") == 0);
	}
}
