#include "leo/transitions.h"
#include "leo/engine.h"
#include "leo/graphics.h"
#include <math.h>
#include <stdlib.h>

typedef struct
{
    leo_TransitionType type;
    float duration;
    float elapsed;
    leo_Color color;
    void (*on_complete)(void *user_data);
    void *user_data;
    bool completed;
} TransitionData;

static bool transition_init(leo_Actor *self)
{
    return true;
}

static void transition_update(leo_Actor *self, float dt)
{
    TransitionData *data = (TransitionData *)leo_actor_userdata(self);

    data->elapsed += dt;

    if (data->elapsed >= data->duration && !data->completed)
    {
        data->completed = true;
        if (data->on_complete)
        {
            data->on_complete(data->user_data);
        }
        leo_actor_kill(self);
    }
}

static void transition_render(leo_Actor *self)
{
    TransitionData *data = (TransitionData *)leo_actor_userdata(self);
    float progress = data->elapsed / data->duration;
    if (progress > 1.0f)
        progress = 1.0f;

    // Transitions need to render in screen space, not world space
    // Check if we're inside a camera transform and exit it temporarily
    bool was_camera_active = leo_IsCameraActive();
    if (was_camera_active) {
        leo_EndMode2D();
    }

    // Get actual screen dimensions
    int screen_width = leo_GetScreenWidth();
    int screen_height = leo_GetScreenHeight();

    switch (data->type)
    {
    case LEO_TRANSITION_FADE: {
        leo_Color fade_color = data->color;
        fade_color.a = (unsigned char)(255 * progress);
        leo_DrawRectangle(0, 0, screen_width, screen_height, fade_color);
        break;
    }
    case LEO_TRANSITION_CIRCLE_IN: {
        float max_radius = sqrtf(screen_width * screen_width + screen_height * screen_height) / 2.0f;
        float radius = max_radius * (1.0f - progress);
        leo_DrawCircle(screen_width / 2, screen_height / 2, radius, data->color);
        break;
    }
    case LEO_TRANSITION_CIRCLE_OUT: {
        float max_radius = sqrtf(screen_width * screen_width + screen_height * screen_height) / 2.0f;
        float radius = max_radius * progress;
        leo_DrawCircle(screen_width / 2, screen_height / 2, radius, data->color);
        break;
    }
    }

    // Restore camera transform if it was active
    if (was_camera_active) {
        leo_Camera2D camera = leo_GetCurrentCamera2D();
        leo_BeginMode2D(camera);
    }
}

static void transition_exit(leo_Actor *self)
{
    TransitionData *data = (TransitionData *)leo_actor_userdata(self);
    free(data);
}

static const leo_ActorVTable transition_vtable = {.on_init = transition_init,
                                                  .on_update = transition_update,
                                                  .on_render = transition_render,
                                                  .on_exit = transition_exit};

leo_Actor *leo_transition_start(leo_Actor *parent, const leo_TransitionDesc *desc)
{
    if (!parent || !desc)
        return NULL;

    TransitionData *data = malloc(sizeof(TransitionData));
    if (!data)
        return NULL;

    data->type = desc->type;
    data->duration = desc->duration;
    data->elapsed = 0.0f;
    data->color = desc->color;
    data->on_complete = desc->on_complete;
    data->user_data = desc->user_data;
    data->completed = false;

    leo_ActorDesc actor_desc = {
        .name = "transition", .vtable = &transition_vtable, .user_data = data, .groups = 0, .start_paused = false};

    leo_Actor *actor = leo_actor_spawn(parent, &actor_desc);
    if (!actor)
    {
        free(data);
        return NULL;
    }

    return actor;
}
