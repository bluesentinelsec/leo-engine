// =============================================
// tests/font_dynamic_rendering_test.cpp
// Test dynamic font rendering with changing strings
// =============================================
#include <catch2/catch_all.hpp>

#include "leo/color.h"
#include "leo/engine.h"
#include "leo/error.h"
#include "leo/font.h"
#include <SDL3/SDL.h>

struct DynamicFontEnv
{
    DynamicFontEnv()
    {
        REQUIRE(leo_InitWindow(640, 360, "font-dynamic"));
    }
    ~DynamicFontEnv()
    {
        leo_CloseWindow();
    }
};

TEST_CASE_METHOD(DynamicFontEnv, "Dynamic font rendering with SDL_snprintf", "[font][dynamic]")
{
    auto font = leo_LoadFont("resources/font/font.ttf", 24);
    REQUIRE(leo_IsFontReady(font));

    char buffer[256];
    
    // Test multiple dynamic string formats
    for (int i = 0; i < 5; i++)
    {
        leo_BeginDrawing();
        leo_ClearBackground(20, 22, 28, 255);

        // Test counter rendering
        SDL_snprintf(buffer, sizeof(buffer), "Counter: %d", i);
        leo_DrawTextEx(font, buffer, leo_Vector2{16, 16}, 24.0f, 1.0f, LEO_WHITE);

        // Test float rendering
        SDL_snprintf(buffer, sizeof(buffer), "Float: %.2f", i * 1.5f);
        leo_DrawTextEx(font, buffer, leo_Vector2{16, 50}, 24.0f, 1.0f, LEO_GREEN);

        // Test string formatting
        SDL_snprintf(buffer, sizeof(buffer), "Frame %d of 5", i + 1);
        leo_DrawTextEx(font, buffer, leo_Vector2{16, 84}, 24.0f, 1.0f, LEO_BLUE);

        leo_EndDrawing();

        // Verify no errors occurred
        const char *err = leo_GetError();
        CHECK(((err == nullptr) || (err[0] == '\0')));
    }

    leo_UnloadFont(&font);
}

TEST_CASE_METHOD(DynamicFontEnv, "Dynamic font rendering with standard snprintf", "[font][dynamic]")
{
    auto font = leo_LoadFont("resources/font/font.ttf", 24);
    REQUIRE(leo_IsFontReady(font));

    char buffer[256];
    
    // Test with standard snprintf to verify it also works
    for (int i = 0; i < 3; i++)
    {
        leo_BeginDrawing();
        leo_ClearBackground(20, 22, 28, 255);

        // Test counter rendering with standard snprintf
        snprintf(buffer, sizeof(buffer), "Standard Counter: %d", i);
        leo_DrawTextEx(font, buffer, leo_Vector2{16, 16}, 24.0f, 1.0f, LEO_WHITE);

        leo_EndDrawing();

        // Verify no errors occurred
        const char *err = leo_GetError();
        CHECK(((err == nullptr) || (err[0] == '\0')));
    }

    leo_UnloadFont(&font);
}

TEST_CASE_METHOD(DynamicFontEnv, "Dynamic font measurement consistency", "[font][dynamic]")
{
    auto font = leo_LoadFont("resources/font/font.ttf", 24);
    REQUIRE(leo_IsFontReady(font));

    char buffer[256];
    
    // Test that measurement works correctly with dynamic strings
    SDL_snprintf(buffer, sizeof(buffer), "Test %d", 123);
    auto size1 = leo_MeasureTextEx(font, buffer, 24.0f, 0.0f);
    
    SDL_snprintf(buffer, sizeof(buffer), "Test %d", 456);
    auto size2 = leo_MeasureTextEx(font, buffer, 24.0f, 0.0f);
    
    // Both strings should have same width (same number of characters)
    CHECK(size1.x == size2.x);
    CHECK(size1.y == size2.y);
    
    // Different length strings should have different widths
    SDL_snprintf(buffer, sizeof(buffer), "Test %d Extra", 123);
    auto size3 = leo_MeasureTextEx(font, buffer, 24.0f, 0.0f);
    CHECK(size3.x > size1.x);

    leo_UnloadFont(&font);
}

TEST_CASE_METHOD(DynamicFontEnv, "Dynamic font rendering with special characters", "[font][dynamic]")
{
    auto font = leo_LoadFont("resources/font/font.ttf", 24);
    REQUIRE(leo_IsFontReady(font));

    char buffer[256];
    
    leo_BeginDrawing();
    leo_ClearBackground(20, 22, 28, 255);

    // Test various format specifiers and special characters
    SDL_snprintf(buffer, sizeof(buffer), "Percent: %d%%", 75);
    leo_DrawTextEx(font, buffer, leo_Vector2{16, 16}, 24.0f, 1.0f, LEO_WHITE);

    SDL_snprintf(buffer, sizeof(buffer), "Hex: 0x%X", 255);
    leo_DrawTextEx(font, buffer, leo_Vector2{16, 50}, 24.0f, 1.0f, LEO_GREEN);

    SDL_snprintf(buffer, sizeof(buffer), "Colon: Time 12:34:56");
    leo_DrawTextEx(font, buffer, leo_Vector2{16, 84}, 24.0f, 1.0f, LEO_BLUE);

    leo_EndDrawing();

    // Verify no errors occurred
    const char *err = leo_GetError();
    CHECK(((err == nullptr) || (err[0] == '\0')));

    leo_UnloadFont(&font);
}
