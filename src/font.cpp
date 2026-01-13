#include "leo/font.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_stdinc.h>
#include <stdexcept>
#include <string>

#include <stb_truetype.h>

namespace
{

constexpr int kFirstCodepoint = 32;
constexpr int kDefaultGlyphCount = 95;
constexpr int kMaxAtlasAttempts = 3;

struct FontGlyphs
{
    stbtt_bakedchar *baked;
};

char *DuplicateString(const char *text)
{
    if (!text)
    {
        text = "";
    }
    size_t len = SDL_strlen(text);
    char *copy = static_cast<char *>(SDL_malloc(len + 1));
    if (!copy)
    {
        throw std::runtime_error("Text allocation failed");
    }
    SDL_memcpy(copy, text, len + 1);
    return copy;
}

} // namespace

namespace engine
{

Font::Font() noexcept
    : atlas(nullptr), glyphs(nullptr), atlas_width(0), atlas_height(0), base_size(0), line_height(0),
      first_codepoint(0), glyph_count(0)
{
}

Font::~Font()
{
    Reset();
}

Font::Font(Font &&other) noexcept
    : atlas(other.atlas), glyphs(other.glyphs), atlas_width(other.atlas_width), atlas_height(other.atlas_height),
      base_size(other.base_size), line_height(other.line_height), first_codepoint(other.first_codepoint),
      glyph_count(other.glyph_count)
{
    other.atlas = nullptr;
    other.glyphs = nullptr;
    other.atlas_width = 0;
    other.atlas_height = 0;
    other.base_size = 0;
    other.line_height = 0;
    other.first_codepoint = 0;
    other.glyph_count = 0;
}

Font &Font::operator=(Font &&other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    Reset();
    atlas = other.atlas;
    glyphs = other.glyphs;
    atlas_width = other.atlas_width;
    atlas_height = other.atlas_height;
    base_size = other.base_size;
    line_height = other.line_height;
    first_codepoint = other.first_codepoint;
    glyph_count = other.glyph_count;

    other.atlas = nullptr;
    other.glyphs = nullptr;
    other.atlas_width = 0;
    other.atlas_height = 0;
    other.base_size = 0;
    other.line_height = 0;
    other.first_codepoint = 0;
    other.glyph_count = 0;
    return *this;
}

void Font::Reset() noexcept
{
    if (atlas)
    {
        SDL_DestroyTexture(atlas);
        atlas = nullptr;
    }
    if (glyphs)
    {
        FontGlyphs *stored = static_cast<FontGlyphs *>(glyphs);
        SDL_free(stored->baked);
        SDL_free(stored);
        glyphs = nullptr;
    }
    atlas_width = 0;
    atlas_height = 0;
    base_size = 0;
    line_height = 0;
    first_codepoint = 0;
    glyph_count = 0;
}

Font Font::LoadFromVfs(VFS &vfs, SDL_Renderer *renderer, const char *vfs_path, int pixel_size)
{
    if (!renderer)
    {
        throw std::runtime_error("Font::LoadFromVfs requires a valid SDL_Renderer");
    }
    if (!vfs_path || !*vfs_path)
    {
        throw std::runtime_error("Font::LoadFromVfs requires a non-empty path");
    }
    if (pixel_size <= 0)
    {
        throw std::runtime_error("Font::LoadFromVfs requires a positive pixel size");
    }

    void *data = nullptr;
    size_t size = 0;
    vfs.ReadAll(vfs_path, &data, &size);
    if (!data || size == 0)
    {
        if (data)
        {
            SDL_free(data);
        }
        throw std::runtime_error("Font::LoadFromVfs received empty data buffer");
    }
    if (size > static_cast<size_t>(SDL_MAX_SINT32))
    {
        SDL_free(data);
        throw std::runtime_error("Font::LoadFromVfs font data too large");
    }

    const unsigned char *ttf = static_cast<const unsigned char *>(data);
    stbtt_fontinfo info = {};
    int font_offset = stbtt_GetFontOffsetForIndex(ttf, 0);
    if (font_offset < 0 || !stbtt_InitFont(&info, ttf, font_offset))
    {
        SDL_free(data);
        throw std::runtime_error("Font::LoadFromVfs failed to init font");
    }

    int atlas_w = 512;
    int atlas_h = 512;
    unsigned char *bitmap = nullptr;
    stbtt_bakedchar *baked = nullptr;
    int bake_result = 0;

    for (int attempt = 0; attempt < kMaxAtlasAttempts; ++attempt)
    {
        SDL_free(bitmap);
        SDL_free(baked);
        bitmap = static_cast<unsigned char *>(SDL_malloc(static_cast<size_t>(atlas_w * atlas_h)));
        baked = static_cast<stbtt_bakedchar *>(SDL_malloc(sizeof(stbtt_bakedchar) * kDefaultGlyphCount));
        if (!bitmap || !baked)
        {
            SDL_free(data);
            SDL_free(bitmap);
            SDL_free(baked);
            throw std::runtime_error("Font::LoadFromVfs out of memory");
        }
        SDL_memset(bitmap, 0, static_cast<size_t>(atlas_w * atlas_h));

        bake_result = stbtt_BakeFontBitmap(ttf, 0, static_cast<float>(pixel_size), bitmap, atlas_w, atlas_h,
                                           kFirstCodepoint, kDefaultGlyphCount, baked);
        if (bake_result > 0)
        {
            break;
        }

        atlas_w *= 2;
        atlas_h *= 2;
    }

    if (bake_result <= 0)
    {
        SDL_free(data);
        SDL_free(bitmap);
        SDL_free(baked);
        throw std::runtime_error("Font::LoadFromVfs failed to bake font atlas");
    }

    int ascent = 0;
    int descent = 0;
    int line_gap = 0;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);
    const float vscale = stbtt_ScaleForPixelHeight(&info, static_cast<float>(pixel_size));
    const int line_height = static_cast<int>(SDL_floorf(((ascent - descent) + line_gap) * vscale + 0.5f));

