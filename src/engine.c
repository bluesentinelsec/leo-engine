#include "leo/engine.h"
#include "leo/error.h"

#include <SDL3/SDL.h>

SDL_Window* globalWindow = NULL;
SDL_Renderer* globalRenderer = NULL;


bool leo_InitWindow(int width, int height, const char* title)
{
	if (width <= 0 || height <= 0)
	{
		leo_SetError("Invalid window dimensions: width=%d, height=%d", width, height);
		return false;
	}
	if (!title)
	{
		leo_SetError("Invalid window title: null pointer");
		return false;
	}

	SDL_InitFlags flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS;
	if (!SDL_Init(flags))
	{
		leo_SetError("%s\n", SDL_GetError());
		return false;
	}

	Uint64 windowFlags = SDL_WINDOW_RESIZABLE;
	globalWindow = SDL_CreateWindow(title, width, height, windowFlags);
	if (!globalWindow)
	{
		leo_SetError("%s\n", SDL_GetError());
		return false;
	}

	globalRenderer = SDL_CreateRenderer(globalWindow, NULL);
	if (!globalRenderer)
	{
		leo_SetError("%s\n", SDL_GetError());
		SDL_DestroyWindow(globalWindow);
		globalWindow = NULL;
		return false;
	}


	return true;
}

void leo_CloseWindow()
{
	if (globalRenderer)
	{
		SDL_DestroyRenderer(globalRenderer);
		globalRenderer = NULL;
	}

	if (globalWindow)
	{
		SDL_DestroyWindow(globalWindow);
		globalWindow = NULL;
	}


	SDL_Quit();
}

LeoWindow leo_GetWindow(void)
{
	return (LeoWindow)globalWindow;
}

LeoRenderer leo_GetRenderer(void)
{
	return (LeoRenderer)globalRenderer;
}

bool leo_SetFullscreen(bool enabled)
{
	if (!globalWindow)
	{
		leo_SetError("leo_SetFullscreen called before leo_InitWindow");
		return false;
	}
	if (SDL_SetWindowFullscreen(globalWindow, enabled) < 0)
	{
		leo_SetError("%s", SDL_GetError());
		return false;
	}
	return true;
}
