// src/gamepad.c
#include "leo/gamepad.h" // public API + enums
#include "leo/engine.h"  // for leo_Vector2
#include "leo/error.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_stdinc.h>
#include <math.h>
#include <stdbool.h>

#ifndef LEO_MAX_GAMEPADS
#define LEO_MAX_GAMEPADS 8
#endif

// ==============================
// Internal state per gamepad
// ==============================
typedef struct
{
    // SDL (production) path:
    SDL_Gamepad *pad;   // NULL if not connected
    SDL_JoystickID jid; // stable id to match remove events
    const char *name;   // SDL-owned, do not free

#ifdef LEO_TEST_BACKEND
    // Test backend (no SDL virtual devices needed)
    bool test_pad; // true if simulated
    char test_name[64];
    float test_axes[LEO_GAMEPAD_AXIS_COUNT]; // normalized [-1,+1], triggers [0,+1]
    uint32_t test_buttons;                   // current mask
#endif

    // Button history (for edge detection)
    uint32_t curr_buttons;
    uint32_t prev_buttons;
    uint32_t pressed_edges;  // curr & ~prev
    uint32_t released_edges; // ~curr & prev

    // Stick -> virtual buttons with hysteresis
    // stick index: 0=left, 1=right; dir index: 0=left,1=right,2=up,3=down
    uint8_t stick_down[2][4];
    uint8_t stick_prev[2][4];
    uint8_t stick_pressed[2][4];
    uint8_t stick_released[2][4];
} LeoGamepadSlot;

static LeoGamepadSlot gpads[LEO_MAX_GAMEPADS];

// Global tuning
static float s_deadzone = 0.15f;          // applied to all axes
static float s_press_threshold = 0.50f;   // stick-as-button press
static float s_release_threshold = 0.40f; // stick-as-button release

// Per-frame “any button pressed” (lowest id)
static int s_last_button_pressed = -1;

// ==============================
// Helpers: mapping & math
// ==============================
static inline uint32_t s_btn_bit(int button)
{
    if (button < 0 || button >= LEO_GAMEPAD_BUTTON_COUNT)
        return 0;
    return (1u << button);
}

static SDL_GamepadButton s_map_button(int b)
{
    switch ((leo_GamepadButton)b)
    {
    case LEO_GAMEPAD_BUTTON_FACE_DOWN:
        return SDL_GAMEPAD_BUTTON_SOUTH; // A/Cross
    case LEO_GAMEPAD_BUTTON_FACE_RIGHT:
        return SDL_GAMEPAD_BUTTON_EAST; // B/Circle
    case LEO_GAMEPAD_BUTTON_FACE_LEFT:
        return SDL_GAMEPAD_BUTTON_WEST; // X/Square
    case LEO_GAMEPAD_BUTTON_FACE_UP:
        return SDL_GAMEPAD_BUTTON_NORTH; // Y/Triangle
    case LEO_GAMEPAD_BUTTON_LEFT_BUMPER:
        return SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
    case LEO_GAMEPAD_BUTTON_RIGHT_BUMPER:
        return SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
    case LEO_GAMEPAD_BUTTON_BACK:
        return SDL_GAMEPAD_BUTTON_BACK;
    case LEO_GAMEPAD_BUTTON_GUIDE:
        return SDL_GAMEPAD_BUTTON_GUIDE;
    case LEO_GAMEPAD_BUTTON_START:
        return SDL_GAMEPAD_BUTTON_START;
    case LEO_GAMEPAD_BUTTON_LEFT_STICK:
        return SDL_GAMEPAD_BUTTON_LEFT_STICK;
    case LEO_GAMEPAD_BUTTON_RIGHT_STICK:
        return SDL_GAMEPAD_BUTTON_RIGHT_STICK;
    case LEO_GAMEPAD_BUTTON_DPAD_UP:
        return SDL_GAMEPAD_BUTTON_DPAD_UP;
    case LEO_GAMEPAD_BUTTON_DPAD_RIGHT:
        return SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
    case LEO_GAMEPAD_BUTTON_DPAD_DOWN:
        return SDL_GAMEPAD_BUTTON_DPAD_DOWN;
    case LEO_GAMEPAD_BUTTON_DPAD_LEFT:
        return SDL_GAMEPAD_BUTTON_DPAD_LEFT;
    default:
        return (SDL_GamepadButton)-1;
    }
}

