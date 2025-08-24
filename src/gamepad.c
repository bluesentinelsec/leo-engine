// src/gamepad.c
#include "leo/engine.h"   // for leo_Vector2
#include "leo/gamepad.h"  // your header (the one you pasted)
#include "leo/error.h"    // optional if you want to set errors (not used here)
#include <SDL3/SDL.h>
#include <string.h>
#include <math.h>

#ifndef LEO_MAX_GAMEPADS
#define LEO_MAX_GAMEPADS 8
#endif

// ------------------------------------------------------------
// Internal state
// ------------------------------------------------------------
typedef struct {
    SDL_Gamepad* pad;                     // NULL if not connected
    SDL_JoystickID jid;                   // stable ID for matching remove events
    const char* name;                     // SDL-owned string (do not free)

    // Buttons
    uint32_t curr_buttons;                // bit i => leo button i is down this frame
    uint32_t prev_buttons;                // prior frame
    uint32_t pressed_edges;               // (curr & ~prev)
    uint32_t released_edges;              // (~curr & prev)

    // Sticks as virtual buttons (hysteresis)
    // stick index: 0=left, 1=right; dir index: 0=left,1=right,2=up,3=down
    uint8_t  stick_down[2][4];            // level this frame
    uint8_t  stick_prev[2][4];            // level last frame
    uint8_t  stick_pressed[2][4];         // edges this frame
    uint8_t  stick_released[2][4];        // edges this frame
} LeoGamepadSlot;

static LeoGamepadSlot gpads[LEO_MAX_GAMEPADS];

// Global tuning
static float s_deadzone = 0.15f;          // applied to all axes (before hysteresis)
static float s_press_threshold   = 0.50f; // stick-as-button press threshold
static float s_release_threshold = 0.40f; // stick-as-button release threshold

// Per-frame “any button pressed” (across all gamepads)
static int s_last_button_pressed = -1;

// ------------------------------------------------------------
// Helpers: enum mappings
// ------------------------------------------------------------
static SDL_GamepadButton s_map_button(int b) {
    switch ((leo_GamepadButton)b) {
        case LEO_GAMEPAD_BUTTON_FACE_DOWN:   return SDL_GAMEPAD_BUTTON_SOUTH; // A / Cross
        case LEO_GAMEPAD_BUTTON_FACE_RIGHT:  return SDL_GAMEPAD_BUTTON_EAST;  // B / Circle
        case LEO_GAMEPAD_BUTTON_FACE_LEFT:   return SDL_GAMEPAD_BUTTON_WEST;  // X / Square
        case LEO_GAMEPAD_BUTTON_FACE_UP:     return SDL_GAMEPAD_BUTTON_NORTH; // Y / Triangle
        case LEO_GAMEPAD_BUTTON_LEFT_BUMPER: return SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
        case LEO_GAMEPAD_BUTTON_RIGHT_BUMPER:return SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
        case LEO_GAMEPAD_BUTTON_BACK:        return SDL_GAMEPAD_BUTTON_BACK;   // View/Select
        case LEO_GAMEPAD_BUTTON_GUIDE:       return SDL_GAMEPAD_BUTTON_GUIDE;  // Logo/Home
        case LEO_GAMEPAD_BUTTON_START:       return SDL_GAMEPAD_BUTTON_START;  // Menu/Start
        case LEO_GAMEPAD_BUTTON_LEFT_STICK:  return SDL_GAMEPAD_BUTTON_LEFT_STICK;
        case LEO_GAMEPAD_BUTTON_RIGHT_STICK: return SDL_GAMEPAD_BUTTON_RIGHT_STICK;
        case LEO_GAMEPAD_BUTTON_DPAD_UP:     return SDL_GAMEPAD_BUTTON_DPAD_UP;
        case LEO_GAMEPAD_BUTTON_DPAD_RIGHT:  return SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
        case LEO_GAMEPAD_BUTTON_DPAD_DOWN:   return SDL_GAMEPAD_BUTTON_DPAD_DOWN;
        case LEO_GAMEPAD_BUTTON_DPAD_LEFT:   return SDL_GAMEPAD_BUTTON_DPAD_LEFT;
        default:                             return (SDL_GamepadButton)-1;
    }
}

