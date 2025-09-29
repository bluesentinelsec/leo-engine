// =============================================
// leo/transitions.h
// =============================================
#pragma once

#include "leo/color.h"
#include "leo/export.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        LEO_TRANSITION_FADE_IN,   // Fade from opaque to transparent
        LEO_TRANSITION_FADE_OUT,  // Fade from transparent to opaque
        LEO_TRANSITION_CIRCLE_IN, // Circle shrinks to reveal scene
        LEO_TRANSITION_CIRCLE_OUT // Circle grows to cover scene
    } leo_TransitionType;

    // Simple global transition system
    LEO_API void leo_StartFadeIn(float duration, leo_Color color);
    LEO_API void leo_StartFadeOut(float duration, leo_Color color, void (*on_complete)(void));
    LEO_API void leo_StartTransition(leo_TransitionType type, float duration, leo_Color color,
                                     void (*on_complete)(void));
    LEO_API void leo_UpdateTransitions(float dt);
    LEO_API void leo_RenderTransitions(void);
    LEO_API bool leo_IsTransitioning(void);

#ifdef __cplusplus
}
#endif
