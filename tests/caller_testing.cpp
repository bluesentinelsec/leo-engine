#include <catch2/catch_all.hpp>

#include "leo/engine.h"
#include "leo/graphics.h"
#include "leo/keyboard.h"
#include "leo/color.h"

#include <SDL3/SDL.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

TEST_CASE("game", "[game]")
{
	const int windowWidth = 800;
	const int windowHeight = 450;

	if (!leo_InitWindow(windowWidth, windowHeight, "leo testing"))
		return;

	REQUIRE(leo_SetLogicalResolution(windowWidth, windowHeight,
		LEO_LOGICAL_PRESENTATION_LETTERBOX, LEO_SCALE_PIXELART) == true);

	const int screenWidth  = leo_GetScreenWidth();
	const int screenHeight = leo_GetScreenHeight();

	srand((unsigned)time(NULL));

	leo_SetTargetFPS(60);

	while (!leo_WindowShouldClose())
	{
		// --- Update ---------------------------------------------------------
		leo_UpdateKeyboard();


		// --- Draw -----------------------------------------------------------
		leo_BeginDrawing();

		leo_EndMode2D();

        break; // comment this when testng

	}

	leo_CloseWindow();
	return;
}
