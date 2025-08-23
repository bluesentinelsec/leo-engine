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

TEST_CASE("leo_WindowShouldClose stays false until a quit/close event arrives", "[engine][loop]")
{
	resetSDLState();
	REQUIRE(leo_InitWindow(640, 360, "Loop Test") == true);

	// Initially, with no events, it should be false.
	CHECK(leo_WindowShouldClose() == false);

	// Push an SDL_QUIT event and verify it latches true.
	SDL_Event quitEv{};
	quitEv.type = SDL_EVENT_QUIT;
	REQUIRE(SDL_PushEvent(&quitEv));

	CHECK(leo_WindowShouldClose() == true);
	// It remains true on subsequent calls (latched).
	CHECK(leo_WindowShouldClose() == true);

	leo_CloseWindow();
	CHECK(SDL_WasInit(0) == 0);
}

TEST_CASE("leo_WindowShouldClose reacts to WINDOW_CLOSE_REQUESTED", "[engine][loop]")
{
	resetSDLState();
	REQUIRE(leo_InitWindow(640, 360, "CloseReq Test") == true);

	SDL_Window* win = (SDL_Window*)leo_GetWindow();
	REQUIRE(win != nullptr);

	// Should be false before any event.
	CHECK(leo_WindowShouldClose() == false);

	// Push a window close request for our window.
	SDL_Event e{};
	e.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
	e.window.windowID = SDL_GetWindowID(win);
	REQUIRE(SDL_PushEvent(&e));

	CHECK(leo_WindowShouldClose() == true);

	leo_CloseWindow();
	CHECK(SDL_WasInit(0) == 0);
}

TEST_CASE("leo_BeginDrawing / leo_EndDrawing basic sequencing", "[engine][draw]")
{
	resetSDLState();
	REQUIRE(leo_InitWindow(320, 200, "BeginEnd Test") == true);

	// Begin → Clear → End should not crash and should present.
	leo_BeginDrawing();
	leo_ClearBackground(12, 34, 56, 78);
	leo_EndDrawing();

	// Another frame works too.
	leo_BeginDrawing();
	leo_ClearBackground(0, 0, 0, 255);
	leo_EndDrawing();

	leo_CloseWindow();
	CHECK(SDL_WasInit(0) == 0);
}

TEST_CASE("leo_ClearBackground sets draw color and clamps values", "[engine][draw]")
{
	resetSDLState();
	REQUIRE(leo_InitWindow(320, 200, "Clear Test") == true);

	SDL_Renderer* renderer = (SDL_Renderer*)leo_GetRenderer();
	REQUIRE(renderer != nullptr);

	// Normal values
	leo_BeginDrawing();
	leo_ClearBackground(10, 20, 30, 255);

	Uint8 r = 0, g = 0, b = 0, a = 0;
	// Verify renderer draw color matches the last clear color
	REQUIRE(SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a) == true);
	CHECK(r == 10);
	CHECK(g == 20);
	CHECK(b == 30);
	CHECK(a == 255);
	leo_EndDrawing();

	// Clamping behavior
	leo_BeginDrawing();
	leo_ClearBackground(-5, 999, 128, -1); // expect clamped to (0,255,128,0)
	REQUIRE(SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a) == true);
	CHECK(r == 0);
	CHECK(g == 255);
	CHECK(b == 128);
	CHECK(a == 0);
	leo_EndDrawing();

	leo_CloseWindow();
	CHECK(SDL_WasInit(0) == 0);
}

// ============================
// Timing API tests
// ============================

TEST_CASE("leo_GetTime is monotonic after InitWindow", "[engine][time]")
{
	resetSDLState();
	REQUIRE(leo_InitWindow(320, 200, "Time Test") == true);

	const double t0 = leo_GetTime();
	// Small sleep to ensure measurable progress (10–20ms)
	SDL_Delay(20);
	const double t1 = leo_GetTime();

	CHECK(t1 >= t0);
	CHECK((t1 - t0) >= 0.010); // at least ~10ms advanced

	leo_CloseWindow();
	CHECK(SDL_WasInit(0) == 0);
}

