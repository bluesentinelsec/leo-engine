// manual_camera_demo.cpp — Minimal 2D camera demo using Leo engine (no text rendering)
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

static inline int irand(int lo, int hi) { return lo + rand() % (hi - lo + 1); }

TEST_CASE("game", "[game]")
{
	// Init
	const int screenWidth = 800;
	const int screenHeight = 450;

	if (!leo_InitWindow(screenWidth, screenHeight, "leo [core] demo - 2D camera"))
		return;

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
		buildings[i].y = (float)screenHeight - 130.0f - buildings[i].height;
		buildings[i].x = -6000.0f + (float)spacing;

		spacing += (int)buildings[i].width;

		buildColors[i] = (leo_Color){
			(unsigned char)irand(200, 240),
			(unsigned char)irand(200, 240),
			(unsigned char)irand(200, 250),
			255
		};
	}

	// Camera
	leo_Camera2D camera{};
	camera.target = (leo_Vector2){ player.x + 20.0f, player.y + 20.0f };
	camera.offset = (leo_Vector2){ screenWidth / 2.0f, screenHeight / 2.0f };
	camera.rotation = 0.0f;
	camera.zoom = 1.0f;

	leo_SetTargetFPS(60);

	// Main loop
	while (!leo_WindowShouldClose())
	{
		// --- Update ---------------------------------------------------------
		leo_UpdateKeyboard();

		// Player movement (arrow keys)
		if (leo_IsKeyDown(SDL_SCANCODE_RIGHT)) player.x += 2;
		else if (leo_IsKeyDown(SDL_SCANCODE_LEFT)) player.x -= 2;

		// Camera follows player
		camera.target = (leo_Vector2){ player.x + 20.0f, player.y + 20.0f };

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

		// Axes through camera target (green)
		const int longX = screenWidth * 10;
		const int longY = screenHeight * 10;
		leo_DrawLine((int)camera.target.x, -longY, (int)camera.target.x, longY, GREEN);
		leo_DrawLine(-longX, (int)camera.target.y, longX, (int)camera.target.y, GREEN);

		leo_EndMode2D();

		// “SCREEN AREA” frame (no text; just the red border & a panel)
		leo_DrawRectangle(0, 0, screenWidth, 5, RED);
		leo_DrawRectangle(0, 5, 5, screenHeight - 10, RED);
		leo_DrawRectangle(screenWidth - 5, 5, 5, screenHeight - 10, RED);
		leo_DrawRectangle(0, screenHeight - 5, screenWidth, 5, RED);

		// Info panel placeholder (filled + outline)
		// (Use this as a visual anchor to confirm camera-only transforms happen inside Begin/EndMode2D)
		// Filled translucent sky-blue
		leo_DrawRectangle(10, 10, 250, 113, (leo_Color){ SKYBLUE.r, SKYBLUE.g, SKYBLUE.b, 128 });
		// Outline (simple 1px “frame”)
		leo_DrawRectangle(10, 10, 250, 2, BLUE);
		leo_DrawRectangle(10, 10 + 113 - 2, 250, 2, BLUE);
		leo_DrawRectangle(10, 10, 2, 113, BLUE);
		leo_DrawRectangle(10 + 250 - 2, 10, 2, 113, BLUE);

		leo_EndDrawing();
		break;
	}

	// Shutdown
	leo_CloseWindow();
	return;
}
