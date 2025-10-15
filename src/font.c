// =============================================
// leo/font.c — Step 2: real loading + draw + measure
// =============================================
#include "leo/font.h"
#include "leo/default_font.h"
#include "leo/engine.h"
#include "leo/error.h"
#include "leo/io.h"

#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb/stb_truetype.h>

// Text cache
#define CACHE_MAX_ENTRIES 128
typedef struct CacheEntry
{
    uint32_t hash;
    SDL_Texture *texture;
    int width, height;
    struct CacheEntry *next;
    int lru_counter;
} CacheEntry;

static CacheEntry *g_cache[CACHE_MAX_ENTRIES] = {0};
static int g_lru_counter = 0;

// --------- ASCII range we bake for step 2 ----------
#define LEO_FONT_FIRST_CODEPOINT 32
#define LEO_FONT_DEFAULT_COUNT 95 // 32..126 inclusive

typedef struct _GlyphTable
{
    stbtt_bakedchar *baked; // baked glyphs [glyphCount]
    int first;              // first codepoint (usually 32)
    int count;              // number of glyphs (usually 95)
} _GlyphTable;

static leo_Font g_default_font = {0};

static inline SDL_Renderer *_ren(void)
{
    return (SDL_Renderer *)leo_GetRenderer();
}

static inline leo_Font _zero(void)
{
    leo_Font f;
    memset(&f, 0, sizeof(f));
    return f;
}

static uint32_t _hash_text(const char *text, void *font, float fontSize, leo_Color color)
{
    uint32_t hash = 2166136261u;
    while (*text)
    {
        hash ^= (uint32_t)*text++;
        hash *= 16777619u;
    }
    hash ^= (uintptr_t)font;
    hash ^= *(uint32_t *)&fontSize;
    hash ^= (uint32_t)color.r | ((uint32_t)color.g << 8) | ((uint32_t)color.b << 16) | ((uint32_t)color.a << 24);
    return hash;
}

static CacheEntry *_cache_get(uint32_t hash)
{
    int bucket = hash % CACHE_MAX_ENTRIES;
    for (CacheEntry *e = g_cache[bucket]; e; e = e->next)
    {
        if (e->hash == hash)
        {
            e->lru_counter = ++g_lru_counter;
            return e;
        }
    }
    return NULL;
}

static void _cache_evict_lru(void)
{
    CacheEntry *oldest = NULL;
    int oldest_bucket = -1;
    CacheEntry *oldest_prev = NULL;

    for (int i = 0; i < CACHE_MAX_ENTRIES; i++)
    {
        CacheEntry *prev = NULL;
        for (CacheEntry *e = g_cache[i]; e; prev = e, e = e->next)
        {
            if (!oldest || e->lru_counter < oldest->lru_counter)
            {
                oldest = e;
                oldest_bucket = i;
                oldest_prev = prev;
            }
        }
    }

    if (oldest)
    {
        if (oldest_prev)
            oldest_prev->next = oldest->next;
        else
            g_cache[oldest_bucket] = oldest->next;

        SDL_DestroyTexture(oldest->texture);
        free(oldest);
    }
}

static void _cache_put(uint32_t hash, SDL_Texture *texture, int width, int height)
{
    static int cache_size = 0;

    if (cache_size >= CACHE_MAX_ENTRIES)
    {
        _cache_evict_lru();
        cache_size--;
    }

    int bucket = hash % CACHE_MAX_ENTRIES;
    CacheEntry *entry = malloc(sizeof(CacheEntry));
    if (!entry)
        return;

    entry->hash = hash;
    entry->texture = texture;
    entry->width = width;
    entry->height = height;
    entry->lru_counter = ++g_lru_counter;
    entry->next = g_cache[bucket];
    g_cache[bucket] = entry;
    cache_size++;
}

static void _cache_clear_font(void *font)
{
    for (int i = 0; i < CACHE_MAX_ENTRIES; i++)
    {
        CacheEntry **curr = &g_cache[i];
        while (*curr)
        {
            // Simple heuristic: if hash contains font pointer bits, remove it
            if (((*curr)->hash ^ (uintptr_t)font) < 0x10000)
            {
                CacheEntry *to_remove = *curr;
                *curr = to_remove->next;
                SDL_DestroyTexture(to_remove->texture);
                free(to_remove);
            }
            else
            {
                curr = &(*curr)->next;
            }
        }
    }
}

