#include <SDL3/SDL.h>
#include <catch2/catch_all.hpp>

extern "C"
{
#include "leo/engine.h"
#include "leo/graphics.h"
}

// Read back a single pixel (x,y) from the current render target via SDL3
static bool read_pixel(SDL_Renderer *r, int x, int y, uint8_t &out_r, uint8_t &out_g, uint8_t &out_b, uint8_t &out_a)
{
    SDL_Rect rect{x, y, 1, 1};
    SDL_Surface *surf = SDL_RenderReadPixels(r, &rect);
    if (!surf)
        return false;

    // One pixel in the 1x1 surface
    const uint32_t pixel = *static_cast<const uint32_t *>(surf->pixels);

    const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(surf->format);
    if (!fmt)
    {
        SDL_DestroySurface(surf);
        return false;
    }

    SDL_GetRGBA(pixel, fmt, NULL, &out_r, &out_g, &out_b, &out_a);
    SDL_DestroySurface(surf);
    return true;
}

struct EngineFixture
{
    EngineFixture(int w = 8, int h = 8, const char *title = "gfx test")
    {
        REQUIRE(leo_InitWindow(w, h, title) == true);
        REQUIRE(leo_GetWindow() != nullptr);
        REQUIRE(leo_GetRenderer() != nullptr);
    }

    ~EngineFixture()
    {
        leo_CloseWindow();
        // Optional: verify video quit; safe to skip to avoid platform flakiness
        // CHECK((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) == 0);
    }
};

TEST_CASE("leo_DrawPixel draws exactly one pixel with the requested color", "[graphics][pixel]")
{
    EngineFixture fx(12, 10, "pixel-test");

    auto *renderer = static_cast<SDL_Renderer *>(leo_GetRenderer());
    REQUIRE(renderer != nullptr);

    // Create an ARGB8888 render target texture to control the pixel format + alpha.
    SDL_Texture *target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 12, 10);
    REQUIRE(target != nullptr);

    // Direct rendering to the texture
    REQUIRE(SDL_SetRenderTarget(renderer, target));

    // Start from a known background (opaque black)
    leo_BeginDrawing();
    leo_ClearBackground(0, 0, 0, 255);

    // Draw one pixel
    const int px = 3, py = 5;
    const leo_Color c = {255, 64, 32, 255};
    leo_DrawPixel(px, py, c);

    // IMPORTANT: Read BEFORE presenting; we're reading from the render target texture anyway.
    uint8_t r = 0, g = 0, b = 0, a = 0;
    REQUIRE(read_pixel(renderer, px, py, r, g, b, a));
    CHECK(a == 255);
    CHECK(r == 255);
    CHECK(g == 64);
    CHECK(b == 32);

    // Neighbor unchanged (still black, opaque)
    uint8_t r2 = 0, g2 = 0, b2 = 0, a2 = 0;
    REQUIRE(read_pixel(renderer, px + 1, py, r2, g2, b2, a2));
    CHECK(a2 == 255);
    CHECK(r2 == 0);
    CHECK(g2 == 0);
    CHECK(b2 == 0);

    // Restore default target and clean up
    REQUIRE(SDL_SetRenderTarget(renderer, nullptr));
    SDL_DestroyTexture(target);

    // You can present now if you want to complete the frame timing
    leo_EndDrawing();
}

