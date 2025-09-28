#pragma once

#include "leo/actor.h"
#include "leo/color.h"
#include "leo/export.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        LEO_TRANSITION_FADE_IN,  // Start opaque, fade to transparent
        LEO_TRANSITION_FADE_OUT, // Start transparent, fade to opaque
        LEO_TRANSITION_CIRCLE_IN,
        LEO_TRANSITION_CIRCLE_OUT
    } leo_TransitionType;

    typedef struct
    {
        leo_TransitionType type;
        float duration;
        leo_Color color;
        void (*on_complete)(void *user_data);
        void *user_data;
        bool manual_render;  // If true, transition won't auto-render (for custom positioning)
    } leo_TransitionDesc;

    LEO_API leo_Actor *leo_transition_start(leo_Actor *parent, const leo_TransitionDesc *desc);
    LEO_API void leo_transition_render_manual(leo_Actor *transition);

#ifdef __cplusplus
}
#endif
