#include "leo/graphics.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{

constexpr float kEpsilon = 1.0e-5f;

struct ScopedRenderState
{
    SDL_Renderer *renderer;
    SDL_Color prev_color;
    SDL_BlendMode prev_blend;

    ScopedRenderState(SDL_Renderer *renderer_ref, leo::Graphics::Color color)
        : renderer(renderer_ref), prev_color({0, 0, 0, 0}), prev_blend(SDL_BLENDMODE_NONE)
    {
        SDL_GetRenderDrawColor(renderer, &prev_color.r, &prev_color.g, &prev_color.b, &prev_color.a);
        SDL_GetRenderDrawBlendMode(renderer, &prev_blend);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    }

    ~ScopedRenderState()
    {
        SDL_SetRenderDrawBlendMode(renderer, prev_blend);
        SDL_SetRenderDrawColor(renderer, prev_color.r, prev_color.g, prev_color.b, prev_color.a);
    }
};

float Cross(const SDL_FPoint &a, const SDL_FPoint &b, const SDL_FPoint &c)
{
    float abx = b.x - a.x;
    float aby = b.y - a.y;
    float acx = c.x - a.x;
    float acy = c.y - a.y;
    return abx * acy - aby * acx;
}

float PolygonArea(const std::vector<SDL_FPoint> &points)
{
    float area = 0.0f;
    size_t count = points.size();
    for (size_t i = 0; i < count; ++i)
    {
        const SDL_FPoint &a = points[i];
        const SDL_FPoint &b = points[(i + 1) % count];
        area += a.x * b.y - b.x * a.y;
    }
    return area * 0.5f;
}

bool NearlyEqual(float a, float b)
{
    return std::fabs(a - b) <= kEpsilon;
}

bool PointEquals(const SDL_FPoint &a, const SDL_FPoint &b)
{
    return NearlyEqual(a.x, b.x) && NearlyEqual(a.y, b.y);
}

void RemoveDuplicatePoints(std::vector<SDL_FPoint> *points)
{
    if (!points || points->size() < 2)
    {
        return;
    }

    std::vector<SDL_FPoint> filtered;
    filtered.reserve(points->size());
    for (const SDL_FPoint &p : *points)
    {
        if (filtered.empty() || !PointEquals(filtered.back(), p))
        {
            filtered.push_back(p);
        }
    }

    if (filtered.size() > 1 && PointEquals(filtered.front(), filtered.back()))
    {
        filtered.pop_back();
    }

    *points = std::move(filtered);
}

void RemoveCollinearPoints(std::vector<SDL_FPoint> *points)
{
    if (!points || points->size() < 3)
    {
        return;
    }

    bool removed = true;
    while (removed && points->size() > 3)
    {
        removed = false;
        size_t count = points->size();
        for (size_t i = 0; i < count; ++i)
        {
            const SDL_FPoint &prev = (*points)[(i + count - 1) % count];
            const SDL_FPoint &curr = (*points)[i];
            const SDL_FPoint &next = (*points)[(i + 1) % count];
            if (std::fabs(Cross(prev, curr, next)) <= kEpsilon)
            {
                points->erase(points->begin() + static_cast<long>(i));
                removed = true;
                break;
            }
        }
    }
}

bool PointInTriangle(const SDL_FPoint &p, const SDL_FPoint &a, const SDL_FPoint &b, const SDL_FPoint &c)
{
    float c1 = Cross(a, b, p);
    float c2 = Cross(b, c, p);
    float c3 = Cross(c, a, p);

    bool has_neg = (c1 < -kEpsilon) || (c2 < -kEpsilon) || (c3 < -kEpsilon);
    bool has_pos = (c1 > kEpsilon) || (c2 > kEpsilon) || (c3 > kEpsilon);
    return !(has_neg && has_pos);
}

