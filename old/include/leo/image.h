// =============================================
// leo/image.h  — GPU-focused image/texture API
// =============================================
#pragma once

#include "leo/engine.h" // for leo_Texture2D
#include "leo/export.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // -----------------------------------------------------------------------------
    // Overview
    // -----------------------------------------------------------------------------
    // - This module is GPU-first: all "load" functions produce leo_Texture2D
    //   objects (resident in GPU memory), suitable for immediate drawing.
    // - Decoding (PNG/JPG/etc.) happens internally; callers never see SDL types.
    // - Ownership: textures returned by the loaders must be released via
    //   leo_UnloadTexture().
    // - On failure, loaders return a zero-initialized leo_Texture2D (width/height
    // 0).
    // -----------------------------------------------------------------------------

    // Optional pixel format for raw uploads (kept minimal for now).
    typedef enum
    {
        LEO_TEXFORMAT_UNDEFINED = 0,
        LEO_TEXFORMAT_R8G8B8A8,   // 32bpp RGBA (most common)
        LEO_TEXFORMAT_R8G8B8,     // 24bpp RGB (will be expanded on upload)
        LEO_TEXFORMAT_GRAY8,      // 8bpp grayscale (will be expanded on upload)
        LEO_TEXFORMAT_GRAY8_ALPHA // 16bpp GA (expanded on upload)
    } leo_TexFormat;

    // -----------------------------------------------------------------------------
    // Loading / Unloading (GPU textures)
    // -----------------------------------------------------------------------------

    // Load texture directly from an image file (PNG, JPG, etc.)
    // - Decodes file, uploads to GPU, and returns a ready-to-use leo_Texture2D.
    // - On error, returns a zeroed texture and sets leo error state.
    LEO_API leo_Texture2D leo_LoadTexture(const char *fileName);

    // Load texture from an in-memory encoded file (e.g., a PNG/JPG buffer).
    // - fileType is the extension (".png" or "png") used to select the decoder.
    // - data is copied/decoded internally.
    LEO_API leo_Texture2D leo_LoadTextureFromMemory(const char *fileType, const unsigned char *data, int dataSize);

    // Create a texture by copying an existing GPU texture (GPU -> GPU).
    // - Useful to duplicate, make immutable copies, or snapshot transient textures.
    LEO_API leo_Texture2D leo_LoadTextureFromTexture(leo_Texture2D source);

    // Create a texture from raw pixel data already in CPU memory.
    // - pixels: pointer to top-left pixel of row 0
    // - width/height: image dimensions in pixels
    // - pitch: bytes per row (>= width * bytesPerPixel). Use 0 for tightly packed.
    // - format: format of the provided CPU pixels (will be normalized/uploaded).
    LEO_API leo_Texture2D leo_LoadTextureFromPixels(const void *pixels, int width, int height, int pitch,
                                                    leo_TexFormat format);

    // Release a GPU texture created by any of the functions above.
    // - Safe to call with a zeroed texture (no-op).
    LEO_API void leo_UnloadTexture(leo_Texture2D *texture);

    // Quick validity check: returns true if texture appears usable (non-zero dims).
    LEO_API bool leo_IsTextureReady(leo_Texture2D texture);

    // -----------------------------------------------------------------------------
    // Optional helpers (query/utility)
    // -----------------------------------------------------------------------------

    // Compute bytes-per-pixel for a CPU-side format (for raw uploads).
    // Returns 0 for unsupported/undefined formats.
    LEO_API int leo_TexFormatBytesPerPixel(leo_TexFormat format);

#ifdef __cplusplus
} // extern "C"
#endif
