// keyboard.c — stubs for keyboard input API
// Replace the bodies with real implementations as you wire up your input system.

#include <stdbool.h>

#include "leo/keyboard.h"

#include <SDL3/SDL.h>

bool leo_IsKeyPressed(int key)
{
	(void)key;
	return false;
}

bool leo_IsKeyPressedRepeat(int key)
{
	(void)key;
	return false;
}

bool leo_IsKeyDown(int key)
{
	(void)key;
	return false;
}

bool leo_IsKeyReleased(int key)
{
	(void)key;
	return false;
}

bool leo_IsKeyUp(int key)
{
	(void)key;
	return true; // Up by default when unimplemented
}

int leo_GetKeyPressed(void)
{
	return 0; // 0 = no key queued
}

int leo_GetCharPressed(void)
{
	return 0; // 0 = no char queued
}

void leo_SetExitKey(int key)
{
	(void)key;
	// Stub: no-op
}