TEST_CASE("leo_DrawLine draws horizontal and vertical lines with correct color", "[graphics][line]")
{
    EngineFixture fx(12, 10, "line-test");

    auto *renderer = static_cast<SDL_Renderer *>(leo_GetRenderer());
    REQUIRE(renderer != nullptr);

    // Create a known, readable render target
    SDL_Texture *target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 12, 10);
    REQUIRE(target != nullptr);
    REQUIRE(SDL_SetRenderTarget(renderer, target));

    // Common clear
    leo_BeginDrawing();
    leo_ClearBackground(0, 0, 0, 255);

    SECTION("Horizontal line exact coverage")
    {
        const int y = 4;
        const int x0 = 2, x1 = 8;
        const leo_Color c = {200, 10, 120, 255};

        leo_DrawLine(x0, y, x1, y, c);

        // Pixels on the line are the requested color
        for (int x = x0; x <= x1; ++x)
        {
            uint8_t r = 0, g = 0, b = 0, a = 0;
            REQUIRE(read_pixel(renderer, x, y, r, g, b, a));
            CHECK(a == 255);
            CHECK(r == 200);
            CHECK(g == 10);
            CHECK(b == 120);
        }

        // Neighbor rows remain black (above and below within the span)
        for (int x = x0; x <= x1; ++x)
        {
            uint8_t r = 0, g = 0, b = 0, a = 0;
            REQUIRE(read_pixel(renderer, x, y - 1, r, g, b, a));
            CHECK(a == 255);
            CHECK(r == 0);
            CHECK(g == 0);
            CHECK(b == 0);

            REQUIRE(read_pixel(renderer, x, y + 1, r, g, b, a));
            CHECK(a == 255);
            CHECK(r == 0);
            CHECK(g == 0);
            CHECK(b == 0);
        }
    }

    SECTION("Vertical line exact coverage")
    {
        // Re-clear before second section
        leo_ClearBackground(0, 0, 0, 255);

        const int x = 6;
        const int y0 = 2, y1 = 8;
        const leo_Color c = {5, 230, 40, 255};

        leo_DrawLine(x, y0, x, y1, c);

        // Pixels on the line are the requested color
        for (int y = y0; y <= y1; ++y)
        {
            uint8_t r = 0, g = 0, b = 0, a = 0;
            REQUIRE(read_pixel(renderer, x, y, r, g, b, a));
            CHECK(a == 255);
            CHECK(r == 5);
            CHECK(g == 230);
            CHECK(b == 40);
        }

        // Neighbor columns remain black (left and right within the span)
        for (int y = y0; y <= y1; ++y)
        {
            uint8_t r = 0, g = 0, b = 0, a = 0;
            REQUIRE(read_pixel(renderer, x - 1, y, r, g, b, a));
            CHECK(a == 255);
            CHECK(r == 0);
            CHECK(g == 0);
            CHECK(b == 0);

            REQUIRE(read_pixel(renderer, x + 1, y, r, g, b, a));
            CHECK(a == 255);
            CHECK(r == 0);
            CHECK(g == 0);
            CHECK(b == 0);
        }
    }

    // Restore & cleanup
    REQUIRE(SDL_SetRenderTarget(renderer, nullptr));
    SDL_DestroyTexture(target);
    leo_EndDrawing();
}

