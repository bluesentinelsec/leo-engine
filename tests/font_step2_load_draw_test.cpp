// =============================================
// tests/font_step2_load_draw_test.cpp
// Step 2: real font loading + draw + measurement
// =============================================
#include <catch2/catch_all.hpp>

#include "leo/engine.h"
#include "leo/font.h"
#include "leo/color.h"
#include "leo/error.h"

struct GfxEnv2
{
	GfxEnv2() { REQUIRE(leo_InitWindow(640, 360, "font-step2")); }
	~GfxEnv2() { leo_CloseWindow(); }
};

TEST_CASE_METHOD(GfxEnv2, "Font load from file is ready", "[font][step2]")
{
	// Use your repo path
	auto font = leo_LoadFont("resources/font/font.ttf", 32);
	REQUIRE(leo_IsFontReady(font));
	CHECK(font.baseSize == 32);
	CHECK(font.glyphCount > 0);
	CHECK(font.lineHeight > 0);

	// Measurements should scale with size
	auto m16 = leo_MeasureTextEx(font, "ABC", 16.0f, 0.0f);
	auto m32 = leo_MeasureTextEx(font, "ABC", 32.0f, 0.0f);
	CHECK(m32.x > m16.x);
	CHECK(m32.y > m16.y);

	leo_UnloadFont(&font);
	CHECK_FALSE(leo_IsFontReady(font));
}

TEST_CASE_METHOD(GfxEnv2, "DrawTextEx and DrawText are safe and render", "[font][step2]")
{
	auto font = leo_LoadFont("resources/font/font.ttf", 24);
	REQUIRE(leo_IsFontReady(font));
	leo_SetDefaultFont(font);

	leo_BeginDrawing();
	leo_ClearBackground(20, 22, 28, 255);

	// Should not crash; rendering correctness is manually verifiable if needed.
	leo_DrawText("Hello, Leo!", 16, 16, 24, (leo_Color){ 255, 255, 255, 255 });
	leo_DrawTextEx(font, "Spacing", (leo_Vector2){ 16, 56 }, 24.0f, 2.0f, (leo_Color){ 200, 240, 255, 255 });
	leo_DrawFPS(16, 88);

	leo_EndDrawing();

	// No error expected
	const char* err = leo_GetError();
	CHECK(((err == nullptr) || (err[0] == '\0')));

	leo_UnloadFont(&font);
	leo_SetDefaultFont((leo_Font){ 0 });
}
