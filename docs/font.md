# Font and Text (Caller Experience)

This document describes the caller-facing API for loading TrueType fonts from
the VFS and drawing text with SDL3. It reflects the current implementation.

## Goals

- Load fonts from VFS paths with fail-fast error handling.
- Keep text setup constructor-driven to avoid ordering issues.
- Make dynamic text efficient by caching layout and only rebuilding when the
  string changes.
- Keep rendering simple: `Text::Draw` is the primary draw call.

## API Surface

```cpp
namespace engine {

struct TextDesc {
    const Font *font;
    const char *text;
    int pixel_size;
    SDL_FPoint position;
    SDL_Color color;
};

class Font {
  public:
    Font() noexcept;
    ~Font();
    Font(Font&&) noexcept;
    Font& operator=(Font&&) noexcept;

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    static Font LoadFromVfs(VFS &vfs, SDL_Renderer *renderer,
                            const char *vfs_path, int pixel_size);

    int GetLineHeight() const noexcept;
    bool IsReady() const noexcept;
    void Reset() noexcept;
};

class Text {
  public:
    Text() noexcept;
    explicit Text(const TextDesc &desc);
    ~Text();
    Text(Text&&) noexcept;
    Text& operator=(Text&&) noexcept;

    Text(const Text&) = delete;
    Text& operator=(const Text&) = delete;

    void Update(const TextDesc &desc);
    void SetString(const char *text);
    void Draw(SDL_Renderer *renderer) const;
};

} // namespace engine
```

## Lifetime and Ownership

- `Font` owns an SDL atlas texture and baked glyph data. When it goes out of
  scope (or `Reset()` is called), those resources are released.
- `Text` does not own the font; it keeps a pointer. The `Font` must outlive
  any `Text` instances that use it.

## Error Handling

- `Font::LoadFromVfs` throws `std::runtime_error` on any failure:
  missing file, decode failure, or atlas/texture creation failure.
- `Text::Update` and `Text::SetString` throw if given invalid inputs (such as
  a null `Font` or non-positive pixel size).

## Rendering Notes

- Text is drawn using the font atlas and `SDL_RenderTexture`.
- `Text` caches layout and rebuilds it only when the string changes.
- `Text::Draw` applies color modulation on the font atlas for the duration of
  the draw call.

## Tutorial

### 1) Load a font in `OnInit`

```cpp
#include "leo/engine_core.h"
#include "leo/font.h"

struct GameState {
    engine::Font font;
    engine::Text fps_text;
};

void OnInit(leo::Engine::Context &ctx, GameState &state)
{
    state.font = engine::Font::LoadFromVfs(*ctx.vfs, ctx.renderer, "font/font.ttf", 24);

    engine::TextDesc desc = {
        .font = &state.font,
        .text = "FPS: 0",
        .pixel_size = 24,
        .position = {16.0f, 16.0f},
        .color = {255, 255, 255, 255},
    };
    state.fps_text = engine::Text(desc);
}
```

### 2) Update dynamic text (only when it changes)

```cpp
void OnRender(leo::Engine::Context &ctx, GameState &state, double fps)
{
    char buffer[32];
    SDL_snprintf(buffer, sizeof(buffer), "FPS: %.1f", fps);
    state.fps_text.SetString(buffer);

    state.fps_text.Draw(ctx.renderer);
}
```

### 3) Cleanup

`Font` and `Text` are RAII types. When they go out of scope they release their
resources. If you need to release a font early:

```cpp
state.font.Reset();
```