static SDL_GamepadAxis s_map_axis(int a)
{
    switch ((leo_GamepadAxis)a)
    {
    case LEO_GAMEPAD_AXIS_LEFT_X:
        return SDL_GAMEPAD_AXIS_LEFTX;
    case LEO_GAMEPAD_AXIS_LEFT_Y:
        return SDL_GAMEPAD_AXIS_LEFTY;
    case LEO_GAMEPAD_AXIS_RIGHT_X:
        return SDL_GAMEPAD_AXIS_RIGHTX;
    case LEO_GAMEPAD_AXIS_RIGHT_Y:
        return SDL_GAMEPAD_AXIS_RIGHTY;
    case LEO_GAMEPAD_AXIS_LEFT_TRIGGER:
        return SDL_GAMEPAD_AXIS_LEFT_TRIGGER;
    case LEO_GAMEPAD_AXIS_RIGHT_TRIGGER:
        return SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
    default:
        return (SDL_GamepadAxis)-1;
    }
}

static inline float s_norm_axis(Sint16 v)
{
    // SDL axis is typically -32768..32767; normalize to [-1,+1]
    if (v >= 0)
        return (float)v / 32767.0f;
    return (float)v / 32768.0f;
}

static inline float s_apply_deadzone(float x)
{
    const float ax = fabsf(x);
    if (ax <= s_deadzone)
        return 0.0f;
    return x; // (no rescale; keep simple)
}

// ==============================
// Helpers: slot management
// ==============================
static inline void s_clear_slot(LeoGamepadSlot *s)
{
    SDL_memset(s, 0, sizeof(*s));
    s->jid = 0;
}

static int s_find_free_slot(void)
{
    for (int i = 0; i < LEO_MAX_GAMEPADS; ++i)
    {
#ifndef LEO_TEST_BACKEND
        if (!gpads[i].pad)
            return i;
#else
        if (!gpads[i].pad && !gpads[i].test_pad)
            return i;
#endif
    }
    return -1;
}

static int s_find_slot_by_jid(SDL_JoystickID jid)
{
    for (int i = 0; i < LEO_MAX_GAMEPADS; ++i)
    {
        if (gpads[i].pad && gpads[i].jid == jid)
            return i;
    }
    return -1;
}

#ifndef LEO_TEST_BACKEND
static void s_open_pad_for_slot(int slot, SDL_JoystickID jid)
{
    if (slot < 0 || slot >= LEO_MAX_GAMEPADS)
        return;
    if (gpads[slot].pad)
        return;

    SDL_Gamepad *pad = SDL_OpenGamepad(jid);
    if (!pad)
        return;

    gpads[slot].pad = pad;
    gpads[slot].jid = jid;
    gpads[slot].name = SDL_GetGamepadName(pad);

    gpads[slot].curr_buttons = gpads[slot].prev_buttons = 0;
    SDL_memset(gpads[slot].stick_down, 0, sizeof(gpads[slot].stick_down));
    SDL_memset(gpads[slot].stick_prev, 0, sizeof(gpads[slot].stick_prev));
    SDL_memset(gpads[slot].stick_pressed, 0, sizeof(gpads[slot].stick_pressed));
    SDL_memset(gpads[slot].stick_released, 0, sizeof(gpads[slot].stick_released));
}
#endif

static void s_close_slot(int slot)
{
    if (slot < 0 || slot >= LEO_MAX_GAMEPADS)
        return;
#ifndef LEO_TEST_BACKEND
    if (gpads[slot].pad)
    {
        SDL_CloseGamepad(gpads[slot].pad);
    }
#endif
    s_clear_slot(&gpads[slot]);
}

