#pragma once

#include "leo/export.h"
#include "leo/engine.h"   // leo_Vector2, leo_Rectangle
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Basic shapes collision detection (Raylib parity, leo_* types)              */
/* -------------------------------------------------------------------------- */

/* Check collision between two axis-aligned rectangles (AABB). */
LEO_API bool leo_CheckCollisionRecs(leo_Rectangle rec1, leo_Rectangle rec2);

/* Check collision between two circles. */
LEO_API bool leo_CheckCollisionCircles(leo_Vector2 center1, float radius1,
                                       leo_Vector2 center2, float radius2);

/* Check collision between a circle and a rectangle (AABB). */
LEO_API bool leo_CheckCollisionCircleRec(leo_Vector2 center, float radius,
                                         leo_Rectangle rec);

/* Check if a circle collides with a line segment [p1..p2]. */
LEO_API bool leo_CheckCollisionCircleLine(leo_Vector2 center, float radius,
                                          leo_Vector2 p1, leo_Vector2 p2);

/* Check if a point is inside a rectangle (AABB). */
LEO_API bool leo_CheckCollisionPointRec(leo_Vector2 point, leo_Rectangle rec);

/* Check if a point is inside a circle. */
LEO_API bool leo_CheckCollisionPointCircle(leo_Vector2 point,
                                           leo_Vector2 center, float radius);

/* Check if a point is inside a triangle (barycentric test). */
LEO_API bool leo_CheckCollisionPointTriangle(leo_Vector2 point,
                                             leo_Vector2 p1,
                                             leo_Vector2 p2,
                                             leo_Vector2 p3);

/* Check if a point lies on the line segment [p1..p2] within |threshold| pixels. */
LEO_API bool leo_CheckCollisionPointLine(leo_Vector2 point,
                                         leo_Vector2 p1, leo_Vector2 p2,
                                         float threshold);

/* Check collision between two line segments [startPos1..endPos1] and
   [startPos2..endPos2]. If they intersect, writes the collision point (if non-NULL). */
LEO_API bool leo_CheckCollisionLines(leo_Vector2 startPos1, leo_Vector2 endPos1,
                                     leo_Vector2 startPos2, leo_Vector2 endPos2,
                                     leo_Vector2* collisionPoint /* optional */);

/* Get the intersection rectangle (overlap) for two rectangles; returns {0,0,0,0} if none. */
LEO_API leo_Rectangle leo_GetCollisionRec(leo_Rectangle rec1, leo_Rectangle rec2);

#ifdef __cplusplus
} /* extern "C" */
#endif
