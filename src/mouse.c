// mouse.c — raylib-style mouse API on top of SDL3 (integrates with engine’s logical coords)

#include "leo/mouse.h"
#include "leo/engine.h"
#include <SDL3/SDL.h>
#include <string.h> /* memset, memcpy */

/* configurable max — we only use indices 1..5 (L, M, R, X1, X2) */
#ifndef LEO_MAX_MOUSE_BUTTONS
#define LEO_MAX_MOUSE_BUTTONS 8
#endif

typedef struct
{
	/* connection */
	int connected;

	/* per-frame button state (index by SDL button index 1..5) */
	Uint8 curr[LEO_MAX_MOUSE_BUTTONS];
	Uint8 prev[LEO_MAX_MOUSE_BUTTONS];

	/* wheel deltas per frame (x,y) */
	float wheelX, wheelY;

	/* absolute position in renderer coords (logical pixels if logical mode is enabled) */
	float x, y;

	/* per-frame accumulated motion since last leo_UpdateMouse() */
	float frameDX, frameDY;

	/* user-configurable transform (raylib parity) */
	int offsetX, offsetY;
	float scaleX, scaleY;
} MouseState;

static MouseState g_ms;

static int clamp_btn_index(int b)
{
	if (b < 0) return 0;
	if (b >= LEO_MAX_MOUSE_BUTTONS) return LEO_MAX_MOUSE_BUTTONS - 1;
	return b;
}

static void apply_offset_scale(float inX, float inY, float* outX, float* outY)
{
	/* (x - offset) / scale — same convention as raylib */
	float sx = (g_ms.scaleX == 0.0f) ? 1.0f : g_ms.scaleX;
	float sy = (g_ms.scaleY == 0.0f) ? 1.0f : g_ms.scaleY;

	float rx = (inX - (float)g_ms.offsetX) / sx;
	float ry = (inY - (float)g_ms.offsetY) / sy;

	if (outX) *outX = rx;
	if (outY) *outY = ry;
}

/* ---------- lifecycle ---------- */

void leo_InitMouse(void)
{
	memset(&g_ms, 0, sizeof(g_ms));
	g_ms.connected = SDL_HasMouse() ? 1 : 0;
	g_ms.scaleX = 1.0f;
	g_ms.scaleY = 1.0f;

	/* seed position if available */
	if (g_ms.connected)
	{
		float fx = 0.0f, fy = 0.0f;
		SDL_GetMouseState(&fx, &fy);
		g_ms.x = fx;
		g_ms.y = fy;
	}
}

void leo_ShutdownMouse(void)
{
	memset(&g_ms, 0, sizeof(g_ms));
}

/* engine should forward raw SDL_Event* here */
void leo_HandleMouseEvent(void* sdl_evt)
{
	const SDL_Event* e = (const SDL_Event*)sdl_evt;
	if (!e) return;

	switch (e->type)
	{
	case SDL_EVENT_MOUSE_ADDED:
		g_ms.connected = 1;
		break;

	case SDL_EVENT_MOUSE_REMOVED:
		g_ms.connected = 0;
		memset(g_ms.curr, 0, sizeof(g_ms.curr));
		memset(g_ms.prev, 0, sizeof(g_ms.prev));
		break;

	case SDL_EVENT_MOUSE_WHEEL:
		/* SDL3 sends float deltas; keep as-is */
		g_ms.wheelX += e->wheel.x;
		g_ms.wheelY += e->wheel.y;
		break;

	case SDL_EVENT_MOUSE_MOTION:
		/* window coords; with logical presentation enabled they’re already logical */
		g_ms.x = e->motion.x;
		g_ms.y = e->motion.y;
	/* accumulate per-frame delta */
		g_ms.frameDX += e->motion.xrel;
		g_ms.frameDY += e->motion.yrel;
		break;

	case SDL_EVENT_MOUSE_BUTTON_DOWN:
		if (e->button.button >= 0 && e->button.button < LEO_MAX_MOUSE_BUTTONS)
			g_ms.curr[e->button.button] = 1;
		break;

	case SDL_EVENT_MOUSE_BUTTON_UP:
		if (e->button.button >= 0 && e->button.button < LEO_MAX_MOUSE_BUTTONS)
			g_ms.curr[e->button.button] = 0;
		break;

	default:
		break;
	}
}

