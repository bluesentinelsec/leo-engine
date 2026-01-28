#ifndef LEO_CAMERA_H
#define LEO_CAMERA_H

#include "leo/engine_config.h"
#include <SDL3/SDL.h>

namespace leo
{
namespace Camera
{

struct Camera2D
{
    SDL_FPoint position;
    SDL_FPoint offset;
    float zoom;
    float rotation;

    SDL_FPoint target;
    SDL_FRect deadzone;
    bool use_deadzone;
    float smooth_time;

    SDL_FRect bounds;
    bool clamp_to_bounds;
};

Camera2D CreateDefault(const engine::Config &config);
Camera2D CreateDefault(float logical_width, float logical_height);
void Update(Camera2D &camera, float dt);

SDL_FPoint WorldToScreen(const Camera2D &camera, SDL_FPoint world);
SDL_FPoint ScreenToWorld(const Camera2D &camera, SDL_FPoint screen);

} // namespace Camera
} // namespace leo

#endif // LEO_CAMERA_H
