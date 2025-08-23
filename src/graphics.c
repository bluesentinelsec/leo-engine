#include "leo/graphics.h"
#include "leo/engine.h"

#include <SDL3/SDL.h>

static inline SDL_Renderer* _gfxRenderer(void)
{
	return (SDL_Renderer*)leo_GetRenderer();
}

static inline void _gfxSetColor(SDL_Renderer* r, leo_Color c)
{
	SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

static inline leo_Vector2 _toScreen(float x, float y)
{
	const leo_Camera2D cam = leo_GetCurrentCamera2D();
	return leo_GetWorldToScreen2D((leo_Vector2){ x, y }, cam);
}

void leo_DrawPixel(int posX, int posY, leo_Color color)
{
	SDL_Renderer* r = _gfxRenderer();
	if (!r) return;

	_gfxSetColor(r, color);

	// World -> screen
	leo_Vector2 s = _toScreen((float)posX, (float)posY);
	SDL_RenderPoint(r, (int)(s.x + 0.5f), (int)(s.y + 0.5f));
}

void leo_DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, leo_Color color)
{
	SDL_Renderer* r = _gfxRenderer();
	if (!r) return;

	_gfxSetColor(r, color);

	// World -> screen
	leo_Vector2 a = _toScreen((float)startPosX, (float)startPosY);
	leo_Vector2 b = _toScreen((float)endPosX, (float)endPosY);
	SDL_RenderLine(r, (int)(a.x + 0.5f), (int)(a.y + 0.5f),
		(int)(b.x + 0.5f), (int)(b.y + 0.5f));
}

void leo_DrawCircle(int centerX, int centerY, float radius, leo_Color color)
{
	SDL_Renderer* r = _gfxRenderer();
	if (!r) return;

	_gfxSetColor(r, color);

	// Transform center; scale radius by camera zoom (rotation does not affect radius)
	const leo_Camera2D cam = leo_GetCurrentCamera2D();
	const float z = (cam.zoom <= 0.0f) ? 1.0f : cam.zoom;
	const float rr = radius * z;
	leo_Vector2 sc = leo_GetWorldToScreen2D((leo_Vector2){ (float)centerX, (float)centerY }, cam);

	const int cx = (int)(sc.x + 0.5f);
	const int cy = (int)(sc.y + 0.5f);
	int rpx = (int)(rr + 0.5f);

	// Midpoint circle algorithm (outline)
	int x = rpx, y = 0, err = 1 - rpx;
	while (x >= y)
	{
		SDL_RenderPoint(r, cx + x, cy + y);
		SDL_RenderPoint(r, cx + y, cy + x);
		SDL_RenderPoint(r, cx - y, cy + x);
		SDL_RenderPoint(r, cx - x, cy + y);
		SDL_RenderPoint(r, cx - x, cy - y);
		SDL_RenderPoint(r, cx - y, cy - x);
		SDL_RenderPoint(r, cx + y, cy - x);
		SDL_RenderPoint(r, cx + x, cy - y);
		if (err < 0)
		{
			y += 1;
			err += 2 * y + 1;
		}
		else
		{
			x -= 1;
			err += 2 * (y - x) + 1;
			y += 1;
		}
	}
}

void leo_DrawRectangle(int posX, int posY, int width, int height, leo_Color color)
{
	SDL_Renderer* r = _gfxRenderer();
	if (!r) return;
	_gfxSetColor(r, color);

	// Transform the 4 corners
	const leo_Camera2D cam = leo_GetCurrentCamera2D();
	leo_Vector2 p0 = leo_GetWorldToScreen2D((leo_Vector2){ (float)posX, (float)posY }, cam);
	leo_Vector2 p1 = leo_GetWorldToScreen2D((leo_Vector2){ (float)(posX + width), (float)posY }, cam);
	leo_Vector2 p2 = leo_GetWorldToScreen2D((leo_Vector2){ (float)(posX + width), (float)(posY + height) }, cam);
	leo_Vector2 p3 = leo_GetWorldToScreen2D((leo_Vector2){ (float)posX, (float)(posY + height) }, cam);

	// Fast path: if edges are axis-aligned in screen space, fill an SDL_FRect.
	const float eps = 0.0001f;
	const bool horiz0 = SDL_fabsf(p0.y - p1.y) < eps;
	const bool horiz2 = SDL_fabsf(p2.y - p3.y) < eps;
	const bool vert0 = SDL_fabsf(p0.x - p3.x) < eps;
	const bool vert1 = SDL_fabsf(p1.x - p2.x) < eps;
	if (horiz0 && horiz2 && vert0 && vert1)
	{
		const int minx = (int)(SDL_min(SDL_min(p0.x, p1.x), SDL_min(p2.x, p3.x)) + 0.5f);
		const int maxx = (int)(SDL_max(SDL_max(p0.x, p1.x), SDL_max(p2.x, p3.x)) + 0.5f);
		const int miny = (int)(SDL_min(SDL_min(p0.y, p1.y), SDL_min(p2.y, p3.y)) + 0.5f);
		const int maxy = (int)(SDL_max(SDL_max(p0.y, p1.y), SDL_max(p2.y, p3.y)) + 0.5f);
		SDL_FRect fr = { (float)minx, (float)miny, (float)(maxx - minx), (float)(maxy - miny) };
		SDL_RenderFillRect(r, &fr);
		return;
	}

	// Fallback: draw outline as a 4-segment polygon
	SDL_RenderLine(r, (int)(p0.x + 0.5f), (int)(p0.y + 0.5f), (int)(p1.x + 0.5f), (int)(p1.y + 0.5f));
	SDL_RenderLine(r, (int)(p1.x + 0.5f), (int)(p1.y + 0.5f), (int)(p2.x + 0.5f), (int)(p2.y + 0.5f));
	SDL_RenderLine(r, (int)(p2.x + 0.5f), (int)(p2.y + 0.5f), (int)(p3.x + 0.5f), (int)(p3.y + 0.5f));
	SDL_RenderLine(r, (int)(p3.x + 0.5f), (int)(p3.y + 0.5f), (int)(p0.x + 0.5f), (int)(p0.y + 0.5f));
}
