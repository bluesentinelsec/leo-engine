#include "leo/transitions.h"
#include "leo/engine.h"
#include "leo/graphics.h"
#include <stdbool.h>
#include <stddef.h>
#include <math.h>

typedef struct
{
    bool active;
    leo_TransitionType type;
    float progress;
    float duration;
    leo_Color color;
    void (*on_complete)(void);
} GlobalTransition;

static GlobalTransition g_transition = {0};

void leo_StartFadeIn(float duration, leo_Color color)
{
    leo_StartTransition(LEO_TRANSITION_FADE_IN, duration, color, NULL);
}

void leo_StartFadeOut(float duration, leo_Color color, void (*on_complete)(void))
{
    leo_StartTransition(LEO_TRANSITION_FADE_OUT, duration, color, on_complete);
}

void leo_StartTransition(leo_TransitionType type, float duration, leo_Color color, void (*on_complete)(void))
{
    g_transition.active = true;
    g_transition.type = type;
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

    int screen_width = leo_GetScreenWidth();
    int screen_height = leo_GetScreenHeight();

    switch (g_transition.type)
    {
    case LEO_TRANSITION_FADE_IN: {
        // Fade-in: start opaque, fade to transparent
        float alpha = 1.0f - g_transition.progress;
        leo_Color fade_color = g_transition.color;
        fade_color.a = (unsigned char)(255 * alpha);
        leo_DrawRectangle(0, 0, screen_width, screen_height, fade_color);
        break;
    }
    case LEO_TRANSITION_FADE_OUT: {
        // Fade-out: start transparent, fade to opaque
        float alpha = g_transition.progress;
        leo_Color fade_color = g_transition.color;
        fade_color.a = (unsigned char)(255 * alpha);
        leo_DrawRectangle(0, 0, screen_width, screen_height, fade_color);
        break;
    }
    case LEO_TRANSITION_CIRCLE_IN: {
        // Circle-in: large circle shrinks to reveal scene
        float max_radius = sqrtf(screen_width * screen_width + screen_height * screen_height) / 2.0f;
        float radius = max_radius * (1.0f - g_transition.progress);
        leo_DrawCircle(screen_width / 2, screen_height / 2, radius, g_transition.color);
        break;
    }
    case LEO_TRANSITION_CIRCLE_OUT: {
        // Circle-out: small circle grows to cover scene
        float max_radius = sqrtf(screen_width * screen_width + screen_height * screen_height) / 2.0f;
        float radius = max_radius * g_transition.progress;
        leo_DrawCircle(screen_width / 2, screen_height / 2, radius, g_transition.color);
        break;
    }
    }
}

bool leo_IsTransitioning(void)
{
    return g_transition.active;
}
