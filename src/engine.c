#include "leo/engine.h"
#include "leo/error.h"

#include <SDL3/SDL.h>


bool leo_InitWindow(int width, int height, const char* title)
{
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		leo_SetError("%s\n", SDL_GetError());
		return false;
	}
	return true;
}

void leo_CloseWindow()
{
	SDL_Quit();
}
