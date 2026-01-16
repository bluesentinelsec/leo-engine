#ifndef LEO_COLLISION_H
#define LEO_COLLISION_H

#include <SDL3/SDL.h>

namespace leo
{
namespace Collision
{

bool CheckCollisionRecs(const SDL_FRect &a, const SDL_FRect &b);
bool CheckCollisionCircles(SDL_FPoint center1, float radius1, SDL_FPoint center2, float radius2);
bool CheckCollisionCircleRec(SDL_FPoint center, float radius, const SDL_FRect &rec);
bool CheckCollisionCircleLine(SDL_FPoint center, float radius, SDL_FPoint p1, SDL_FPoint p2);
bool CheckCollisionPointRec(SDL_FPoint point, const SDL_FRect &rec);
bool CheckCollisionPointCircle(SDL_FPoint point, SDL_FPoint center, float radius);
bool CheckCollisionPointTriangle(SDL_FPoint point, SDL_FPoint p1, SDL_FPoint p2, SDL_FPoint p3);
bool CheckCollisionPointLine(SDL_FPoint point, SDL_FPoint p1, SDL_FPoint p2);
bool CheckCollisionPointPoly(SDL_FPoint point, const SDL_FPoint *points, int count);
bool CheckCollisionLines(SDL_FPoint p1, SDL_FPoint p2, SDL_FPoint p3, SDL_FPoint p4);

} // namespace Collision
} // namespace leo

#endif // LEO_COLLISION_H
