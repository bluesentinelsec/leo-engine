#ifndef LEO_ENGINE_CORE_H
#define LEO_ENGINE_CORE_H

#include "engine_config.h"
#include "leo/gamepad.h"
#include "leo/keyboard.h"
#include "leo/mouse.h"
#include "leo/vfs.h"
#include <SDL3/SDL.h>
#include <memory>

namespace engine
{

class LuaRuntime;

} // namespace engine

namespace leo
{
namespace Engine
{

using Config = ::engine::Config;
using WindowMode = ::engine::WindowMode;

struct InputFrame
{
    bool quit_requested;
    Uint32 frame_index;
    ::engine::KeyboardState keyboard;
    ::engine::MouseState mouse;
    ::engine::GamepadState gamepads[2];
};

struct Context
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    ::engine::VFS *vfs;
    const Config *config;
    Uint32 frame_index;
};

class Simulation
{
  public:
    explicit Simulation(Config &config);
    ~Simulation();
    int Run();

  private:
    void OnInit(Context &ctx);
    void OnUpdate(Context &ctx, const InputFrame &input, float dt);
    void OnRender(Context &ctx);
    void OnExit(Context &ctx);

    Config &config;
    ::engine::VFS vfs;
    SDL_Window *window;
    SDL_Renderer *renderer;
    std::unique_ptr<::engine::LuaRuntime> lua;
};

} // namespace Engine
} // namespace leo

#endif // LEO_ENGINE_CORE_H
