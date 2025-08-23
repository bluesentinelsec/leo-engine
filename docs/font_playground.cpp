#include <catch2/catch_all.hpp>

#include "leo/engine.h"
#include "leo/graphics.h"
#include "leo/color.h"
#include "leo/audio.h"
#include "leo/font.h"
#include "leo/error.h"

#include <cstdlib>   // getenv
#include <cmath>     // sinf, fmodf
#include <cstdio>    // snprintf

// Tiny helpers
static inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

static inline bool is_ci()
{
	const char* ci = std::getenv("CI");
	return ci && (ci[0] != '\0');
}

TEST_CASE("game", "[game]")
{
	// Window + timing
	REQUIRE(leo_InitWindow(960, 540, "Leo Font Playground"));
	leo_SetTargetFPS(60);

	// Load a font and set default (deterministic: no hidden auto load)
	leo_Font font = leo_LoadFont("resources/font/font.ttf", 24);
	if (!leo_IsFontReady(font))
	{
		// Graceful exit if the font isn’t present
		const char* err = leo_GetError();
		WARN(std::string("Could not load resources/font/font.ttf — skipping playground. Err: ")
			+ (err ? err : "(none)"));
		leo_ClearError();
		leo_CloseWindow();
		SUCCEED();
		return;
	}
	leo_SetDefaultFont(font);

	// Animation state
	float t = 0.0f;
	float baseSize = 24.0f;

	// CI mode: render one frame and exit fast
	const bool CI = is_ci();
	const double demoDuration = CI ? 0.01 : 1e9; // effectively infinite if not CI
	const double startTime = leo_GetTime();

	while (!leo_WindowShouldClose() && (leo_GetTime() - startTime) < demoDuration)
	{
		t += leo_GetFrameTime();

		// Animate parameters
		float size = baseSize + 12.0f * std::sinf(t * 1.2f); // 24..36
		float spacing = 2.0f + 2.0f * std::sinf(t * 0.7f); // 0..4 approx (clamped next)
		float rot = 15.0f * std::sinf(t * 0.9f); // -15..+15

		spacing = clampf(spacing, 0.0f, 6.0f);
		size = clampf(size, 14.0f, 48.0f);

		// Measure a sample string with current params
		const char* sample = "The quick brown fox";
		leo_Vector2 measure = leo_MeasureTextEx(font, sample, size, spacing);

		// Frame
		leo_BeginDrawing();
		leo_ClearBackground(18, 20, 26, 255);

		// Title
		leo_DrawText("Leo Font Playground", 20, 14, 24, (leo_Color){ 240, 242, 255, 255 });

		// HUD: parameters and measurement
		char hud[256];
		SDL_snprintf(hud, sizeof(hud),
			"size: %.1f  spacing: %.1f  rot: %.1f  |  measure(\"%s\") = (%.1f, %.1f)",
			size, spacing, rot, sample, measure.x, measure.y);
		leo_DrawText(hud, 20, 46, 18, (leo_Color){ 200, 220, 255, 255 });

		// DrawText (default font, no spacing/rotation)
		leo_DrawText("DrawText (default font)", 20, 84, (int)size, (leo_Color){ 255, 255, 255, 255 });

		// DrawTextEx (explicit font + spacing)
		leo_DrawTextEx(font, "DrawTextEx (spacing demo) →", (leo_Vector2){ 20, 128 },
			size, spacing, (leo_Color){ 210, 235, 255, 255 });
		leo_DrawTextEx(font, "A B C 1 2 3 ! ?", (leo_Vector2){ 20, 160 },
			size, spacing, (leo_Color){ 210, 235, 255, 255 });

		// DrawTextPro (rotation around origin)
		// We'll rotate around the left/top of the string for clarity.
		leo_Vector2 pos = { 20.0f, 220.0f };
		leo_Vector2 origin = { 0.0f, 0.0f };
		leo_DrawTextPro(font, "DrawTextPro (rotation demo)", pos, origin, rot,
			size, 1.0f, (leo_Color){ 255, 230, 150, 255 });

		// Show baseline / measurement guide (simple line)
		// draw a minimal guide using a texture rect if you like — here we just draw more text hints.
		leo_DrawText("Guide: top-left anchored; rotating about origin (0,0)", 20, 260, 16,
			(leo_Color){ 170, 190, 210, 255 });

		// FPS
		leo_DrawFPS(20, 500);

		leo_EndDrawing();

		if (CI) break; // single frame for CI
	}

	// Cleanup
	leo_UnloadFont(&font);
	leo_SetDefaultFont((leo_Font){ 0 });
	leo_CloseWindow();

	SUCCEED();
}
