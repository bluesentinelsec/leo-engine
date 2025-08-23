// engine.c — Leo runtime: window, timing, 2D camera, render textures, texture blits.

#include "leo/engine.h"
#include "leo/error.h"
#include "leo/color.h"
#include "leo/keyboard.h"

#include <SDL3/SDL.h>
#include <math.h>
#include <string.h>

SDL_Window* globalWindow = NULL;
SDL_Renderer* globalRenderer = NULL;

/* ---------- internal state for the simple game loop ---------- */
static int s_inFrame = 0; /* are we between Begin/EndDrawing? */
static int s_quit = 0; /* latched quit flag */
static Uint8 s_clearR = 0, s_clearG = 0, s_clearB = 0, s_clearA = 255; /* last clear color */

/* ---------- timing state ---------- */
static int s_targetFPS = 0;
static double s_targetFrameSecs = 0.0;

static Uint64 s_perfFreq = 0;
static Uint64 s_startCounter = 0; /* when InitWindow succeeded */
static Uint64 s_frameStartCounter = 0; /* set in BeginDrawing */

static float s_lastFrameTime = 0.0f; /* seconds */
static int s_fpsCounter = 0; /* frames counted in the current 1s window */
static int s_currentFPS = 0; /* last computed FPS value */
static Uint64 s_fpsWindowStart = 0; /* start counter for FPS window */

/* ---------- camera state ---------- */
// A small stack is enough for typical nesting (UI overlays, etc.)
static leo_Camera2D s_camStack[8];
static int s_camTop = -1;

// Current affine: screen <- world (row-major 2x3)
static float s_m11 = 1.0f, s_m12 = 0.0f, s_tx = 0.0f;
static float s_m21 = 0.0f, s_m22 = 1.0f, s_ty = 0.0f;

static inline float _deg2rad(float deg) { return deg * 0.01745329251994329577f; } // pi/180

static void _rebuildCameraMatrix(const leo_Camera2D* c)
{
	if (!c)
	{
		s_m11 = 1.0f;
		s_m12 = 0.0f;
		s_tx = 0.0f;
		s_m21 = 0.0f;
		s_m22 = 1.0f;
		s_ty = 0.0f;
		return;
	}
	const float z = (c->zoom <= 0.0f) ? 1.0f : c->zoom;
	const float rad = _deg2rad(c->rotation);
	const float cr = cosf(rad);
	const float sr = sinf(rad);

	// A = S*R
	const float a = z * cr; // m11
	const float b = z * sr; // m12
	const float c21 = -z * sr; // m21
	const float d = z * cr; // m22

	// T = offset - A * target
	const float Txx = c->offset.x - (a * c->target.x + b * c->target.y);
	const float Tyy = c->offset.y - (c21 * c->target.x + d * c->target.y);

	s_m11 = a;
	s_m12 = b;
	s_tx = Txx;
	s_m21 = c21;
	s_m22 = d;
	s_ty = Tyy;
}

// Build a one-off matrix from a provided camera (for public helpers)
static void _buildCam3x2(const leo_Camera2D* c,
	float* m11, float* m12, float* m21, float* m22, float* tx, float* ty)
{
	const float z = (c->zoom <= 0.0f) ? 1.0f : c->zoom;
	const float rad = _deg2rad(c->rotation);
	const float cr = cosf(rad);
	const float sr = sinf(rad);
	const float a = z * cr, b = z * sr, c21 = -z * sr, d = z * cr;
	const float Txx = c->offset.x - (a * c->target.x + b * c->target.y);
	const float Tyy = c->offset.y - (c21 * c->target.x + d * c->target.y);
	if (m11) *m11 = a;
	if (m12) *m12 = b;
	if (m21) *m21 = c21;
	if (m22) *m22 = d;
	if (tx) *tx = Txx;
	if (ty) *ty = Tyy;
}

/* ---------- render-target stack (textures / backbuffer) ---------- */
#ifndef LEO_RT_STACK_MAX
#define LEO_RT_STACK_MAX 8
#endif
static SDL_Texture* s_rtStack[LEO_RT_STACK_MAX];
static int s_rtTop = -1;

static void _rtPush(SDL_Texture* t)
{
	if (s_rtTop + 1 < LEO_RT_STACK_MAX) ++s_rtTop;
	s_rtStack[s_rtTop] = t;
}

static SDL_Texture* _rtPop(void)
{
	if (s_rtTop < 0) return NULL;
	SDL_Texture* t = s_rtStack[s_rtTop];
	--s_rtTop;
	return t;
}