static leo_Font _load_default_font(int pixelSize)
{
    // Always succeeds if SDL renderer is available
    return leo_LoadFontFromMemory(".ttf", default_font_ttf, (int)default_font_ttf_len, pixelSize);
}

bool leo_IsFontReady(leo_Font font)
{
    return font._atlas != NULL && font._glyphs != NULL && font.baseSize > 0 && font.glyphCount > 0 &&
           font.lineHeight > 0;
}

// ---- internal load helper from TTF bytes ----
static leo_Font _load_from_ttf_bytes(const unsigned char *ttf, int ttfSize, int pixelSize)
{
    if (!ttf || ttfSize <= 0 || pixelSize <= 0)
    {
        leo_SetError("leo_LoadFont: bad args");
        return _zero();
    }
    SDL_Renderer *r = _ren();
    if (!r)
    {
        leo_SetError("leo_LoadFont: renderer is null");
        return _zero();
    }

    // Parse font once for metrics (line height, etc.)
    stbtt_fontinfo finfo;
    int fontOffset = stbtt_GetFontOffsetForIndex(ttf, 0);
    // Try up to 3 atlas sizes if the first is too small.
    int atlasW = 512, atlasH = 512;
    unsigned char *bitmap = NULL;
    stbtt_bakedchar *baked = NULL;
    int bakeRes = 0;

    int attempt;
    for (attempt = 0; attempt < 3; ++attempt)
    {
        free(bitmap);
        free(baked);
        bitmap = (unsigned char *)malloc((size_t)atlasW * atlasH);
        baked = (stbtt_bakedchar *)malloc(sizeof(stbtt_bakedchar) * LEO_FONT_DEFAULT_COUNT);
        if (!bitmap || !baked)
        {
            leo_SetError("leo_LoadFont: OOM");
            goto fail;
        }
        memset(bitmap, 0, (size_t)atlasW * atlasH);

        bakeRes = stbtt_BakeFontBitmap(ttf, 0, (float)pixelSize, bitmap, atlasW, atlasH, LEO_FONT_FIRST_CODEPOINT,
                                       LEO_FONT_DEFAULT_COUNT, baked);
        if (bakeRes > 0)
            break; // success
        atlasW *= 2;
        atlasH *= 2; // grow and try again
    }

    if (bakeRes <= 0)
    {
        leo_SetError("stbtt_BakeFontBitmap failed");
        goto fail;
    }

    if (!stbtt_InitFont(&finfo, ttf, fontOffset))
    {
        leo_SetError("leo_LoadFont: stbtt_InitFont failed");
        goto fail;
    }
    // Proper line height at the requested base pixel size.
    int ascent = 0, descent = 0, lineGap = 0;
    stbtt_GetFontVMetrics(&finfo, &ascent, &descent, &lineGap);
    const float vscale = stbtt_ScaleForPixelHeight(&finfo, (float)pixelSize);

    // Expand grayscale -> RGBA (white text with alpha)
    unsigned char *rgba = (unsigned char *)malloc((size_t)atlasW * atlasH * 4);
    if (!rgba)
    {
        leo_SetError("leo_LoadFont: OOM (rgba)");
        goto fail;
    }
    int i;
    for (i = 0; i < atlasW * atlasH; ++i)
    {
        unsigned char a = bitmap[i];
        rgba[4 * i + 0] = 255;
        rgba[4 * i + 1] = 255;
        rgba[4 * i + 2] = 255;
        rgba[4 * i + 3] = a;
    }

    SDL_Texture *tex = SDL_CreateTexture(r, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, atlasW, atlasH);
    if (!tex)
    {
        leo_SetError("SDL_CreateTexture failed");
        free(rgba);
        goto fail;
    }
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

    if (!SDL_UpdateTexture(tex, NULL, rgba, atlasW * 4))
    {
        leo_SetError("SDL_UpdateTexture failed");
        SDL_DestroyTexture(tex);
        free(rgba);
        goto fail;
    }
    free(rgba);
    free(bitmap);
    bitmap = NULL;

    _GlyphTable *tbl = (_GlyphTable *)malloc(sizeof(_GlyphTable));
    if (!tbl)
    {
        leo_SetError("leo_LoadFont: OOM (_GlyphTable)");
        SDL_DestroyTexture(tex);
        goto fail;
    }
    tbl->baked = baked; // take ownership
    tbl->first = LEO_FONT_FIRST_CODEPOINT;
    tbl->count = LEO_FONT_DEFAULT_COUNT;

    leo_Font out = _zero();
    out._atlas = (void *)tex;
    out._glyphs = (void *)tbl;
    out._atlasW = atlasW;
    out._atlasH = atlasH;
    out.baseSize = pixelSize;
    out.glyphCount = LEO_FONT_DEFAULT_COUNT;
    out.lineHeight = (int)floorf(((ascent - descent) + lineGap) * vscale + 0.5f);
    return out;

fail:
    free(bitmap);
    free(baked);
    return _zero();
}

