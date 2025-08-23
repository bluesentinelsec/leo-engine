// =============================================
// leo/graphics.c
// =============================================

#include "leo/graphics.h"
#include "leo/engine.h"

#include <SDL3/SDL.h>
#include <math.h>

// -------------------------------
// Small internal helpers
// -------------------------------

static inline SDL_Renderer* _gfxRenderer(void)
{
	return (SDL_Renderer*)leo_GetRenderer();
}

static inline void _gfxSetColor(SDL_Renderer* r, leo_Color c)
{
	SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

static inline int _roundi(float v)
{
	return (int)SDL_floorf(v + 0.5f);
}

static inline SDL_FPoint _toScreenF(float x, float y, const leo_Camera2D* cam)
{
	const leo_Vector2 s = leo_GetWorldToScreen2D((leo_Vector2){ x, y }, *cam);
	return (SDL_FPoint){ s.x, s.y };
}

// Helper: filled quad via SDL_RenderGeometry (two triangles)
static inline void _render_filled_quad(SDL_Renderer* r,
	SDL_FPoint p0, SDL_FPoint p1,
	SDL_FPoint p2, SDL_FPoint p3,
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

	const float s = 1.0f / 255.0f;
	const SDL_FColor fc = (SDL_FColor){ color.r * s, color.g * s, color.b * s, color.a * s };
	v[0].color = v[1].color = v[2].color = v[3].color = fc;

	for (int i = 0; i < 4; ++i)
	{
		v[i].tex_coord.x = 0.0f;
		v[i].tex_coord.y = 0.0f;
	}

	static const int idx[6] = { 0, 1, 2, 0, 2, 3 };
	SDL_RenderGeometry(r, NULL, v, 4, idx, 6);
}

static inline int _is_axis_aligned(SDL_FPoint p0, SDL_FPoint p1, SDL_FPoint p2, SDL_FPoint p3)
{
	const float eps = 0.0001f;
	const int horiz0 = SDL_fabsf(p0.y - p1.y) < eps;
	const int horiz2 = SDL_fabsf(p2.y - p3.y) < eps;
	const int vert0 = SDL_fabsf(p0.x - p3.x) < eps;
	const int vert1 = SDL_fabsf(p1.x - p2.x) < eps;
	return (horiz0 && horiz2 && vert0 && vert1);
}

// -------------------------------
// Public API (unchanged signatures)
// -------------------------------

void leo_DrawPixel(int posX, int posY, leo_Color color)
{
	SDL_Renderer* r = _gfxRenderer();
	if (!r) return;

	_gfxSetColor(r, color);

	const leo_Camera2D cam = leo_GetCurrentCamera2D();
	const SDL_FPoint s = _toScreenF((float)posX, (float)posY, &cam);
	SDL_RenderPoint(r, _roundi(s.x), _roundi(s.y));
}

void leo_DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, leo_Color color)
{
	SDL_Renderer* r = _gfxRenderer();
	if (!r) return;

	_gfxSetColor(r, color);

	const leo_Camera2D cam = leo_GetCurrentCamera2D();
	const SDL_FPoint a = _toScreenF((float)startPosX, (float)startPosY, &cam);
	const SDL_FPoint b = _toScreenF((float)endPosX, (float)endPosY, &cam);

	SDL_RenderLine(r, _roundi(a.x), _roundi(a.y), _roundi(b.x), _roundi(b.y));
}

void leo_DrawCircle(int centerX, int centerY, float radius, leo_Color color)
{
	if (radius <= 0.0f) return;

	SDL_Renderer* r = _gfxRenderer();
	if (!r) return;

	_gfxSetColor(r, color);

	// Transform center; scale radius by camera zoom (rotation does not affect radius)
	const leo_Camera2D cam = leo_GetCurrentCamera2D();
	const float z = (cam.zoom <= 0.0f) ? 1.0f : cam.zoom;
	const float rr = radius * z;

	const SDL_FPoint sc = _toScreenF((float)centerX, (float)centerY, &cam);
	const int cx = _roundi(sc.x);
	const int cy = _roundi(sc.y);
	int rpx = _roundi(rr);

	// Guard huge radii to avoid very long loops
	if (rpx > 1 << 15) rpx = 1 << 15;

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

	const leo_Camera2D cam = leo_GetCurrentCamera2D();

	// Transform the 4 corners (world -> screen)
	const SDL_FPoint p0 = _toScreenF((float)posX, (float)posY, &cam);
	const SDL_FPoint p1 = _toScreenF((float)(posX + width), (float)posY, &cam);
	const SDL_FPoint p2 = _toScreenF((float)(posX + width), (float)(posY + height), &cam);
	const SDL_FPoint p3 = _toScreenF((float)posX, (float)(posY + height), &cam);

	if (_is_axis_aligned(p0, p1, p2, p3))
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

	// General case: draw a filled rotated quad (uses per-vertex color)
	_render_filled_quad(r, p0, p1, p2, p3, color);
}
