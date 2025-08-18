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
	resetSDLState();

	SECTION("Valid parameters create window and renderer")
	{
		REQUIRE(leo_InitWindow(800, 600, "Test Window") == true);
		REQUIRE(leo_GetError() == std::string(""));
		REQUIRE(leo_GetWindow() != nullptr);
		REQUIRE(leo_GetRenderer() != nullptr);

		SDL_Window* window = (SDL_Window*)leo_GetWindow();
		SDL_Renderer* renderer = (SDL_Renderer*)leo_GetRenderer();
		REQUIRE(window != nullptr);
		REQUIRE(renderer != nullptr);

		int width = 0, height = 0;
		SDL_GetWindowSize(window, &width, &height);
		CHECK(width == 800);
		CHECK(height == 600);
		CHECK(std::string(SDL_GetWindowTitle(window)) == "Test Window");

		Uint64 flags = SDL_GetWindowFlags(window);
		CHECK((flags & SDL_WINDOW_RESIZABLE) != 0);

		leo_CloseWindow();
		CHECK(leo_GetWindow() == nullptr);
		CHECK(leo_GetRenderer() == nullptr);
		CHECK(SDL_WasInit(0) == 0);
		CHECK(leo_GetError() == std::string(""));
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

TEST_CASE("leo_SetFullscreen toggles fullscreen on and off", "[engine]")
{
	resetSDLState();

	REQUIRE(leo_InitWindow(800, 600, "Test Window") == true);
	SDL_Window* window = (SDL_Window*)leo_GetWindow();
	REQUIRE(window != nullptr);

	// Turn on fullscreen
	REQUIRE(leo_SetFullscreen(true) == true);

	int fs_w = 0, fs_h = 0;
	SDL_GetWindowSize(window, &fs_w, &fs_h);
	// Don't assert exact flags on macOS; just ensure it's a sensible, non-trivial size
	CHECK(fs_w > 0);
	CHECK(fs_h > 0);

	// Turn it back off; SDL should restore prior windowed size
	REQUIRE(leo_SetFullscreen(false) == true);

	int ww = 0, wh = 0;
	SDL_GetWindowSize(window, &ww, &wh);
	CHECK(ww == 800);
	CHECK(wh == 600);

	leo_CloseWindow();
	CHECK(leo_GetWindow() == nullptr);
	CHECK(leo_GetRenderer() == nullptr);
	CHECK(SDL_WasInit(0) == 0);
	CHECK(leo_GetError() == std::string(""));
}
