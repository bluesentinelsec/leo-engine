#include "leo/engine.h"
#include "leo/error.h"

#include <SDL3/SDL.h>

SDL_Window* globalWindow = NULL;
SDL_Renderer* globalRenderer = NULL;

/* ---------- internal state for the simple game loop ---------- */
static int s_inFrame = 0; /* are we between Begin/EndDrawing? */
static int s_quit = 0; /* latched quit flag */
static Uint8 s_clearR = 0, s_clearG = 0, s_clearB = 0, s_clearA = 255; /* last clear color */


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

	/* initialize loop state */
	s_inFrame = 0;
	s_quit = 0;
	s_clearR = 0;
	s_clearG = 0;
	s_clearB = 0;
	s_clearA = 255;


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

	s_inFrame = 0;
	s_quit = 0;

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

bool leo_WindowShouldClose(void)
{
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
		case SDL_EVENT_QUIT:
			s_quit = 1;
			break;

		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			s_quit = 1;
			break;

		default:
			break;
		}
	}

	return s_quit != 0;
}

void leo_BeginDrawing(void)
{
	if (!globalRenderer) return;
	s_inFrame = 1;
	/* Raylib pattern: user calls ClearBackground() explicitly after BeginDrawing().
	   We don’t clear automatically here to preserve that feel. */
}

void leo_ClearBackground(int r, int g, int b, int a)
{
	if (!globalRenderer) return;

	/* Clamp to 0..255 to be safe */
	if (r < 0) r = 0;
	if (r > 255) r = 255;
	if (g < 0) g = 0;
	if (g > 255) g = 255;
	if (b < 0) b = 0;
	if (b > 255) b = 255;
	if (a < 0) a = 0;
	if (a > 255) a = 255;

	s_clearR = (Uint8)r;
	s_clearG = (Uint8)g;
	s_clearB = (Uint8)b;
	s_clearA = (Uint8)a;

	/* Clear the current render target immediately */
	SDL_SetRenderDrawColor(globalRenderer, s_clearR, s_clearG, s_clearB, s_clearA);
	SDL_RenderClear(globalRenderer);
}

void leo_EndDrawing(void)
{
	if (!globalRenderer) return;
	/* Present the backbuffer */
	SDL_RenderPresent(globalRenderer);
	s_inFrame = 0;
}
