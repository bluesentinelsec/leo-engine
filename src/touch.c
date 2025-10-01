// ==========================================================
// File: src/touch.c
// Touch and gesture input implementation
// ==========================================================

#include "leo/touch.h"
#include <SDL3/SDL.h>

#define MAX_TOUCH_POINTS 10

typedef struct
{
    bool down;
    bool pressed;
    bool released;
    float x, y;
    SDL_FingerID id;
} TouchPoint;

typedef struct
{
    TouchPoint points[MAX_TOUCH_POINTS];
    int count;
    int enabled_gestures;
    int current_gesture;
    float hold_duration;
    leo_Vector2Touch drag_vector;
    float drag_angle;
    leo_Vector2Touch pinch_vector;
    float pinch_angle;
} TouchState;

static TouchState s_touch = {0};

// Touch detection
bool leo_IsTouchDown(int touch)
{
    if (touch < 0 || touch >= MAX_TOUCH_POINTS)
        return false;
    return s_touch.points[touch].down;
}

bool leo_IsTouchPressed(int touch)
{
    if (touch < 0 || touch >= MAX_TOUCH_POINTS)
        return false;
    return s_touch.points[touch].pressed;
}

bool leo_IsTouchReleased(int touch)
{
    if (touch < 0 || touch >= MAX_TOUCH_POINTS)
        return false;
    return s_touch.points[touch].released;
}

// Touch position
leo_Vector2Touch leo_GetTouchPosition(int touch)
{
    if (touch < 0 || touch >= MAX_TOUCH_POINTS)
        return (leo_Vector2Touch){0.0f, 0.0f};
    return (leo_Vector2Touch){s_touch.points[touch].x, s_touch.points[touch].y};
}

int leo_GetTouchX(int touch)
{
    if (touch < 0 || touch >= MAX_TOUCH_POINTS)
        return 0;
    return (int)s_touch.points[touch].x;
}

int leo_GetTouchY(int touch)
{
    if (touch < 0 || touch >= MAX_TOUCH_POINTS)
        return 0;
    return (int)s_touch.points[touch].y;
}

// Multi-touch
int leo_GetTouchPointCount(void)
{
    return s_touch.count;
}

int leo_GetTouchPointId(int index)
{
    if (index < 0 || index >= s_touch.count)
        return -1;
    return (int)s_touch.points[index].id;
}

// Gestures
bool leo_IsGestureDetected(int gesture)
{
    return (s_touch.current_gesture & gesture) != 0;
}

int leo_GetGestureDetected(void)
{
    return s_touch.current_gesture;
}

float leo_GetGestureHoldDuration(void)
{
    return s_touch.hold_duration;
}

leo_Vector2Touch leo_GetGestureDragVector(void)
{
    return s_touch.drag_vector;
}

float leo_GetGestureDragAngle(void)
{
    return s_touch.drag_angle;
}

leo_Vector2Touch leo_GetGesturePinchVector(void)
{
    return s_touch.pinch_vector;
}

float leo_GetGesturePinchAngle(void)
{
    return s_touch.pinch_angle;
}

// Configuration
void leo_SetGesturesEnabled(int flags)
{
    s_touch.enabled_gestures = flags;
}
