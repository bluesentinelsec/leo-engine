// =============================================
// leo/font.h — GPU-first font + text drawing
// - Single API (CPU/GPU abstracted; GPU preferred internally)
// - stb_truetype baked atlas -> SDL texture (opaque)
// - Raylib-style DrawText/Ex/Pro/DrawFPS
// - Measurement helpers
// - Default font management
// =============================================
#pragma once

#include "leo/export.h"
#include "leo/color.h"
#include "leo/engine.h"   // leo_Vector2
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Overview
// -----------------------------------------------------------------------------
// - This module is GPU-first: loaded fonts create a baked glyph atlas uploaded
//   to the GPU. Callers never see SDL types.
// - On failure, loaders return a zero-initialized leo_Font and set leo error.
// - DrawText* APIs render directly to the current render target.
// - A default font can be set/used for convenience (DrawText/DrawFPS).
// -----------------------------------------------------------------------------

// Public value handle; fields that start with "_" are opaque.
// Kept small to pass by value (like leo_Texture2D).
typedef struct leo_Font
{
    void* _atlas;      // internal GPU texture (SDL_Texture*)
    void* _glyphs;     // internal glyph metadata (implementation-defined)
    int   baseSize;    // pixel height used when baking (e.g., 32)
    int   lineHeight;  // recommended line advance at baseSize (pixels)
    int   glyphCount;  // number of baked glyphs
    int   _atlasW;     // internal (debug/info)
    int   _atlasH;     // internal (debug/info)
} leo_Font;

// -----------------------------------------------------------------------------
// Loading / Unloading (GPU-backed fonts)
// -----------------------------------------------------------------------------

// Load font from file at the given pixel height (baked atlas -> GPU).
// On error: returns zeroed font and sets leo error.
LEO_API leo_Font leo_LoadFont(const char* fileName, int pixelSize);

// Load font from an in-memory encoded TTF/OTF buffer.
// fileType is optional (".ttf"/"ttf"), used for diagnostics only.
LEO_API leo_Font leo_LoadFontFromMemory(const char* fileType,
                                        const unsigned char* data,
                                        int dataSize,
                                        int pixelSize);

// Release GPU atlas and internal glyph data. Safe on zeroed fonts.
LEO_API void     leo_UnloadFont(leo_Font* font);

// Quick validity check.
LEO_API bool     leo_IsFontReady(leo_Font font);

// -----------------------------------------------------------------------------
// Default font (used by DrawText / DrawFPS)
// -----------------------------------------------------------------------------

// Set or clear the global default font. Passing a zeroed font clears it.
LEO_API void     leo_SetDefaultFont(leo_Font font);

// Get the current default font (may be zeroed if unset).
LEO_API leo_Font leo_GetDefaultFont(void);

// -----------------------------------------------------------------------------
// Drawing
// -----------------------------------------------------------------------------

// Draw current FPS (uses default font if available).
LEO_API void     leo_DrawFPS(int posX, int posY);

// Draw text using the default font at integer position.
// fontSize in pixels; color tints the glyphs.
LEO_API void     leo_DrawText(const char* text,
                              int posX, int posY,
                              int fontSize,
                              leo_Color color);

// Draw text using an explicit font with subpixel position and extra spacing
// between glyphs (in pixels at the target size).
LEO_API void     leo_DrawTextEx(leo_Font font,
                                const char* text,
                                leo_Vector2 position,
                                float fontSize,
                                float spacing,
                                leo_Color tint);

// Draw text with origin and rotation (degrees). Rotation is applied around
// 'origin' relative to 'position' (Raylib-style).
LEO_API void     leo_DrawTextPro(leo_Font font,
                                 const char* text,
                                 leo_Vector2 position,
                                 leo_Vector2 origin,
                                 float rotation,
                                 float fontSize,
                                 float spacing,
                                 leo_Color tint);

// -----------------------------------------------------------------------------
// Measurement
// -----------------------------------------------------------------------------

// Measure the rendered size of 'text' for the given font/size/spacing.
// Returns (width, height) in pixels.
LEO_API leo_Vector2 leo_MeasureTextEx(leo_Font font,
                                      const char* text,
                                      float fontSize,
                                      float spacing);

// Convenience: width in pixels using the default font and integer size.
// Returns 0 if default font is not set/available.
LEO_API int      leo_MeasureText(const char* text, int fontSize);

// Advisory: line height in pixels at a given target fontSize.
LEO_API int      leo_GetFontLineHeight(leo_Font font, float fontSize);

// Base pixel size that the font was baked at (useful for scaling decisions).
LEO_API int      leo_GetFontBaseSize(leo_Font font);

#ifdef __cplusplus
} // extern "C"
#endif
