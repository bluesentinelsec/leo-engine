// =============================================
// tests/collisions_test.cpp
// =============================================
#include <catch2/catch_all.hpp>

#include "leo/collisions.h"
#include "leo/engine.h"

struct GfxEnvCollisions
{
    GfxEnvCollisions()
    {
        REQUIRE(leo_InitWindow(320, 180, "collisions"));
    }
    ~GfxEnvCollisions()
    {
        leo_CloseWindow();
    }
};

TEST_CASE_METHOD(GfxEnvCollisions, "AABB vs AABB", "[collisions]")
{
    leo_Rectangle a{10, 10, 40, 20};
    leo_Rectangle b{40, 25, 50, 20}; // overlaps at one corner
    REQUIRE(leo_CheckCollisionRecs(a, b));

    // Just touching at the edge counts as collision
    leo_Rectangle c{50, 10, 20, 10}; // right edge of 'a' meets left edge of 'c'
    REQUIRE(leo_CheckCollisionRecs(a, c));

    // Separated
    leo_Rectangle d{200, 200, 10, 10};
    REQUIRE_FALSE(leo_CheckCollisionRecs(a, d));

    // Intersection rectangle
    auto inter = leo_GetCollisionRec(a, b);
    CHECK(inter.width > 0.0f);
    CHECK(inter.height > 0.0f);
}

TEST_CASE_METHOD(GfxEnvCollisions, "Circle vs Circle", "[collisions]")
{
    leo_Vector2 c1{50, 50}, c2{90, 50};
    float r1 = 20.f, r2 = 20.f;

    // Touching (distance == r1 + r2)
    REQUIRE(leo_CheckCollisionCircles(c1, r1, c2, r2));

    // Separated
    leo_Vector2 c3{200, 50};
    REQUIRE_FALSE(leo_CheckCollisionCircles(c1, r1, c3, r2));
}

TEST_CASE_METHOD(GfxEnvCollisions, "Circle vs AABB", "[collisions]")
{
    leo_Rectangle box{40, 40, 40, 30};
    leo_Vector2 c{30, 55};
    float r = 10.f;

    // Circle touches the left edge
    REQUIRE(leo_CheckCollisionCircleRec(c, r, box));

    // Move further left
    leo_Vector2 c2{25, 55};
    REQUIRE_FALSE(leo_CheckCollisionCircleRec(c2, r, box));
}

TEST_CASE_METHOD(GfxEnvCollisions, "Circle vs Line segment", "[collisions]")
{
    leo_Vector2 center{50, 50};
    float r = 10.f;
    leo_Vector2 p1{40, 40}, p2{100, 40};

    // Vertical distance 10 => touches
    REQUIRE(leo_CheckCollisionCircleLine(center, r, p1, p2));

    // Shift line up by 1 (distance 11) => no touch
    leo_Vector2 p1u{40, 39}, p2u{100, 39};
    REQUIRE_FALSE(leo_CheckCollisionCircleLine(center, r, p1u, p2u));
}

TEST_CASE_METHOD(GfxEnvCollisions, "Point in shapes", "[collisions]")
{
    leo_Vector2 P{50, 50};

    // Point in rect (edge inclusive)
    leo_Rectangle R{40, 40, 20, 10};
    REQUIRE(leo_CheckCollisionPointRec(P, R));

    // Point in circle
    leo_Vector2 C{55, 55};
    REQUIRE(leo_CheckCollisionPointCircle(P, C, 8.0f));
    REQUIRE_FALSE(leo_CheckCollisionPointCircle(P, C, 6.0f));

    // Point on line (threshold)
    leo_Vector2 L1{0, 0}, L2{100, 100};
    REQUIRE(leo_CheckCollisionPointLine(P, L1, L2, 2.0f));
    REQUIRE_FALSE(leo_CheckCollisionPointLine(leo_Vector2{50, 53}, L1, L2, 2.0f));

    // Point in triangle
    leo_Vector2 A{40, 40}, B{80, 40}, Ctri{60, 80};
    REQUIRE(leo_CheckCollisionPointTriangle(leo_Vector2{60, 55}, A, B, Ctri));
    REQUIRE_FALSE(leo_CheckCollisionPointTriangle(leo_Vector2{20, 55}, A, B, Ctri));
}

TEST_CASE_METHOD(GfxEnvCollisions, "Segment vs Segment", "[collisions]")
{
    leo_Vector2 a1{10, 10}, a2{100, 100};
    leo_Vector2 b1{10, 100}, b2{100, 10};

    leo_Vector2 hit{};
    bool ok = leo_CheckCollisionLines(a1, a2, b1, b2, &hit);
    REQUIRE(ok);

    // The intersection should be close to the midpoint (55,55)
    CHECK(hit.x == Catch::Approx(55.0f).margin(0.01f));
    CHECK(hit.y == Catch::Approx(55.0f).margin(0.01f));

    // Parallel non-intersecting
    ok = leo_CheckCollisionLines(leo_Vector2{0, 0}, leo_Vector2{100, 0}, leo_Vector2{0, 10}, leo_Vector2{100, 10},
                                 nullptr);
    REQUIRE_FALSE(ok);

    // Collinear overlapping segments -> considered "collision"
    ok = leo_CheckCollisionLines(leo_Vector2{0, 0}, leo_Vector2{100, 0}, leo_Vector2{50, 0}, leo_Vector2{150, 0}, &hit);
    REQUIRE(ok);
}
