#ifndef LEO_LUA_RUNTIME_H
#define LEO_LUA_RUNTIME_H

#include "engine_config.h"
#include <SDL3/SDL.h>

struct lua_State;

namespace engine
{

class VFS;
class Font;

} // namespace engine

namespace leo
{
namespace Engine
{

struct InputFrame;

} // namespace Engine
} // namespace leo

namespace engine
{

class LuaRuntime
{
  public:
    LuaRuntime() noexcept;
    ~LuaRuntime();

    LuaRuntime(const LuaRuntime &) = delete;
    LuaRuntime &operator=(const LuaRuntime &) = delete;

    void Init(VFS &vfs, SDL_Window *window, SDL_Renderer *renderer, const engine::Config &config);
    void LoadScript(const char *vfs_path);

    void SetFrameInfo(Uint32 tick_index, float tick_dt);
    void CallLoad();
    void CallUpdate(float dt, const ::leo::Engine::InputFrame &input);
    void CallDraw();
    void CallShutdown();

    bool WantsQuit() const noexcept;
    void ClearQuitRequest() noexcept;
    bool IsLoaded() const noexcept;
    void RequestQuit() noexcept;
    WindowMode GetWindowMode() const noexcept;
    void SetWindowMode(WindowMode mode) noexcept;

    VFS &GetVfs() const;
    SDL_Window *GetWindow() const noexcept;
    SDL_Renderer *GetRenderer() const noexcept;
    const engine::Config *GetConfig() const noexcept;
    Uint32 GetTickIndex() const noexcept;
    float GetTickDt() const noexcept;
    SDL_Color GetDrawColor() const noexcept;
    void SetDrawColor(const SDL_Color &color) noexcept;
    engine::Font *GetCurrentFont() const noexcept;
    int GetCurrentFontSize() const noexcept;
    int GetCurrentFontRef() const noexcept;
    void SetCurrentFont(engine::Font *font, int pixel_size, int ref) noexcept;
    void ClearCurrentFontRef(lua_State *L);

  private:
    lua_State *L;
    VFS *vfs;
    SDL_Window *window;
    SDL_Renderer *renderer;
    const engine::Config *config;
    Uint32 tick_index;
    float tick_dt;
    bool loaded;
    bool quit_requested;
    SDL_Color draw_color;
    WindowMode window_mode;
    int current_font_ref;
    engine::Font *current_font_ptr;
    int current_font_size;
};

} // namespace engine

#endif // LEO_LUA_RUNTIME_H
