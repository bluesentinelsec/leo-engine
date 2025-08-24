// =============================================
// leo/image.c — GPU-focused image/texture impl
// =============================================

#include "leo/image.h"
#include "leo/error.h"
#include "leo/engine.h"
#include "leo/io.h"

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// stb_image (single TU)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_THREAD_LOCALS
#include <stb/stb_image.h>

// -------------------------------
// Small internal helpers
// -------------------------------

static inline leo_Texture2D _zero_tex(void)
{
	leo_Texture2D t;
	t.width = 0;
	t.height = 0;
	t._handle = NULL;
	return t;
}

static inline SDL_Renderer* _ren(void)
{
	return (SDL_Renderer*)leo_GetRenderer();
}

static inline void _apply_default_texture_state(SDL_Texture* tex)
{
	// Default to pixel-art friendly sampling; ignore failures.
	if (tex) SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
	// You might also choose to set blend mode here if your engine expects it:
	// SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
}

static inline leo_Texture2D _wrap_tex(SDL_Texture* sdltex, int w, int h)
{
	leo_Texture2D t = _zero_tex();
	if (!sdltex) return t;

	// Prefer caller-provided dimensions; fall back to querying SDL just in case.
	if (w <= 0 || h <= 0)
	{
		float fw = 0, fh = 0;
		if (SDL_GetTextureSize(sdltex, &fw, &fh))
		{
			w = (int)fw;
			h = (int)fh;
		}
	}
	t.width = w;
	t.height = h;
	t._handle = (void*)sdltex;
	return t;
}

static SDL_Texture* _create_tex_rgba_static(SDL_Renderer* r, int w, int h)
{
	return SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, w, h);
}

static SDL_Texture* _create_tex_rgba_target(SDL_Renderer* r, int w, int h)
{
	return SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, w, h);
}

static leo_Texture2D _upload_rgba(SDL_Renderer* r, const void* pixels, int w, int h, int pitch)
{
	if (!r || !pixels || w <= 0 || h <= 0) return _zero_tex();

	SDL_Texture* tex = _create_tex_rgba_static(r, w, h);
	if (!tex)
	{
		leo_SetError("SDL_CreateTexture failed: %s", SDL_GetError());
		return _zero_tex();
	}

	const int row_pitch = (pitch > 0) ? pitch : (w * 4);
	if (!SDL_UpdateTexture(tex, NULL, pixels, row_pitch))
	{
		leo_SetError("SDL_UpdateTexture failed: %s", SDL_GetError());
		SDL_DestroyTexture(tex);
		return _zero_tex();
	}

	_apply_default_texture_state(tex);
	return _wrap_tex(tex, w, h);
}

// Try to load an encoded image via Leo VFS. Returns RGBA8 pixels (malloc'd by STB) on success.
// On failure, returns NULL (caller may fall back to direct file I/O).
static stbi_uc* _stbi_load_from_vfs_rgba(const char* logicalName, int* w, int* h, int* comp)
{
	if (!logicalName || !*logicalName) return NULL;

	size_t need = 0;
	// Probe size first (buffer==NULL -> size in out_total).
	if (leo_ReadAsset(logicalName, NULL, 0, &need) != 0 || need == 0)
	{
		// If leo_ReadAsset returned non-zero here it means “probe only” succeeded. We just keep going.
	}
	if (need == 0) return NULL; // not found in mounts

	unsigned char* buf = (unsigned char*)malloc(need);
	if (!buf)
	{
		leo_SetError("leo_LoadTexture: OOM reading '%s' from VFS", logicalName);
		return NULL;
	}
	size_t got = leo_ReadAsset(logicalName, buf, need, NULL);
	if (got != need)
	{
		free(buf);
		return NULL;
	}
	stbi_uc* px = stbi_load_from_memory((const stbi_uc*)buf, (int)need, w, h, comp, 4);
	free(buf);
	return px;
}
// -------------------------------
// Public API
// -------------------------------