// ==============================
// Unified axis read (prod/test)
// ==============================
static float s_read_axis(int index, int axis /* leo enum */)
{
    LeoGamepadSlot *s = &gpads[index];
#ifndef LEO_TEST_BACKEND
    if (!s->pad)
        return 0.0f;
    SDL_GamepadAxis a = s_map_axis(axis);
    if (a < 0)
        return 0.0f;
    float v = s_apply_deadzone(s_norm_axis(SDL_GetGamepadAxis(s->pad, a)));
    // public contract: triggers clamped to [0,+1]
    if (axis == LEO_GAMEPAD_AXIS_LEFT_TRIGGER || axis == LEO_GAMEPAD_AXIS_RIGHT_TRIGGER)
    {
        if (v < 0.0f)
            v = 0.0f;
    }
    return v;
#else
    if (!s->test_pad)
        return 0.0f;
    float v = s->test_axes[axis];
    // public contract: triggers clamped to [0,+1]
    if (axis == LEO_GAMEPAD_AXIS_LEFT_TRIGGER || axis == LEO_GAMEPAD_AXIS_RIGHT_TRIGGER)
    {
        if (v < 0.0f)
            v = 0.0f;
    }
    return s_apply_deadzone(v);
#endif
}

// ==============================
// Poll helpers
// ==============================
static void s_update_buttons_for_slot(int i)
{
    LeoGamepadSlot *s = &gpads[i];

#ifndef LEO_TEST_BACKEND
    if (!s->pad)
        return;
#else
    if (!s->test_pad)
        return;
#endif

    uint32_t mask = 0;

#ifndef LEO_TEST_BACKEND
    for (int b = 0; b < LEO_GAMEPAD_BUTTON_COUNT; ++b)
    {
        SDL_GamepadButton sb = s_map_button(b);
        if (sb < 0)
            continue;
        if (SDL_GetGamepadButton(s->pad, sb))
            mask |= s_btn_bit(b);
    }
#else
    mask = s->test_buttons;
#endif

    s->pressed_edges = (mask & ~s->curr_buttons);
    s->released_edges = (~mask & s->curr_buttons);

    if (s_last_button_pressed < 0 && s->pressed_edges)
    {
        for (int b = 0; b < LEO_GAMEPAD_BUTTON_COUNT; ++b)
        {
            if (s->pressed_edges & s_btn_bit(b))
            {
                s_last_button_pressed = b;
                break;
            }
        }
    }

    s->prev_buttons = s->curr_buttons;
    s->curr_buttons = mask;
}

static void s_eval_stick_hysteresis_for_slot(int i)
{
    LeoGamepadSlot *s = &gpads[i];
#ifndef LEO_TEST_BACKEND
    if (!s->pad)
        return;
#else
    if (!s->test_pad)
        return;
#endif

    float lx = s_read_axis(i, LEO_GAMEPAD_AXIS_LEFT_X);
    float ly = s_read_axis(i, LEO_GAMEPAD_AXIS_LEFT_Y);
    float rx = s_read_axis(i, LEO_GAMEPAD_AXIS_RIGHT_X);
    float ry = s_read_axis(i, LEO_GAMEPAD_AXIS_RIGHT_Y);

    float stick_x[2] = {lx, rx};
    float stick_y[2] = {ly, ry};

    for (int stick = 0; stick < 2; ++stick)
    {
        float vx = stick_x[stick], vy = stick_y[stick];
        float mags[4] = {
            fabsf(vx < 0 ? vx : 0.0f), // LEFT  uses -vx
            fabsf(vx > 0 ? vx : 0.0f), // RIGHT uses +vx
            fabsf(vy < 0 ? vy : 0.0f), // UP    uses -vy
            fabsf(vy > 0 ? vy : 0.0f)  // DOWN  uses +vy
        };

        for (int dir = 0; dir < 4; ++dir)
        {
            const uint8_t was_down = s->stick_down[stick][dir];
            uint8_t now_down = was_down;

            if (!was_down)
            {
                if (mags[dir] >= s_press_threshold)
                    now_down = 1;
            }
            else
            {
                if (mags[dir] <= s_release_threshold)
                    now_down = 0;
            }

            s->stick_pressed[stick][dir] = (!was_down && now_down) ? 1 : 0;
            s->stick_released[stick][dir] = (was_down && !now_down) ? 1 : 0;
            s->stick_prev[stick][dir] = was_down;
            s->stick_down[stick][dir] = now_down;
        }
    }
}

