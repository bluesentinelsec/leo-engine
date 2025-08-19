#pragma once

#include "leo/export.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* LeoWindow;
typedef void* LeoRenderer;

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


#ifdef __cplusplus
}
#endif
