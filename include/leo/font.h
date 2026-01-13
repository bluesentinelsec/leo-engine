#ifndef LEO_FONT_H
#define LEO_FONT_H

#include "leo/vfs.h"
#include <SDL3/SDL.h>

namespace engine
{

struct TextDesc
{
    const class Font *font;
    const char *text;
    int pixel_size;
    SDL_FPoint position;
    SDL_Color color;
};

class Font
{
  public:
    Font() noexcept;
    ~Font();
    Font(Font &&other) noexcept;
    Font &operator=(Font &&other) noexcept;

    Font(const Font &) = delete;
    Font &operator=(const Font &) = delete;

    static Font LoadFromVfs(VFS &vfs, SDL_Renderer *renderer, const char *vfs_path, int pixel_size);

    int GetLineHeight() const noexcept;
    bool IsReady() const noexcept;
    void Reset() noexcept;

  private:
    friend class Text;

    SDL_Texture *atlas;
    void *glyphs;
    int atlas_width;
    int atlas_height;
    int base_size;
    int line_height;
    int first_codepoint;
    int glyph_count;
};

class Text
{
  public:
    Text() noexcept;
    explicit Text(const TextDesc &desc);
    ~Text();
    Text(Text &&other) noexcept;
    Text &operator=(Text &&other) noexcept;

    Text(const Text &) = delete;
    Text &operator=(const Text &) = delete;

    void Update(const TextDesc &desc);
    void SetString(const char *text);
    void Draw(SDL_Renderer *renderer) const;

  private:
    void ClearLayout() noexcept;
    void RebuildLayout();
    void SetTextCopy(const char *text);

    const Font *font;
    char *text;
    int pixel_size;
    SDL_FPoint position;
    SDL_Color color;

    SDL_FRect *src_quads;
    SDL_FRect *dst_quads;
    int quad_count;
};

} // namespace engine

#endif // LEO_FONT_H