int leo_TexFormatBytesPerPixel(leo_TexFormat format)
{
	switch (format)
	{
	case LEO_TEXFORMAT_R8G8B8A8: return 4;
	case LEO_TEXFORMAT_R8G8B8: return 3;
	case LEO_TEXFORMAT_GRAY8: return 1;
	case LEO_TEXFORMAT_GRAY8_ALPHA: return 2;
	default: return 0;
	}
}

bool leo_IsTextureReady(leo_Texture2D texture)
{
	return texture._handle != NULL && texture.width > 0 && texture.height > 0;
}

void leo_UnloadTexture(leo_Texture2D* texture)
{
	if (!texture) return;
	if (texture->_handle)
	{
		SDL_DestroyTexture((SDL_Texture*)texture->_handle);
	}
	texture->_handle = NULL;
	texture->width = 0;
	texture->height = 0;
}

// Load from file (PNG/JPG/etc.)
leo_Texture2D leo_LoadTexture(const char* fileName)
{
	if (!fileName || !*fileName) {
		leo_SetError("leo_LoadTexture: invalid fileName");
		return _zero_tex();
	}

	int w = 0, h = 0, n = 0;
	stbi_uc* px = NULL;

	// 1) Try VFS (logical path inside mounted packs/dirs).
	px = _stbi_load_from_vfs_rgba(fileName, &w, &h, &n);

	// 2) Fallback to filesystem path for back-compat.
	if (!px) {
		px = stbi_load(fileName, &w, &h, &n, 4); // force RGBA8
	}
	if (!px) {
		leo_SetError("leo_LoadTexture: not found or unsupported image '%s'", fileName);
		return _zero_tex();
	}

	SDL_Renderer* r = _ren();
	if (!r)
	{
		leo_SetError("leo_LoadTexture: renderer is null");
		stbi_image_free(px);
		return _zero_tex();
	}

	leo_Texture2D tex = _upload_rgba(r, px, w, h, w * 4);
	stbi_image_free(px);
	return tex;
}

// Load from encoded buffer in memory
leo_Texture2D leo_LoadTextureFromMemory(const char* fileType,
	const unsigned char* data,
	int dataSize)
{
	(void)fileType; // decoding is automatic; we don't need the extension here

	if (!data || dataSize <= 0)
	{
		leo_SetError("leo_LoadTextureFromMemory: invalid buffer");
		return _zero_tex();
	}

	int w = 0, h = 0, n = 0;
	stbi_uc* px = stbi_load_from_memory((const stbi_uc*)data, dataSize, &w, &h, &n, 4);
	if (!px)
	{
		leo_SetError("stbi_load_from_memory failed");
		return _zero_tex();
	}

	SDL_Renderer* r = _ren();
	if (!r)
	{
		leo_SetError("leo_LoadTextureFromMemory: renderer is null");
		stbi_image_free(px);
		return _zero_tex();
	}

	leo_Texture2D tex = _upload_rgba(r, px, w, h, w * 4);
	stbi_image_free(px);
	return tex;
}

// GPU -> GPU copy
leo_Texture2D leo_LoadTextureFromTexture(leo_Texture2D source)
{
	if (!leo_IsTextureReady(source))
	{
		leo_SetError("leo_LoadTextureFromTexture: source not ready");
		return _zero_tex();
	}

	SDL_Renderer* r = _ren();
	if (!r)
	{
		leo_SetError("leo_LoadTextureFromTexture: renderer is null");
		return _zero_tex();
	}

	const int w = source.width;
	const int h = source.height;

	SDL_Texture* dst = _create_tex_rgba_target(r, w, h);
	if (!dst)
	{
		leo_SetError("SDL_CreateTexture(TARGET) failed: %s", SDL_GetError());
		return _zero_tex();
	}
	_apply_default_texture_state(dst);

	// Render source into destination render-target
	SDL_Texture* prev_target = SDL_GetRenderTarget(r);
	if (!SDL_SetRenderTarget(r, dst))
	{
		leo_SetError("SDL_SetRenderTarget failed: %s", SDL_GetError());
		SDL_DestroyTexture(dst);
		return _zero_tex();
	}

	// Clear is optional; exact copy will overwrite all pixels.
	// SDL_SetRenderDrawColor(r, 0, 0, 0, 0); SDL_RenderClear(r);

	if (!SDL_RenderTexture(r, (SDL_Texture*)source._handle, NULL, NULL))
	{
		leo_SetError("SDL_RenderTexture (copy) failed: %s", SDL_GetError());
		// Restore target before destroying
		SDL_SetRenderTarget(r, prev_target);
		SDL_DestroyTexture(dst);
		return _zero_tex();
	}

	// Present to the texture (flush). In SDL3 this is implicit per draw call;
	// keep API usage minimal and just restore the previous target.
	SDL_SetRenderTarget(r, prev_target);

	return _wrap_tex(dst, w, h);
}

