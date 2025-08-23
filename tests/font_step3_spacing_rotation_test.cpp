// =============================================
// tests/font_step3_spacing_rotation_test.cpp
// Step 3: spacing correctness, rotation sanity, line-height scaling
// =============================================
#include <catch2/catch_all.hpp>

#include "leo/engine.h"
#include "leo/font.h"
#include "leo/color.h"
#include "leo/error.h"

struct GfxEnv3 {
    GfxEnv3()  { REQUIRE(leo_InitWindow(640, 360, "font-step3")); }
    ~GfxEnv3() { leo_CloseWindow(); }
};

TEST_CASE_METHOD(GfxEnv3, "MeasureTextEx: spacing increases width but no trailing gap", "[font][step3]")
{
    auto font = leo_LoadFont("resources/font/font.ttf", 24);
    REQUIRE(leo_IsFontReady(font));

    const char* s = "ABC";
    auto m0 = leo_MeasureTextEx(font, s, 24.0f, 0.0f);
    auto m2 = leo_MeasureTextEx(font, s, 24.0f, 2.0f);

    // Spacing should increase width
    CHECK(m2.x > m0.x);

    // With N glyphs, spacing should be applied (N-1) times, not N times.
    // We can sanity-check that the increment is bounded by 2 * (len-1).
    const int len = 3;
    float extra = m2.x - m0.x;
    CHECK(extra > 0.0f);
    CHECK(extra <= Catch::Approx(2.0f * (len - 1) + 0.01f)); // allow tiny float noise

    leo_UnloadFont(&font);
}

TEST_CASE_METHOD(GfxEnv3, "DrawTextPro: rotation path renders without crashing", "[font][step3]")
{
    auto font = leo_LoadFont("resources/font/font.ttf", 24);
    REQUIRE(leo_IsFontReady(font));

    leo_BeginDrawing();
    leo_ClearBackground(30, 32, 40, 255);

    // Rotate around the start of the string
    leo_DrawTextPro(font, "Rotate!", leo_Vector2{320, 180}, leo_Vector2{320, 180},
                    30.0f, 24.0f, 1.0f, leo_Color{255, 220, 120, 255});
    leo_EndDrawing();

    const char* err = leo_GetError();
    CHECK((!err || err[0] == '\0'));

    leo_UnloadFont(&font);
}

TEST_CASE_METHOD(GfxEnv3, "GetFontLineHeight scales with fontSize", "[font][step3]")
{
    auto font = leo_LoadFont("resources/font/font.ttf", 24);
    REQUIRE(leo_IsFontReady(font));

    int lh24 = leo_GetFontLineHeight(font, 24.0f);
    int lh48 = leo_GetFontLineHeight(font, 48.0f);

    CHECK(lh24 > 0);
    CHECK(lh48 > lh24);
    // 48 should be roughly double 24; allow slack for rounding
    CHECK(lh48 >= lh24 * 2 - 1);

    leo_UnloadFont(&font);
}
