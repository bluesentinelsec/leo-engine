// keyboard.c — robust keyboard input implementation using SDL3

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "leo/keyboard.h"

#include <SDL3/SDL.h>

// Global keyboard state variables
#ifdef TESTING
// When testing, make variables accessible to test code
int s_numKeys = 0;
bool* s_currentKeys = NULL;
bool* s_prevKeys = NULL;
static bool s_testMode = false; // Test mode flag
#else
// In production, keep variables static for encapsulation
static int s_numKeys = 0;
static bool* s_currentKeys = NULL;
static bool* s_prevKeys = NULL;
#endif
static int s_exitKey = SDL_SCANCODE_ESCAPE;
static bool s_keyboardInitialized = false;

// Initialize keyboard system
static void leo_InitKeyboard(void)
{
	if (!s_keyboardInitialized)
	{
#ifdef TESTING
		if (!s_testMode)
		{
			// Only get SDL state when not in test mode
			SDL_GetKeyboardState(&s_numKeys); // get numKeys
		}
		else
		{
			// In test mode, use a reasonable default if not set
			if (s_numKeys == 0)
			{
				s_numKeys = 512; // Reasonable default for testing
			}
		}
#else
		SDL_GetKeyboardState(&s_numKeys); // get numKeys
#endif
		s_currentKeys = (bool*)malloc(s_numKeys * sizeof(bool));
		s_prevKeys = (bool*)malloc(s_numKeys * sizeof(bool));
		memset(s_currentKeys, 0, s_numKeys * sizeof(bool));
		memset(s_prevKeys, 0, s_numKeys * sizeof(bool));
		s_keyboardInitialized = true;
	}
}

// Update keyboard state (should be called each frame)
void leo_UpdateKeyboard(void)
{
	leo_InitKeyboard();

	// Copy current state to previous state
	memcpy(s_prevKeys, s_currentKeys, s_numKeys * sizeof(bool));

#ifdef TESTING
	if (!s_testMode)
	{
		// Get current SDL keyboard state (normal mode)
		const bool* sdlKeys = SDL_GetKeyboardState(NULL);
		memcpy(s_currentKeys, sdlKeys, s_numKeys * sizeof(bool));
	}
	// In test mode, don't overwrite the test state
#else
	// Get current SDL keyboard state
	const bool* sdlKeys = SDL_GetKeyboardState(NULL);
	memcpy(s_currentKeys, sdlKeys, s_numKeys * sizeof(bool));
#endif
}

#ifdef TESTING
// Test mode functions
void leo_EnableTestMode(void)
{
	s_testMode = true;
}

void leo_DisableTestMode(void)
{
	s_testMode = false;
}

void leo_SetTestKeyState(SDL_Scancode scancode, bool pressed)
{
	if (scancode >= 0 && scancode < s_numKeys)
	{
		s_currentKeys[scancode] = pressed;
	}
}
#endif

bool leo_IsKeyPressed(int key)
{
	leo_InitKeyboard();

	SDL_Scancode scancode;

	// Check if the key is already a scan code (0-511 range)
	if (key >= 0 && key < s_numKeys)
	{
		scancode = (SDL_Scancode)key;
	}
	else
	{
		// Convert key code to scan code if needed
		SDL_Keymod modstate;
		scancode = SDL_GetScancodeFromKey(key, &modstate);
		if (scancode == SDL_SCANCODE_UNKNOWN)
		{
			scancode = (SDL_Scancode)key; // Assume it's already a scan code
		}
	}

	// Ensure the scan code is within bounds before accessing the array
	if (scancode < 0 || scancode >= s_numKeys)
	{
		return false; // Out of bounds
	}

	// Key is pressed if it's down now but wasn't down before
	return s_currentKeys[scancode] && !s_prevKeys[scancode];
}

bool leo_IsKeyPressedRepeat(int key)
{
	leo_InitKeyboard();

	SDL_Scancode scancode;

	// Check if the key is already a scan code (0-511 range)
	if (key >= 0 && key < s_numKeys)
	{
		scancode = (SDL_Scancode)key;
	}
	else
	{
		// Convert key code to scan code if needed
		SDL_Keymod modstate;
		scancode = SDL_GetScancodeFromKey(key, &modstate);
		if (scancode == SDL_SCANCODE_UNKNOWN)
		{
			scancode = (SDL_Scancode)key; // Assume it's already a scan code
		}
	}

	// Ensure the scan code is within bounds before accessing the array
	if (scancode < 0 || scancode >= s_numKeys)
	{
		return false; // Out of bounds
	}

	// Key is "pressed again" if it's currently down and was down before
	return s_currentKeys[scancode] && s_prevKeys[scancode];
}