leo_Font leo_LoadFont(const char *fileName, int pixelSize)
{
    if (!fileName || !*fileName || pixelSize <= 0)
    {
        leo_SetError("leo_LoadFont: invalid args");
        return _zero();
    }

    // 1) Try VFS first
    size_t sz = 0;
    unsigned char *buf = (unsigned char *)leo_LoadAsset(fileName, &sz);
    if (buf && sz > 0)
    {
        leo_Font font = _load_from_ttf_bytes(buf, (int)sz, pixelSize);
        free(buf);
        return font;
    }

    // 2) Fallback to direct filesystem path
    FILE *f = fopen(fileName, "rb");
    if (!f)
    {
        leo_SetError("leo_LoadFont: not found '%s'", fileName);
        return _zero();
    }
    fseek(f, 0, SEEK_END);
    long fsz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsz <= 0)
    {
        fclose(f);
        leo_SetError("leo_LoadFont: empty file '%s'", fileName);
        return _zero();
    }

    unsigned char *fbuf = (unsigned char *)malloc((size_t)fsz);
    if (!fbuf)
    {
        fclose(f);
        leo_SetError("leo_LoadFont: OOM");
        return _zero();
    }

    fread(fbuf, 1, (size_t)fsz, f);
    fclose(f);

    leo_Font font = _load_from_ttf_bytes(fbuf, (int)fsz, pixelSize);
    free(fbuf);
    return font;
}

leo_Font leo_LoadFontFromMemory(const char *fileType, const unsigned char *data, int dataSize, int pixelSize)
{
    if (!data || dataSize <= 0 || pixelSize <= 0)
    {
        leo_SetError("leo_LoadFontFromMemory: invalid args");
        return _zero();
    }
    return _load_from_ttf_bytes(data, dataSize, pixelSize);
}

void leo_UnloadFont(leo_Font *font)
{
    if (!font)
        return;

    _cache_clear_font(font->_atlas);

    if (font->_atlas)
        SDL_DestroyTexture((SDL_Texture *)font->_atlas);
    if (font->_glyphs)
    {
        _GlyphTable *tbl = (_GlyphTable *)font->_glyphs;
        free(tbl->baked);
        free(tbl);
    }
    *font = _zero();
}

void leo_SetDefaultFont(leo_Font font)
{
    g_default_font = font;
}

leo_Font leo_GetDefaultFont(void)
{
    return g_default_font;
}

