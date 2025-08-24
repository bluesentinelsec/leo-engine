#pragma once

#include "leo/engine.h" /* leo_Vector2 */
#include "leo/export.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* -----------------------------------------------------------------------------
   Gamepad API (Raylib parity, SDL3-backed)
   - Multiple gamepads are identified by a small integer index [0..N)
   - “Pressed/Released” are per-frame edges; call leo_UpdateGamepads() once/frame
   - Axes return normalized floats in [-1, +1] (triggers usually [0, +1])
----------------------------------------------------------------------------- */

/* Maximum number of gamepads Leo tracks (implementation may clamp). */
#ifndef LEO_MAX_GAMEPADS
#define LEO_MAX_GAMEPADS 8
#endif

    /* Raylib-like buttons (map internally to SDL_GamepadButton). */
    typedef enum
    {
        LEO_GAMEPAD_BUTTON_UNKNOWN = -1,

        LEO_GAMEPAD_BUTTON_FACE_DOWN = 0, /* A / Cross */
        LEO_GAMEPAD_BUTTON_FACE_RIGHT,    /* B / Circle */
        LEO_GAMEPAD_BUTTON_FACE_LEFT,     /* X / Square */
        LEO_GAMEPAD_BUTTON_FACE_UP,       /* Y / Triangle */

        LEO_GAMEPAD_BUTTON_LEFT_BUMPER,
        LEO_GAMEPAD_BUTTON_RIGHT_BUMPER,

        LEO_GAMEPAD_BUTTON_BACK,  /* View/Select */
        LEO_GAMEPAD_BUTTON_GUIDE, /* Logo/Home */
        LEO_GAMEPAD_BUTTON_START, /* Menu/Start */

        LEO_GAMEPAD_BUTTON_LEFT_STICK,  /* L3 */
        LEO_GAMEPAD_BUTTON_RIGHT_STICK, /* R3 */

        LEO_GAMEPAD_BUTTON_DPAD_UP,
        LEO_GAMEPAD_BUTTON_DPAD_RIGHT,
        LEO_GAMEPAD_BUTTON_DPAD_DOWN,
        LEO_GAMEPAD_BUTTON_DPAD_LEFT,

        LEO_GAMEPAD_BUTTON_COUNT
    } leo_GamepadButton;

    /* Raylib-like axes (map internally to SDL_GamepadAxis). */
    typedef enum
    {
        LEO_GAMEPAD_AXIS_LEFT_X = 0,
        LEO_GAMEPAD_AXIS_LEFT_Y,
        LEO_GAMEPAD_AXIS_RIGHT_X,
        LEO_GAMEPAD_AXIS_RIGHT_Y,
        LEO_GAMEPAD_AXIS_LEFT_TRIGGER,  /* Usually 0..+1 */
        LEO_GAMEPAD_AXIS_RIGHT_TRIGGER, /* Usually 0..+1 */
        LEO_GAMEPAD_AXIS_COUNT
    } leo_GamepadAxis;

    /* Sticks and directions for virtual-button helpers. */
    typedef enum
    {
        LEO_GAMEPAD_STICK_LEFT = 0,
        LEO_GAMEPAD_STICK_RIGHT = 1
    } leo_GamepadStick;

    typedef enum
    {
        LEO_GAMEPAD_DIR_LEFT = 0,
        LEO_GAMEPAD_DIR_RIGHT = 1,
        LEO_GAMEPAD_DIR_UP = 2,
        LEO_GAMEPAD_DIR_DOWN = 3
    } leo_GamepadDir;

    /* Lifecycle — engine calls these; exposed for embedders/tests. */
    LEO_API void leo_InitGamepads(void);
    LEO_API void leo_UpdateGamepads(void);              /* call once per frame after polling events */
    LEO_API void leo_HandleGamepadEvent(void *sdl_evt); /* engine forwards SDL_Event* here */
    LEO_API void leo_ShutdownGamepads(void);

    /* Availability / info */
    LEO_API bool leo_IsGamepadAvailable(int gamepad);    /* index-based */
    LEO_API const char *leo_GetGamepadName(int gamepad); /* UTF-8 name or NULL */

    /* Buttons (edge + level) */
    LEO_API bool leo_IsGamepadButtonPressed(int gamepad, int button);  /* pressed this frame */
    LEO_API bool leo_IsGamepadButtonDown(int gamepad, int button);     /* currently down */
    LEO_API bool leo_IsGamepadButtonReleased(int gamepad, int button); /* released this frame */
    LEO_API bool leo_IsGamepadButtonUp(int gamepad, int button);       /* currently up */

    /* Last button pressed (any gamepad). Returns leo_GamepadButton or -1 if none this frame. */
    LEO_API int leo_GetGamepadButtonPressed(void);

    /* Axes */
    LEO_API int leo_GetGamepadAxisCount(int gamepad);                /* up to LEO_GAMEPAD_AXIS_COUNT */
    LEO_API float leo_GetGamepadAxisMovement(int gamepad, int axis); /* normalized [-1,+1] (triggers 0..+1) */

    /* Vibration (duration in seconds; left/right are [0..1]). Returns true if supported & started. */
    LEO_API bool leo_SetGamepadVibration(int gamepad, float leftMotor, float rightMotor, float duration);

    /* per-axis deadzone (applied in GetGamepadAxisMovement). */
    LEO_API void leo_SetGamepadAxisDeadzone(float deadzone); /* e.g., 0.15f typical */

    /* -------------------------------------------------------------------------- */
    /* Stick helpers: treat analog sticks as virtual buttons with hysteresis      */
    /* -------------------------------------------------------------------------- */

    /* Set thresholds for stick-as-button evaluation (with hysteresis).
       - press_threshold   : magnitude to consider a direction newly PRESSED (e.g., 0.50)
       - release_threshold : magnitude to consider it RELEASED (e.g., 0.40)
       Must satisfy 0 <= release_threshold <= press_threshold <= 1.0. */
    LEO_API void leo_SetGamepadStickThreshold(float press_threshold, float release_threshold);

    /* Get the current stick vector (raw, post-deadzone) for LEFT/RIGHT stick.
       Note: Y follows the usual controller convention (up often negative). */
    LEO_API leo_Vector2 leo_GetGamepadStick(int gamepad, int stick);

    /* Virtual-button queries on stick directions using the configured thresholds. */
    LEO_API bool leo_IsGamepadStickPressed(int gamepad, int stick, int dir);  /* edge: went past press */
    LEO_API bool leo_IsGamepadStickDown(int gamepad, int stick, int dir);     /* level: currently past release */
    LEO_API bool leo_IsGamepadStickReleased(int gamepad, int stick, int dir); /* edge: crossed back below release */
    LEO_API bool leo_IsGamepadStickUp(int gamepad, int stick, int dir);       /* level: currently below release */

/* -----------------------------------------------------------------------------
   Test backend hooks (compiled only when LEO_TEST_BACKEND is defined)
   These let tests attach a simulated pad and drive buttons/axes directly.
----------------------------------------------------------------------------- */
#ifdef LEO_TEST_BACKEND
    LEO_API int leo__test_attach_pad(const char *name); /* returns index (>=0) or -1 */
    LEO_API void leo__test_detach_pad(int index);
    LEO_API void leo__test_set_button(int index, int button, bool down);
    LEO_API void leo__test_set_axis(int index, int axis, float value); /* [-1..+1], triggers use [0..+1] */
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
