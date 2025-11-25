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

static inline SDL_Renderer *_gfxRenderer(void)
{
    return (SDL_Renderer *)leo_GetRenderer();
}

static inline void _gfxSetColor(SDL_Renderer *r, leo_Color c)
{
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

static inline void _gfxEnableBlending(SDL_Renderer *r)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
}

static inline int _roundi(float v)
{
    return (int)SDL_floorf(v + 0.5f);
}

static inline SDL_FPoint _toScreenF(float x, float y, const leo_Camera2D *cam)
{
    const leo_Vector2 s = leo_GetWorldToScreen2D((leo_Vector2){x, y}, *cam);
    return (SDL_FPoint){s.x, s.y};
}

// Helper: filled quad via SDL_RenderGeometry (two triangles)
static inline void _render_filled_quad(SDL_Renderer *r, SDL_FPoint p0, SDL_FPoint p1, SDL_FPoint p2, SDL_FPoint p3,
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
    const SDL_FColor fc = (SDL_FColor){color.r * s, color.g * s, color.b * s, color.a * s};
    v[0].color = v[1].color = v[2].color = v[3].color = fc;

    for (int i = 0; i < 4; ++i)
    {
        v[i].tex_coord.x = 0.0f;
        v[i].tex_coord.y = 0.0f;
    }

    static const int idx[6] = {0, 1, 2, 0, 2, 3};
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
    SDL_Renderer *r = _gfxRenderer();
    if (!r)
        return;

    _gfxSetColor(r, color);

    const leo_Camera2D cam = leo_GetCurrentCamera2D();
    const SDL_FPoint s = _toScreenF((float)posX, (float)posY, &cam);
    SDL_RenderPoint(r, _roundi(s.x), _roundi(s.y));
}

void leo_DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, leo_Color color)
{
    SDL_Renderer *r = _gfxRenderer();
    if (!r)
        return;

    _gfxSetColor(r, color);

    const leo_Camera2D cam = leo_GetCurrentCamera2D();
    const SDL_FPoint a = _toScreenF((float)startPosX, (float)startPosY, &cam);
    const SDL_FPoint b = _toScreenF((float)endPosX, (float)endPosY, &cam);

    SDL_RenderLine(r, _roundi(a.x), _roundi(a.y), _roundi(b.x), _roundi(b.y));
}

void leo_DrawCircle(int centerX, int centerY, float radius, leo_Color color)
{
    if (radius <= 0.0f)
        return;

    SDL_Renderer *r = _gfxRenderer();
    if (!r)
        return;

    _gfxEnableBlending(r);
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
    if (rpx > 1 << 15)
        rpx = 1 << 15;

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
    SDL_Renderer *r = _gfxRenderer();
    if (!r)
        return;

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

        SDL_FRect fr = {minx, miny, maxx - minx, maxy - miny};
        _gfxEnableBlending(r); // Enable alpha blending
        _gfxSetColor(r, color);
        SDL_RenderFillRect(r, &fr);
        return;
    }

    // General case: draw a filled rotated quad (uses per-vertex color)
    _render_filled_quad(r, p0, p1, p2, p3, color);
}
// Helper: validate and transform circle parameters
static inline int _prepare_circle(int centerX, int centerY, float radius, SDL_Renderer **r, int *cx, int *cy, int *rpx)
{
    if (radius <= 0.0f)
        return 0;

    *r = _gfxRenderer();
    if (!*r)
        return 0;

    const leo_Camera2D cam = leo_GetCurrentCamera2D();
    const float z = (cam.zoom <= 0.0f) ? 1.0f : cam.zoom;
    const float rr = radius * z;

    const SDL_FPoint sc = _toScreenF((float)centerX, (float)centerY, &cam);
    *cx = _roundi(sc.x);
    *cy = _roundi(sc.y);
    *rpx = _roundi(rr);

    if (*rpx > 1 << 15)
        *rpx = 1 << 15;

    return 1;
}

void leo_DrawCircleFilled(int centerX, int centerY, float radius, leo_Color color)
{
    SDL_Renderer *r;
    int cx, cy, rpx;
    if (!_prepare_circle(centerX, centerY, radius, &r, &cx, &cy, &rpx))
        return;

    _gfxEnableBlending(r);
    _gfxSetColor(r, color);

    // Optimized scan line fill
    for (int y = -rpx; y <= rpx; y++)
    {
        int x = (int)SDL_sqrtf((float)(rpx * rpx - y * y));
        SDL_RenderLine(r, cx - x, cy + y, cx + x, cy + y);
    }
}