static SDL_GamepadAxis s_map_axis(int a) {
    switch ((leo_GamepadAxis)a) {
        case LEO_GAMEPAD_AXIS_LEFT_X:       return SDL_GAMEPAD_AXIS_LEFTX;
        case LEO_GAMEPAD_AXIS_LEFT_Y:       return SDL_GAMEPAD_AXIS_LEFTY;
        case LEO_GAMEPAD_AXIS_RIGHT_X:      return SDL_GAMEPAD_AXIS_RIGHTX;
        case LEO_GAMEPAD_AXIS_RIGHT_Y:      return SDL_GAMEPAD_AXIS_RIGHTY;
        case LEO_GAMEPAD_AXIS_LEFT_TRIGGER: return SDL_GAMEPAD_AXIS_LEFT_TRIGGER;
        case LEO_GAMEPAD_AXIS_RIGHT_TRIGGER:return SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
        default:                             return (SDL_GamepadAxis)-1;
    }
}

// ------------------------------------------------------------
// Helpers: slot management
// ------------------------------------------------------------
static int s_find_free_slot(void) {
    for (int i = 0; i < LEO_MAX_GAMEPADS; ++i)
        if (!gpads[i].pad) return i;
    return -1;
}

static int s_find_slot_by_jid(SDL_JoystickID jid) {
    for (int i = 0; i < LEO_MAX_GAMEPADS; ++i)
        if (gpads[i].pad && gpads[i].jid == jid) return i;
    return -1;
}

static inline void s_clear_slot(LeoGamepadSlot* s) {
    memset(s, 0, sizeof(*s));
    s->jid = 0;
}

static void s_open_pad_for_slot(int slot, SDL_JoystickID jid) {
    if (slot < 0 || slot >= LEO_MAX_GAMEPADS) return;
    if (gpads[slot].pad) return; // occupied

    SDL_Gamepad* pad = SDL_OpenGamepad(jid);
    if (!pad) return;

    gpads[slot].pad  = pad;
    gpads[slot].jid  = jid;
    gpads[slot].name = SDL_GetGamepadName(pad);
    // Reset button/stick histories
    gpads[slot].curr_buttons = gpads[slot].prev_buttons = 0;
    memset(gpads[slot].stick_down, 0, sizeof(gpads[slot].stick_down));
    memset(gpads[slot].stick_prev, 0, sizeof(gpads[slot].stick_prev));
    memset(gpads[slot].stick_pressed, 0, sizeof(gpads[slot].stick_pressed));
    memset(gpads[slot].stick_released, 0, sizeof(gpads[slot].stick_released));
}

static void s_close_slot(int slot) {
    if (slot < 0 || slot >= LEO_MAX_GAMEPADS) return;
    if (gpads[slot].pad) {
        SDL_CloseGamepad(gpads[slot].pad);
    }
    s_clear_slot(&gpads[slot]);
}

// ------------------------------------------------------------
// Helpers: math
// ------------------------------------------------------------
static inline float s_norm_axis(Sint16 v) {
    // SDL axes are typically -32768..32767 (triggers often 0..32767)
    if (v >= 0) return (float)v / 32767.0f;
    return (float)v / 32768.0f; // negative side
}

static inline float s_apply_deadzone(float x) {
    float ax = fabsf(x);
    if (ax <= s_deadzone) return 0.0f;
    // optional rescale to 0..1 after deadzone; keeping it simple: just zero inside DZ
    return x;
}

static inline uint32_t s_btn_bit(int button) {
    if (button < 0 || button >= LEO_GAMEPAD_BUTTON_COUNT) return 0;
    return (1u << button);
}

// ------------------------------------------------------------
// Public API
// ------------------------------------------------------------
void leo_InitGamepads(void) {
    memset(gpads, 0, sizeof(gpads));
    s_last_button_pressed = -1;

    // Open any already-present gamepads
    int num = 0;
    SDL_JoystickID* list = SDL_GetGamepads(&num);
    if (list) {
        for (int i = 0; i < num && i < LEO_MAX_GAMEPADS; ++i) {
            int slot = s_find_free_slot();
            if (slot >= 0) s_open_pad_for_slot(slot, list[i]);
        }
        SDL_free(list);
    }
}

