#pragma once

#include "leo/export.h"
#include "leo/color.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* LeoWindow;
typedef void* LeoRenderer;

typedef struct
{
	float x, y, width, height;
} leo_Rectangle;

typedef struct
{
	int width;
	int height;
	// opaque handle to an SDL texture (color target), kept private in .c
	void* _handle;
} leo_Texture2D;

typedef struct
{
	leo_Texture2D texture; // color texture (like raylib's RenderTexture.texture)
	int width;
	int height;
	void* _rt_handle; // same as texture._handle; kept for symmetry/extension
} leo_RenderTexture2D;

// --- Basic math types (raylib-like) ---
typedef struct
{
	float x, y;
} leo_Vector2;

typedef struct
{
	leo_Vector2 target; // world-space point the camera looks at
	leo_Vector2 offset; // screen-space offset in pixels (where 'target' appears)
	float rotation; // degrees, positive rotates clockwise
	float zoom; // 1.0 = no zoom
} leo_Camera2D;

LEO_API bool leo_InitWindow(int width, int height, const char* title);

LEO_API void leo_CloseWindow();

LEO_API LeoWindow leo_GetWindow(void);

LEO_API LeoRenderer leo_GetRenderer(void);

LEO_API bool leo_SetFullscreen(bool enabled);

LEO_API bool leo_WindowShouldClose(void);

LEO_API void leo_ClearBackground(int r, int g, int b, int a);

LEO_API void leo_BeginDrawing(void);

LEO_API void leo_EndDrawing(void);

// Timing-related functions
LEO_API void leo_SetTargetFPS(int fps); // Set target FPS (maximum)
LEO_API float leo_GetFrameTime(void); // Get time in seconds for last frame drawn (delta time)
LEO_API double leo_GetTime(void); // Get elapsed time in seconds since InitWindow()
LEO_API int leo_GetFPS(void); // Get current FPS

LEO_API int leo_GetScreenWidth(void);

LEO_API int leo_GetScreenHeight(void);

// --- 2D Camera API (raylib-style) ---
LEO_API void leo_BeginMode2D(leo_Camera2D camera);

LEO_API void leo_EndMode2D(void);

LEO_API leo_Vector2 leo_GetWorldToScreen2D(leo_Vector2 position, leo_Camera2D camera);

LEO_API leo_Vector2 leo_GetScreenToWorld2D(leo_Vector2 position, leo_Camera2D camera);

LEO_API leo_Camera2D leo_GetCurrentCamera2D(void);

// --- RenderTexture API (raylib-style subset) ---
LEO_API leo_RenderTexture2D leo_LoadRenderTexture(int width, int height);

LEO_API void leo_UnloadRenderTexture(leo_RenderTexture2D target);

LEO_API void leo_BeginTextureMode(leo_RenderTexture2D target);

LEO_API void leo_EndTextureMode(void);

// --- Texture drawing (for RenderTexture outputs) ---
// Draws a sub-rectangle of 'tex' at 'position' in screen pixels.
// If src.width or src.height is negative, it flips along that axis (raylib-compatible).
LEO_API void leo_DrawTextureRec(leo_Texture2D tex, leo_Rectangle src, leo_Vector2 position, leo_Color tint);

#ifdef __cplusplus
}
#endif
