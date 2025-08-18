#include <catch2/catch_all.hpp>
#include "leo/engine.h"
#include "leo/error.h"
#include <SDL3/SDL.h>
#include <string>

// Helper function to reset SDL state
void resetSDLState()
{
	leo_CloseWindow(); // Use leo_CloseWindow for cleanup
	leo_ClearError(); // Clear any existing error
}

TEST_CASE("leo_InitWindow initializes SDL, window, and renderer correctly", "[engine]")
{
	// Ensure clean state before each test case
	resetSDLState();

	SECTION("Valid parameters create window and renderer")
	{
		REQUIRE(leo_InitWindow(800, 600, "Test Window") == true);
		REQUIRE(leo_GetError() == std::string("")); // No error on success
		REQUIRE(leo_GetWindow() != nullptr);
		REQUIRE(leo_GetRenderer() != nullptr);

		// Cast opaque types to SDL types for testing
		SDL_Window* window = (SDL_Window*)leo_GetWindow();
		SDL_Renderer* renderer = (SDL_Renderer*)leo_GetRenderer();
		REQUIRE(window != nullptr);
		REQUIRE(renderer != nullptr);

		// Verify window properties
		int width, height;
		SDL_GetWindowSize(window, &width, &height);
		CHECK(width == 800);
		CHECK(height == 600);
		CHECK(std::string(SDL_GetWindowTitle(window)) == "Test Window");

		// Verify window flags
		Uint64 flags = SDL_GetWindowFlags(window);
#ifdef DEBUG
		CHECK((flags & SDL_WINDOW_RESIZABLE) != 0);
#else
        CHECK((flags & SDL_WINDOW_FULLSCREEN) != 0);
#endif

		// Cleanup
		leo_CloseWindow();
		CHECK(leo_GetWindow() == nullptr);
		CHECK(leo_GetRenderer() == nullptr);
		CHECK(SDL_WasInit(0) == 0); // Verify SDL_Quit was called
		CHECK(leo_GetError() == std::string("")); // No error after cleanup
	}

	SECTION("Invalid width fails to create window")
	{
		REQUIRE(leo_InitWindow(0, 600, "Test Window") == false);
		CHECK(leo_GetWindow() == nullptr);
		CHECK(leo_GetRenderer() == nullptr);
		CHECK(std::string(leo_GetError()).find("Invalid window dimensions") != std::string::npos);
		resetSDLState();
	}

	SECTION("Invalid height fails to create window")
	{
		REQUIRE(leo_InitWindow(800, 0, "Test Window") == false);
		CHECK(leo_GetWindow() == nullptr);
		CHECK(leo_GetRenderer() == nullptr);
		CHECK(std::string(leo_GetError()).find("Invalid window dimensions") != std::string::npos);
		resetSDLState();
	}

	SECTION("Null title fails to create window")
	{
		REQUIRE(leo_InitWindow(800, 600, nullptr) == false);
		CHECK(leo_GetWindow() == nullptr);
		CHECK(leo_GetRenderer() == nullptr);
		CHECK(std::string(leo_GetError()).find("Invalid window title") != std::string::npos);
		resetSDLState();
	}
}

TEST_CASE("leo_CloseWindow cleans up resources properly", "[engine]")
{
	// Ensure clean state before each test case
	resetSDLState();

	SECTION("Closes initialized window and renderer")
	{
		REQUIRE(leo_InitWindow(800, 600, "Test Window") == true);
		REQUIRE(leo_GetWindow() != nullptr);
		REQUIRE(leo_GetRenderer() != nullptr);
		REQUIRE(leo_GetError() == std::string(""));

		leo_CloseWindow();
		CHECK(leo_GetWindow() == nullptr);
		CHECK(leo_GetRenderer() == nullptr);
		CHECK(SDL_WasInit(0) == 0); // Verify SDL_Quit was called
		CHECK(leo_GetError() == std::string("")); // No error on cleanup
	}

	SECTION("Safe to call when nothing is initialized")
	{
		// Should be clean after resetSDLState
		CHECK(leo_GetWindow() == nullptr);
		CHECK(leo_GetRenderer() == nullptr);
		CHECK(leo_GetError() == std::string(""));

		leo_CloseWindow();
		CHECK(leo_GetWindow() == nullptr);
		CHECK(leo_GetRenderer() == nullptr);
		CHECK(SDL_WasInit(0) == 0);
		CHECK(leo_GetError() == std::string(""));
	}
}

TEST_CASE("Multiple init and close cycles", "[engine]")
{
	// Ensure clean state before each test case
	resetSDLState();

	SECTION("Initialize and close window multiple times")
	{
		for (int i = 0; i < 3; ++i)
		{
			REQUIRE(leo_InitWindow(800, 600, "Test Window") == true);
			REQUIRE(leo_GetWindow() != nullptr);
			REQUIRE(leo_GetRenderer() != nullptr);
			REQUIRE(leo_GetError() == std::string(""));

			leo_CloseWindow();
			CHECK(leo_GetWindow() == nullptr);
			CHECK(leo_GetRenderer() == nullptr);
			CHECK(SDL_WasInit(0) == 0);
			CHECK(leo_GetError() == std::string(""));
		}
	}
}
