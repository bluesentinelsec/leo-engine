// keyboard.c — robust keyboard input implementation using SDL3

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "leo/keyboard.h"
#include <SDL3/SDL.h>

// ---------------------- Internal state ----------------------
static int s_numKeys = 0;
static bool *s_currentKeys = NULL;
static bool *s_prevKeys = NULL;

static int s_exitKey = SDL_SCANCODE_ESCAPE;
static bool s_keyboardInitialized = false;

#ifdef TESTING
static bool s_testMode = false; // If true, avoid overwriting s_currentKeys from SDL
#endif

// ---------------------- Initialization ----------------------
static void leo_InitKeyboard(void)
{
    if (s_keyboardInitialized)
        return;

#ifdef TESTING
    if (!s_testMode)
    {
        SDL_GetKeyboardState(&s_numKeys); // get count from SDL
    }
    else
    {
        if (s_numKeys == 0)
        {
            s_numKeys = 512; // reasonable default for tests
        }
    }
#else
    SDL_GetKeyboardState(&s_numKeys); // get count from SDL
#endif

    if (s_numKeys <= 0)
    {
        s_numKeys = 512; // safety fallback
    }

    s_currentKeys = (bool *)malloc((size_t)s_numKeys * sizeof(bool));
    s_prevKeys = (bool *)malloc((size_t)s_numKeys * sizeof(bool));

    if (!s_currentKeys || !s_prevKeys)
    {
        // Best-effort: avoid UB if allocation fails
        free(s_currentKeys);
        s_currentKeys = NULL;
        free(s_prevKeys);
        s_prevKeys = NULL;
        s_numKeys = 0;
        return;
    }

    memset(s_currentKeys, 0, (size_t)s_numKeys * sizeof(bool));
    memset(s_prevKeys, 0, (size_t)s_numKeys * sizeof(bool));
    s_keyboardInitialized = true;
}

// ---------------------- Frame update ------------------------
void leo_UpdateKeyboard(void)
{
    leo_InitKeyboard();
    if (!s_keyboardInitialized)
        return;

    // Roll previous
    memcpy(s_prevKeys, s_currentKeys, (size_t)s_numKeys * sizeof(bool));

#ifdef TESTING
    if (!s_testMode)
    {
        int len = 0;
        const bool *sdl = SDL_GetKeyboardState(&len);
        if (sdl && len > 0)
        {
            const int n = (len < s_numKeys) ? len : s_numKeys;
            for (int i = 0; i < n; ++i)
                s_currentKeys[i] = sdl[i];
        }
    }
#else
    int len = 0;
    const bool *sdl = SDL_GetKeyboardState(&len);
    if (sdl && len > 0)
    {
        const int n = (len < s_numKeys) ? len : s_numKeys;
        for (int i = 0; i < n; ++i)
            s_currentKeys[i] = sdl[i];
    }
#endif
}

// ---------------------- Query helpers -----------------------
static SDL_Scancode key_to_scancode(int key)
{
    if (key >= 0 && key < s_numKeys)
    {
        return (SDL_Scancode)key; // treat as scancode
    }
    else
    {
        SDL_Keymod modstate;
        SDL_Scancode sc = SDL_GetScancodeFromKey(key, &modstate);
        if (sc == SDL_SCANCODE_UNKNOWN)
            sc = (SDL_Scancode)key;
        return sc;
    }
}

bool leo_IsKeyPressed(int key)
{
    leo_InitKeyboard();
    if (!s_keyboardInitialized)
        return false;

    SDL_Scancode sc = key_to_scancode(key);
    if (sc < 0 || sc >= s_numKeys)
        return false;
    return (s_currentKeys[sc] && !s_prevKeys[sc]);
}

bool leo_IsKeyPressedRepeat(int key)
{
    leo_InitKeyboard();
    if (!s_keyboardInitialized)
        return false;

    SDL_Scancode sc = key_to_scancode(key);
    if (sc < 0 || sc >= s_numKeys)
        return false;
    return (s_currentKeys[sc] && s_prevKeys[sc]);
}

bool leo_IsKeyDown(int key)
{
    leo_InitKeyboard();
    if (!s_keyboardInitialized)
        return false;

    SDL_Scancode sc = key_to_scancode(key);
    if (sc < 0 || sc >= s_numKeys)
        return false;
    return s_currentKeys[sc];
}

bool leo_IsKeyReleased(int key)
{
    leo_InitKeyboard();
    if (!s_keyboardInitialized)
        return false;

    SDL_Scancode sc = key_to_scancode(key);
    if (sc < 0 || sc >= s_numKeys)
        return false;
    return (!s_currentKeys[sc] && s_prevKeys[sc]);
}

bool leo_IsKeyUp(int key)
{
    leo_InitKeyboard();
    if (!s_keyboardInitialized)
        return true;

    SDL_Scancode sc = key_to_scancode(key);
    if (sc < 0 || sc >= s_numKeys)
        return true;
    return !s_currentKeys[sc];
}

int leo_GetKeyPressed(void)
{
    leo_InitKeyboard();
    if (!s_keyboardInitialized)
        return 0;

    for (int i = 0; i < s_numKeys; ++i)
    {
        if (s_currentKeys[i] && !s_prevKeys[i])
        {
            return SDL_GetKeyFromScancode((SDL_Scancode)i, SDL_KMOD_NONE, false);
        }
    }
    return 0;
}

int leo_GetCharPressed(void)
{
    int key = leo_GetKeyPressed();
    if (key >= 32 && key <= 126)
        return key; // printable ASCII
    return 0;
}

void leo_SetExitKey(int key)
{
    s_exitKey = key;
}

bool leo_IsExitKeyPressed(void)
{
    return leo_IsKeyPressed(s_exitKey);
}

void leo_CleanupKeyboard(void)
{
    free(s_currentKeys);
    s_currentKeys = NULL;
    free(s_prevKeys);
    s_prevKeys = NULL;
    s_numKeys = 0;
    s_keyboardInitialized = false;
#ifdef TESTING
    // keep s_testMode as-is; tests may toggle it explicitly
#endif
}

// ---------------------- TESTING helpers ---------------------
#ifdef TESTING
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
    leo_InitKeyboard();
    if (!s_keyboardInitialized)
        return;
    if (scancode < 0 || scancode >= s_numKeys)
        return;
    s_currentKeys[scancode] = pressed;
}

// Accessors to avoid DLL data imports on Windows
int leo__test_num_keys(void)
{
    return s_numKeys;
}
const bool *leo__test_current(void)
{
    return s_currentKeys;
}
const bool *leo__test_prev(void)
{
    return s_prevKeys;
}
#endif
