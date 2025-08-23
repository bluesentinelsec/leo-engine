#include <catch2/catch_all.hpp>

#include "leo/engine.h"
#include "leo/graphics.h"
#include "leo/keyboard.h"
#include "leo/color.h"
#include "leo/image.h"  // <-- add this

TEST_CASE("game", "[game]")
{
	const int windowWidth = 800;
	const int windowHeight = 450;

	if (!leo_InitWindow(windowWidth, windowHeight, "leo testing"))
		return;

	REQUIRE(leo_SetLogicalResolution(windowWidth, windowHeight,
		LEO_LOGICAL_PRESENTATION_LETTERBOX, LEO_SCALE_PIXELART) == true);

	const int screenWidth = leo_GetScreenWidth();
	const int screenHeight = leo_GetScreenHeight();

	leo_SetTargetFPS(60);

	// --- Load texture (GPU) ---
	leo_Texture2D tex = leo_LoadTexture("resources/images/character_64x64.png");
	REQUIRE(leo_IsTextureReady(tex));

	// Full source rect
	leo_Rectangle src = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
	// Centered position
	leo_Vector2 pos = {
		(float)((screenWidth - tex.width) / 2),
		(float)((screenHeight - tex.height) / 2)
	};
	// White tint (no color modulation)
	leo_Color tint = { 255, 255, 255, 255 };

	while (!leo_WindowShouldClose())
	{
		// --- Update ---
		leo_UpdateKeyboard();

		// --- Draw ---
		leo_BeginDrawing();
		leo_ClearBackground(20, 20, 24, 255);

		// If you use a camera elsewhere, call leo_BeginMode2D(cam) before drawing.
		// Here we draw in screen space:
		leo_DrawTextureRec(tex, src, pos, tint);

		leo_EndDrawing();

		break; // remove to see it animate / run normally
	}

	// Cleanup
	leo_UnloadTexture(&tex);
	leo_CloseWindow();
}