/* -------------------------
   Public: window & loop
------------------------- */
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

	/* initialize timing */
	s_perfFreq = SDL_GetPerformanceFrequency();
	s_startCounter = SDL_GetPerformanceCounter();
	s_frameStartCounter = s_startCounter;
	s_fpsWindowStart = s_startCounter;
	s_lastFrameTime = 0.0f;
	s_fpsCounter = 0;
	s_currentFPS = 0;
	s_targetFPS = 0;
	s_targetFrameSecs = 0.0;

	// camera to identity
	s_camTop = -1;
	_rebuildCameraMatrix(NULL);

	// render-target stack
	s_rtTop = -1;

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

	/* reset timing */
	s_perfFreq = 0;
	s_startCounter = 0;
	s_frameStartCounter = 0;
	s_fpsWindowStart = 0;
	s_lastFrameTime = 0.0f;
	s_fpsCounter = 0;
	s_currentFPS = 0;
	s_targetFPS = 0;
	s_targetFrameSecs = 0.0;

	// camera reset
	s_camTop = -1;
	_rebuildCameraMatrix(NULL);

	// render-target stack reset
	s_rtTop = -1;

	leo_CleanupKeyboard();
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
	if (!SDL_SetWindowFullscreen(globalWindow, enabled))
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
	leo_UpdateKeyboard();
	if (leo_IsExitKeyPressed())
	{
		s_quit = 1;
	}

	return s_quit != 0;
}

void leo_BeginDrawing(void)
{
	if (!globalRenderer) return;
	s_inFrame = 1;
	/* Raylib pattern: user calls ClearBackground() explicitly after BeginDrawing().
	   We don’t clear automatically here to preserve that feel. */
	s_frameStartCounter = SDL_GetPerformanceCounter();
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

	/* Present only if we are drawing to the backbuffer (not to a texture). */
	if (SDL_GetRenderTarget(globalRenderer) == NULL)
	{
		SDL_RenderPresent(globalRenderer);
	}

	Uint64 now = SDL_GetPerformanceCounter();
	double elapsed = 0.0;
	if (s_perfFreq)
	{
		elapsed = (double)(now - s_frameStartCounter) / (double)s_perfFreq;
	}

	/* If a target FPS is set, sleep the remainder to hit the desired frame time. */
	if (s_targetFPS > 0 && s_targetFrameSecs > 0.0 && elapsed < s_targetFrameSecs)
	{
		double remaining = s_targetFrameSecs - elapsed;
		if (remaining > 0.0)
		{
			/* Convert to ms for SDL_Delay; clamp at 0 */
			double ms = remaining * 1000.0;
			if (ms > 0.0) SDL_Delay((Uint32)(ms + 0.5)); /* round */
		}
		/* Recompute total frame time including the delay */
		now = SDL_GetPerformanceCounter();
		if (s_perfFreq)
		{
			elapsed = (double)(now - s_frameStartCounter) / (double)s_perfFreq;
		}
	}

	/* Update last frame time (float seconds) */
	s_lastFrameTime = (float)elapsed;

	/* Rolling FPS: count frames inside ~1s windows */
	s_fpsCounter += 1;
	if (s_perfFreq && (now - s_fpsWindowStart) >= s_perfFreq)
	{
		s_currentFPS = s_fpsCounter;
		s_fpsCounter = 0;
		s_fpsWindowStart = now;
	}

	s_inFrame = 0;
}

void leo_SetTargetFPS(int fps)
{
	if (fps <= 0)
	{
		s_targetFPS = 0;
		s_targetFrameSecs = 0.0;
		return;
	}
	if (fps > 1000) fps = 1000; /* guard against silly values */
	s_targetFPS = fps;
	s_targetFrameSecs = 1.0 / (double)fps;
}

float leo_GetFrameTime(void)
{
	return s_lastFrameTime; /* seconds for the most recently completed frame */
}

double leo_GetTime(void)
{
	if (!s_perfFreq || !s_startCounter) return 0.0;
	Uint64 now = SDL_GetPerformanceCounter();
	return (double)(now - s_startCounter) / (double)s_perfFreq;
}

int leo_GetFPS(void)
{
	/* If we haven’t accumulated a full second yet, estimate from the last frame time. */
	if (s_currentFPS == 0 && s_lastFrameTime > 0.0f)
	{
		int est = (int)(1.0f / s_lastFrameTime + 0.5f);
		if (est < 0) est = 0;
		return est;
	}
	return s_currentFPS;
}

int leo_GetScreenWidth(void)
{
	if (!globalWindow) return 0;
	int w = 0, h = 0;
	SDL_GetWindowSizeInPixels(globalWindow, &w, &h);
	return w;
}

int leo_GetScreenHeight(void)
{
	if (!globalWindow) return 0;
	int w = 0, h = 0;
	SDL_GetWindowSizeInPixels(globalWindow, &w, &h);
	return h;
}

/* -------------------------
   Camera2D API
------------------------- */
void leo_BeginMode2D(leo_Camera2D camera)
{
	const int cap = (int)(sizeof(s_camStack) / sizeof(s_camStack[0]));
	if (s_camTop + 1 < cap)
	{
		s_camTop++;
	}
	else
	{
		// degrade gracefully: overwrite top
	}
	// Guard rails for raylib-style behavior
	if (camera.zoom <= 0.0f) camera.zoom = 1.0f;
	// Optional: keep rotation bounded (raylib doesn't require it, but it's harmless)
	if (camera.rotation > 360.0f || camera.rotation < -360.0f)
	{
		int k = (int)(camera.rotation / 360.0f);
		camera.rotation -= 360.0f * (float)k;
	}
	s_camStack[s_camTop] = camera;
	_rebuildCameraMatrix(&s_camStack[s_camTop]);
}

