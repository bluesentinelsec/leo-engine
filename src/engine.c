#include "leo/engine.h"
#include "leo/error.h"

#include <SDL3/SDL.h>

SDL_Window* globalWindow = NULL;
SDL_Renderer* globalRenderer = NULL;


bool leo_InitWindow(int width, int height, const char* title)
{
	SDL_InitFlags flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS;
	if (!SDL_Init(flags))
	{
		leo_SetError("%s\n", SDL_GetError());
		return false;
	}

#ifdef DEBUG
	Uint64 windowFlags = SDL_WINDOW_RESIZABLE;
#else
	Uint64 windowFlags = SDL_WINDOW_FULLSCREEN;
#endif

	globalWindow = SDL_CreateWindow(title, width, height, windowFlags);
	if (!globalWindow)
	{
		leo_SetError("%s\n", SDL_GetError());
		return false;
	}


	return true;
}

void leo_CloseWindow()
{
	if (globalWindow)
	{
		SDL_DestroyWindow(globalWindow);
	}
	SDL_Quit();
}
