// =============================================
// tests/font_step4_memory_errorpaths_test.cpp
// Step 4: FromMemory parity + error-path robustness
// =============================================
#include <catch2/catch_all.hpp>

#include "leo/color.h"
#include "leo/engine.h"
#include "leo/error.h"
#include "leo/font.h"

#include <cstdio>
#include <vector>

struct GfxEnv4
{
    GfxEnv4()
    {
        REQUIRE(leo_InitWindow(640, 360, "font-step4"));
    }
    ~GfxEnv4()
    {
        leo_CloseWindow();
    }
};

static std::vector<unsigned char> read_all(const char *path)
{
    std::vector<unsigned char> buf;
    FILE *f = std::fopen(path, "rb");
    REQUIRE(f != nullptr);
    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    buf.resize((size_t)sz);
    REQUIRE(std::fread(buf.data(), 1, (size_t)sz, f) == (size_t)sz);
    std::fclose(f);
    return buf;
}

TEST_CASE_METHOD(GfxEnv4, "LoadFontFromMemory ~parity with LoadFont", "[font][step4]")
{
    const char *path = "resources/font/font.ttf";
    auto fileFont = leo_LoadFont(path, 24);
    REQUIRE(leo_IsFontReady(fileFont));

    auto bytes = read_all(path);
    auto memFont = leo_LoadFontFromMemory(".ttf", bytes.data(), (int)bytes.size(), 24);
    REQUIRE(leo_IsFontReady(memFont));

    // Basic metrics should match or be extremely close
    CHECK(memFont.baseSize == fileFont.baseSize);
    CHECK(memFont.glyphCount == fileFont.glyphCount);
    CHECK(memFont.lineHeight == fileFont.lineHeight);

    // Measure the same string; allow tiny float/rounding noise
    const char *s = "Memory==File?";
    auto mFile = leo_MeasureTextEx(fileFont, s, 24.0f, 1.0f);
    auto mMem = leo_MeasureTextEx(memFont, s, 24.0f, 1.0f);
    CHECK(mMem.x == Catch::Approx(mFile.x).margin(0.5f));
    CHECK(mMem.y == Catch::Approx(mFile.y).margin(0.5f));

    leo_UnloadFont(&memFont);
    leo_UnloadFont(&fileFont);
}

TEST_CASE_METHOD(GfxEnv4, "Error paths: bad args set error and return not-ready", "[font][step4]")
{
    // Bad path
    {
        auto f = leo_LoadFont(nullptr, 24);
        CHECK_FALSE(leo_IsFontReady(f));
        const char *err = leo_GetError();
        REQUIRE(err != nullptr);
        CHECK(std::strlen(err) > 0);
        leo_ClearError();
    }
    // Bad size
    {
        auto f = leo_LoadFont("resources/font/font.ttf", 0);
        CHECK_FALSE(leo_IsFontReady(f));
        const char *err = leo_GetError();
        REQUIRE(err != nullptr);
        CHECK(std::strlen(err) > 0);
        leo_ClearError();
    }
    // Bad memory buffer
    {
        auto f = leo_LoadFontFromMemory(".ttf", nullptr, 0, 24);
        CHECK_FALSE(leo_IsFontReady(f));
        const char *err = leo_GetError();
        REQUIRE(err != nullptr);
        CHECK(std::strlen(err) > 0);
        leo_ClearError();
    }
}

TEST_CASE_METHOD(GfxEnv4, "Unload is safe and idempotent; draw after unload is a no-op", "[font][step4]")
{
    auto font = leo_LoadFont("resources/font/font.ttf", 24);
    REQUIRE(leo_IsFontReady(font));

    // Make it the default, then unload it; draw should no-op, no errors.
    leo_SetDefaultFont(font);
    leo_UnloadFont(&font);
    CHECK_FALSE(leo_IsFontReady(font));

    leo_BeginDrawing();
    leo_ClearBackground(8, 10, 16, 255);
    leo_DrawText("after unload", 10, 10, 24, (leo_Color){255, 255, 255, 255});
    leo_EndDrawing();

    const char *err = leo_GetError();
    CHECK((!err) || (err[0] == '\0'));

    // Double-unload should be safe.
    leo_UnloadFont(&font);
    CHECK_FALSE(leo_IsFontReady(font));
}

TEST_CASE_METHOD(GfxEnv4, "MeasureText falls back to 0 for not-ready fonts", "[font][step4]")
{
    leo_Font zero = {};
    auto m = leo_MeasureTextEx(zero, "noop", 24.0f, 0.0f);
    CHECK(m.x == 0.0f);
    CHECK(m.y == 0.0f);
    CHECK(leo_MeasureText("noop", 24) == 0);
}
