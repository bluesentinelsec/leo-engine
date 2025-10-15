// =============================================
// tests/font_cache_test.cpp
// Verify text cache works correctly
// =============================================
#include <catch2/catch_all.hpp>

#include "leo/leo.h"

struct CacheEnv
{
    CacheEnv()
    {
        REQUIRE(leo_InitWindow(320, 240, "cache-test"));
    }
    ~CacheEnv()
    {
        leo_CloseWindow();
    }
};

TEST_CASE_METHOD(CacheEnv, "Text cache: same text renders without error", "[font][cache]")
{
    auto font = leo_LoadFont("resources/font/font.ttf", 16);
    REQUIRE(leo_IsFontReady(font));

    leo_BeginDrawing();
    leo_ClearBackground(0, 0, 0, 255);

    // First render - cache miss
    leo_DrawTextEx(font, "Cached Text", leo_Vector2{10, 10}, 16.0f, 0.0f, leo_Color{255, 255, 255, 255});

    // Second render - should be cache hit
    leo_DrawTextEx(font, "Cached Text", leo_Vector2{10, 30}, 16.0f, 0.0f, leo_Color{255, 255, 255, 255});

    // Different text - cache miss
    leo_DrawTextEx(font, "Different Text", leo_Vector2{10, 50}, 16.0f, 0.0f, leo_Color{255, 255, 255, 255});

    leo_EndDrawing();

    // Should not have any errors
    const char *err = leo_GetError();
    CHECK(((err == nullptr) || (err[0] == '\0')));

    leo_UnloadFont(&font);
}
