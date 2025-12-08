#pragma once

#include "leo/export.h"
#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // -------------------------------
    // Public keyboard API
    // -------------------------------

    // Check if a key has been pressed once
    LEO_API bool leo_IsKeyPressed(int key);

    // Check if a key has been pressed again (held/repeat)
    LEO_API bool leo_IsKeyPressedRepeat(int key);

    // Check if a key is being pressed
    LEO_API bool leo_IsKeyDown(int key);

    // Check if a key has been released once
    LEO_API bool leo_IsKeyReleased(int key);

    // Check if a key is NOT being pressed
    LEO_API bool leo_IsKeyUp(int key);

    // Get key pressed (keycode); call multiple times for keys queued, returns 0
    // when empty
    LEO_API int leo_GetKeyPressed(void);

    // Get char pressed (unicode); call multiple times for chars queued, returns 0
    // when empty
    LEO_API int leo_GetCharPressed(void);

    // Set a custom key to exit program (default is ESC)
    LEO_API void leo_SetExitKey(int key);

    // Update keyboard state (call this each frame)
    LEO_API void leo_UpdateKeyboard(void);

    // Check if the exit key is pressed
    LEO_API bool leo_IsExitKeyPressed(void);

    // Cleanup keyboard resources (call this when shutting down)
    LEO_API void leo_CleanupKeyboard(void);

// -------------------------------
// Test-only helpers (behind TESTING)
// -------------------------------
#ifdef TESTING
    // Enable/disable test mode (bypasses SDL_GetKeyboardState writes)
    LEO_API void leo_EnableTestMode(void);

    LEO_API void leo_DisableTestMode(void);

    // Directly set a key state in test mode
    LEO_API void leo_SetTestKeyState(SDL_Scancode scancode, bool pressed);

    // Accessors to internal state for tests (avoid raw globals)
    LEO_API int leo__test_num_keys(void);

    LEO_API const bool *leo__test_current(void);

    LEO_API const bool *leo__test_prev(void);
#endif

#ifdef __cplusplus
} // extern "C"
#endif
