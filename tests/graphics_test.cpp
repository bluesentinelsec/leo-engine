#include <catch2/catch_all.hpp>
#include <SDL3/SDL.h>

extern "C" {
#include "leo/engine.h"
#include "leo/graphics.h"
}

// Read back a single pixel (x,y) from the current render target via SDL3
static bool read_pixel(SDL_Renderer* r, int x, int y,
	uint8_t& out_r, uint8_t& out_g, uint8_t& out_b, uint8_t& out_a)
{
	SDL_Rect rect{ x, y, 1, 1 };
	SDL_Surface* surf = SDL_RenderReadPixels(r, &rect);
	if (!surf)
		return false;

	// One pixel in the 1x1 surface
	const uint32_t pixel = *static_cast<const uint32_t*>(surf->pixels);

	const SDL_PixelFormatDetails* fmt = SDL_GetPixelFormatDetails(surf->format);
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
	EngineFixture(int w = 8, int h = 8, const char* title = "gfx test")
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

	auto* renderer = static_cast<SDL_Renderer*>(leo_GetRenderer());
	REQUIRE(renderer != nullptr);

	// Create an ARGB8888 render target texture to control the pixel format + alpha.
	SDL_Texture* target = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_TARGET,
		12, 10);
	REQUIRE(target != nullptr);

	// Direct rendering to the texture
	REQUIRE(SDL_SetRenderTarget(renderer, target));

	// Start from a known background (opaque black)
	leo_BeginDrawing();
	leo_ClearBackground(0, 0, 0, 255);

	// Draw one pixel
	const int px = 3, py = 5;
	const leo_Color c = { 255, 64, 32, 255 };
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
	REQUIRE(read_pixel(renderer, px+1, py, r2, g2, b2, a2));
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

	auto* renderer = static_cast<SDL_Renderer*>(leo_GetRenderer());
	REQUIRE(renderer != nullptr);

	// Create a known, readable render target
	SDL_Texture* target = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_TARGET,
		12, 10);
	REQUIRE(target != nullptr);
	REQUIRE(SDL_SetRenderTarget(renderer, target));

	// Common clear
	leo_BeginDrawing();
	leo_ClearBackground(0, 0, 0, 255);

	SECTION("Horizontal line exact coverage")
	{
		const int y = 4;
		const int x0 = 2, x1 = 8;
		const leo_Color c = { 200, 10, 120, 255 };

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
			REQUIRE(read_pixel(renderer, x, y-1, r, g, b, a));
			CHECK(a == 255);
			CHECK(r == 0);
			CHECK(g == 0);
			CHECK(b == 0);

			REQUIRE(read_pixel(renderer, x, y+1, r, g, b, a));
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
		const leo_Color c = { 5, 230, 40, 255 };

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
			REQUIRE(read_pixel(renderer, x-1, y, r, g, b, a));
			CHECK(a == 255);
			CHECK(r == 0);
			CHECK(g == 0);
			CHECK(b == 0);

			REQUIRE(read_pixel(renderer, x+1, y, r, g, b, a));
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
