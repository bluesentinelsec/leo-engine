#include "leo/graphics.h"
#include "leo/engine.h"

#include <SDL3/SDL.h>

void leo_DrawPixel(int posX, int posY, leo_Color color)
{
	SDL_Renderer* renderer = (SDL_Renderer*)leo_GetRenderer();
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderPoint(renderer, posX, posY);
}
