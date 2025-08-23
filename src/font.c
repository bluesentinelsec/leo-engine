// =============================================
// leo/font.c — Step 2: real loading + draw + measure
// =============================================
#include "leo/font.h"
#include "leo/error.h"
#include "leo/engine.h"

#include <SDL3/SDL.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb/stb_truetype.h>

// --------- ASCII range we bake for step 2 ----------
#define LEO_FONT_FIRST_CODEPOINT 32
#define LEO_FONT_DEFAULT_COUNT   95   // 32..126 inclusive

typedef struct _GlyphTable
{
	stbtt_bakedchar* baked; // baked glyphs [glyphCount]
	int first; // first codepoint (usually 32)
	int count; // number of glyphs (usually 95)
} _GlyphTable;

static leo_Font g_default_font = { 0 };

static inline SDL_Renderer* _ren(void)
{
	return (SDL_Renderer*)leo_GetRenderer();
}

static inline leo_Font _zero(void)
{
	leo_Font f;
	memset(&f, 0, sizeof(f));
	return f;
}

bool leo_IsFontReady(leo_Font font)
{
	return font._atlas != NULL && font._glyphs != NULL
	       && font.baseSize > 0 && font.glyphCount > 0 && font.lineHeight > 0;
}

// ---- internal load helper from TTF bytes ----
static leo_Font _load_from_ttf_bytes(const unsigned char* ttf, int ttfSize, int pixelSize)
{
	if (!ttf || ttfSize <= 0 || pixelSize <= 0)
	{
		leo_SetError("leo_LoadFont: bad args");
		return _zero();
	}
	SDL_Renderer* r = _ren();
	if (!r)
	{
		leo_SetError("leo_LoadFont: renderer is null");
		return _zero();
	}

	// Try up to 3 atlas sizes if the first is too small.
	int atlasW = 512, atlasH = 512;
	unsigned char* bitmap = NULL;
	stbtt_bakedchar* baked = NULL;
	int bakeRes = 0;

	int attempt;
	for (attempt = 0; attempt < 3; ++attempt)
	{
		free(bitmap);
		free(baked);
		bitmap = (unsigned char*)malloc((size_t)atlasW * atlasH);
		baked = (stbtt_bakedchar*)malloc(sizeof(stbtt_bakedchar) * LEO_FONT_DEFAULT_COUNT);
		if (!bitmap || !baked)
		{
			leo_SetError("leo_LoadFont: OOM");
			goto fail;
		}
		memset(bitmap, 0, (size_t)atlasW * atlasH);

		bakeRes = stbtt_BakeFontBitmap(ttf, 0, (float)pixelSize,
			bitmap, atlasW, atlasH,
			LEO_FONT_FIRST_CODEPOINT, LEO_FONT_DEFAULT_COUNT,
			baked);
		if (bakeRes > 0) break; // success
		atlasW *= 2;
		atlasH *= 2; // grow and try again
	}

	if (bakeRes <= 0)
	{
		leo_SetError("stbtt_BakeFontBitmap failed");
		goto fail;
	}

	// Expand grayscale -> RGBA (white text with alpha)
	unsigned char* rgba = (unsigned char*)malloc((size_t)atlasW * atlasH * 4);
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

	SDL_Texture* tex =
			SDL_CreateTexture(r, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, atlasW, atlasH);
	if (!tex)
	{
		leo_SetError("SDL_CreateTexture failed");
		free(rgba);
		goto fail;
	}
	SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);

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

	_GlyphTable* tbl = (_GlyphTable*)malloc(sizeof(_GlyphTable));
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
	out._atlas = (void*)tex;
	out._glyphs = (void*)tbl;
	out._atlasW = atlasW;
	out._atlasH = atlasH;
	out.baseSize = pixelSize;
	out.glyphCount = LEO_FONT_DEFAULT_COUNT;
	out.lineHeight = bakeRes; // stbtt returns pixel height used
	return out;

fail:
	free(bitmap);
	free(baked);
	return _zero();
}