TEST_CASE("leo_GetFrameTime updates after Begin/EndDrawing", "[engine][time]")
{
	resetSDLState();
	REQUIRE(leo_InitWindow(320, 200, "FrameTime Test") == true);

	// First frame with some work (~8ms) so it's measurable
	leo_BeginDrawing();
	SDL_Delay(8);
	leo_EndDrawing();

	float dt = leo_GetFrameTime();
	CHECK(dt > 0.0f);
	CHECK(dt >= 0.006f); // should be at least ~6ms given the delay

	// A quick second frame should still be non-zero
	leo_BeginDrawing();
	leo_EndDrawing();
	dt = leo_GetFrameTime();
	CHECK(dt >= 0.0f);

	leo_CloseWindow();
	CHECK(SDL_WasInit(0) == 0);
}

// --- Minimal, CI-friendly timing smoke tests ---

TEST_CASE("leo_SetTargetFPS accepts typical values without side effects", "[engine][time][smoke]")
{
	resetSDLState();
	REQUIRE(leo_InitWindow(320, 200, "SetTargetFPS Smoke") == true);

	// A few representative values, including edge-ish cases.
	const int values[] = { 0, 1, 30, 60, 240, 10000, -5 };
	for (int fps: values)
	{
		// Should not crash and should not set any error.
		leo_SetTargetFPS(fps);

		// Run a tiny frame to ensure nothing explodes and frame time is well-formed.
		leo_BeginDrawing();
		leo_EndDrawing();

		float dt = leo_GetFrameTime();
		CHECK(std::isfinite(dt));
		CHECK(dt >= 0.0f);
	}

	leo_CloseWindow();
	CHECK(SDL_WasInit(0) == 0);
}

TEST_CASE("leo_GetFPS returns a non-negative integer", "[engine][time][smoke]")
{
	resetSDLState();
	REQUIRE(leo_InitWindow(320, 200, "GetFPS Smoke") == true);

	// Drive a few frames quickly (no timing guarantees).
	for (int i = 0; i < 5; ++i)
	{
		leo_BeginDrawing();
		leo_EndDrawing();
	}

	// We don't assert a range; CI can be noisy. Just ensure it's well-formed.
	int fps = leo_GetFPS();
	CHECK(fps >= 0);

	// Call again after another frame, still well-formed.
	leo_BeginDrawing();
	leo_EndDrawing();
	fps = leo_GetFPS();
	CHECK(fps >= 0);

	leo_CloseWindow();
	CHECK(SDL_WasInit(0) == 0);
}

TEST_CASE("Camera round-trip world<->screen", "[camera]")
{
	leo_Camera2D cam{};
	cam.target = { 100.f, 50.f };
	cam.offset = { 10.f, 20.f };
	cam.rotation = 30.f;
	cam.zoom = 2.f;

	leo_Vector2 w{ 123.f, 77.f };
	auto s = leo_GetWorldToScreen2D(w, cam);
	auto w2 = leo_GetScreenToWorld2D(s, cam);

	CHECK(Catch::Approx(w2.x).margin(1e-4) == w.x);
	CHECK(Catch::Approx(w2.y).margin(1e-4) == w.y);
}

TEST_CASE("Begin/EndMode2D stack", "[camera]")
{
	auto base = leo_GetCurrentCamera2D();
	CHECK(base.zoom == 1.f);

	leo_Camera2D a{};
	a.zoom = 2.f;
	leo_BeginMode2D(a);
	CHECK(leo_GetCurrentCamera2D().zoom == 2.f);

	leo_Camera2D b{};
	b.zoom = 3.f;
	leo_BeginMode2D(b);
	CHECK(leo_GetCurrentCamera2D().zoom == 3.f);

	leo_EndMode2D();
	CHECK(leo_GetCurrentCamera2D().zoom == 2.f);

	leo_EndMode2D();
	CHECK(leo_GetCurrentCamera2D().zoom == 1.f);
}
