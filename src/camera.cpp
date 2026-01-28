#include "leo/camera.h"

#include <algorithm>
#include <cmath>

namespace
{

float Clamp(float value, float min_value, float max_value)
{
    return std::max(min_value, std::min(max_value, value));
}

} // namespace

namespace leo
{
namespace Camera
{

Camera2D CreateDefault(const engine::Config &config)
{
    return CreateDefault(static_cast<float>(config.logical_width), static_cast<float>(config.logical_height));
}

Camera2D CreateDefault(float logical_width, float logical_height)
{
    Camera2D camera = {};
    camera.position = {0.0f, 0.0f};
    camera.target = camera.position;
    camera.zoom = 1.0f;
    camera.rotation = 0.0f;
    camera.offset = {logical_width * 0.5f, logical_height * 0.5f};
    camera.deadzone = {0.0f, 0.0f, 0.0f, 0.0f};
    camera.use_deadzone = false;
    camera.smooth_time = 0.0f;
    camera.bounds = {0.0f, 0.0f, 0.0f, 0.0f};
    camera.clamp_to_bounds = false;
    return camera;
}

void Update(Camera2D &camera, float dt)
{
    SDL_FPoint desired = camera.target;

    if (camera.use_deadzone && camera.deadzone.w > 0.0f && camera.deadzone.h > 0.0f)
    {
        float half_w = camera.deadzone.w * 0.5f;
        float half_h = camera.deadzone.h * 0.5f;

        desired = camera.position;
        float dx = camera.target.x - camera.position.x;
        float dy = camera.target.y - camera.position.y;

        if (dx > half_w)
        {
            desired.x = camera.target.x - half_w;
        }
        else if (dx < -half_w)
        {
            desired.x = camera.target.x + half_w;
        }

        if (dy > half_h)
        {
            desired.y = camera.target.y - half_h;
        }
        else if (dy < -half_h)
        {
            desired.y = camera.target.y + half_h;
        }
    }

    if (camera.smooth_time > 0.0f && dt > 0.0f)
    {
        float t = 1.0f - std::exp(-dt / camera.smooth_time);
        camera.position.x += (desired.x - camera.position.x) * t;
        camera.position.y += (desired.y - camera.position.y) * t;
    }
    else
    {
        camera.position = desired;
    }

    if (camera.clamp_to_bounds && camera.bounds.w > 0.0f && camera.bounds.h > 0.0f)
    {
        float zoom = camera.zoom > 0.0f ? camera.zoom : 1.0f;
        float half_w = camera.offset.x / zoom;
        float half_h = camera.offset.y / zoom;

        float min_x = camera.bounds.x + half_w;
        float max_x = camera.bounds.x + camera.bounds.w - half_w;
        float min_y = camera.bounds.y + half_h;
        float max_y = camera.bounds.y + camera.bounds.h - half_h;

        if (min_x > max_x)
        {
            camera.position.x = camera.bounds.x + camera.bounds.w * 0.5f;
        }
        else
        {
            camera.position.x = Clamp(camera.position.x, min_x, max_x);
        }

        if (min_y > max_y)
        {
            camera.position.y = camera.bounds.y + camera.bounds.h * 0.5f;
        }
        else
        {
            camera.position.y = Clamp(camera.position.y, min_y, max_y);
        }
    }
}

SDL_FPoint WorldToScreen(const Camera2D &camera, SDL_FPoint world)
{
    float x = (world.x - camera.position.x) * camera.zoom;
    float y = (world.y - camera.position.y) * camera.zoom;

    if (camera.rotation != 0.0f)
    {
        float cos_r = std::cos(camera.rotation);
        float sin_r = std::sin(camera.rotation);
        float rx = x * cos_r - y * sin_r;
        float ry = x * sin_r + y * cos_r;
        x = rx;
        y = ry;
    }

    return {x + camera.offset.x, y + camera.offset.y};
}

SDL_FPoint ScreenToWorld(const Camera2D &camera, SDL_FPoint screen)
{
    float x = (screen.x - camera.offset.x);
    float y = (screen.y - camera.offset.y);

    if (camera.rotation != 0.0f)
    {
        float cos_r = std::cos(-camera.rotation);
        float sin_r = std::sin(-camera.rotation);
        float rx = x * cos_r - y * sin_r;
        float ry = x * sin_r + y * cos_r;
        x = rx;
        y = ry;
    }

    float zoom = camera.zoom != 0.0f ? camera.zoom : 1.0f;
    return {x / zoom + camera.position.x, y / zoom + camera.position.y};
}

} // namespace Camera
} // namespace leo