leo_Font leo_LoadFont(const char* fileName, int pixelSize)
{
	if (!fileName || !*fileName || pixelSize <= 0)
	{
		leo_SetError("leo_LoadFont: invalid args");
		return _zero();
	}

	FILE* f = fopen(fileName, "rb");
	if (!f)
	{
		leo_SetError("leo_LoadFont: fopen failed");
		return _zero();
	}
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	unsigned char* buf = (unsigned char*)malloc((size_t)sz);
	if (!buf)
	{
		fclose(f);
		leo_SetError("leo_LoadFont: OOM");
		return _zero();
	}
	fread(buf, 1, (size_t)sz, f);
	fclose(f);

	leo_Font font = _load_from_ttf_bytes(buf, (int)sz, pixelSize);
	free(buf);
	return font;
}

leo_Font leo_LoadFontFromMemory(const char* fileType,
	const unsigned char* data, int dataSize,
	int pixelSize)
{
	if (!data || dataSize <= 0 || pixelSize <= 0)
	{
		leo_SetError("leo_LoadFontFromMemory: invalid args");
		return _zero();
	}
	return _load_from_ttf_bytes(data, dataSize, pixelSize);
}

void leo_UnloadFont(leo_Font* font)
{
	if (!font) return;
	if (font->_atlas) SDL_DestroyTexture((SDL_Texture*)font->_atlas);
	if (font->_glyphs)
	{
		_GlyphTable* tbl = (_GlyphTable*)font->_glyphs;
		free(tbl->baked);
		free(tbl);
	}
	*font = _zero();
}

void leo_SetDefaultFont(leo_Font font) { g_default_font = font; }
leo_Font leo_GetDefaultFont(void) { return g_default_font; }

// ------------- internal draw core ----------------
static void _draw_text_impl(leo_Font font, const char* text,
	float x, float y, float fontSize, float spacing,
	leo_Color tint, float rotationDeg,
	float originX, float originY)
{
	if (!text || !*text) return;
	if (!leo_IsFontReady(font)) return;

	SDL_Renderer* r = _ren();
	if (!r) return;

	SDL_Texture* atlas = (SDL_Texture*)font._atlas;
	_GlyphTable* tbl = (_GlyphTable*)font._glyphs;
	stbtt_bakedchar* cdata = tbl->baked;

	// Apply tint
	SDL_SetTextureColorMod(atlas, (Uint8)tint.r, (Uint8)tint.g, (Uint8)tint.b);
	SDL_SetTextureAlphaMod(atlas, (Uint8)tint.a);

	float scale = fontSize / (float)font.baseSize;
	if (scale <= 0.0f) return;

	float penX = x;
	float penY = y;

	const double angle = (double)rotationDeg;
	const bool doRotate = (rotationDeg != 0.0f);

	const unsigned char* p;
	for (p = (const unsigned char*)text; *p; ++p)
	{
		int ch = (int)*p;
		if (ch < tbl->first || ch >= tbl->first + tbl->count)
		{
			// Skip unsupported glyphs (ASCII-only in step 2).
			continue;
		}

		stbtt_aligned_quad q;
		float qx = penX, qy = penY;
		stbtt_GetBakedQuad(cdata, font._atlasW, font._atlasH, ch - tbl->first, &qx, &qy, &q, 1);

		// Scale the quad
		float dx0 = (q.x0 - x) * scale + x;
		float dy0 = (q.y0 - y) * scale + y;
		float dx1 = (q.x1 - x) * scale + x;
		float dy1 = (q.y1 - y) * scale + y;

		SDL_FRect src = {
			q.s0 * font._atlasW,
			q.t0 * font._atlasH,
			(q.s1 - q.s0) * font._atlasW,
			(q.t1 - q.t0) * font._atlasH
		};
		SDL_FRect dst = {
			dx0,
			dy0,
			dx1 - dx0,
			dy1 - dy0
		};

		if (doRotate)
		{
			SDL_FPoint center = { originX, originY };
			SDL_RenderTextureRotated(r, atlas, &src, &dst, angle, &center, SDL_FLIP_NONE);
		}
		else
		{
			SDL_RenderTexture(r, atlas, &src, &dst);
		}

		// Advance pen: stbtt_GetBakedQuad updated qx,qy as if scale==1; re-scale advance.
		// Apply extra spacing only if there is a next character.
		if (*(p + 1) != '\0')
		{
			penX = x + (qx - x) * scale + spacing;
		}
		else
		{
			penX = x + (qx - x) * scale;
		}
		penY = y + (qy - y) * scale;
	}
}

