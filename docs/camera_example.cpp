// manual_camera_demo.cpp — Minimal 2D camera demo using Leo engine with logical (virtual) resolution
#include <catch2/catch_all.hpp>

#include "leo/engine.h"
#include "leo/graphics.h"
#include "leo/keyboard.h"
#include "leo/color.h"

#include <SDL3/SDL.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define MAX_BUILDINGS 100

// Colors (roughly raylib-like)
static const leo_Color RAYWHITE = { 245, 245, 245, 255 };
static const leo_Color DARKGRAY = { 80, 80, 80, 255 };
static const leo_Color GREEN = { 0, 228, 48, 255 };
static const leo_Color BLUE = { 0, 121, 241, 255 };
static const leo_Color SKYBLUE = { 102, 191, 255, 255 };
static const leo_Color RED = { 230, 41, 55, 255 };
static const leo_Color BLACK = { 0, 0, 0, 255 };
static const leo_Color DARKBLUE = { 0, 82, 172, 255 };

static inline int irand(int lo, int hi) { return lo + rand() % (hi - lo + 1); };

TEST_CASE("game", "[game]")
{
	// Init
	const int windowWidth = 800;
	const int windowHeight = 450;

	if (!leo_InitWindow(windowWidth, windowHeight, "leo [core] demo - 2D camera"))
		return;

	// Enable logical (virtual) resolution so all drawing uses logical pixels regardless of window size.
	// Choose presentation/scale per your project’s aesthetic.
	REQUIRE(leo_SetLogicalResolution(windowWidth, windowHeight,
		LEO_LOGICAL_PRESENTATION_LETTERBOX, LEO_SCALE_PIXELART) == true);

	// Query current logical size (equals windowWidth/Height while logical is enabled).
	const int screenWidth  = leo_GetScreenWidth();
	const int screenHeight = leo_GetScreenHeight();

	srand((unsigned)time(NULL));

	// World objects
	leo_Rectangle player = { 400, 280, 40, 40 };

	leo_Rectangle buildings[MAX_BUILDINGS] = { 0 };
	leo_Color buildColors[MAX_BUILDINGS] = { 0 };

	int spacing = 0;
	for (int i = 0; i < MAX_BUILDINGS; ++i)
	{
		buildings[i].width = (float)irand(50, 200);
		buildings[i].height = (float)irand(100, 800);
		buildings[i].y = (float)screenHeight - 130.0f - buildings[i].height; // use logical height
		buildings[i].x = -6000.0f + (float)spacing;

		spacing += (int)buildings[i].width;

		buildColors[i] = (leo_Color){
			(unsigned char)irand(200, 240),
			(unsigned char)irand(200, 240),
			(unsigned char)irand(200, 250),
			255
		};
	}

	// Camera in logical space
	leo_Camera2D camera{};
	camera.target = (leo_Vector2){ player.x + 20.0f, player.y + 20.0f };
	camera.offset = (leo_Vector2){ screenWidth / 2.0f, screenHeight / 2.0f }; // logical center
	camera.rotation = 0.0f;
	camera.zoom = 1.0f;

	leo_SetTargetFPS(60);

	// Main loop
	while (!leo_WindowShouldClose())
	{
		// If logical resolution could change at runtime, refresh dimensions here.
		const int LW = leo_GetScreenWidth();
		const int LH = leo_GetScreenHeight();

		// --- Update ---------------------------------------------------------
		leo_UpdateKeyboard();

		// Player movement (arrow keys)
		if (leo_IsKeyDown(SDL_SCANCODE_RIGHT)) player.x += 2;
		else if (leo_IsKeyDown(SDL_SCANCODE_LEFT)) player.x -= 2;

		// Camera follows player
		camera.target = (leo_Vector2){ player.x + 20.0f, player.y + 20.0f };
		camera.offset = (leo_Vector2){ LW / 2.0f, LH / 2.0f }; // keep centered in logical space

		// Camera rotation (A/S like the original)
		if (leo_IsKeyDown(SDL_SCANCODE_A)) camera.rotation -= 1.0f;
		else if (leo_IsKeyDown(SDL_SCANCODE_S)) camera.rotation += 1.0f;

		if (camera.rotation > 40.0f) camera.rotation = 40.0f;
		else if (camera.rotation < -40.0f) camera.rotation = -40.0f;

		// Camera zoom controls (Q/E). Use log scaling like raylib’s sample.
		// Q zoom-in, E zoom-out
		if (leo_IsKeyDown(SDL_SCANCODE_Q))
			camera.zoom = expf(logf(camera.zoom) + 0.1f);
		if (leo_IsKeyDown(SDL_SCANCODE_E))
			camera.zoom = expf(logf(camera.zoom) - 0.1f);

		if (camera.zoom > 3.0f) camera.zoom = 3.0f;
		else if (camera.zoom < 0.1f) camera.zoom = 0.1f;

		// Reset
		if (leo_IsKeyPressed(SDL_SCANCODE_R))
		{
			camera.zoom = 1.0f;
			camera.rotation = 0.0f;
		}

		// --- Draw -----------------------------------------------------------
		leo_BeginDrawing();
		leo_ClearBackground(RAYWHITE.r, RAYWHITE.g, RAYWHITE.b, RAYWHITE.a);

		leo_BeginMode2D(camera);

		// Big ground strip
		leo_DrawRectangle(-6000, 320, 13000, 8000, DARKGRAY);

		// Buildings
		for (int i = 0; i < MAX_BUILDINGS; ++i)
		{
			const leo_Rectangle r = buildings[i];
			leo_DrawRectangle((int)r.x, (int)r.y, (int)r.width, (int)r.height, buildColors[i]);
		}

		// Player
		leo_DrawRectangle((int)player.x, (int)player.y, (int)player.width, (int)player.height, RED);

		// Axes through camera target (green) — make them “very long” in logical space
		const int longX = LW * 10;
		const int longY = LH * 10;
		leo_DrawLine((int)camera.target.x, -longY, (int)camera.target.x, longY, GREEN);
		leo_DrawLine(-longX, (int)camera.target.y, longX, (int)camera.target.y, GREEN);

		leo_EndMode2D();

		// “SCREEN AREA” frame (no text; just the red border & a panel) in logical coordinates
		leo_DrawRectangle(0, 0, LW, 5, RED);
		leo_DrawRectangle(0, 5, 5, LH - 10, RED);
		leo_DrawRectangle(LW - 5, 5, 5, LH - 10, RED);
		leo_DrawRectangle(0, LH - 5, LW, 5, RED);

		// Info panel placeholder (filled + outline)
		leo_DrawRectangle(10, 10, 250, 113, (leo_Color){ SKYBLUE.r, SKYBLUE.g, SKYBLUE.b, 128 });
		leo_DrawRectangle(10, 10, 250, 2, BLUE);
		leo_DrawRectangle(10, 10 + 113 - 2, 250, 2, BLUE);
		leo_DrawRectangle(10, 10, 2, 113, BLUE);
		leo_DrawRectangle(10 + 250 - 2, 10, 2, 113, BLUE);

		leo_EndDrawing();
		//break;
	}

	// Shutdown
	leo_CloseWindow();
	return;
}
