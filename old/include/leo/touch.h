#pragma once

#include "leo/export.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        float x, y;
    } leo_Vector2Touch;

    // Touch detection
    LEO_API bool leo_IsTouchDown(int touch);
    LEO_API bool leo_IsTouchPressed(int touch);
    LEO_API bool leo_IsTouchReleased(int touch);

    // Touch position
    LEO_API leo_Vector2Touch leo_GetTouchPosition(int touch);
    LEO_API int leo_GetTouchX(int touch);
    LEO_API int leo_GetTouchY(int touch);

    // Multi-touch
    LEO_API int leo_GetTouchPointCount(void);
    LEO_API int leo_GetTouchPointId(int index);

    // Gestures
    LEO_API bool leo_IsGestureDetected(int gesture);
    LEO_API int leo_GetGestureDetected(void);
    LEO_API float leo_GetGestureHoldDuration(void);
    LEO_API leo_Vector2Touch leo_GetGestureDragVector(void);
    LEO_API float leo_GetGestureDragAngle(void);
    LEO_API leo_Vector2Touch leo_GetGesturePinchVector(void);
    LEO_API float leo_GetGesturePinchAngle(void);

// Gesture types
#define LEO_GESTURE_NONE 0
#define LEO_GESTURE_TAP 1
#define LEO_GESTURE_DOUBLETAP 2
#define LEO_GESTURE_HOLD 4
#define LEO_GESTURE_DRAG 8
#define LEO_GESTURE_SWIPE_RIGHT 16
#define LEO_GESTURE_SWIPE_LEFT 32
#define LEO_GESTURE_SWIPE_UP 64
#define LEO_GESTURE_SWIPE_DOWN 128
#define LEO_GESTURE_PINCH_IN 256
#define LEO_GESTURE_PINCH_OUT 512

    // Configuration
    LEO_API void leo_SetGesturesEnabled(int flags);

#ifdef __cplusplus
}
#endif