void leo_EndMode2D(void)
{
	if (s_camTop >= 0) s_camTop--;
	_rebuildCameraMatrix(s_camTop >= 0 ? &s_camStack[s_camTop] : NULL);
}

leo_Camera2D leo_GetCurrentCamera2D(void)
{
	if (s_camTop >= 0) return s_camStack[s_camTop];
	// identity camera
	leo_Camera2D id;
	id.target.x = 0.0f;
	id.target.y = 0.0f;
	id.offset.x = 0.0f;
	id.offset.y = 0.0f;
	id.rotation = 0.0f;
	id.zoom = 1.0f;
	return id;
}

leo_Vector2 leo_GetWorldToScreen2D(leo_Vector2 p, leo_Camera2D cam)
{
	if (cam.zoom <= 0.0f) cam.zoom = 1.0f;
	float m11, m12, m21, m22, tx, ty;
	_buildCam3x2(&cam, &m11, &m12, &m21, &m22, &tx, &ty);
	leo_Vector2 out = {
		m11 * p.x + m12 * p.y + tx,
		m21 * p.x + m22 * p.y + ty
	};
	return out;
}

leo_Vector2 leo_GetScreenToWorld2D(leo_Vector2 p, leo_Camera2D cam)
{
	if (cam.zoom <= 0.0f) cam.zoom = 1.0f;
	float m11, m12, m21, m22, tx, ty;
	_buildCam3x2(&cam, &m11, &m12, &m21, &m22, &tx, &ty);
	const float sx = p.x - tx;
	const float sy = p.y - ty;
	const float det = m11 * m22 - m12 * m21;
	const float inv = (det != 0.0f) ? (1.0f / det) : 1.0f;
	leo_Vector2 out = {
		inv * (m22 * sx - m12 * sy),
		inv * (-m21 * sx + m11 * sy)
	};
	return out;
}

/* -------------------------
   RenderTexture API (raylib-style subset)
------------------------- */
leo_RenderTexture2D leo_LoadRenderTexture(int width, int height)
{
	leo_RenderTexture2D rt;
	memset(&rt, 0, sizeof(rt));
	if (!globalRenderer || width <= 0 || height <= 0) return rt;

	Uint32 fmt = SDL_PIXELFORMAT_RGBA32;
	SDL_Texture* tex = SDL_CreateTexture(globalRenderer, fmt, SDL_TEXTUREACCESS_TARGET, width, height);
	if (!tex) return rt;

	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

	rt.width = width;
	rt.height = height;
	rt.texture.width = width;
	rt.texture.height = height;
	rt.texture._handle = tex;
	rt._rt_handle = tex;
	return rt;
}

void leo_UnloadRenderTexture(leo_RenderTexture2D target)
{
	if (target._rt_handle) SDL_DestroyTexture((SDL_Texture*)target._rt_handle);
}

void leo_BeginTextureMode(leo_RenderTexture2D target)
{
	if (!globalRenderer || !target._rt_handle) return;
	SDL_Texture* current = SDL_GetRenderTarget(globalRenderer);
	_rtPush(current);
	SDL_SetRenderTarget(globalRenderer, (SDL_Texture*)target._rt_handle);
}

void leo_EndTextureMode(void)
{
	if (!globalRenderer) return;
	SDL_Texture* prev = _rtPop();
	SDL_SetRenderTarget(globalRenderer, prev); // NULL means backbuffer
}

/* -------------------------
   Texture drawing
------------------------- */
void leo_DrawTextureRec(leo_Texture2D tex, leo_Rectangle src, leo_Vector2 position, leo_Color tint)
{
	if (!globalRenderer || !tex._handle) return;

	SDL_FRect s = { src.x, src.y, src.width, src.height };
	// raylib-style flip: negative width/height flips & shifts origin
	if (s.w < 0.0f)
	{
		s.x += s.w;
		s.w = -s.w;
	}
	if (s.h < 0.0f)
	{
		s.y += s.h;
		s.h = -s.h;
	}

	SDL_FRect d = { position.x, position.y, s.w, s.h };

	SDL_Texture* t = (SDL_Texture*)tex._handle;

	// Apply tint
	SDL_SetTextureColorMod(t, tint.r, tint.g, tint.b);
	SDL_SetTextureAlphaMod(t, tint.a);

	// SDL3 blit
	SDL_RenderTexture(globalRenderer, t, &s, &d);

	// Restore neutral modulation (avoid accidental tinting later)
	SDL_SetTextureColorMod(t, 255, 255, 255);
	SDL_SetTextureAlphaMod(t, 255);
}