    unsigned char *rgba = static_cast<unsigned char *>(SDL_malloc(static_cast<size_t>(atlas_w * atlas_h * 4)));
    if (!rgba)
    {
        SDL_free(data);
        SDL_free(bitmap);
        SDL_free(baked);
        throw std::runtime_error("Font::LoadFromVfs out of memory for atlas");
    }
    for (int i = 0; i < atlas_w * atlas_h; ++i)
    {
        unsigned char a = bitmap[i];
        rgba[4 * i + 0] = 255;
        rgba[4 * i + 1] = 255;
        rgba[4 * i + 2] = 255;
        rgba[4 * i + 3] = a;
    }

    SDL_Texture *atlas =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, atlas_w, atlas_h);
    if (!atlas)
    {
        SDL_free(data);
        SDL_free(bitmap);
        SDL_free(baked);
        SDL_free(rgba);
        throw std::runtime_error(std::string("Font::LoadFromVfs failed to create texture: ") + SDL_GetError());
    }
    SDL_SetTextureScaleMode(atlas, SDL_SCALEMODE_LINEAR);
    SDL_SetTextureBlendMode(atlas, SDL_BLENDMODE_BLEND);

    if (!SDL_UpdateTexture(atlas, nullptr, rgba, atlas_w * 4))
    {
        SDL_free(data);
        SDL_free(bitmap);
        SDL_free(baked);
        SDL_free(rgba);
        SDL_DestroyTexture(atlas);
        throw std::runtime_error(std::string("Font::LoadFromVfs failed to upload atlas: ") + SDL_GetError());
    }

    SDL_free(data);
    SDL_free(bitmap);
    SDL_free(rgba);

    FontGlyphs *glyph_storage = static_cast<FontGlyphs *>(SDL_malloc(sizeof(FontGlyphs)));
    if (!glyph_storage)
    {
        SDL_free(baked);
        SDL_DestroyTexture(atlas);
        throw std::runtime_error("Font::LoadFromVfs out of memory for glyph storage");
    }
    glyph_storage->baked = baked;

    Font font;
    font.atlas = atlas;
    font.glyphs = glyph_storage;
    font.atlas_width = atlas_w;
    font.atlas_height = atlas_h;
    font.base_size = pixel_size;
    font.line_height = line_height;
    font.first_codepoint = kFirstCodepoint;
    font.glyph_count = kDefaultGlyphCount;
    return font;
}

int Font::GetLineHeight() const noexcept
{
    return line_height;
}

bool Font::IsReady() const noexcept
{
    return atlas != nullptr && glyphs != nullptr && base_size > 0 && line_height > 0 && glyph_count > 0;
}

Text::Text() noexcept
    : font(nullptr), text(nullptr), pixel_size(0), position({0.0f, 0.0f}), color({255, 255, 255, 255}),
      src_quads(nullptr), dst_quads(nullptr), quad_count(0)
{
}

Text::Text(const TextDesc &desc) : Text()
{
    Update(desc);
}

Text::~Text()
{
    ClearLayout();
    if (text)
    {
        SDL_free(text);
        text = nullptr;
    }
}

Text::Text(Text &&other) noexcept
    : font(other.font), text(other.text), pixel_size(other.pixel_size), position(other.position), color(other.color),
      src_quads(other.src_quads), dst_quads(other.dst_quads), quad_count(other.quad_count)
{
    other.font = nullptr;
    other.text = nullptr;
    other.pixel_size = 0;
    other.position = {0.0f, 0.0f};
    other.color = {255, 255, 255, 255};
    other.src_quads = nullptr;
    other.dst_quads = nullptr;
    other.quad_count = 0;
}

Text &Text::operator=(Text &&other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    ClearLayout();
    if (text)
    {
        SDL_free(text);
    }

    font = other.font;
    text = other.text;
    pixel_size = other.pixel_size;
    position = other.position;
    color = other.color;
    src_quads = other.src_quads;
    dst_quads = other.dst_quads;
    quad_count = other.quad_count;

    other.font = nullptr;
    other.text = nullptr;
    other.pixel_size = 0;
    other.position = {0.0f, 0.0f};
    other.color = {255, 255, 255, 255};
    other.src_quads = nullptr;
    other.dst_quads = nullptr;
    other.quad_count = 0;
    return *this;
}