void leo_DrawRectangleLines(int posX, int posY, int width, int height, leo_Color color)
{
    if (width <= 0 || height <= 0)
        return;

    // Optimized: draw 4 lines for rectangle outline
    leo_DrawLine(posX, posY, posX + width - 1, posY, color);                           // top
    leo_DrawLine(posX + width - 1, posY, posX + width - 1, posY + height - 1, color);  // right
    leo_DrawLine(posX + width - 1, posY + height - 1, posX, posY + height - 1, color); // bottom
    leo_DrawLine(posX, posY + height - 1, posX, posY, color);                          // left
}

// Helper: render filled triangle using SDL geometry
static inline void _render_triangle_filled(int x1, int y1, int x2, int y2, int x3, int y3, leo_Color color)
{
    SDL_Renderer *r = _gfxRenderer();
    if (!r)
        return;

    const leo_Camera2D cam = leo_GetCurrentCamera2D();
    const SDL_FPoint p1 = _toScreenF((float)x1, (float)y1, &cam);
    const SDL_FPoint p2 = _toScreenF((float)x2, (float)y2, &cam);
    const SDL_FPoint p3 = _toScreenF((float)x3, (float)y3, &cam);

    SDL_Vertex v[3] = {{.position = p1, .tex_coord = {0.0f, 0.0f}},
                       {.position = p2, .tex_coord = {0.0f, 0.0f}},
                       {.position = p3, .tex_coord = {0.0f, 0.0f}}};

    const float s = 1.0f / 255.0f;
    const SDL_FColor fc = {color.r * s, color.g * s, color.b * s, color.a * s};
    v[0].color = v[1].color = v[2].color = fc;

    SDL_RenderGeometry(r, NULL, v, 3, NULL, 0);
}

void leo_DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, leo_Color color)
{
    leo_DrawLine(x1, y1, x2, y2, color);
    leo_DrawLine(x2, y2, x3, y3, color);
    leo_DrawLine(x3, y3, x1, y1, color);
}

void leo_DrawTriangleFilled(int x1, int y1, int x2, int y2, int x3, int y3, leo_Color color)
{
    _render_triangle_filled(x1, y1, x2, y2, x3, y3, color);
}

void leo_DrawPoly(int *points, int numPoints, leo_Color color)
{
    if (!points || numPoints < 3)
        return;

    for (int i = 0; i < numPoints; i++)
    {
        int next = (i + 1) % numPoints;
        leo_DrawLine(points[i * 2], points[i * 2 + 1], points[next * 2], points[next * 2 + 1], color);
    }
}

void leo_DrawPolyFilled(int *points, int numPoints, leo_Color color)
{
    if (!points || numPoints < 3)
        return;

    SDL_Renderer *r = _gfxRenderer();
    if (!r)
        return;

    const leo_Camera2D cam = leo_GetCurrentCamera2D();
    const float s = 1.0f / 255.0f;
    const SDL_FColor fc = {color.r * s, color.g * s, color.b * s, color.a * s};

    // Stack allocation for small polygons, heap for large ones
    SDL_Vertex stack_vertices[16];
    int stack_indices[42]; // (16-2)*3 = 42

    SDL_Vertex *vertices = (numPoints <= 16) ? stack_vertices : SDL_malloc(numPoints * sizeof(SDL_Vertex));
    int *indices = (numPoints <= 16) ? stack_indices : SDL_malloc((numPoints - 2) * 3 * sizeof(int));

    if (!vertices || !indices)
    {
        if (numPoints > 16)
        {
            SDL_free(vertices);
            SDL_free(indices);
        }
        return;
    }

    // Transform points to screen space
    for (int i = 0; i < numPoints; i++)
    {
        vertices[i].position = _toScreenF((float)points[i * 2], (float)points[i * 2 + 1], &cam);
        vertices[i].color = fc;
        vertices[i].tex_coord.x = vertices[i].tex_coord.y = 0.0f;
    }

    // Triangle fan triangulation
    for (int i = 0; i < numPoints - 2; i++)
    {
        indices[i * 3] = 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = i + 2;
    }

    SDL_RenderGeometry(r, NULL, vertices, numPoints, indices, (numPoints - 2) * 3);

    if (numPoints > 16)
    {
        SDL_free(vertices);
        SDL_free(indices);
    }
}
