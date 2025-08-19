#include "leo/graphics.h"
#include "leo/engine.h"

#include <SDL3/SDL.h>

void leo_DrawPixel(int posX, int posY, leo_Color color)
{
	SDL_Renderer* renderer = (SDL_Renderer*)leo_GetRenderer();
	if (!renderer) return;

	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderPoint(renderer, posX, posY);
}

void leo_DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, leo_Color color)
{
	// Implement this function using SDL3 functions
	SDL_Renderer* renderer = (SDL_Renderer*)leo_GetRenderer();
	if (!renderer) return;

	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderLine(renderer, startPosX, startPosY, endPosX, endPosY);
}
