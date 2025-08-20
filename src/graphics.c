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

void leo_DrawCircle(int centerX, int centerY, float radius, leo_Color color)
{
	SDL_Renderer* renderer = (SDL_Renderer*)leo_GetRenderer();
	if (!renderer) return;

	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

	// Convert float radius to integer for the algorithm
	int r = (int)(radius + 0.5f);
	
	// Midpoint Circle Algorithm (Bresenham's circle algorithm)
	int x = r;
	int y = 0;
	int err = 1 - r;

	while (x >= y)
	{
		// Draw 8 octants of the circle
		SDL_RenderPoint(renderer, centerX + x, centerY + y);
		SDL_RenderPoint(renderer, centerX + y, centerY + x);
		SDL_RenderPoint(renderer, centerX - y, centerY + x);
		SDL_RenderPoint(renderer, centerX - x, centerY + y);
		SDL_RenderPoint(renderer, centerX - x, centerY - y);
		SDL_RenderPoint(renderer, centerX - y, centerY - x);
		SDL_RenderPoint(renderer, centerX + y, centerY - x);
		SDL_RenderPoint(renderer, centerX + x, centerY - y);

		if (err < 0)
		{
			y += 1;
			err += 2 * y + 1;
		}
		else
		{
			x -= 1;
			err -= 2 * x + 1;
		}
	}
}

void leo_DrawRectangle(int posX, int posY, int width, int height, leo_Color color)
{
	SDL_Renderer* renderer = (SDL_Renderer*)leo_GetRenderer();
	if (!renderer) return;

	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

	// Try using SDL's built-in rectangle filling first
	SDL_Rect rect = { posX, posY, width, height };
	
	// For filled rectangles, we need to draw horizontal lines
	// This is more efficient than individual pixels
	for (int y = 0; y < height; y++)
	{
		SDL_RenderLine(renderer, posX, posY + y, posX + width - 1, posY + y);
	}
}
