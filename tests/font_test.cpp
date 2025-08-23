// =============================================
// tests/font_step1_api_test.cpp
// Step 1: API surfaces & defaults (no GPU font load yet)
// =============================================
#include <catch2/catch_all.hpp>

#include "leo/engine.h"
#include "leo/font.h"
#include "leo/color.h"
#include "leo/error.h"

struct GfxEnv
{
	GfxEnv() { REQUIRE(leo_InitWindow(320, 180, "font-step1")); }
	~GfxEnv() { leo_CloseWindow(); }
};

TEST_CASE_METHOD(GfxEnv, "Font API: zero font is not ready", "[font][step1]")
{
	leo_Font f = {};
	CHECK_FALSE(leo_IsFontReady(f));
	CHECK(leo_GetFontBaseSize(f) == 0);
	CHECK(leo_GetFontLineHeight(f, 16.0f) == 0);
}

TEST_CASE_METHOD(GfxEnv, "Font API: default font set/get/clear", "[font][step1]")
{
	leo_Font none = {};
	leo_SetDefaultFont(none);
	auto got = leo_GetDefaultFont();
	CHECK_FALSE(leo_IsFontReady(got));

	// Setting a zero font again should still be fine.
	leo_SetDefaultFont(none);
	got = leo_GetDefaultFont();
	CHECK_FALSE(leo_IsFontReady(got));
}

TEST_CASE_METHOD(GfxEnv, "Font API: draw calls are safe without a font", "[font][step1]")
{
	leo_BeginDrawing();
	leo_ClearBackground(10, 10, 14, 255);

	// Should be no-ops; mainly verifying no crash and no error state.
	leo_DrawText("hello", 8, 8, 16, leo_Color{ 255, 255, 255, 255 });
	leo_DrawFPS(8, 28);

	leo_EndDrawing();

	const char* err = leo_GetError();
	if (err)
	{
		// On step 1, draw should not set an error.
		CHECK(err[0] == '\0');
	}
}