// ==============================
// Public API
// ==============================
void leo_InitGamepads(void)
{
    SDL_memset(gpads, 0, sizeof(gpads));
    s_last_button_pressed = -1;

#ifndef LEO_TEST_BACKEND
    // Ensure subsystems for real pads
    SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS);

    int num = 0;
    SDL_JoystickID *list = SDL_GetGamepads(&num);
    if (list)
    {
        for (int i = 0; i < num && i < LEO_MAX_GAMEPADS; ++i)
        {
            int slot = s_find_free_slot();
            if (slot >= 0)
                s_open_pad_for_slot(slot, list[i]);
        }
        SDL_free(list);
    }
#endif
}

void leo_ShutdownGamepads(void)
{
    for (int i = 0; i < LEO_MAX_GAMEPADS; ++i)
        s_close_slot(i);
}

void leo_HandleGamepadEvent(void *sdl_evt)
{
#ifndef LEO_TEST_BACKEND
    if (!sdl_evt)
        return;
    SDL_Event *e = (SDL_Event *)sdl_evt;

    switch (e->type)
    {
    case SDL_EVENT_GAMEPAD_ADDED: {
        SDL_JoystickID jid = e->gdevice.which;
        int slot = s_find_free_slot();
        if (slot >= 0)
            s_open_pad_for_slot(slot, jid);
    }
    break;
    case SDL_EVENT_GAMEPAD_REMOVED: {
        SDL_JoystickID jid = e->gdevice.which;
        int slot = s_find_slot_by_jid(jid);
        if (slot >= 0)
            s_close_slot(slot);
    }
    break;
    default:
        break;
    }
#else
    (void)sdl_evt; // no-op in test backend
#endif
}

void leo_UpdateGamepads(void)
{
    s_last_button_pressed = -1;
    for (int i = 0; i < LEO_MAX_GAMEPADS; ++i)
    {
#ifndef LEO_TEST_BACKEND
        if (!gpads[i].pad)
            continue;
#else
        if (!gpads[i].test_pad)
            continue;
#endif
        s_update_buttons_for_slot(i);
        s_eval_stick_hysteresis_for_slot(i);
    }
}

bool leo_IsGamepadAvailable(int gamepad)
{
    if (gamepad < 0 || gamepad >= LEO_MAX_GAMEPADS)
        return false;
#ifndef LEO_TEST_BACKEND
    return gpads[gamepad].pad != NULL;
#else
    return gpads[gamepad].test_pad;
#endif
}

const char *leo_GetGamepadName(int gamepad)
{
    if (!leo_IsGamepadAvailable(gamepad))
        return NULL;
#ifndef LEO_TEST_BACKEND
    return gpads[gamepad].name;
#else
    return gpads[gamepad].test_name[0] ? gpads[gamepad].test_name : "Leo Test Pad";
#endif
}

bool leo_IsGamepadButtonPressed(int gamepad, int button)
{
    if (!leo_IsGamepadAvailable(gamepad))
        return false;
    if (button < 0 || button >= LEO_GAMEPAD_BUTTON_COUNT)
        return false;
    return (gpads[gamepad].pressed_edges & s_btn_bit(button)) != 0;
}

bool leo_IsGamepadButtonDown(int gamepad, int button)
{
    if (!leo_IsGamepadAvailable(gamepad))
        return false;
    if (button < 0 || button >= LEO_GAMEPAD_BUTTON_COUNT)
        return false;
    return (gpads[gamepad].curr_buttons & s_btn_bit(button)) != 0;
}

