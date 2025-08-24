// =============================================
// src/collisions.c
// =============================================
#include "leo/collisions.h"
#include <math.h>

#ifndef LEO_INLINE
#if defined(_MSC_VER)
#define LEO_INLINE __inline
#else
#define LEO_INLINE inline
#endif
#endif

/* Keep comparisons tolerant but tight */
static const float LEO_EPS = 1e-6f;

static LEO_INLINE float leo__minf(float a, float b)
{
    return (a < b) ? a : b;
}
static LEO_INLINE float leo__maxf(float a, float b)
{
    return (a > b) ? a : b;
}
static LEO_INLINE float leo__clampf(float x, float lo, float hi)
{
    if (x < lo)
        return lo;
    if (x > hi)
        return hi;
    return x;
}
static LEO_INLINE float leo__dot2(leo_Vector2 a, leo_Vector2 b)
{
    return a.x * b.x + a.y * b.y;
}
static LEO_INLINE float leo__len2_sq(leo_Vector2 a)
{
    return a.x * a.x + a.y * a.y;
}

static LEO_INLINE leo_Vector2 leo__sub2(leo_Vector2 a, leo_Vector2 b)
{
    leo_Vector2 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    return r;
}
static LEO_INLINE leo_Vector2 leo__add2(leo_Vector2 a, leo_Vector2 b)
{
    leo_Vector2 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    return r;
}
static LEO_INLINE leo_Vector2 leo__scale2(leo_Vector2 a, float s)
{
    leo_Vector2 r;
    r.x = a.x * s;
    r.y = a.y * s;
    return r;
}

/* Rectangle helpers */
static LEO_INLINE float rec_left(leo_Rectangle r)
{
    return r.x;
}
static LEO_INLINE float rec_right(leo_Rectangle r)
{
    return r.x + r.width;
}
static LEO_INLINE float rec_top(leo_Rectangle r)
{
    return r.y;
}
static LEO_INLINE float rec_bot(leo_Rectangle r)
{
    return r.y + r.height;
}

bool leo_CheckCollisionRecs(leo_Rectangle a, leo_Rectangle b)
{
    /* Edge-touch counts as collision => use <= / >= in the separating axis test complement */
    if (rec_right(a) < rec_left(b))
        return false;
    if (rec_right(b) < rec_left(a))
        return false;
    if (rec_bot(a) < rec_top(b))
        return false;
    if (rec_bot(b) < rec_top(a))
        return false;
    return true;
}

bool leo_CheckCollisionCircles(leo_Vector2 c1, float r1, leo_Vector2 c2, float r2)
{
    float rr = r1 + r2;
    leo_Vector2 d = leo__sub2(c2, c1);
    return leo__len2_sq(d) <= rr * rr + LEO_EPS;
}

bool leo_CheckCollisionCircleRec(leo_Vector2 c, float r, leo_Rectangle rec)
{
    float cx = leo__clampf(c.x, rec_left(rec), rec_right(rec));
    float cy = leo__clampf(c.y, rec_top(rec), rec_bot(rec));
    float dx = c.x - cx, dy = c.y - cy;
    return (dx * dx + dy * dy) <= r * r + LEO_EPS;
}

bool leo_CheckCollisionCircleLine(leo_Vector2 center, float radius, leo_Vector2 p1, leo_Vector2 p2)
{
    /* Distance from point to segment <= radius? */
    leo_Vector2 v = leo__sub2(p2, p1);
    float len2 = leo__len2_sq(v);
    if (len2 <= LEO_EPS)
    {
        /* Degenerate segment -> point distance */
        float dx = center.x - p1.x, dy = center.y - p1.y;
        return (dx * dx + dy * dy) <= radius * radius + LEO_EPS;
    }
    float t = leo__dot2(leo__sub2(center, p1), v) / len2;
    t = leo__clampf(t, 0.0f, 1.0f);
    leo_Vector2 proj = leo__add2(p1, leo__scale2(v, t));
    float dx = center.x - proj.x, dy = center.y - proj.y;
    return (dx * dx + dy * dy) <= radius * radius + LEO_EPS;
}

bool leo_CheckCollisionPointRec(leo_Vector2 p, leo_Rectangle r)
{
    return (p.x >= rec_left(r) - LEO_EPS) && (p.x <= rec_right(r) + LEO_EPS) && (p.y >= rec_top(r) - LEO_EPS) &&
           (p.y <= rec_bot(r) + LEO_EPS);
}

bool leo_CheckCollisionPointCircle(leo_Vector2 p, leo_Vector2 c, float r)
{
    float dx = p.x - c.x, dy = p.y - c.y;
    return (dx * dx + dy * dy) <= r * r + LEO_EPS;
}

static LEO_INLINE float sign_area(leo_Vector2 a, leo_Vector2 b, leo_Vector2 c)
{
    return (a.x - c.x) * (b.y - c.y) - (b.x - c.x) * (a.y - c.y);
}

