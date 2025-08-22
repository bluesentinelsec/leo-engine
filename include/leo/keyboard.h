#ifndef LEO_KEYBOARD_H
#define LEO_KEYBOARD_H

#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma once

// Input-related functions: keyboard
// Check if a key has been pressed once
bool leo_IsKeyPressed(int key);

// Check if a key has been pressed again
bool leo_IsKeyPressedRepeat(int key);

// Check if a key is being pressed
bool leo_IsKeyDown(int key);

// Check if a key has been released once
bool leo_IsKeyReleased(int key);

// Check if a key is NOT being pressed
bool leo_IsKeyUp(int key);

// Get key pressed (keycode), call it multiple times for keys queued, returns 0 when the queue is empty
int leo_GetKeyPressed(void);

// Get char pressed (unicode), call it multiple times for chars queued, returns 0 when the queue is empty
int leo_GetCharPressed(void);

// Set a custom key to exit program (default is ESC)
void leo_SetExitKey(int key);

// Update keyboard state (call this each frame)
void leo_UpdateKeyboard(void);

// Check if the exit key is pressed
bool leo_IsExitKeyPressed(void);

// Cleanup keyboard resources (call this when shutting down)
void leo_CleanupKeyboard(void);

#ifdef TESTING
// Expose internal state for testing purposes
// These variables allow keyboard_test.cpp to directly manipulate and verify state
extern int s_numKeys;
extern bool* s_currentKeys;
extern bool* s_prevKeys;

// Test mode functions
void leo_EnableTestMode(void);
void leo_DisableTestMode(void);
void leo_SetTestKeyState(SDL_Scancode scancode, bool pressed);
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LEO_KEYBOARD_H