bool leo_IsGamepadButtonReleased(int gamepad, int button)
{
    if (!leo_IsGamepadAvailable(gamepad))
        return false;
    if (button < 0 || button >= LEO_GAMEPAD_BUTTON_COUNT)
        return false;
    return (gpads[gamepad].released_edges & s_btn_bit(button)) != 0;
}

bool leo_IsGamepadButtonUp(int gamepad, int button)
{
    if (!leo_IsGamepadAvailable(gamepad))
        return false;
    if (button < 0 || button >= LEO_GAMEPAD_BUTTON_COUNT)
        return false;
    return (gpads[gamepad].curr_buttons & s_btn_bit(button)) == 0;
}

int leo_GetGamepadButtonPressed(void)
{
    return s_last_button_pressed; // -1 if none this frame
}

int leo_GetGamepadAxisCount(int gamepad)
{
    return leo_IsGamepadAvailable(gamepad) ? (int)LEO_GAMEPAD_AXIS_COUNT : 0;
}

float leo_GetGamepadAxisMovement(int gamepad, int axis)
{
    if (!leo_IsGamepadAvailable(gamepad))
        return 0.0f;
    if (axis < 0 || axis >= (int)LEO_GAMEPAD_AXIS_COUNT)
        return 0.0f;
    return s_read_axis(gamepad, axis);
}

// ---- Stick virtual buttons (public API) -------------------------------------

static inline int s_check_stick_args(int gamepad, int stick, int dir)
{
    if (gamepad < 0 || gamepad >= LEO_MAX_GAMEPADS)
        return 0;
#ifndef LEO_TEST_BACKEND
    if (!gpads[gamepad].pad)
        return 0;
#else
    if (!gpads[gamepad].test_pad)
        return 0;
#endif
    if (stick != LEO_GAMEPAD_STICK_LEFT && stick != LEO_GAMEPAD_STICK_RIGHT)
        return 0;
    if (dir < LEO_GAMEPAD_DIR_LEFT || dir > LEO_GAMEPAD_DIR_DOWN)
        return 0;
    return 1;
}

bool leo_IsGamepadStickPressed(int gamepad, int stick, int dir)
{
    if (!s_check_stick_args(gamepad, stick, dir))
        return false;
    return gpads[gamepad].stick_pressed[stick][dir] != 0;
}

bool leo_IsGamepadStickDown(int gamepad, int stick, int dir)
{
    if (!s_check_stick_args(gamepad, stick, dir))
        return false;
    return gpads[gamepad].stick_down[stick][dir] != 0;
}

bool leo_IsGamepadStickReleased(int gamepad, int stick, int dir)
{
    if (!s_check_stick_args(gamepad, stick, dir))
        return false;
    return gpads[gamepad].stick_released[stick][dir] != 0;
}

bool leo_IsGamepadStickUp(int gamepad, int stick, int dir)
{
    if (!s_check_stick_args(gamepad, stick, dir))
        return false;
    return gpads[gamepad].stick_down[stick][dir] == 0;
}

bool leo_SetGamepadVibration(int gamepad, float leftMotor, float rightMotor, float duration)
{
#ifndef LEO_TEST_BACKEND
    if (!leo_IsGamepadAvailable(gamepad))
        return false;

    if (leftMotor < 0.f)
        leftMotor = 0.f;
    if (leftMotor > 1.f)
        leftMotor = 1.f;
    if (rightMotor < 0.f)
        rightMotor = 0.f;
    if (rightMotor > 1.f)
        rightMotor = 1.f;
    if (duration < 0.f)
        duration = 0.f;

    Uint16 low = (Uint16)(leftMotor * 0xFFFFu);
    Uint16 high = (Uint16)(rightMotor * 0xFFFFu);
    Uint32 ms = (Uint32)(duration * 1000.0f + 0.5f);
    return SDL_RumbleGamepad(gpads[gamepad].pad, low, high, ms);
#else
    (void)gamepad;
    (void)leftMotor;
    (void)rightMotor;
    (void)duration;
    // Pretend success in tests; virtual pads often don't support rumble.
    return true;
#endif
}