bool TriangulatePolygon(std::vector<SDL_FPoint> points, std::vector<SDL_FPoint> *out_triangles)
{
    if (!out_triangles)
    {
        return false;
    }

    out_triangles->clear();
    RemoveDuplicatePoints(&points);
    RemoveCollinearPoints(&points);

    if (points.size() < 3)
    {
        return false;
    }

    if (PolygonArea(points) < 0.0f)
    {
        std::reverse(points.begin(), points.end());
    }

    std::vector<int> indices(points.size());
    for (size_t i = 0; i < points.size(); ++i)
    {
        indices[i] = static_cast<int>(i);
    }

    size_t guard = 0;
    while (indices.size() > 3 && guard < points.size() * points.size())
    {
        bool ear_found = false;
        size_t count = indices.size();
        for (size_t i = 0; i < count; ++i)
        {
            int prev_index = indices[(i + count - 1) % count];
            int curr_index = indices[i];
            int next_index = indices[(i + 1) % count];

            const SDL_FPoint &a = points[prev_index];
            const SDL_FPoint &b = points[curr_index];
            const SDL_FPoint &c = points[next_index];

            if (Cross(a, b, c) <= kEpsilon)
            {
                continue;
            }

            bool contains_point = false;
            for (size_t j = 0; j < count; ++j)
            {
                int test_index = indices[j];
                if (test_index == prev_index || test_index == curr_index || test_index == next_index)
                {
                    continue;
                }
                if (PointInTriangle(points[test_index], a, b, c))
                {
                    contains_point = true;
                    break;
                }
            }

            if (contains_point)
            {
                continue;
            }

            out_triangles->push_back(a);
            out_triangles->push_back(b);
            out_triangles->push_back(c);
            indices.erase(indices.begin() + static_cast<long>(i));
            ear_found = true;
            break;
        }

        if (!ear_found)
        {
            return false;
        }
        ++guard;
    }

    if (indices.size() == 3)
    {
        out_triangles->push_back(points[indices[0]]);
        out_triangles->push_back(points[indices[1]]);
        out_triangles->push_back(points[indices[2]]);
    }

    return out_triangles->size() >= 3;
}

SDL_FColor ToFColor(leo::Graphics::Color color)
{
    constexpr float kInv255 = 1.0f / 255.0f;
    SDL_FColor result;
    result.r = static_cast<float>(color.r) * kInv255;
    result.g = static_cast<float>(color.g) * kInv255;
    result.b = static_cast<float>(color.b) * kInv255;
    result.a = static_cast<float>(color.a) * kInv255;
    return result;
}

void RenderTriangleGeometry(SDL_Renderer *renderer, const SDL_FPoint &a, const SDL_FPoint &b, const SDL_FPoint &c,
                            leo::Graphics::Color color)
{
    SDL_Vertex verts[3];
    verts[0].position = a;
    verts[0].color = ToFColor(color);
    verts[0].tex_coord = {0.0f, 0.0f};
    verts[1].position = b;
    verts[1].color = ToFColor(color);
    verts[1].tex_coord = {0.0f, 0.0f};
    verts[2].position = c;
    verts[2].color = ToFColor(color);
    verts[2].tex_coord = {0.0f, 0.0f};

    int indices[3] = {0, 1, 2};
    SDL_RenderGeometry(renderer, nullptr, verts, 3, indices, 3);
}

void DrawHorizontal(SDL_Renderer *renderer, float x1, float x2, float y)
{
    SDL_RenderLine(renderer, x1, y, x2, y);
}

void DrawCircleOutlinePoints(SDL_Renderer *renderer, float cx, float cy, int x, int y)
{
    SDL_RenderPoint(renderer, cx + x, cy + y);
    SDL_RenderPoint(renderer, cx - x, cy + y);
    SDL_RenderPoint(renderer, cx + x, cy - y);
    SDL_RenderPoint(renderer, cx - x, cy - y);
    SDL_RenderPoint(renderer, cx + y, cy + x);
    SDL_RenderPoint(renderer, cx - y, cy + x);
    SDL_RenderPoint(renderer, cx + y, cy - x);
    SDL_RenderPoint(renderer, cx - y, cy - x);
}

enum class Quadrant
{
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
};

void DrawQuarterOutlinePoints(SDL_Renderer *renderer, float cx, float cy, int x, int y, Quadrant quadrant)
{
    switch (quadrant)
    {
    case Quadrant::TopLeft:
        SDL_RenderPoint(renderer, cx - x, cy - y);
        SDL_RenderPoint(renderer, cx - y, cy - x);
        break;
    case Quadrant::TopRight:
        SDL_RenderPoint(renderer, cx + x, cy - y);
        SDL_RenderPoint(renderer, cx + y, cy - x);
        break;
    case Quadrant::BottomLeft:
        SDL_RenderPoint(renderer, cx - x, cy + y);
        SDL_RenderPoint(renderer, cx - y, cy + x);
        break;
    case Quadrant::BottomRight:
        SDL_RenderPoint(renderer, cx + x, cy + y);
        SDL_RenderPoint(renderer, cx + y, cy + x);
        break;
    }
}

