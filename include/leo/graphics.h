#ifndef LEO_GRAPHICS_H
#define LEO_GRAPHICS_H

#include <SDL3/SDL.h>

namespace leo
{
namespace Graphics
{

struct Color
{
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;
};

void DrawPixel(SDL_Renderer *renderer, float x, float y, Color color);
void DrawLine(SDL_Renderer *renderer, float x1, float y1, float x2, float y2, Color color);
void DrawCircleFilled(SDL_Renderer *renderer, float cx, float cy, float radius, Color color);
void DrawCircleOutline(SDL_Renderer *renderer, float cx, float cy, float radius, Color color);
void DrawRectangleFilled(SDL_Renderer *renderer, float x, float y, float w, float h, Color color);
void DrawRectangleOutline(SDL_Renderer *renderer, float x, float y, float w, float h, Color color);
void DrawRectangleRoundedFilled(SDL_Renderer *renderer, float x, float y, float w, float h, float radius, Color color);
void DrawRectangleRoundedOutline(SDL_Renderer *renderer, float x, float y, float w, float h, float radius, Color color);
void DrawTriangleFilled(SDL_Renderer *renderer, SDL_FPoint a, SDL_FPoint b, SDL_FPoint c, Color color);
void DrawTriangleOutline(SDL_Renderer *renderer, SDL_FPoint a, SDL_FPoint b, SDL_FPoint c, Color color);
void DrawPolyFilled(SDL_Renderer *renderer, const SDL_FPoint *points, int count, Color color);
void DrawPolyOutline(SDL_Renderer *renderer, const SDL_FPoint *points, int count, Color color);

} // namespace Graphics
} // namespace leo

#endif // LEO_GRAPHICS_H