// ------------- internal draw core ----------------
static void _draw_text_impl(leo_Font font, const char *text, float x, float y, float fontSize, float spacing,
                            leo_Color tint, float rotationDeg, float originX, float originY)
{
    if (!text || !*text)
        return;
    if (!leo_IsFontReady(font))
        return;

    SDL_Renderer *r = _ren();
    if (!r)
        return;

    // Check cache first
    uint32_t hash = _hash_text(text, font._atlas, fontSize, tint);
    CacheEntry *cached = _cache_get(hash);

    if (cached)
    {
        // Cache hit - draw cached texture
        SDL_FRect dst = {x, y, (float)cached->width, (float)cached->height};
        if (rotationDeg != 0.0f)
        {
            SDL_FPoint center = {originX - dst.x, originY - dst.y};
            SDL_RenderTextureRotated(r, cached->texture, NULL, &dst, rotationDeg, &center, SDL_FLIP_NONE);
        }
        else
        {
            SDL_RenderTexture(r, cached->texture, NULL, &dst);
        }
        return;
    }

    // Cache miss - measure text first
    leo_Vector2 textSize = leo_MeasureTextEx(font, text, fontSize, spacing);
    if (textSize.x <= 0 || textSize.y <= 0)
        return;

    int texW = (int)(textSize.x + 0.5f);
    int texH = (int)(textSize.y + 0.5f);

    // Create render target
    SDL_Texture *renderTarget = SDL_CreateTexture(r, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, texW, texH);
    if (!renderTarget)
        return;

    SDL_SetTextureBlendMode(renderTarget, SDL_BLENDMODE_BLEND);

    // Render to texture
    SDL_Texture *oldTarget = SDL_GetRenderTarget(r);
    SDL_SetRenderTarget(r, renderTarget);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
    SDL_RenderClear(r);

    // Original rendering code
    SDL_Texture *atlas = (SDL_Texture *)font._atlas;
    _GlyphTable *tbl = (_GlyphTable *)font._glyphs;
    stbtt_bakedchar *cdata = tbl->baked;

    SDL_SetTextureColorMod(atlas, (Uint8)tint.r, (Uint8)tint.g, (Uint8)tint.b);
    SDL_SetTextureAlphaMod(atlas, (Uint8)tint.a);

    const float lineAdvanceBase = (float)font.lineHeight;
    const float scale = fontSize / (float)font.baseSize;
    if (scale <= 0.0f)
    {
        SDL_SetRenderTarget(r, oldTarget);
        SDL_DestroyTexture(renderTarget);
        return;
    }

    float penX = 0.0f;
    float penY = 0.0f;

    const unsigned char *p;
    for (p = (const unsigned char *)text; *p; ++p)
    {
        int ch = (int)*p;
        if (ch == '\n')
        {
            penX = 0.0f;
            penY += lineAdvanceBase;
            continue;
        }
        if (ch < tbl->first || ch >= tbl->first + tbl->count)
        {
            continue;
        }

        stbtt_aligned_quad q;
        float qx = penX, qy = penY;
        stbtt_GetBakedQuad(cdata, font._atlasW, font._atlasH, ch - tbl->first, &qx, &qy, &q, 1);

        float dx0 = q.x0 * scale;
        float dy0 = q.y0 * scale;
        float dx1 = q.x1 * scale;
        float dy1 = q.y1 * scale;

        SDL_FRect src = {q.s0 * font._atlasW, q.t0 * font._atlasH, (q.s1 - q.s0) * font._atlasW,
                         (q.t1 - q.t0) * font._atlasH};
        SDL_FRect dst = {dx0, dy0, dx1 - dx0, dy1 - dy0};

        SDL_RenderTexture(r, atlas, &src, &dst);

        penX = qx;
        penY = qy;
        if (*(p + 1) != '\0' && *(p + 1) != '\n')
        {
            penX += (spacing / ((font.baseSize > 0) ? (fontSize / (float)font.baseSize) : 1.0f));
        }
    }

    SDL_SetTextureColorMod(atlas, 255, 255, 255);
    SDL_SetTextureAlphaMod(atlas, 255);

    // Restore render target
    SDL_SetRenderTarget(r, oldTarget);

    // Cache the texture
    _cache_put(hash, renderTarget, texW, texH);

    // Draw the cached texture
    SDL_FRect dst = {x, y, (float)texW, (float)texH};
    if (rotationDeg != 0.0f)
    {
        SDL_FPoint center = {originX - dst.x, originY - dst.y};
        SDL_RenderTextureRotated(r, renderTarget, NULL, &dst, rotationDeg, &center, SDL_FLIP_NONE);
    }
    else
    {
        SDL_RenderTexture(r, renderTarget, NULL, &dst);
    }
}

void leo_DrawFPS(int x, int y)
{
    if (!leo_IsFontReady(g_default_font))
    {
        g_default_font = _load_default_font(16);
    }

    int fps = leo_GetFPS();
    char buf[32];
    SDL_snprintf(buf, sizeof(buf), "%d FPS", fps);
    leo_Color green;
    green.r = 0;
    green.g = 255;
    green.b = 0;
    green.a = 255;
    leo_DrawText(buf, x, y, g_default_font.baseSize, green);
}

void leo_DrawText(const char *text, int posX, int posY, int fontSize, leo_Color color)
{
    if (!leo_IsFontReady(g_default_font))
    {
        g_default_font = _load_default_font(fontSize > 0 ? fontSize : 16);
    }
    _draw_text_impl(g_default_font, text, (float)posX, (float)posY, (float)fontSize, 0.0f, color, 0.0f, 0.0f, 0.0f);
}

void leo_DrawTextEx(leo_Font font, const char *text, leo_Vector2 pos, float fontSize, float spacing, leo_Color tint)
{
    if (!leo_IsFontReady(font))
    {
        if (!leo_IsFontReady(g_default_font))
            g_default_font = _load_default_font(fontSize > 0 ? fontSize : 16);
        font = g_default_font;
    }
    _draw_text_impl(font, text, pos.x, pos.y, fontSize, spacing, tint, 0.0f, 0.0f, 0.0f);
}

