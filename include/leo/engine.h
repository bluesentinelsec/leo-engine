#pragma once

#include "leo/export.h"

#include <SDL3/SDL.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int WindowWidth;
    int WindowHeight;
    int VirtualRenderWidth;
    int VirtualRenderHeight;
    const char *AppName; // also used as window title
    const char *AppVersion;
    const char *AppIdentifier; // A unique string in reverse-domain format that identifies this app ("com.example.mygame2").
    bool toggleFullscreen;
    bool toggleVSYNC;

} leo_EngineState;

typedef struct leo_Engine leo_Engine;

typedef struct leo_Camera2D leo_Camera2D;

typedef struct leo_Color leoColor;

// deprecated
LEO_API int leo_sum(int a, int b);

// returns pointer to leo_EngineState initialized with sane default values.
LEO_API leo_EngineState * leo_NewEngineState();
LEO_API void leo_FreeEngineState(leo_EngineState *);

// returns pointer to initialized game engine object, which, can be
// orchestrated to create the game window, renderer, etc.
LEO_API leo_Engine * leo_NewEngine(leo_EngineState *);
LEO_API void leo_FreeEngine(leo_Engine *);

// Check if application should close (KEY_ESCAPE pressed or windows close icon clicked)
LEO_API bool leo_WindowShouldClose(leo_Engine *);

// Setup canvas (framebuffer) to start drawing
void leo_BeginDrawing(leo_Engine *);

// End canvas drawing and swap buffers (double buffering)
void leo_EndDrawing(leo_Engine *);  

// Begin drawing to render texture
LEO_API void leo_BeginTextureMode(leo_Engine *, SDL_Texture *);

// Ends drawing to render texture
LEO_API void leo_EndTextureMode(leo_Engine *);

// Begin 2D mode with custom camera (2D)
LEO_API void leo_BeginMode2D(leo_Engine *, leo_Camera2D);

// Ends 2D mode with custom camera
LEO_API void leo_EndMode2D(leo_Engine *);


void leo_ClearBackground(leo_Engine *, leoColor);



#ifdef __cplusplus
}
#endif