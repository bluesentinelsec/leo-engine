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

    // Simple global transition system
    LEO_API void leo_StartFadeIn(float duration, leo_Color color);
    LEO_API void leo_StartFadeOut(float duration, leo_Color color, void (*on_complete)(void));
    LEO_API void leo_UpdateTransitions(float dt);
    LEO_API void leo_RenderTransitions(void);
    LEO_API bool leo_IsTransitioning(void);

#ifdef __cplusplus
}
#endif