bool leo_IsKeyDown(int key)
{
	leo_InitKeyboard();

	SDL_Scancode scancode;

	// Check if the key is already a scan code (0-511 range)
	if (key >= 0 && key < s_numKeys)
	{
		scancode = (SDL_Scancode)key;
	}
	else
	{
		// Convert key code to scan code if needed
		SDL_Keymod modstate;
		scancode = SDL_GetScancodeFromKey(key, &modstate);
		if (scancode == SDL_SCANCODE_UNKNOWN)
		{
			scancode = (SDL_Scancode)key; // Assume it's already a scan code
		}
	}

	// Ensure the scan code is within bounds before accessing the array
	if (scancode < 0 || scancode >= s_numKeys)
	{
		return false; // Out of bounds
	}

	// Key is down if it's currently pressed
	return s_currentKeys[scancode];
}

bool leo_IsKeyReleased(int key)
{
	leo_InitKeyboard();

	SDL_Scancode scancode;

	// Check if the key is already a scan code (0-511 range)
	if (key >= 0 && key < s_numKeys)
	{
		scancode = (SDL_Scancode)key;
	}
	else
	{
		// Convert key code to scan code if needed
		SDL_Keymod modstate;
		scancode = SDL_GetScancodeFromKey(key, &modstate);
		if (scancode == SDL_SCANCODE_UNKNOWN)
		{
			scancode = (SDL_Scancode)key; // Assume it's already a scan code
		}
	}

	// Ensure the scan code is within bounds before accessing the array
	if (scancode < 0 || scancode >= s_numKeys)
	{
		return false; // Out of bounds
	}

	// Key is released if it was down before but isn't down now
	return !s_currentKeys[scancode] && s_prevKeys[scancode];
}

bool leo_IsKeyUp(int key)
{
	leo_InitKeyboard();

	SDL_Scancode scancode;

	// Check if the key is already a scan code (0-511 range)
	if (key >= 0 && key < s_numKeys)
	{
		scancode = (SDL_Scancode)key;
	}
	else
	{
		// Convert key code to scan code if needed
		SDL_Keymod modstate;
		scancode = SDL_GetScancodeFromKey(key, &modstate);
		if (scancode == SDL_SCANCODE_UNKNOWN)
		{
			scancode = (SDL_Scancode)key; // Assume it's already a scan code
		}
	}

	// Ensure the scan code is within bounds before accessing the array
	if (scancode < 0 || scancode >= s_numKeys)
	{
		return true; // Out of bounds keys are considered "up"
	}

	// Key is up if it's not currently pressed
	return !s_currentKeys[scancode];
}

int leo_GetKeyPressed(void)
{
	leo_InitKeyboard();

	// Find the first key that was just pressed this frame
	for (int i = 0; i < s_numKeys; i++)
	{
		if (s_currentKeys[i] && !s_prevKeys[i])
		{
			// Convert scan code to key code
			return SDL_GetKeyFromScancode((SDL_Scancode)i, SDL_KMOD_NONE, false);
		}
	}

	return 0; // 0 = no key queued
}

int leo_GetCharPressed(void)
{
	// Get the first key that was just pressed
	int keyPressed = leo_GetKeyPressed();

	// Convert to character if it's a printable ASCII character
	if (keyPressed >= 32 && keyPressed <= 126) // Printable ASCII range
	{
		return keyPressed;
	}

	return 0; // 0 = no char queued
}

void leo_SetExitKey(int key)
{
	s_exitKey = key;
}

// Helper function to check if exit key is pressed
bool leo_IsExitKeyPressed(void)
{
	return leo_IsKeyPressed(s_exitKey);
}

// Cleanup function (should be called when shutting down)
void leo_CleanupKeyboard(void)
{
	if (s_currentKeys)
	{
		free(s_currentKeys);
		s_currentKeys = NULL;
	}
	if (s_prevKeys)
	{
		free(s_prevKeys);
		s_prevKeys = NULL;
	}
	s_keyboardInitialized = false;
}