void Text::Update(const TextDesc &desc)
{
    if (!desc.font)
    {
        throw std::runtime_error("Text::Update requires a valid Font");
    }
    if (desc.pixel_size <= 0)
    {
        throw std::runtime_error("Text::Update requires a positive pixel size");
    }

    font = desc.font;
    pixel_size = desc.pixel_size;
    position = desc.position;
    color = desc.color;
    SetTextCopy(desc.text);
    RebuildLayout();
}

void Text::SetString(const char *new_text)
{
    if (!font)
    {
        throw std::runtime_error("Text::SetString requires a valid Font");
    }

    if (!new_text)
    {
        new_text = "";
    }

    if (text && SDL_strcmp(text, new_text) == 0)
    {
        return;
    }

    SetTextCopy(new_text);
    RebuildLayout();
}

void Text::Draw(SDL_Renderer *renderer) const
{
    if (!renderer || !font || !font->atlas || quad_count == 0)
    {
        return;
    }

    SDL_SetTextureColorMod(font->atlas, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(font->atlas, color.a);

    for (int i = 0; i < quad_count; ++i)
    {
        SDL_FRect dst = dst_quads[i];
        dst.x += position.x;
        dst.y += position.y;
        SDL_RenderTexture(renderer, font->atlas, &src_quads[i], &dst);
    }

    SDL_SetTextureColorMod(font->atlas, 255, 255, 255);
    SDL_SetTextureAlphaMod(font->atlas, 255);
}

void Text::ClearLayout() noexcept
{
    if (src_quads)
    {
        SDL_free(src_quads);
        src_quads = nullptr;
    }
    if (dst_quads)
    {
        SDL_free(dst_quads);
        dst_quads = nullptr;
    }
    quad_count = 0;
}

void Text::SetTextCopy(const char *new_text)
{
    if (text)
    {
        SDL_free(text);
        text = nullptr;
    }
    text = DuplicateString(new_text);
}

void Text::RebuildLayout()
{
    ClearLayout();

    if (!font || !font->IsReady() || !text || pixel_size <= 0)
    {
        return;
    }

    const FontGlyphs *glyph_storage = static_cast<const FontGlyphs *>(font->glyphs);
    const stbtt_bakedchar *baked = glyph_storage->baked;
    if (!baked)
    {
        return;
    }

    int glyphs_needed = 0;
    for (const unsigned char *p = reinterpret_cast<const unsigned char *>(text); *p; ++p)
    {
        int ch = static_cast<int>(*p);
        if (ch == '\n')
        {
            continue;
        }
        if (ch < font->first_codepoint || ch >= font->first_codepoint + font->glyph_count)
        {
            continue;
        }
        glyphs_needed++;
    }

    if (glyphs_needed == 0)
    {
        return;
    }

    src_quads = static_cast<SDL_FRect *>(SDL_malloc(sizeof(SDL_FRect) * glyphs_needed));
    dst_quads = static_cast<SDL_FRect *>(SDL_malloc(sizeof(SDL_FRect) * glyphs_needed));
    if (!src_quads || !dst_quads)
    {
        ClearLayout();
        throw std::runtime_error("Text::RebuildLayout out of memory");
    }

    const float scale = static_cast<float>(pixel_size) / static_cast<float>(font->base_size);
    const float line_advance = static_cast<float>(font->line_height);

    float pen_x = 0.0f;
    float pen_y = 0.0f;
    int quad_index = 0;

    for (const unsigned char *p = reinterpret_cast<const unsigned char *>(text); *p; ++p)
    {
        int ch = static_cast<int>(*p);
        if (ch == '\n')
        {
            pen_x = 0.0f;
            pen_y += line_advance;
            continue;
        }
        if (ch < font->first_codepoint || ch >= font->first_codepoint + font->glyph_count)
        {
            continue;
        }

        stbtt_aligned_quad quad;
        float qx = pen_x;
        float qy = pen_y;
        stbtt_GetBakedQuad(baked, font->atlas_width, font->atlas_height, ch - font->first_codepoint, &qx, &qy, &quad,
                           1);

        float dx0 = quad.x0 * scale;
        float dy0 = quad.y0 * scale;
        float dx1 = quad.x1 * scale;
        float dy1 = quad.y1 * scale;

        float dst_w = dx1 - dx0;
        float dst_h = dy1 - dy0;
        if (dst_w > 0.0f && dst_h > 0.0f)
        {
            src_quads[quad_index] = {quad.s0 * font->atlas_width, quad.t0 * font->atlas_height,
                                     (quad.s1 - quad.s0) * font->atlas_width, (quad.t1 - quad.t0) * font->atlas_height};
            dst_quads[quad_index] = {dx0, dy0, dst_w, dst_h};
            quad_index++;
        }

        pen_x = qx;
        pen_y = qy;
    }

    quad_count = quad_index;
}

} // namespace engine