void leo_DrawFPS(int x, int y)
{
	// Only draw if there’s a default font; otherwise be a no-op (deterministic).
	if (!leo_IsFontReady(g_default_font)) return;

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

void leo_DrawText(const char* text, int posX, int posY, int fontSize, leo_Color color)
{
	if (!leo_IsFontReady(g_default_font))
	{
		// Deterministic: no hidden auto-load. Just no-op.
		return;
	}
	_draw_text_impl(g_default_font, text, (float)posX, (float)posY,
		(float)fontSize, 0.0f, color, 0.0f, 0.0f, 0.0f);
}

void leo_DrawTextEx(leo_Font font, const char* text,
	leo_Vector2 position, float fontSize, float spacing, leo_Color tint)
{
	_draw_text_impl(font, text, position.x, position.y, fontSize, spacing, tint, 0.0f, 0.0f, 0.0f);
}

void leo_DrawTextPro(leo_Font font, const char* text,
	leo_Vector2 position, leo_Vector2 origin, float rotation,
	float fontSize, float spacing, leo_Color tint)
{
	_draw_text_impl(font, text, position.x, position.y, fontSize, spacing, tint, rotation, origin.x, origin.y);
}

// ---------- measurement ----------
leo_Vector2 leo_MeasureTextEx(leo_Font font, const char* text, float fontSize, float spacing)
{
	leo_Vector2 sz = { 0, 0 };
	if (!text || !*text || !leo_IsFontReady(font) || font.baseSize <= 0) return sz;

	_GlyphTable* tbl = (_GlyphTable*)font._glyphs;
	stbtt_bakedchar* cdata = tbl->baked;

	float scale = fontSize / (float)font.baseSize;
	float x = 0.0f, y = 0.0f;
	float maxRight = 0.0f;

	const unsigned char* p;
	for (p = (const unsigned char*)text; *p; ++p)
	{
		int ch = (int)*p;
		if (ch < tbl->first || ch >= tbl->first + tbl->count) continue;

		stbtt_aligned_quad q;
		float qx = x, qy = y;
		stbtt_GetBakedQuad(cdata, font._atlasW, font._atlasH, ch - tbl->first, &qx, &qy, &q, 1);

		float w = (q.x1 - q.x0) * scale;
		float right = (q.x0 * scale) + w;
		if (right > maxRight) maxRight = right;

		x = (qx) * scale; // advance at target size
		if (*(p + 1) != '\0')
		{
			x += spacing; // spacing only between glyphs
		}
		y = (qy) * scale;
	}

	sz.x = (x > maxRight) ? x : maxRight;
	sz.y = (float)font.lineHeight * scale;
	return sz;
}

int leo_MeasureText(const char* text, int fontSize)
{
	if (!leo_IsFontReady(g_default_font)) return 0;
	leo_Vector2 m = leo_MeasureTextEx(g_default_font, text, (float)fontSize, 0.0f);
	return (int)(m.x + 0.5f);
}

int leo_GetFontLineHeight(leo_Font font, float fontSize)
{
	if (!leo_IsFontReady(font)) return 0;
	if (font.baseSize <= 0) return 0;
	float scale = fontSize / (float)font.baseSize;
	if (scale <= 0.0f) return 0;
	return (int)(font.lineHeight * scale + 0.5f);
}

int leo_GetFontBaseSize(leo_Font font)
{
	return leo_IsFontReady(font) ? font.baseSize : 0;
}