TEST_CASE("leo_DrawCircle draws circles with correct color and coverage", "[graphics][circle]")
{
    EngineFixture fx(20, 20, "circle-test");

    auto *renderer = static_cast<SDL_Renderer *>(leo_GetRenderer());
    REQUIRE(renderer != nullptr);

    // Create a known, readable render target
    SDL_Texture *target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 20, 20);
    REQUIRE(target != nullptr);
    REQUIRE(SDL_SetRenderTarget(renderer, target));

    // Common clear
    leo_BeginDrawing();
    leo_ClearBackground(0, 0, 0, 255);

    SECTION("Small circle (radius 3) exact coverage")
    {
        const int centerX = 10, centerY = 10;
        const float radius = 3.0f;
        const leo_Color c = {100, 150, 200, 255};

        leo_DrawCircle(centerX, centerY, radius, c);

        // Test key points on the circle perimeter
        // Rightmost point
        uint8_t r = 0, g = 0, b = 0, a = 0;
        REQUIRE(read_pixel(renderer, centerX + 3, centerY, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 100);
        CHECK(g == 150);
        CHECK(b == 200);

        // Bottom point
        REQUIRE(read_pixel(renderer, centerX, centerY + 3, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 100);
        CHECK(g == 150);
        CHECK(b == 200);

        // Leftmost point
        REQUIRE(read_pixel(renderer, centerX - 3, centerY, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 100);
        CHECK(g == 150);
        CHECK(b == 200);

        // Top point
        REQUIRE(read_pixel(renderer, centerX, centerY - 3, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 100);
        CHECK(g == 150);
        CHECK(b == 200);

        // Diagonal points (should be drawn)
        REQUIRE(read_pixel(renderer, centerX + 2, centerY + 2, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 100);
        CHECK(g == 150);
        CHECK(b == 200);

        // Test that pixels outside the circle are not drawn
        REQUIRE(read_pixel(renderer, centerX + 4, centerY, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 0);
        CHECK(g == 0);
        CHECK(b == 0);

        REQUIRE(read_pixel(renderer, centerX, centerY + 4, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 0);
        CHECK(g == 0);
        CHECK(b == 0);
    }

    SECTION("Medium circle (radius 5) with different color")
    {
        // Re-clear before second section
        leo_ClearBackground(0, 0, 0, 255);

        const int centerX = 10, centerY = 10;
        const float radius = 5.0f;
        const leo_Color c = {255, 0, 128, 255};

        leo_DrawCircle(centerX, centerY, radius, c);

        // Test key points on the circle perimeter
        uint8_t r = 0, g = 0, b = 0, a = 0;

        // Rightmost point
        REQUIRE(read_pixel(renderer, centerX + 5, centerY, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 255);
        CHECK(g == 0);
        CHECK(b == 128);

        // Bottom point
        REQUIRE(read_pixel(renderer, centerX, centerY + 5, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 255);
        CHECK(g == 0);
        CHECK(b == 128);

        // Test that pixels outside the circle are not drawn
        REQUIRE(read_pixel(renderer, centerX + 6, centerY, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 0);
        CHECK(g == 0);
        CHECK(b == 0);
    }

    SECTION("Circle at edge of texture")
    {
        // Re-clear before third section
        leo_ClearBackground(0, 0, 0, 255);

        const int centerX = 5, centerY = 5;
        const float radius = 4.0f;
        const leo_Color c = {0, 255, 0, 255};

        leo_DrawCircle(centerX, centerY, radius, c);

        // Test that the circle is drawn correctly even near edges
        uint8_t r = 0, g = 0, b = 0, a = 0;

        // Rightmost point (should be at x=9)
        REQUIRE(read_pixel(renderer, centerX + 4, centerY, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 0);
        CHECK(g == 255);
        CHECK(b == 0);

        // Test that pixels outside the circle are not drawn
        REQUIRE(read_pixel(renderer, centerX + 5, centerY, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 0);
        CHECK(g == 0);
        CHECK(b == 0);
    }

    SECTION("Float radius handling")
    {
        // Re-clear before fourth section
        leo_ClearBackground(0, 0, 0, 255);

        const int centerX = 10, centerY = 10;
        const float radius = 2.7f; // Should round to 3
        const leo_Color c = {128, 128, 128, 255};

        leo_DrawCircle(centerX, centerY, radius, c);

        // Test that the circle is drawn with rounded radius
        uint8_t r = 0, g = 0, b = 0, a = 0;

        // Should draw at radius 3 (rounded from 2.7)
        REQUIRE(read_pixel(renderer, centerX + 3, centerY, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 128);
        CHECK(g == 128);
        CHECK(b == 128);

        // Should not draw at radius 4
        REQUIRE(read_pixel(renderer, centerX + 4, centerY, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 0);
        CHECK(g == 0);
        CHECK(b == 0);
    }

    // Restore & cleanup
    REQUIRE(SDL_SetRenderTarget(renderer, nullptr));
    SDL_DestroyTexture(target);
    leo_EndDrawing();
}

TEST_CASE("leo_DrawRectangle draws filled rectangles with correct color and coverage", "[graphics][rectangle]")
{
    EngineFixture fx(20, 20, "rectangle-test");

    auto *renderer = static_cast<SDL_Renderer *>(leo_GetRenderer());
    REQUIRE(renderer != nullptr);

    // Create a known, readable render target
    SDL_Texture *target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 20, 20);
    REQUIRE(target != nullptr);
    REQUIRE(SDL_SetRenderTarget(renderer, target));

    // Common clear
    leo_BeginDrawing();
    leo_ClearBackground(0, 0, 0, 255);

    SECTION("Small rectangle (5x3) exact coverage")
    {
        const int posX = 5, posY = 5;
        const int width = 5, height = 3;
        const leo_Color c = {100, 150, 200, 255};

        leo_DrawRectangle(posX, posY, width, height, c);

        // Test corners of the rectangle
        uint8_t r = 0, g = 0, b = 0, a = 0;

        // Top-left corner
        REQUIRE(read_pixel(renderer, posX, posY, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 100);
        CHECK(g == 150);
        CHECK(b == 200);

        // Top-right corner
        REQUIRE(read_pixel(renderer, posX + width - 1, posY, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 100);
        CHECK(g == 150);
        CHECK(b == 200);

        // Bottom-left corner
        REQUIRE(read_pixel(renderer, posX, posY + height - 1, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 100);
        CHECK(g == 150);
        CHECK(b == 200);

        // Bottom-right corner
        REQUIRE(read_pixel(renderer, posX + width - 1, posY + height - 1, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 100);
        CHECK(g == 150);
        CHECK(b == 200);

        // Test center of the rectangle
        REQUIRE(read_pixel(renderer, posX + width / 2, posY + height / 2, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 100);
        CHECK(g == 150);
        CHECK(b == 200);

        // Test that pixels outside the rectangle are not drawn
        REQUIRE(read_pixel(renderer, posX - 1, posY, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 0);
        CHECK(g == 0);
        CHECK(b == 0);

        REQUIRE(read_pixel(renderer, posX + width, posY, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 0);
        CHECK(g == 0);
        CHECK(b == 0);

        REQUIRE(read_pixel(renderer, posX, posY - 1, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 0);
        CHECK(g == 0);
        CHECK(b == 0);

        REQUIRE(read_pixel(renderer, posX, posY + height, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 0);
        CHECK(g == 0);
        CHECK(b == 0);
    }

    SECTION("Large rectangle (8x6) with different color")
    {
        // Re-clear before second section
        leo_ClearBackground(0, 0, 0, 255);

        const int posX = 2, posY = 2;
        const int width = 8, height = 6;
        const leo_Color c = {255, 0, 128, 255};

        leo_DrawRectangle(posX, posY, width, height, c);

        // Test key points inside the rectangle
        uint8_t r = 0, g = 0, b = 0, a = 0;

        // Top-left corner
        REQUIRE(read_pixel(renderer, posX, posY, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 255);
        CHECK(g == 0);
        CHECK(b == 128);

        // Bottom-right corner
        REQUIRE(read_pixel(renderer, posX + width - 1, posY + height - 1, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 255);
        CHECK(g == 0);
        CHECK(b == 128);

        // Center point
        REQUIRE(read_pixel(renderer, posX + width / 2, posY + height / 2, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 255);
        CHECK(g == 0);
        CHECK(b == 128);

        // Test that pixels outside are not drawn
        REQUIRE(read_pixel(renderer, posX + width, posY, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 0);
        CHECK(g == 0);
        CHECK(b == 0);
    }

    SECTION("Rectangle at edge of texture")
    {
        // Re-clear before third section
        leo_ClearBackground(0, 0, 0, 255);

        const int posX = 15, posY = 15;
        const int width = 4, height = 4;
        const leo_Color c = {0, 255, 0, 255};

        leo_DrawRectangle(posX, posY, width, height, c);

        // Test that the rectangle is drawn correctly even near edges
        uint8_t r = 0, g = 0, b = 0, a = 0;

        // Top-left corner (should be at 15,15)
        REQUIRE(read_pixel(renderer, posX, posY, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 0);
        CHECK(g == 255);
        CHECK(b == 0);

        // Bottom-right corner (should be at 18,18)
        REQUIRE(read_pixel(renderer, posX + width - 1, posY + height - 1, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 0);
        CHECK(g == 255);
        CHECK(b == 0);

        // Test that pixels outside the texture bounds are not drawn
        // This should fail gracefully or not draw
        REQUIRE(read_pixel(renderer, posX + width, posY, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 0);
        CHECK(g == 0);
        CHECK(b == 0);
    }

    SECTION("Single pixel rectangle")
    {
        // Re-clear before fourth section
        leo_ClearBackground(0, 0, 0, 255);

        const int posX = 10, posY = 10;
        const int width = 1, height = 1;
        const leo_Color c = {128, 128, 128, 255};

        leo_DrawRectangle(posX, posY, width, height, c);

        // Test that the single pixel is drawn
        uint8_t r = 0, g = 0, b = 0, a = 0;

        REQUIRE(read_pixel(renderer, posX, posY, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 128);
        CHECK(g == 128);
        CHECK(b == 128);

        // Test that neighboring pixels are not drawn
        REQUIRE(read_pixel(renderer, posX + 1, posY, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 0);
        CHECK(g == 0);
        CHECK(b == 0);

        REQUIRE(read_pixel(renderer, posX, posY + 1, r, g, b, a));
        CHECK(a == 255);
        CHECK(r == 0);
        CHECK(g == 0);
        CHECK(b == 0);
    }

    // Restore & cleanup
    REQUIRE(SDL_SetRenderTarget(renderer, nullptr));
    SDL_DestroyTexture(target);
    leo_EndDrawing();
}
TEST_CASE("leo_DrawCircleFilled draws filled circles with correct color and coverage", "[graphics][circle][filled]")
{
    EngineFixture fx(20, 20, "circle-filled-test");

    auto *renderer = static_cast<SDL_Renderer *>(leo_GetRenderer());
    REQUIRE(renderer != nullptr);

    SDL_Texture *target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 20, 20);
    REQUIRE(target != nullptr);
    REQUIRE(SDL_SetRenderTarget(renderer, target));

    leo_BeginDrawing();
    leo_ClearBackground(0, 0, 0, 255);

    const int centerX = 10, centerY = 10;
    const float radius = 3.0f;
    const leo_Color c = {255, 128, 64, 255};

    leo_DrawCircleFilled(centerX, centerY, radius, c);

    // Test center is filled
    uint8_t r = 0, g = 0, b = 0, a = 0;
    REQUIRE(read_pixel(renderer, centerX, centerY, r, g, b, a));
    CHECK(r == 255);
    CHECK(g == 128);
    CHECK(b == 64);

    // Test interior points are filled
    REQUIRE(read_pixel(renderer, centerX + 1, centerY, r, g, b, a));
    CHECK(r == 255);
    CHECK(g == 128);
    CHECK(b == 64);

    REQUIRE(SDL_SetRenderTarget(renderer, nullptr));
    SDL_DestroyTexture(target);
    leo_EndDrawing();
}

TEST_CASE("leo_DrawRectangleLines draws rectangle outlines with correct color", "[graphics][rectangle][lines]")
{
    EngineFixture fx(20, 20, "rectangle-lines-test");

    auto *renderer = static_cast<SDL_Renderer *>(leo_GetRenderer());
    REQUIRE(renderer != nullptr);

    SDL_Texture *target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 20, 20);
    REQUIRE(target != nullptr);
    REQUIRE(SDL_SetRenderTarget(renderer, target));

    leo_BeginDrawing();
    leo_ClearBackground(0, 0, 0, 255);

    const int posX = 5, posY = 5;
    const int width = 6, height = 4;
    const leo_Color c = {200, 100, 50, 255};

    leo_DrawRectangleLines(posX, posY, width, height, c);

    // Test corners are drawn
    uint8_t r = 0, g = 0, b = 0, a = 0;
    REQUIRE(read_pixel(renderer, posX, posY, r, g, b, a));
    CHECK(r == 200);
    CHECK(g == 100);
    CHECK(b == 50);

    // Test interior is NOT filled
    REQUIRE(read_pixel(renderer, posX + 1, posY + 1, r, g, b, a));
    CHECK(r == 0);
    CHECK(g == 0);
    CHECK(b == 0);

    REQUIRE(SDL_SetRenderTarget(renderer, nullptr));
    SDL_DestroyTexture(target);
    leo_EndDrawing();
}

TEST_CASE("leo_DrawTriangle draws triangle outlines with correct color", "[graphics][triangle]")
{
    EngineFixture fx(20, 20, "triangle-test");

    auto *renderer = static_cast<SDL_Renderer *>(leo_GetRenderer());
    REQUIRE(renderer != nullptr);

    SDL_Texture *target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 20, 20);
    REQUIRE(target != nullptr);
    REQUIRE(SDL_SetRenderTarget(renderer, target));

    leo_BeginDrawing();
    leo_ClearBackground(0, 0, 0, 255);

    const leo_Color c = {150, 200, 100, 255};
    leo_DrawTriangle(10, 5, 5, 15, 15, 15, c);

    // Test vertices are drawn
    uint8_t r = 0, g = 0, b = 0, a = 0;
    REQUIRE(read_pixel(renderer, 10, 5, r, g, b, a));
    CHECK(r == 150);
    CHECK(g == 200);
    CHECK(b == 100);

    REQUIRE(SDL_SetRenderTarget(renderer, nullptr));
    SDL_DestroyTexture(target);
    leo_EndDrawing();
}

TEST_CASE("leo_DrawTriangleFilled draws filled triangles with correct color", "[graphics][triangle][filled]")
{
    EngineFixture fx(20, 20, "triangle-filled-test");

    auto *renderer = static_cast<SDL_Renderer *>(leo_GetRenderer());
    REQUIRE(renderer != nullptr);

    SDL_Texture *target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 20, 20);
    REQUIRE(target != nullptr);
    REQUIRE(SDL_SetRenderTarget(renderer, target));

    leo_BeginDrawing();
    leo_ClearBackground(0, 0, 0, 255);

    const leo_Color c = {100, 255, 150, 255};
    leo_DrawTriangleFilled(10, 5, 5, 15, 15, 15, c);

    // Test interior is filled
    uint8_t r = 0, g = 0, b = 0, a = 0;
    REQUIRE(read_pixel(renderer, 10, 10, r, g, b, a));
    CHECK(r == 100);
    CHECK(g == 255);
    CHECK(b == 150);

    REQUIRE(SDL_SetRenderTarget(renderer, nullptr));
    SDL_DestroyTexture(target);
    leo_EndDrawing();
}

TEST_CASE("leo_DrawPoly draws polygon outlines with correct color", "[graphics][polygon]")
{
    EngineFixture fx(20, 20, "polygon-test");

    auto *renderer = static_cast<SDL_Renderer *>(leo_GetRenderer());
    REQUIRE(renderer != nullptr);

    SDL_Texture *target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 20, 20);
    REQUIRE(target != nullptr);
    REQUIRE(SDL_SetRenderTarget(renderer, target));

    leo_BeginDrawing();
    leo_ClearBackground(0, 0, 0, 255);

    int points[] = {10, 5, 15, 10, 10, 15, 5, 10};
    const leo_Color c = {255, 200, 100, 255};
    leo_DrawPoly(points, 4, c);

    // Test first vertex is drawn
    uint8_t r = 0, g = 0, b = 0, a = 0;
    REQUIRE(read_pixel(renderer, 10, 5, r, g, b, a));
    CHECK(r == 255);
    CHECK(g == 200);
    CHECK(b == 100);

    REQUIRE(SDL_SetRenderTarget(renderer, nullptr));
    SDL_DestroyTexture(target);
    leo_EndDrawing();
}

TEST_CASE("leo_DrawPolyFilled draws filled polygons with correct color", "[graphics][polygon][filled]")
{
    EngineFixture fx(20, 20, "polygon-filled-test");

    auto *renderer = static_cast<SDL_Renderer *>(leo_GetRenderer());
    REQUIRE(renderer != nullptr);

    SDL_Texture *target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 20, 20);
    REQUIRE(target != nullptr);
    REQUIRE(SDL_SetRenderTarget(renderer, target));

    leo_BeginDrawing();
    leo_ClearBackground(0, 0, 0, 255);

    int points[] = {10, 5, 15, 10, 10, 15, 5, 10};
    const leo_Color c = {50, 150, 255, 255};
    leo_DrawPolyFilled(points, 4, c);

    // Test interior is filled
    uint8_t r = 0, g = 0, b = 0, a = 0;
    REQUIRE(read_pixel(renderer, 10, 10, r, g, b, a));
    CHECK(r == 50);
    CHECK(g == 150);
    CHECK(b == 255);

    REQUIRE(SDL_SetRenderTarget(renderer, nullptr));
    SDL_DestroyTexture(target);
    leo_EndDrawing();
}