/* call once per frame AFTER the SDL_PollEvent loop (engine does this) */
void leo_UpdateMouse(void)
{
	/* carry previous button state into prev for edge detection */
	memcpy(g_ms.prev, g_ms.curr, sizeof(g_ms.curr));

	/* refresh connection & absolute position (fallback in case no motion events arrived) */
	g_ms.connected = SDL_HasMouse() ? 1 : 0;
	if (g_ms.connected)
	{
		float fx = 0.0f, fy = 0.0f;
		(void)SDL_GetMouseState(&fx, &fy);
		g_ms.x = fx;
		g_ms.y = fy;
	}

	/* reset per-frame accumulators for the NEXT frame */
	g_ms.wheelX = 0.0f;
	g_ms.wheelY = 0.0f;
	g_ms.frameDX = 0.0f;
	g_ms.frameDY = 0.0f;
}

/* ---------- Queries (raylib-style) ---------- */

bool leo_IsMouseButtonDown(int button)
{
	int idx = clamp_btn_index(button);
	if (!g_ms.connected) return false;
	return g_ms.curr[idx] ? true : false;
}

bool leo_IsMouseButtonUp(int button)
{
	return !leo_IsMouseButtonDown(button);
}

bool leo_IsMouseButtonPressed(int button)
{
	int idx = clamp_btn_index(button);
	if (!g_ms.connected) return false;
	return (g_ms.curr[idx] && !g_ms.prev[idx]) ? true : false;
}

bool leo_IsMouseButtonReleased(int button)
{
	int idx = clamp_btn_index(button);
	if (!g_ms.connected) return false;
	return (!g_ms.curr[idx] && g_ms.prev[idx]) ? true : false;
}

int leo_GetMouseX(void)
{
	float x = 0.0f, y = 0.0f;
	apply_offset_scale(g_ms.x, g_ms.y, &x, &y);
	(void)y;
	return (int)(x + 0.5f);
}

int leo_GetMouseY(void)
{
	float x = 0.0f, y = 0.0f;
	apply_offset_scale(g_ms.x, g_ms.y, &x, &y);
	(void)x;
	return (int)(y + 0.5f);
}

leo_Vector2 leo_GetMousePosition(void)
{
	float x = 0.0f, y = 0.0f;
	apply_offset_scale(g_ms.x, g_ms.y, &x, &y);
	leo_Vector2 v;
	v.x = x;
	v.y = y;
	return v;
}

leo_Vector2 leo_GetMouseDelta(void)
{
	/* per-frame delta accumulated from motion events since last Update() */
	float sx = (g_ms.scaleX == 0.0f) ? 1.0f : g_ms.scaleX;
	float sy = (g_ms.scaleY == 0.0f) ? 1.0f : g_ms.scaleY;

	leo_Vector2 v;
	v.x = g_ms.frameDX / sx;
	v.y = g_ms.frameDY / sy;
	return v;
}

void leo_SetMousePosition(int x, int y)
{
	SDL_Window* w = (SDL_Window*)leo_GetWindow();
	if (!w) return;

	/* inverse of apply_offset_scale: window/render coords = offset + pos*scale */
	float sx = (g_ms.scaleX == 0.0f) ? 1.0f : g_ms.scaleX;
	float sy = (g_ms.scaleY == 0.0f) ? 1.0f : g_ms.scaleY;

	float rx = (float)g_ms.offsetX + (float)x * sx;
	float ry = (float)g_ms.offsetY + (float)y * sy;

	SDL_WarpMouseInWindow(w, rx, ry);

	/* update cached absolute position immediately */
	g_ms.x = rx;
	g_ms.y = ry;
}

void leo_SetMouseOffset(int offsetX, int offsetY)
{
	g_ms.offsetX = offsetX;
	g_ms.offsetY = offsetY;
}

void leo_SetMouseScale(float scaleX, float scaleY)
{
	g_ms.scaleX = (scaleX == 0.0f) ? 1.0f : scaleX;
	g_ms.scaleY = (scaleY == 0.0f) ? 1.0f : scaleY;
}

float leo_GetMouseWheelMove(void)
{
	/* dominant axis magnitude with its sign (raylib parity) */
	float ax = (g_ms.wheelX >= 0.0f) ? g_ms.wheelX : -g_ms.wheelX;
	float ay = (g_ms.wheelY >= 0.0f) ? g_ms.wheelY : -g_ms.wheelY;
	return (ay >= ax) ? g_ms.wheelY : g_ms.wheelX;
}

leo_Vector2 leo_GetMouseWheelMoveV(void)
{
	leo_Vector2 v;
	v.x = g_ms.wheelX;
	v.y = g_ms.wheelY;
	return v;
}