// Upload from raw CPU pixel buffer
leo_Texture2D leo_LoadTextureFromPixels(const void* pixels,
	int width,
	int height,
	int pitch,
	leo_TexFormat format)
{
	if (!pixels || width <= 0 || height <= 0)
	{
		leo_SetError("leo_LoadTextureFromPixels: invalid args");
		return _zero_tex();
	}

	SDL_Renderer* r = _ren();
	if (!r)
	{
		leo_SetError("leo_LoadTextureFromPixels: renderer is null");
		return _zero_tex();
	}

	const int src_bpp = leo_TexFormatBytesPerPixel(format);
	if (src_bpp == 0)
	{
		leo_SetError("leo_LoadTextureFromPixels: unsupported format");
		return _zero_tex();
	}

	// If already RGBA8 and tightly packed, upload directly; otherwise expand.
	if (format == LEO_TEXFORMAT_R8G8B8A8)
	{
		const int src_pitch = (pitch > 0) ? pitch : (width * 4);
		return _upload_rgba(r, pixels, width, height, src_pitch);
	}

	// Expand to RGBA8
	const size_t dst_pitch = (size_t)width * 4;
	const size_t dst_size = (size_t)height * dst_pitch;
	uint8_t* dst = (uint8_t*)malloc(dst_size);
	if (!dst)
	{
		leo_SetError("leo_LoadTextureFromPixels: OOM");
		return _zero_tex();
	}
	const uint8_t* src = (const uint8_t*)pixels;
	const int src_pitch = (pitch > 0) ? pitch : (width * src_bpp);

	for (int y = 0; y < height; ++y)
	{
		const uint8_t* srow = src + (size_t)y * src_pitch;
		uint8_t* drow = dst + (size_t)y * dst_pitch;

		switch (format)
		{
		case LEO_TEXFORMAT_R8G8B8:
		{
			for (int x = 0; x < width; ++x)
			{
				const uint8_t r8 = srow[x * 3 + 0];
				const uint8_t g8 = srow[x * 3 + 1];
				const uint8_t b8 = srow[x * 3 + 2];
				drow[x * 4 + 0] = r8;
				drow[x * 4 + 1] = g8;
				drow[x * 4 + 2] = b8;
				drow[x * 4 + 3] = 255;
			}
		}
		break;

		case LEO_TEXFORMAT_GRAY8:
		{
			for (int x = 0; x < width; ++x)
			{
				const uint8_t g = srow[x];
				drow[x * 4 + 0] = g;
				drow[x * 4 + 1] = g;
				drow[x * 4 + 2] = g;
				drow[x * 4 + 3] = 255;
			}
		}
		break;

		case LEO_TEXFORMAT_GRAY8_ALPHA:
		{
			for (int x = 0; x < width; ++x)
			{
				const uint8_t g = srow[x * 2 + 0];
				const uint8_t a = srow[x * 2 + 1];
				drow[x * 4 + 0] = g;
				drow[x * 4 + 1] = g;
				drow[x * 4 + 2] = g;
				drow[x * 4 + 3] = a;
			}
		}
		break;

		default:
			free(dst);
			leo_SetError("leo_LoadTextureFromPixels: unhandled format");
			return _zero_tex();
		}
	}

	leo_Texture2D tex = _upload_rgba(r, dst, width, height, (int)dst_pitch);
	free(dst);
	return tex;
}
