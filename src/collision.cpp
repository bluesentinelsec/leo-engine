#include "leo/collision.h"

#include <algorithm>
#include <cmath>

namespace
{

constexpr float kEpsilon = 1.0e-4f;

struct RectBounds
{
    float left;
    float top;
    float right;
    float bottom;
};

RectBounds NormalizeRect(const SDL_FRect &rect)
{
    float x1 = rect.x;
    float x2 = rect.x + rect.w;
    float y1 = rect.y;
    float y2 = rect.y + rect.h;
    if (x1 > x2)
    {
        std::swap(x1, x2);
    }
    if (y1 > y2)
    {
        std::swap(y1, y2);
    }
    return {x1, y1, x2, y2};
}

float DistanceSquared(float x1, float y1, float x2, float y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    return dx * dx + dy * dy;
}

float Cross(const SDL_FPoint &a, const SDL_FPoint &b, const SDL_FPoint &c)
{
    float abx = b.x - a.x;
    float aby = b.y - a.y;
    float acx = c.x - a.x;
    float acy = c.y - a.y;
    return abx * acy - aby * acx;
}

bool NearlyZero(float value)
{
    return std::fabs(value) <= kEpsilon;
}

float Clamp(float v, float min_v, float max_v)
{
    return std::max(min_v, std::min(max_v, v));
}

bool OnSegment(const SDL_FPoint &p, const SDL_FPoint &a, const SDL_FPoint &b)
{
    return p.x >= std::min(a.x, b.x) - kEpsilon && p.x <= std::max(a.x, b.x) + kEpsilon &&
           p.y >= std::min(a.y, b.y) - kEpsilon && p.y <= std::max(a.y, b.y) + kEpsilon;
}

int Orientation(const SDL_FPoint &a, const SDL_FPoint &b, const SDL_FPoint &c)
{
    float val = Cross(a, b, c);
    if (NearlyZero(val))
    {
        return 0;
    }
    return (val > 0.0f) ? 1 : 2;
}

} // namespace

namespace leo
{
namespace Collision
{

bool CheckCollisionRecs(const SDL_FRect &a, const SDL_FRect &b)
{
    RectBounds ra = NormalizeRect(a);
    RectBounds rb = NormalizeRect(b);
    return (ra.left <= rb.right && ra.right >= rb.left && ra.top <= rb.bottom && ra.bottom >= rb.top);
}

bool CheckCollisionCircles(SDL_FPoint center1, float radius1, SDL_FPoint center2, float radius2)
{
    if (radius1 <= 0.0f || radius2 <= 0.0f)
    {
        return false;
    }
    float r = radius1 + radius2;
    return DistanceSquared(center1.x, center1.y, center2.x, center2.y) <= r * r;
}

bool CheckCollisionCircleRec(SDL_FPoint center, float radius, const SDL_FRect &rec)
{
    if (radius <= 0.0f)
    {
        return false;
    }
    RectBounds r = NormalizeRect(rec);
    float closest_x = Clamp(center.x, r.left, r.right);
    float closest_y = Clamp(center.y, r.top, r.bottom);
    return DistanceSquared(center.x, center.y, closest_x, closest_y) <= radius * radius;
}

bool CheckCollisionCircleLine(SDL_FPoint center, float radius, SDL_FPoint p1, SDL_FPoint p2)
{
    if (radius <= 0.0f)
    {
        return false;
    }

    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float length_sq = dx * dx + dy * dy;
    if (NearlyZero(length_sq))
    {
        return DistanceSquared(center.x, center.y, p1.x, p1.y) <= radius * radius;
    }

    float t = ((center.x - p1.x) * dx + (center.y - p1.y) * dy) / length_sq;
    t = Clamp(t, 0.0f, 1.0f);
    float closest_x = p1.x + t * dx;
    float closest_y = p1.y + t * dy;
    return DistanceSquared(center.x, center.y, closest_x, closest_y) <= radius * radius;
}

bool CheckCollisionPointRec(SDL_FPoint point, const SDL_FRect &rec)
{
    RectBounds r = NormalizeRect(rec);
    return point.x >= r.left && point.x <= r.right && point.y >= r.top && point.y <= r.bottom;
}

bool CheckCollisionPointCircle(SDL_FPoint point, SDL_FPoint center, float radius)
{
    if (radius <= 0.0f)
    {
        return false;
    }
    return DistanceSquared(point.x, point.y, center.x, center.y) <= radius * radius;
}

bool CheckCollisionPointTriangle(SDL_FPoint point, SDL_FPoint p1, SDL_FPoint p2, SDL_FPoint p3)
{
    float d1 = Cross(point, p1, p2);
    float d2 = Cross(point, p2, p3);
    float d3 = Cross(point, p3, p1);

    bool has_neg = (d1 < -kEpsilon) || (d2 < -kEpsilon) || (d3 < -kEpsilon);
    bool has_pos = (d1 > kEpsilon) || (d2 > kEpsilon) || (d3 > kEpsilon);
    return !(has_neg && has_pos);
}

bool CheckCollisionPointLine(SDL_FPoint point, SDL_FPoint p1, SDL_FPoint p2)
{
    float cross = Cross(p1, p2, point);
    if (!NearlyZero(cross))
    {
        return false;
    }

    return OnSegment(point, p1, p2);
}

bool CheckCollisionPointPoly(SDL_FPoint point, const SDL_FPoint *points, int count)
{
    if (!points || count < 3)
    {
        return false;
    }

    bool inside = false;
    for (int i = 0, j = count - 1; i < count; j = i++)
    {
        SDL_FPoint a = points[i];
        SDL_FPoint b = points[j];

        if (CheckCollisionPointLine(point, a, b))
        {
            return true;
        }

        bool intersect = ((a.y > point.y) != (b.y > point.y)) &&
                         (point.x < (b.x - a.x) * (point.y - a.y) / (b.y - a.y + kEpsilon) + a.x);
        if (intersect)
        {
            inside = !inside;
        }
    }

    return inside;
}

bool CheckCollisionLines(SDL_FPoint p1, SDL_FPoint p2, SDL_FPoint p3, SDL_FPoint p4)
{
    int o1 = Orientation(p1, p2, p3);
    int o2 = Orientation(p1, p2, p4);
    int o3 = Orientation(p3, p4, p1);
    int o4 = Orientation(p3, p4, p2);

    if (o1 != o2 && o3 != o4)
    {
        return true;
    }

    if (o1 == 0 && OnSegment(p3, p1, p2))
        return true;
    if (o2 == 0 && OnSegment(p4, p1, p2))
        return true;
    if (o3 == 0 && OnSegment(p1, p3, p4))
        return true;
    if (o4 == 0 && OnSegment(p2, p3, p4))
        return true;

    return false;
}

} // namespace Collision
} // namespace leo
