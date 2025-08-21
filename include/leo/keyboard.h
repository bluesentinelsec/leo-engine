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

