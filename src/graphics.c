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


// Helper: filled quad via SDL_RenderGeometry (two triangles).
static inline void _render_filled_quad(SDL_Renderer* r,
	const leo_Vector2 p0, const leo_Vector2 p1,
	const leo_Vector2 p2, const leo_Vector2 p3,
	leo_Color color)
{
	SDL_Vertex v[4];
	v[0].position.x = p0.x;
	v[0].position.y = p0.y;
	v[1].position.x = p1.x;
	v[1].position.y = p1.y;
	v[2].position.x = p2.x;
	v[2].position.y = p2.y;
	v[3].position.x = p3.x;
	v[3].position.y = p3.y;

	// Convert to floats (0–1)
	const float rf = color.r / 255.0f;
	const float gf = color.g / 255.0f;
	const float bf = color.b / 255.0f;
	const float af = color.a / 255.0f;

	SDL_FColor fc = { rf, gf, bf, af };
	v[0].color = fc;
	v[1].color = fc;
	v[2].color = fc;
	v[3].color = fc;

	// No texture coords needed, but zero them anyway
	for (int i = 0; i < 4; ++i)
	{
		v[i].tex_coord.x = 0.0f;
		v[i].tex_coord.y = 0.0f;
	}

	// Two triangles: (0,1,2) and (0,2,3)
	static const int idx[6] = { 0, 1, 2, 0, 2, 3 };

	SDL_RenderGeometry(r, /*texture*/NULL, v, 4, idx, 6);
}


void leo_DrawRectangle(int posX, int posY, int width, int height, leo_Color color)
{
	SDL_Renderer* r = _gfxRenderer();
	if (!r) return;

	// Transform the 4 corners (world -> screen)
	const leo_Camera2D cam = leo_GetCurrentCamera2D();
	leo_Vector2 p0 = leo_GetWorldToScreen2D((leo_Vector2){ (float)posX, (float)posY }, cam);
	leo_Vector2 p1 = leo_GetWorldToScreen2D((leo_Vector2){ (float)(posX + width), (float)posY }, cam);
	leo_Vector2 p2 = leo_GetWorldToScreen2D((leo_Vector2){ (float)(posX + width), (float)(posY + height) }, cam);
	leo_Vector2 p3 = leo_GetWorldToScreen2D((leo_Vector2){ (float)posX, (float)(posY + height) }, cam);

	// If the screen-space quad is axis-aligned, FillRect is cheaper.
	const float eps = 0.0001f;
	const bool horiz0 = SDL_fabsf(p0.y - p1.y) < eps;
	const bool horiz2 = SDL_fabsf(p2.y - p3.y) < eps;
	const bool vert0 = SDL_fabsf(p0.x - p3.x) < eps;
	const bool vert1 = SDL_fabsf(p1.x - p2.x) < eps;

	if (horiz0 && horiz2 && vert0 && vert1)
	{
		// Axis-aligned fast path
		const float minx = SDL_min(SDL_min(p0.x, p1.x), SDL_min(p2.x, p3.x));
		const float maxx = SDL_max(SDL_max(p0.x, p1.x), SDL_max(p2.x, p3.x));
		const float miny = SDL_min(SDL_min(p0.y, p1.y), SDL_min(p2.y, p3.y));
		const float maxy = SDL_max(SDL_max(p0.y, p1.y), SDL_max(p2.y, p3.y));

		SDL_FRect fr = { minx, miny, maxx - minx, maxy - miny };
		_gfxSetColor(r, color);
		SDL_RenderFillRect(r, &fr);
		return;
	}

	// General case: draw a filled rotated quad
	_render_filled_quad(r, p0, p1, p2, p3, color);
}
