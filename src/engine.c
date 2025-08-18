#include "leo/engine.h"
#include "leo/error.h"

#include <SDL3/SDL.h>

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;


bool leo_InitWindow(int width, int height, const char* title)
{
	SDL_InitFlags flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS;
	if (!SDL_Init(flags))
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