void leo_ShutdownGamepads(void) {
    for (int i = 0; i < LEO_MAX_GAMEPADS; ++i) {
        s_close_slot(i);
    }
}

void leo_HandleGamepadEvent(void* sdl_evt) {
    if (!sdl_evt) return;
    SDL_Event* e = (SDL_Event*)sdl_evt;

    switch (e->type) {
        case SDL_EVENT_GAMEPAD_ADDED: {
            SDL_JoystickID jid = e->gdevice.which;
            // Find or create a slot
            int existing = s_find_slot_by_jid(jid);
            if (existing < 0) {
                int slot = s_find_free_slot();
                if (slot >= 0) s_open_pad_for_slot(slot, jid);
            }
        } break;
        case SDL_EVENT_GAMEPAD_REMOVED: {
            SDL_JoystickID jid = e->gdevice.which;
            int slot = s_find_slot_by_jid(jid);
            if (slot >= 0) s_close_slot(slot);
        } break;

        // We do not depend on button/axis events to track state (we poll in Update),
        // but handling them wouldn’t hurt if you wanted immediate reaction in-event.
        default: break;
    }
}

static void s_update_buttons_for_slot(int i) {
    LeoGamepadSlot* s = &gpads[i];
    if (!s->pad) return;

    uint32_t mask = 0;
    for (int b = 0; b < LEO_GAMEPAD_BUTTON_COUNT; ++b) {
        SDL_GamepadButton sb = s_map_button(b);
        if (sb < 0) continue;
        if (SDL_GetGamepadButton(s->pad, sb)) {
            mask |= s_btn_bit(b);
        }
    }

    s->pressed_edges  = (mask & ~s->curr_buttons); // edges vs previous "curr" (updated just below)
    s->released_edges = (~mask & s->curr_buttons);

    // Track "any button pressed" for this frame
    if (s_last_button_pressed < 0 && s->pressed_edges) {
        // return the lowest-numbered pressed button
        for (int b = 0; b < LEO_GAMEPAD_BUTTON_COUNT; ++b) {
            if (s->pressed_edges & s_btn_bit(b)) { s_last_button_pressed = b; break; }
        }
    }

    s->prev_buttons = s->curr_buttons;
    s->curr_buttons = mask;
}

static void s_eval_stick_hysteresis_for_slot(int i) {
    LeoGamepadSlot* s = &gpads[i];
    if (!s->pad) return;

    // Read normalized, deadzoned axes
    float lx = s_apply_deadzone(s_norm_axis(SDL_GetGamepadAxis(s->pad, SDL_GAMEPAD_AXIS_LEFTX)));
    float ly = s_apply_deadzone(s_norm_axis(SDL_GetGamepadAxis(s->pad, SDL_GAMEPAD_AXIS_LEFTY)));
    float rx = s_apply_deadzone(s_norm_axis(SDL_GetGamepadAxis(s->pad, SDL_GAMEPAD_AXIS_RIGHTX)));
    float ry = s_apply_deadzone(s_norm_axis(SDL_GetGamepadAxis(s->pad, SDL_GAMEPAD_AXIS_RIGHTY)));

    // Stick vectors
    float stick_x[2] = { lx, rx };
    float stick_y[2] = { ly, ry };

    for (int stick = 0; stick < 2; ++stick) {
        // Compute desired level using hysteresis per direction
        // LEFT:  negative X, RIGHT: positive X, UP: negative Y, DOWN: positive Y
        float vx = stick_x[stick];
        float vy = stick_y[stick];

        float mags[4] = {
            fabsf(vx < 0 ? vx : 0.0f),  // LEFT  (use -vx)
            fabsf(vx > 0 ? vx : 0.0f),  // RIGHT (use +vx)
            fabsf(vy < 0 ? vy : 0.0f),  // UP    (use -vy)
            fabsf(vy > 0 ? vy : 0.0f)   // DOWN  (use +vy)
        };

        for (int dir = 0; dir < 4; ++dir) {
            const uint8_t was_down = s->stick_down[stick][dir];
            uint8_t now_down = was_down;

            if (!was_down) {
                // Not down -> press if we exceeded press threshold
                if (mags[dir] >= s_press_threshold) now_down = 1;
            } else {
                // Down -> remain down until we go below release threshold
                if (mags[dir] <= s_release_threshold) now_down = 0;
            }

            s->stick_pressed [stick][dir] = (!was_down && now_down) ? 1 : 0;
            s->stick_released[stick][dir] = ( was_down && !now_down) ? 1 : 0;
            s->stick_prev    [stick][dir] = was_down;
            s->stick_down    [stick][dir] = now_down;
        }
    }
}