void DrawQuarterCircleOutline(SDL_Renderer *renderer, float cx, float cy, float radius, Quadrant quadrant)
{
    int r = static_cast<int>(radius + 0.5f);
    int x = 0;
    int y = r;
    int d = 1 - r;

    while (y >= x)
    {
        DrawQuarterOutlinePoints(renderer, cx, cy, x, y, quadrant);
        if (d < 0)
        {
            d += 2 * x + 3;
        }
        else
        {
            d += 2 * (x - y) + 5;
            --y;
        }
        ++x;
    }
}

void DrawQuarterCircleFilled(SDL_Renderer *renderer, float cx, float cy, float radius, Quadrant quadrant)
{
    if (radius <= 0.0f)
    {
        return;
    }

    float r2 = radius * radius;
    int r = static_cast<int>(radius + 0.5f);
    for (int y = 0; y <= r; ++y)
    {
        float x = std::sqrt(std::max(0.0f, r2 - static_cast<float>(y * y)));
        switch (quadrant)
        {
        case Quadrant::TopLeft:
            DrawHorizontal(renderer, cx - x, cx, cy - static_cast<float>(y));
            break;
        case Quadrant::TopRight:
            DrawHorizontal(renderer, cx, cx + x, cy - static_cast<float>(y));
            break;
        case Quadrant::BottomLeft:
            DrawHorizontal(renderer, cx - x, cx, cy + static_cast<float>(y));
            break;
        case Quadrant::BottomRight:
            DrawHorizontal(renderer, cx, cx + x, cy + static_cast<float>(y));
            break;
        }
    }
}

} // namespace

