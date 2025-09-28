#include "leo/transitions.h"
#include "leo/engine.h"
#include "leo/graphics.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct
{
    bool active;
    bool fade_in; // true = fade in, false = fade out
    float progress;
    float duration;
    leo_Color color;
    void (*on_complete)(void);
} GlobalTransition;

static GlobalTransition g_transition = {0};

void leo_StartFadeIn(float duration, leo_Color color)
{
    g_transition.active = true;
    g_transition.fade_in = true;
    g_transition.progress = 0.0f;
    g_transition.duration = duration;
    g_transition.color = color;
    g_transition.on_complete = NULL;
}

void leo_StartFadeOut(float duration, leo_Color color, void (*on_complete)(void))
{
    g_transition.active = true;
    g_transition.fade_in = false;
    g_transition.progress = 0.0f;
    g_transition.duration = duration;
    g_transition.color = color;
    g_transition.on_complete = on_complete;
}

void leo_UpdateTransitions(float dt)
{
    if (!g_transition.active)
        return;

    g_transition.progress += dt / g_transition.duration;
    if (g_transition.progress >= 1.0f)
    {
        g_transition.progress = 1.0f;
        g_transition.active = false;

        if (g_transition.on_complete)
        {
            g_transition.on_complete();
        }
    }
}

void leo_RenderTransitions(void)
{
    if (!g_transition.active)
        return;

    float alpha;
    if (g_transition.fade_in)
    {
        // Fade-in: start opaque, fade to transparent
        alpha = 1.0f - g_transition.progress;
    }
    else
    {
        // Fade-out: start transparent, fade to opaque
        alpha = g_transition.progress;
    }

    leo_Color fade_color = g_transition.color;
    fade_color.a = (unsigned char)(255 * alpha);
    leo_DrawRectangle(0, 0, leo_GetScreenWidth(), leo_GetScreenHeight(), fade_color);
}

bool leo_IsTransitioning(void)
{
    return g_transition.active;
}