void leo_UpdateGamepads(void) {
    // Reset “any button pressed this frame”
    s_last_button_pressed = -1;

    // Poll current state for every connected pad
    for (int i = 0; i < LEO_MAX_GAMEPADS; ++i) {
        if (!gpads[i].pad) continue;
        s_update_buttons_for_slot(i);
        s_eval_stick_hysteresis_for_slot(i);
    }
}

bool leo_IsGamepadAvailable(int gamepad) {
    return (gamepad >= 0 && gamepad < LEO_MAX_GAMEPADS && gpads[gamepad].pad != NULL);
}

const char* leo_GetGamepadName(int gamepad) {
    if (!leo_IsGamepadAvailable(gamepad)) return NULL;
    return gpads[gamepad].name;
}

bool leo_IsGamepadButtonPressed(int gamepad, int button) {
    if (!leo_IsGamepadAvailable(gamepad)) return false;
    if (button < 0 || button >= LEO_GAMEPAD_BUTTON_COUNT) return false;
    return (gpads[gamepad].pressed_edges & s_btn_bit(button)) != 0;
}

bool leo_IsGamepadButtonDown(int gamepad, int button) {
    if (!leo_IsGamepadAvailable(gamepad)) return false;
    if (button < 0 || button >= LEO_GAMEPAD_BUTTON_COUNT) return false;
    return (gpads[gamepad].curr_buttons & s_btn_bit(button)) != 0;
}

bool leo_IsGamepadButtonReleased(int gamepad, int button) {
    if (!leo_IsGamepadAvailable(gamepad)) return false;
    if (button < 0 || button >= LEO_GAMEPAD_BUTTON_COUNT) return false;
    return (gpads[gamepad].released_edges & s_btn_bit(button)) != 0;
}

bool leo_IsGamepadButtonUp(int gamepad, int button) {
    if (!leo_IsGamepadAvailable(gamepad)) return false;
    if (button < 0 || button >= LEO_GAMEPAD_BUTTON_COUNT) return false;
    return (gpads[gamepad].curr_buttons & s_btn_bit(button)) == 0;
}

int leo_GetGamepadButtonPressed(void) {
    return s_last_button_pressed;  // -1 if none this frame
}

int leo_GetGamepadAxisCount(int gamepad) {
    return leo_IsGamepadAvailable(gamepad) ? (int)LEO_GAMEPAD_AXIS_COUNT : 0;
}

float leo_GetGamepadAxisMovement(int gamepad, int axis) {
    if (!leo_IsGamepadAvailable(gamepad)) return 0.0f;
    SDL_GamepadAxis a = s_map_axis(axis);
    if (a < 0) return 0.0f;

    Sint16 raw = SDL_GetGamepadAxis(gpads[gamepad].pad, a);
    float v = s_apply_deadzone(s_norm_axis(raw));

    // Public contract says: triggers are usually [0,+1]
    if (axis == LEO_GAMEPAD_AXIS_LEFT_TRIGGER || axis == LEO_GAMEPAD_AXIS_RIGHT_TRIGGER) {
        if (v < 0.0f) v = 0.0f;
    }

    return v;
}

