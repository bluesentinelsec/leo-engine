// mouse.c — raylib-style mouse API on top of SDL3 (integrates with engine’s logical coords)

#include "leo/mouse.h"
#include "leo/engine.h"
#include <SDL3/SDL.h>
#include <string.h> /* memset, memcpy */
#include <math.h>

#ifndef LEO_MAX_MOUSE_BUTTONS
#define LEO_MAX_MOUSE_BUTTONS 8
#endif

typedef struct {
    /* connection */
    int connected;

    /* per-frame button state (index by SDL button index 1..5; we keep a small array) */
    Uint8 curr[LEO_MAX_MOUSE_BUTTONS];
    Uint8 prev[LEO_MAX_MOUSE_BUTTONS];

    /* wheel deltas per frame (x,y) */
    float wheelX, wheelY;

    /* pos/delta in renderer coordinates (logical pixels when logical mode enabled) */
    float x, y;
    float prevX, prevY;

    /* user-configurable transform (raylib parity) */
    int   offsetX, offsetY;
    float scaleX, scaleY;

    /* accumulated motion since last Update (fallback if needed) */
    float accumDX, accumDY;
} MouseState;

static MouseState g_ms;

static int clamp_btn_index(int b) {
    if (b < 0) return 0;
    if (b >= LEO_MAX_MOUSE_BUTTONS) return LEO_MAX_MOUSE_BUTTONS - 1;
    return b;
}

void leo_InitMouse(void)
{
    memset(&g_ms, 0, sizeof(g_ms));
    g_ms.connected = SDL_HasMouse() ? 1 : 0;
    g_ms.scaleX = 1.0f;
    g_ms.scaleY = 1.0f;

    /* seed position */
    if (g_ms.connected) {
        float fx = 0.0f, fy = 0.0f;
        SDL_GetMouseState(&fx, &fy);
        g_ms.x = fx; g_ms.y = fy;
        g_ms.prevX = fx; g_ms.prevY = fy;
    }
}

void leo_ShutdownMouse(void)
{
    memset(&g_ms, 0, sizeof(g_ms));
}

/* engine should forward raw SDL_Event* here */
void leo_HandleMouseEvent(void* sdl_evt)
{
    const SDL_Event* e = (const SDL_Event*)sdl_evt;
    if (!e) return;

    switch (e->type) {
    case SDL_EVENT_MOUSE_ADDED:
        g_ms.connected = 1;
        break;
    case SDL_EVENT_MOUSE_REMOVED:
        g_ms.connected = 0;
        memset(g_ms.curr, 0, sizeof(g_ms.curr));
        memset(g_ms.prev, 0, sizeof(g_ms.prev));
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        /* SDL sends float precise deltas in 3.0; keep as-is */
        g_ms.wheelX += e->wheel.x;
        g_ms.wheelY += e->wheel.y;
        break;
    case SDL_EVENT_MOUSE_MOTION:
        /* SDL event motion.x/y are window coords; with SDL logical presentation they are logical */
        g_ms.x = e->motion.x;
        g_ms.y = e->motion.y;
        /* accumulate raw delta */
        g_ms.accumDX += e->motion.xrel;
        g_ms.accumDY += e->motion.yrel;
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (e->button.button >= 0 && e->button.button < LEO_MAX_MOUSE_BUTTONS)
            g_ms.curr[e->button.button] = 1;
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (e->button.button >= 0 && e->button.button < LEO_MAX_MOUSE_BUTTONS)
            g_ms.curr[e->button.button] = 0;
        break;
    default:
        break;
    }
}

/* call once per frame AFTER the SDL_PollEvent loop (engine does this) */
void leo_UpdateMouse(void)
{
    int i;

    /* carry previous */
    memcpy(g_ms.prev, g_ms.curr, sizeof(g_ms.curr));
    g_ms.prevX = g_ms.x; g_ms.prevY = g_ms.y;

    /* refresh connected + absolute position as a fallback (covers cases with no motion events) */
    g_ms.connected = SDL_HasMouse() ? 1 : 0;
    if (g_ms.connected) {
        float fx = 0.0f, fy = 0.0f;
        SDL_MouseButtonFlags flags = SDL_GetMouseState(&fx, &fy);
        g_ms.x = fx; g_ms.y = fy;

        /* reconstruct button bits (SDL3 uses *_MASK constants / SDL_BUTTON_MASK(i)) */
        for (i = 1; i <= 5 && i < LEO_MAX_MOUSE_BUTTONS; ++i) {
            SDL_MouseButtonFlags mask = 0;
            switch (i) {
            case 1: mask = SDL_BUTTON_LMASK;  break;
            case 2: mask = SDL_BUTTON_MMASK;  break;
            case 3: mask = SDL_BUTTON_RMASK;  break;
            case 4: mask = SDL_BUTTON_X1MASK; break;
            case 5: mask = SDL_BUTTON_X2MASK; break;
            default: mask = 0; break;
            }
            g_ms.curr[i] = (Uint8)((flags & mask) ? 1 : 0);
        }
    }

    /* reset per-frame wheel deltas & accum rel motion; they’re “this frame” values */
    g_ms.wheelX = 0.0f;
    g_ms.wheelY = 0.0f;
    g_ms.accumDX = 0.0f;
    g_ms.accumDY = 0.0f;
}