void leo_SetGamepadAxisDeadzone(float deadzone)
{
    if (deadzone < 0.f)
        deadzone = 0.f;
    if (deadzone > 1.f)
        deadzone = 1.f;
    s_deadzone = deadzone;
}

void leo_SetGamepadStickThreshold(float press_threshold, float release_threshold)
{
    if (press_threshold < 0.f)
        press_threshold = 0.f;
    if (press_threshold > 1.f)
        press_threshold = 1.f;
    if (release_threshold < 0.f)
        release_threshold = 0.f;
    if (release_threshold > press_threshold)
        release_threshold = press_threshold;
    s_press_threshold = press_threshold;
    s_release_threshold = release_threshold;
}

leo_Vector2 leo_GetGamepadStick(int gamepad, int stick)
{
    leo_Vector2 v = {0.f, 0.f};
    if (!leo_IsGamepadAvailable(gamepad))
        return v;
    if (stick != LEO_GAMEPAD_STICK_LEFT && stick != LEO_GAMEPAD_STICK_RIGHT)
        return v;

    if (stick == LEO_GAMEPAD_STICK_LEFT)
    {
        v.x = s_read_axis(gamepad, LEO_GAMEPAD_AXIS_LEFT_X);
        v.y = s_read_axis(gamepad, LEO_GAMEPAD_AXIS_LEFT_Y);
    }
    else
    {
        v.x = s_read_axis(gamepad, LEO_GAMEPAD_AXIS_RIGHT_X);
        v.y = s_read_axis(gamepad, LEO_GAMEPAD_AXIS_RIGHT_Y);
    }
    return v;
}

// ============================================
// Test-only helpers (compiled with test runner)
// ============================================
#ifdef LEO_TEST_BACKEND
// Return attached index (0..N-1) or -1 on failure
int leo__test_attach_pad(const char *name)
{
    int slot = s_find_free_slot();
    if (slot < 0)
        return -1;
    s_clear_slot(&gpads[slot]);

    gpads[slot].test_pad = true;
    if (name && name[0])
    {
        strncpy(gpads[slot].test_name, name, sizeof(gpads[slot].test_name) - 1);
        gpads[slot].test_name[sizeof(gpads[slot].test_name) - 1] = '\0';
    }
    else
    {
        gpads[slot].test_name[0] = '\0';
    }
    // start neutral
    SDL_memset(gpads[slot].test_axes, 0, sizeof(gpads[slot].test_axes));
    gpads[slot].test_buttons = 0;
    return slot;
}

void leo__test_detach_pad(int index)
{
    if (index < 0 || index >= LEO_MAX_GAMEPADS)
        return;
    s_close_slot(index);
}

void leo__test_set_button(int index, int button, bool down)
{
    if (index < 0 || index >= LEO_MAX_GAMEPADS)
        return;
    LeoGamepadSlot *s = &gpads[index];
    if (!s->test_pad)
        return;
    if (button < 0 || button >= LEO_GAMEPAD_BUTTON_COUNT)
        return;

    uint32_t bit = s_btn_bit(button);
    if (down)
        s->test_buttons |= bit;
    else
        s->test_buttons &= ~bit;
}

void leo__test_set_axis(int index, int axis, float value)
{
    if (index < 0 || index >= LEO_MAX_GAMEPADS)
        return;
    LeoGamepadSlot *s = &gpads[index];
    if (!s->test_pad)
        return;
    if (axis < 0 || axis >= (int)LEO_GAMEPAD_AXIS_COUNT)
        return;

    if (value > 1.f)
        value = 1.f;
    if (value < -1.f)
        value = -1.f;
    s->test_axes[axis] = value;
}
#endif