bool leo_SetGamepadVibration(int gamepad, float leftMotor, float rightMotor, float duration) {
    if (!leo_IsGamepadAvailable(gamepad)) return false;

    if (leftMotor  < 0.f) leftMotor  = 0.f; if (leftMotor  > 1.f) leftMotor  = 1.f;
    if (rightMotor < 0.f) rightMotor = 0.f; if (rightMotor > 1.f) rightMotor = 1.f;
    if (duration   < 0.f) duration   = 0.f;

    Uint16 low  = (Uint16)(leftMotor  * 0xFFFFu);
    Uint16 high = (Uint16)(rightMotor * 0xFFFFu);
    Uint32 ms   = (Uint32)(duration * 1000.0f + 0.5f);

    // SDL3: SDL_RumbleGamepad(SDL_Gamepad*, low, high, ms) -> bool
    return SDL_RumbleGamepad(gpads[gamepad].pad, low, high, ms) == true;
}

void leo_SetGamepadAxisDeadzone(float deadzone) {
    if (deadzone < 0.f) deadzone = 0.f;
    if (deadzone > 1.f) deadzone = 1.f;
    s_deadzone = deadzone;
}

void leo_SetGamepadStickThreshold(float press_threshold, float release_threshold) {
    if (press_threshold < 0.f) press_threshold = 0.f;
    if (press_threshold > 1.f) press_threshold = 1.f;
    if (release_threshold < 0.f) release_threshold = 0.f;
    if (release_threshold > press_threshold) release_threshold = press_threshold;
    s_press_threshold   = press_threshold;
    s_release_threshold = release_threshold;
}

leo_Vector2 leo_GetGamepadStick(int gamepad, int stick) {
    leo_Vector2 v = {0.0f, 0.0f};
    if (!leo_IsGamepadAvailable(gamepad)) return v;
    if (stick != LEO_GAMEPAD_STICK_LEFT && stick != LEO_GAMEPAD_STICK_RIGHT) return v;

    if (stick == LEO_GAMEPAD_STICK_LEFT) {
        v.x = s_apply_deadzone(s_norm_axis(SDL_GetGamepadAxis(gpads[gamepad].pad, SDL_GAMEPAD_AXIS_LEFTX)));
        v.y = s_apply_deadzone(s_norm_axis(SDL_GetGamepadAxis(gpads[gamepad].pad, SDL_GAMEPAD_AXIS_LEFTY)));
    } else {
        v.x = s_apply_deadzone(s_norm_axis(SDL_GetGamepadAxis(gpads[gamepad].pad, SDL_GAMEPAD_AXIS_RIGHTX)));
        v.y = s_apply_deadzone(s_norm_axis(SDL_GetGamepadAxis(gpads[gamepad].pad, SDL_GAMEPAD_AXIS_RIGHTY)));
    }
    return v;
}

static inline int s_check_stick_args(int gamepad, int stick, int dir) {
    if (!leo_IsGamepadAvailable(gamepad)) return 0;
    if (stick != LEO_GAMEPAD_STICK_LEFT && stick != LEO_GAMEPAD_STICK_RIGHT) return 0;
    if (dir < LEO_GAMEPAD_DIR_LEFT || dir > LEO_GAMEPAD_DIR_DOWN) return 0;
    return 1;
}

bool leo_IsGamepadStickPressed(int gamepad, int stick, int dir) {
    if (!s_check_stick_args(gamepad, stick, dir)) return false;
    return gpads[gamepad].stick_pressed[stick][dir] != 0;
}

bool leo_IsGamepadStickDown(int gamepad, int stick, int dir) {
    if (!s_check_stick_args(gamepad, stick, dir)) return false;
    return gpads[gamepad].stick_down[stick][dir] != 0;
}

bool leo_IsGamepadStickReleased(int gamepad, int stick, int dir) {
    if (!s_check_stick_args(gamepad, stick, dir)) return false;
    return gpads[gamepad].stick_released[stick][dir] != 0;
}

bool leo_IsGamepadStickUp(int gamepad, int stick, int dir) {
    if (!s_check_stick_args(gamepad, stick, dir)) return false;
    return gpads[gamepad].stick_down[stick][dir] == 0;
}