namespace leo
{
namespace Graphics
{

void DrawPixel(SDL_Renderer *renderer, float x, float y, Color color)
{
    if (!renderer)
    {
        return;
    }
    ScopedRenderState state(renderer, color);
    SDL_RenderPoint(renderer, x, y);
}

void DrawLine(SDL_Renderer *renderer, float x1, float y1, float x2, float y2, Color color)
{
    if (!renderer)
    {
        return;
    }
    ScopedRenderState state(renderer, color);
    SDL_RenderLine(renderer, x1, y1, x2, y2);
}

void DrawCircleFilled(SDL_Renderer *renderer, float cx, float cy, float radius, Color color)
{
    if (!renderer || radius <= 0.0f)
    {
        return;
    }
    ScopedRenderState state(renderer, color);

    int r = static_cast<int>(radius + 0.5f);
    int x = 0;
    int y = r;
    int d = 1 - r;

    while (y >= x)
    {
        DrawHorizontal(renderer, cx - x, cx + x, cy + y);
        DrawHorizontal(renderer, cx - x, cx + x, cy - y);
        DrawHorizontal(renderer, cx - y, cx + y, cy + x);
        DrawHorizontal(renderer, cx - y, cx + y, cy - x);
        if (d < 0)
        {
            d += 2 * x + 3;
        }
        else
        {
            d += 2 * (x - y) + 5;
            --y;
        }
        ++x;
    }
}

void DrawCircleOutline(SDL_Renderer *renderer, float cx, float cy, float radius, Color color)
{
    if (!renderer || radius <= 0.0f)
    {
        return;
    }
    ScopedRenderState state(renderer, color);

    int r = static_cast<int>(radius + 0.5f);
    int x = 0;
    int y = r;
    int d = 1 - r;

    while (y >= x)
    {
        DrawCircleOutlinePoints(renderer, cx, cy, x, y);
        if (d < 0)
        {
            d += 2 * x + 3;
        }
        else
        {
            d += 2 * (x - y) + 5;
            --y;
        }
        ++x;
    }
}

void DrawRectangleFilled(SDL_Renderer *renderer, float x, float y, float w, float h, Color color)
{
    if (!renderer || w <= 0.0f || h <= 0.0f)
    {
        return;
    }
    ScopedRenderState state(renderer, color);
    SDL_FRect rect = {x, y, w, h};
    SDL_RenderFillRect(renderer, &rect);
}

void DrawRectangleOutline(SDL_Renderer *renderer, float x, float y, float w, float h, Color color)
{
    if (!renderer || w <= 0.0f || h <= 0.0f)
    {
        return;
    }
    ScopedRenderState state(renderer, color);
    SDL_FRect rect = {x, y, w, h};
    SDL_RenderRect(renderer, &rect);
}

void DrawRectangleRoundedFilled(SDL_Renderer *renderer, float x, float y, float w, float h, float radius,
                                Color color)
{
    if (!renderer || w <= 0.0f || h <= 0.0f)
    {
        return;
    }

    float r = std::min(radius, 0.5f * std::min(w, h));
    if (r <= 0.0f)
    {
        DrawRectangleFilled(renderer, x, y, w, h, color);
        return;
    }

    ScopedRenderState state(renderer, color);

    float right = x + w;
    float bottom = y + h;

    SDL_FRect center = {x + r, y, w - 2.0f * r, h};
    SDL_RenderFillRect(renderer, &center);

    SDL_FRect left = {x, y + r, r, h - 2.0f * r};
    SDL_RenderFillRect(renderer, &left);

    SDL_FRect right_rect = {right - r, y + r, r, h - 2.0f * r};
    SDL_RenderFillRect(renderer, &right_rect);

    DrawQuarterCircleFilled(renderer, x + r, y + r, r, Quadrant::TopLeft);
    DrawQuarterCircleFilled(renderer, right - r, y + r, r, Quadrant::TopRight);
    DrawQuarterCircleFilled(renderer, x + r, bottom - r, r, Quadrant::BottomLeft);
    DrawQuarterCircleFilled(renderer, right - r, bottom - r, r, Quadrant::BottomRight);
}

void DrawRectangleRoundedOutline(SDL_Renderer *renderer, float x, float y, float w, float h, float radius,
                                 Color color)
{
    if (!renderer || w <= 0.0f || h <= 0.0f)
    {
        return;
    }

    float r = std::min(radius, 0.5f * std::min(w, h));
    if (r <= 0.0f)
    {
        DrawRectangleOutline(renderer, x, y, w, h, color);
        return;
    }

    ScopedRenderState state(renderer, color);

    float right = x + w;
    float bottom = y + h;

    SDL_RenderLine(renderer, x + r, y, right - r, y);
    SDL_RenderLine(renderer, x + r, bottom, right - r, bottom);
    SDL_RenderLine(renderer, x, y + r, x, bottom - r);
    SDL_RenderLine(renderer, right, y + r, right, bottom - r);

    DrawQuarterCircleOutline(renderer, x + r, y + r, r, Quadrant::TopLeft);
    DrawQuarterCircleOutline(renderer, right - r, y + r, r, Quadrant::TopRight);
    DrawQuarterCircleOutline(renderer, x + r, bottom - r, r, Quadrant::BottomLeft);
    DrawQuarterCircleOutline(renderer, right - r, bottom - r, r, Quadrant::BottomRight);
}

void DrawTriangleFilled(SDL_Renderer *renderer, SDL_FPoint a, SDL_FPoint b, SDL_FPoint c, Color color)
{
    if (!renderer)
    {
        return;
    }

    ScopedRenderState state(renderer, color);
    RenderTriangleGeometry(renderer, a, b, c, color);
}

void DrawTriangleOutline(SDL_Renderer *renderer, SDL_FPoint a, SDL_FPoint b, SDL_FPoint c, Color color)
{
    if (!renderer)
    {
        return;
    }

    ScopedRenderState state(renderer, color);
    SDL_RenderLine(renderer, a.x, a.y, b.x, b.y);
    SDL_RenderLine(renderer, b.x, b.y, c.x, c.y);
    SDL_RenderLine(renderer, c.x, c.y, a.x, a.y);
}

void DrawPolyFilled(SDL_Renderer *renderer, const SDL_FPoint *points, int count, Color color)
{
    if (!renderer || !points || count < 3)
    {
        return;
    }

    std::vector<SDL_FPoint> point_list(points, points + count);
    std::vector<SDL_FPoint> triangles;
    if (!TriangulatePolygon(point_list, &triangles))
    {
        return;
    }

    ScopedRenderState state(renderer, color);
    for (size_t i = 0; i + 2 < triangles.size(); i += 3)
    {
        RenderTriangleGeometry(renderer, triangles[i], triangles[i + 1], triangles[i + 2], color);
    }
}

void DrawPolyOutline(SDL_Renderer *renderer, const SDL_FPoint *points, int count, Color color)
{
    if (!renderer || !points || count < 2)
    {
        return;
    }

    std::vector<SDL_FPoint> line_points(points, points + count);
    if (count >= 2)
    {
        line_points.push_back(points[0]);
    }

    ScopedRenderState state(renderer, color);
    SDL_RenderLines(renderer, line_points.data(), static_cast<int>(line_points.size()));
}

} // namespace Graphics
} // namespace leo