void leo_DrawTextPro(leo_Font font, const char *text, leo_Vector2 position, leo_Vector2 origin, float rotation,
                     float fontSize, float spacing, leo_Color tint)
{
    if (!leo_IsFontReady(font))
    {
        if (!leo_IsFontReady(g_default_font))
            g_default_font = _load_default_font(fontSize > 0 ? fontSize : 16);
        font = g_default_font;
    }
    _draw_text_impl(font, text, position.x, position.y, fontSize, spacing, tint, rotation, origin.x, origin.y);
}

// ---------- measurement ----------
leo_Vector2 leo_MeasureTextEx(leo_Font font, const char *text, float fontSize, float spacing)
{
    if (!leo_IsFontReady(font))
    {
        if (!leo_IsFontReady(g_default_font))
            g_default_font = _load_default_font(fontSize > 0 ? fontSize : 16);
        font = g_default_font;
    }

    leo_Vector2 sz = (leo_Vector2){0, 0};
    if (!text || !*text || !leo_IsFontReady(font) || font.baseSize <= 0)
        return sz;

    _GlyphTable *tbl = (_GlyphTable *)font._glyphs;
    stbtt_bakedchar *cdata = tbl->baked;

    const float scale = fontSize / (float)font.baseSize;
    const float lineH_base = (float)font.lineHeight; // base-size units

    // pen in base units; we scale only when producing the final size
    float x_base = 0.0f, y_base = 0.0f;
    float lineMaxRight_base = 0.0f;    // max right-extent for current line
    float overallMaxRight_base = 0.0f; // max across all lines
    int lineCount = 1;

    const unsigned char *p;
    for (p = (const unsigned char *)text; *p; ++p)
    {
        int ch = (int)*p;

        if (ch == '\n')
        {
            // finalize current line
            float lineWidth_base = (x_base > lineMaxRight_base) ? x_base : lineMaxRight_base;
            if (lineWidth_base > overallMaxRight_base)
                overallMaxRight_base = lineWidth_base;
            // next line
            x_base = 0.0f;
            y_base += lineH_base;
            lineMaxRight_base = 0.0f;
            lineCount += 1;
            continue;
        }

        if (ch < tbl->first || ch >= tbl->first + tbl->count)
            continue;

        stbtt_aligned_quad q;
        float qx = x_base, qy = y_base;
        stbtt_GetBakedQuad(cdata, font._atlasW, font._atlasH, ch - tbl->first, &qx, &qy, &q, 1);

        // compute right extent for this glyph in base units
        float right_base = q.x1; // q.x1 is already in base units (since we asked stbtt with scale==1)
        if (right_base > lineMaxRight_base)
            lineMaxRight_base = right_base;

        // advance pen (base units)
        x_base = qx;
        y_base = qy;
        // apply extra spacing only between glyphs on the same line
        if (*(p + 1) != '\0' && *(p + 1) != '\n')
        {
            // spacing is at target size; convert to base units
            x_base += (spacing / ((font.baseSize > 0) ? (fontSize / (float)font.baseSize) : 1.0f));
        }
    }

    // finalize last line
    {
        float lineWidth_base = (x_base > lineMaxRight_base) ? x_base : lineMaxRight_base;
        if (lineWidth_base > overallMaxRight_base)
            overallMaxRight_base = lineWidth_base;
    }

    sz.x = overallMaxRight_base * scale;
    sz.y = (float)lineCount * lineH_base * scale;
    return sz;
}

int leo_MeasureText(const char *text, int fontSize)
{
    if (!leo_IsFontReady(g_default_font))
        g_default_font = _load_default_font(fontSize > 0 ? fontSize : 16);

    leo_Vector2 m = leo_MeasureTextEx(g_default_font, text, (float)fontSize, 0.0f);
    return (int)(m.x + 0.5f);
}

int leo_GetFontLineHeight(leo_Font font, float fontSize)
{
    if (!leo_IsFontReady(font))
    {
        if (!leo_IsFontReady(g_default_font))
            g_default_font = _load_default_font(fontSize > 0 ? fontSize : 16);
        font = g_default_font;
    }

    if (font.baseSize <= 0)
        return 0;

    float scale = fontSize / (float)font.baseSize;
    if (scale <= 0.0f)
        return 0;

    return (int)(font.lineHeight * scale + 0.5f);
}

int leo_GetFontBaseSize(leo_Font font)
{
    return leo_IsFontReady(font) ? font.baseSize : 0;
}