/* --------- Queries (apply offset/scale like raylib) --------- */

static void apply_offset_scale(float inX, float inY, float* outX, float* outY)
{
    /* (x - offset) / scale  — same convention as raylib */
    float sx = (float)g_ms.scaleX;
    float sy = (float)g_ms.scaleY;
    if (sx == 0.0f) sx = 1.0f;
    if (sy == 0.0f) sy = 1.0f;

    float rx = (inX - (float)g_ms.offsetX) / sx;
    float ry = (inY - (float)g_ms.offsetY) / sy;
    if (outX) *outX = rx;
    if (outY) *outY = ry;
}

bool leo_IsMouseButtonDown(int button)
{
    int idx = clamp_btn_index(button);
    if (!g_ms.connected) return false;
    return g_ms.curr[idx] ? true : false;
}

bool leo_IsMouseButtonUp(int button) { return !leo_IsMouseButtonDown(button); }

bool leo_IsMouseButtonPressed(int button)
{
    int idx = clamp_btn_index(button);
    if (!g_ms.connected) return false;
    return (g_ms.curr[idx] && !g_ms.prev[idx]) ? true : false;
}

bool leo_IsMouseButtonReleased(int button)
{
    int idx = clamp_btn_index(button);
    if (!g_ms.connected) return false;
    return (!g_ms.curr[idx] && g_ms.prev[idx]) ? true : false;
}

int leo_GetMouseX(void)
{
    float x = 0.0f, y = 0.0f;
    apply_offset_scale(g_ms.x, g_ms.y, &x, &y);
    (void)y;
    return (int)(x + 0.5f);
}

int leo_GetMouseY(void)
{
    float x = 0.0f, y = 0.0f;
    apply_offset_scale(g_ms.x, g_ms.y, &x, &y);
    (void)x;
    return (int)(y + 0.5f);
}

leo_Vector2 leo_GetMousePosition(void)
{
    float x = 0.0f, y = 0.0f;
    leo_Vector2 v;
    apply_offset_scale(g_ms.x, g_ms.y, &x, &y);
    v.x = x; v.y = y;
    return v;
}

leo_Vector2 leo_GetMouseDelta(void)
{
    /* compute from previous absolute positions, then apply scale (offset cancels) */
    float dx = g_ms.x - g_ms.prevX;
    float dy = g_ms.y - g_ms.prevY;
    float sx = (g_ms.scaleX == 0.0f) ? 1.0f : g_ms.scaleX;
    float sy = (g_ms.scaleY == 0.0f) ? 1.0f : g_ms.scaleY;
    leo_Vector2 v;
    v.x = dx / sx;
    v.y = dy / sy;
    return v;
}

void leo_SetMousePosition(int x, int y)
{
    SDL_Window* w = (SDL_Window*)leo_GetWindow();
    if (!w) return;

    /* inverse of apply_offset_scale: window/render coords = offset + pos*scale */
    float rx = (float)g_ms.offsetX + (float)x * ((g_ms.scaleX == 0.0f) ? 1.0f : g_ms.scaleX);
    float ry = (float)g_ms.offsetY + (float)y * ((g_ms.scaleY == 0.0f) ? 1.0f : g_ms.scaleY);
    SDL_WarpMouseInWindow(w, rx, ry);

    /* update cached position to avoid a 1-frame mismatch if no motion event arrives */
    g_ms.x = rx; g_ms.y = ry; g_ms.prevX = rx; g_ms.prevY = ry;
}

void leo_SetMouseOffset(int offsetX, int offsetY)
{
    g_ms.offsetX = offsetX;
    g_ms.offsetY = offsetY;
}

void leo_SetMouseScale(float scaleX, float scaleY)
{
    g_ms.scaleX = (scaleX == 0.0f) ? 1.0f : scaleX;
    g_ms.scaleY = (scaleY == 0.0f) ? 1.0f : scaleY;
}

float leo_GetMouseWheelMove(void)
{
    /* pick the dominant axis and keep its sign */
    float ax = (g_ms.wheelX >= 0.0f) ? g_ms.wheelX : -g_ms.wheelX;
    float ay = (g_ms.wheelY >= 0.0f) ? g_ms.wheelY : -g_ms.wheelY;
    if (ay >= ax) return g_ms.wheelY;
    return g_ms.wheelX;
}

leo_Vector2 leo_GetMouseWheelMoveV(void)
{
    leo_Vector2 v;
    v.x = g_ms.wheelX; v.y = g_ms.wheelY;
    return v;
}