bool leo_CheckCollisionPointTriangle(leo_Vector2 p, leo_Vector2 a, leo_Vector2 b, leo_Vector2 c)
{
    /* Barycentric sign method, edge-inclusive */
    float s1 = sign_area(p, a, b);
    float s2 = sign_area(p, b, c);
    float s3 = sign_area(p, c, a);

    bool has_neg = (s1 < -LEO_EPS) || (s2 < -LEO_EPS) || (s3 < -LEO_EPS);
    bool has_pos = (s1 > LEO_EPS) || (s2 > LEO_EPS) || (s3 > LEO_EPS);
    return !(has_neg && has_pos); /* inside if not both signs present */
}

bool leo_CheckCollisionPointLine(leo_Vector2 point, leo_Vector2 p1, leo_Vector2 p2, float threshold)
{
    if (threshold < 0.0f)
        threshold = -threshold;
    /* Distance from point to the segment */
    leo_Vector2 v = leo__sub2(p2, p1);
    float len2 = leo__len2_sq(v);
    if (len2 <= LEO_EPS)
    {
        float dx = point.x - p1.x, dy = point.y - p1.y;
        return (dx * dx + dy * dy) <= threshold * threshold + LEO_EPS;
    }
    float t = leo__dot2(leo__sub2(point, p1), v) / len2;
    t = leo__clampf(t, 0.0f, 1.0f);
    leo_Vector2 proj = leo__add2(p1, leo__scale2(v, t));
    float dx = point.x - proj.x, dy = point.y - proj.y;
    return (dx * dx + dy * dy) <= threshold * threshold + LEO_EPS;
}

bool leo_CheckCollisionLines(leo_Vector2 a1, leo_Vector2 a2, leo_Vector2 b1, leo_Vector2 b2, leo_Vector2 *out_pt)
{
    /* Solve a1 + t*(a2-a1) = b1 + u*(b2-b1), with t,u in [0,1] */
    float x1 = a1.x, y1 = a1.y, x2 = a2.x, y2 = a2.y;
    float x3 = b1.x, y3 = b1.y, x4 = b2.x, y4 = b2.y;

    float den = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);

    if (fabsf(den) <= LEO_EPS)
    {
        /* Parallel or collinear */
        /* Check collinearity via cross product with one endpoint */
        float cross = (x2 - x1) * (y3 - y1) - (y2 - y1) * (x3 - x1);
        if (fabsf(cross) > 1e-4f)
            return false; /* parallel, non-overlapping */

        /* Project onto X or Y and test interval overlap */
        float ax0 = leo__minf(x1, x2), ax1 = leo__maxf(x1, x2);
        float bx0 = leo__minf(x3, x4), bx1 = leo__maxf(x3, x4);
        float ay0 = leo__minf(y1, y2), ay1 = leo__maxf(y1, y2);
        float by0 = leo__minf(y3, y4), by1 = leo__maxf(y3, y4);

        bool overlapX = !(ax1 < bx0 - LEO_EPS || bx1 < ax0 - LEO_EPS);
        bool overlapY = !(ay1 < by0 - LEO_EPS || by1 < ay0 - LEO_EPS);
        if (!(overlapX && overlapY))
            return false;

        if (out_pt)
        {
            /* Choose a reasonable point from the overlapping region:
               use the midpoint of the overlapping endpoint pair projected on the longer axis */
            if (fabsf(x1 - x2) >= fabsf(y1 - y2))
            {
                float s0 = leo__maxf(ax0, bx0);
                float s1 = leo__minf(ax1, bx1);
                float xm = 0.5f * (s0 + s1);
                /* solve for y along segment a */
                float t = (fabsf(x2 - x1) <= LEO_EPS) ? 0.0f : (xm - x1) / (x2 - x1);
                t = leo__clampf(t, 0.0f, 1.0f);
                out_pt->x = xm;
                out_pt->y = y1 + t * (y2 - y1);
            }
            else
            {
                float s0 = leo__maxf(ay0, by0);
                float s1 = leo__minf(ay1, by1);
                float ym = 0.5f * (s0 + s1);
                float t = (fabsf(y2 - y1) <= LEO_EPS) ? 0.0f : (ym - y1) / (y2 - y1);
                t = leo__clampf(t, 0.0f, 1.0f);
                out_pt->y = ym;
                out_pt->x = x1 + t * (x2 - x1);
            }
        }
        return true;
    }

    float t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / den;
    float u = ((x1 - x3) * (y1 - y2) - (y1 - y3) * (x1 - x2)) / den;

    if (t < -LEO_EPS || t > 1.0f + LEO_EPS || u < -LEO_EPS || u > 1.0f + LEO_EPS)
    {
        return false;
    }

    if (out_pt)
    {
        out_pt->x = x1 + t * (x2 - x1);
        out_pt->y = y1 + t * (y2 - y1);
    }
    return true;
}

leo_Rectangle leo_GetCollisionRec(leo_Rectangle a, leo_Rectangle b)
{
    float left = leo__maxf(rec_left(a), rec_left(b));
    float top = leo__maxf(rec_top(a), rec_top(b));
    float right = leo__minf(rec_right(a), rec_right(b));
    float bottom = leo__minf(rec_bot(a), rec_bot(b));

    leo_Rectangle r;
    if (right >= left && bottom >= top)
    {
        r.x = left;
        r.y = top;
        r.width = right - left;
        r.height = bottom - top;
    }
    else
    {
        r.x = r.y = r.width = r.height = 0.0f;
    }
    return r;
}
