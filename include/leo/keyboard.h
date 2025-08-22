#pragma once


#include <SDL3/SDL.h>

#include "leo/export.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma once

// Input-related functions: keyboard
// Check if a key has been pressed once
LEO_API bool leo_IsKeyPressed(int key);

// Check if a key has been pressed again
LEO_API bool leo_IsKeyPressedRepeat(int key);

// Check if a key is being pressed
LEO_API bool leo_IsKeyDown(int key);

// Check if a key has been released once
LEO_API bool leo_IsKeyReleased(int key);

// Check if a key is NOT being pressed
LEO_API bool leo_IsKeyUp(int key);

// Get key pressed (keycode), call it multiple times for keys queued, returns 0 when the queue is empty
LEO_API int leo_GetKeyPressed(void);

// Get char pressed (unicode), call it multiple times for chars queued, returns 0 when the queue is empty
LEO_API int leo_GetCharPressed(void);

// Set a custom key to exit program (default is ESC)
LEO_API void leo_SetExitKey(int key);

// Update keyboard state (call this each frame)
LEO_API void leo_UpdateKeyboard(void);

// Check if the exit key is pressed
LEO_API bool leo_IsExitKeyPressed(void);

// Cleanup keyboard resources (call this when shutting down)
LEO_API void leo_CleanupKeyboard(void);

#ifdef TESTING
// Expose internal state for testing purposes
// These variables allow keyboard_test.cpp to directly manipulate and verify state
extern LEO_API int s_numKeys;
extern LEO_API bool* s_currentKeys;
extern LEO_API bool* s_prevKeys;

// Test mode functions
LEO_API void leo_EnableTestMode(void);

LEO_API void leo_DisableTestMode(void);

LEO_API void leo_SetTestKeyState(SDL_Scancode scancode, bool pressed);
#endif

#ifdef __cplusplus
} // extern "C"
#endif
