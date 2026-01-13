# Texture Loader (Caller Experience)

This document describes the caller-facing API for loading textures from the VFS
and rendering sprites with SDL3. It reflects the current implementation.

## Goals

- Load textures from VFS paths with fail-fast error handling.
- Keep the API small and predictable for use in `OnInit` and `OnRender`.
- Expose texture size for sprite sheet math.
- Make renderer ownership explicit (textures are renderer-specific).

## API Surface

```cpp
namespace engine {

struct Texture {
    SDL_Texture *handle;
    int width;
    int height;

    Texture() noexcept;
    Texture(SDL_Texture *handle, int width, int height) noexcept;
    ~Texture();

    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;

    Texture(Texture &&other) noexcept;
    Texture &operator=(Texture &&other) noexcept;

    void Reset() noexcept;
};

class TextureLoader {
  public:
    TextureLoader(VFS &vfs, SDL_Renderer *renderer);
    Texture Load(const char *vfs_path);
};

} // namespace engine
```

## Lifetime and Ownership

- `Texture` is an owning, move-only handle. When it goes out of scope it calls
  `SDL_DestroyTexture`.
- A texture's lifetime must be shorter than the `SDL_Renderer` that created it.
- `TextureLoader` is a thin helper; it does not own the `VFS` or renderer it
  is constructed with.
- Call `Texture::Reset()` if you want to release the texture early.

## Error Handling

- `TextureLoader::Load` throws `std::runtime_error` on any failure:
  missing file, decode failure, or `SDL_Texture` creation failure.
- Errors should surface during `OnInit` so they fail fast at startup.

## VFS Path Rules

- `vfs_path` is always relative to the mounted VFS root and uses `/` separators.
- The loader reads from the VFS search path (resources), not the write dir.

## Rendering Notes

- `SDL_RenderTextureRotated` is the primary sprite rendering call.
- `Texture::width`/`height` exist to make sprite sheet math trivial.
- The loader does not imply any batching or atlas behavior at this stage.

## Tutorial

### 1) Load textures in `OnInit`

```cpp
#include "leo/engine_core.h"
#include "leo/texture_loader.h"

struct GameState {
    engine::Texture background;
    engine::Texture character;
};

void OnInit(leo::Engine::Context &ctx, GameState &state)
{
    engine::TextureLoader loader(*ctx.vfs, ctx.renderer);
    state.background = loader.Load("images/background_1920x1080.png");
    state.character = loader.Load("images/character_64x64.png");
}
```

### 2) Draw with `SDL_RenderTextureRotated`

```cpp
void OnRender(leo::Engine::Context &ctx, GameState &state, double angle_degrees)
{
    SDL_SetRenderDrawColor(ctx.renderer, 0, 0, 0, 255);
    SDL_RenderClear(ctx.renderer);

    if (state.background.handle) {
        SDL_FRect dst = {0.0f, 0.0f,
                         static_cast<float>(state.background.width),
                         static_cast<float>(state.background.height)};
        SDL_RenderTextureRotated(ctx.renderer, state.background.handle, nullptr, &dst, 0.0, nullptr, SDL_FLIP_NONE);
    }

    if (state.character.handle) {
        SDL_FRect src = {0.0f, 0.0f,
                         static_cast<float>(state.character.width),
                         static_cast<float>(state.character.height)};
        SDL_FRect dst = {128.0f, 128.0f, src.w, src.h};
        SDL_FPoint center = {dst.w * 0.5f, dst.h * 0.5f};
        SDL_RenderTextureRotated(ctx.renderer, state.character.handle, &src, &dst, angle_degrees, &center,
                                 SDL_FLIP_NONE);
    }

    SDL_RenderPresent(ctx.renderer);
}
```

### 3) Cleanup

Textures release automatically when `GameState` goes out of scope. If you need
to release them earlier, call `Reset()`:

```cpp
state.background.Reset();
state.character.Reset();
```
