/**
 * @file engine.h
 * @brief Leo Engine core window, rendering, and timing interface
 *
 * Provides window management, frame loop control, 2D camera transforms,
 * render targets, and logical resolution scaling. All coordinates are
 * in pixels unless otherwise specified.
 */

#pragma once

#include "leo/color.h"
#include "leo/export.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief Opaque handle to the engine's window */
    typedef void *LeoWindow;

    /** @brief Opaque handle to the engine's renderer */
    typedef void *LeoRenderer;

    /** @brief Rectangle defined by position and size */
    typedef struct
    {
        float x, y, width, height;
    } leo_Rectangle;

    /** @brief 2D texture handle with dimensions */
    typedef struct
    {
        int width;
        int height;
        /** @brief Opaque handle to SDL texture - do not access directly */
        void *_handle;
    } leo_Texture2D;

    /** @brief Render target texture for offscreen rendering */
    typedef struct
    {
        leo_Texture2D texture; /**< Color texture for rendering results */
        int width;
        int height;
        /** @brief Opaque render target handle - do not access directly */
        void *_rt_handle;
    } leo_RenderTexture2D;

    /** @brief 2D vector with x,y components */
    typedef struct
    {
        float x, y;
    } leo_Vector2;

    /** @brief 2D camera for world-to-screen transforms */
    typedef struct
    {
        leo_Vector2 target; /**< World-space point the camera looks at */
        leo_Vector2 offset; /**< Screen-space offset where target appears */
        float rotation;     /**< Rotation in degrees, positive counterclockwise */
        float zoom;         /**< Zoom factor, 1.0 = no zoom */
    } leo_Camera2D;

    /**
     * @brief Initialize window and rendering context
     * @param width Window width in pixels
     * @param height Window height in pixels
     * @param title Window title string
     * @return true on success, false on failure
     *
     * Creates the main window and initializes SDL subsystems.
     * Must be called before any other engine functions.
     */
    LEO_API bool leo_InitWindow(int width, int height, const char *title);

    /**
     * @brief Close window and cleanup resources
     *
     * Destroys the window, renderer, and shuts down SDL.
     * Should be called at program exit.
     */
    LEO_API void leo_CloseWindow();

    /**
     * @brief Get opaque window handle
     * @return Window handle or NULL if not initialized
     */
    LEO_API LeoWindow leo_GetWindow(void);

    /**
     * @brief Get opaque renderer handle
     * @return Renderer handle or NULL if not initialized
     */
    LEO_API LeoRenderer leo_GetRenderer(void);

    /**
     * @brief Toggle fullscreen mode
     * @param enabled true for fullscreen, false for windowed
     * @return true on success, false on failure
     */
    LEO_API bool leo_SetFullscreen(bool enabled);

    /** @brief Window display modes */
    typedef enum leo_WindowMode
    {
        LEO_WINDOW_MODE_WINDOWED = 0,          /**< Normal windowed mode */
        LEO_WINDOW_MODE_BORDERLESS_FULLSCREEN, /**< Borderless fullscreen at desktop resolution */
        LEO_WINDOW_MODE_FULLSCREEN_EXCLUSIVE   /**< Exclusive fullscreen with mode switching */
    } leo_WindowMode;

    /**
     * @brief Set window display mode
     * @param mode Desired window mode
     * @return true on success, false on failure
     */
    LEO_API bool leo_SetWindowMode(leo_WindowMode mode);

    /**
     * @brief Get current window display mode
     * @return Current window mode
     */
    LEO_API leo_WindowMode leo_GetWindowMode(void);

    /**
     * @brief Check if window should close
     * @return true if close requested, false otherwise
     *
     * Processes window events and input. Call once per frame
     * in main loop condition.
     */
    LEO_API bool leo_WindowShouldClose(void);

    /**
     * @brief Clear render target with solid color
     * @param r Red component (0-255)
     * @param g Green component (0-255)
     * @param b Blue component (0-255)
     * @param a Alpha component (0-255)
     */
    LEO_API void leo_ClearBackground(int r, int g, int b, int a);

    /**
     * @brief Begin frame rendering
     *
     * Call at start of each frame before drawing operations.
     * Must be paired with leo_EndDrawing().
     */
    LEO_API void leo_BeginDrawing(void);

    /**
     * @brief End frame rendering and present
     *
     * Presents the frame and handles timing for target FPS.
     * Must be paired with leo_BeginDrawing().
     */
    LEO_API void leo_EndDrawing(void);

    /**
     * @brief Set target frame rate
     * @param fps Target frames per second (0 = unlimited)
     */
    LEO_API void leo_SetTargetFPS(int fps);

    /**
     * @brief Get last frame duration
     * @return Frame time in seconds
     */
    LEO_API float leo_GetFrameTime(void);

    /**
     * @brief Get elapsed time since initialization
     * @return Time in seconds since leo_InitWindow()
     */
    LEO_API double leo_GetTime(void);

    /**
     * @brief Get current frame rate
     * @return Current FPS
     */
    LEO_API int leo_GetFPS(void);

    /**
     * @brief Get screen width
     * @return Screen width in pixels (logical if enabled)
     */
    LEO_API int leo_GetScreenWidth(void);

    /**
     * @brief Get screen height
     * @return Screen height in pixels (logical if enabled)
     */
    LEO_API int leo_GetScreenHeight(void);

    /**
     * @brief Begin 2D camera transform
     * @param camera Camera configuration
     *
     * Applies world-to-screen transform for subsequent drawing.
     * Must be paired with leo_EndMode2D().
     */
    LEO_API void leo_BeginMode2D(leo_Camera2D camera);

    /**
     * @brief End 2D camera transform
     *
     * Restores previous camera state or identity transform.
     */
    LEO_API void leo_EndMode2D(void);

    /**
     * @brief Check if camera transform is active
     * @return true if inside BeginMode2D/EndMode2D block
     */
    LEO_API bool leo_IsCameraActive(void);

    /**
     * @brief Transform world coordinates to screen coordinates
     * @param position World-space position
     * @param camera Camera configuration
     * @return Screen-space position
     */
    LEO_API leo_Vector2 leo_GetWorldToScreen2D(leo_Vector2 position, leo_Camera2D camera);

    /**
     * @brief Transform screen coordinates to world coordinates
     * @param position Screen-space position
     * @param camera Camera configuration
     * @return World-space position
     */
    LEO_API leo_Vector2 leo_GetScreenToWorld2D(leo_Vector2 position, leo_Camera2D camera);

    /**
     * @brief Get current active camera
     * @return Current camera or identity camera if none active
     */
    LEO_API leo_Camera2D leo_GetCurrentCamera2D(void);

    /**
     * @brief Create render target texture
     * @param width Texture width in pixels
     * @param height Texture height in pixels
     * @return Render target handle
     *
     * Creates offscreen texture for rendering. Use with
     * leo_BeginTextureMode/leo_EndTextureMode.
     */
    LEO_API leo_RenderTexture2D leo_LoadRenderTexture(int width, int height);

    /**
     * @brief Destroy render target texture
     * @param target Render target to destroy
     */
    LEO_API void leo_UnloadRenderTexture(leo_RenderTexture2D target);

    /**
     * @brief Begin rendering to texture
     * @param target Render target texture
     *
     * Redirects rendering to offscreen texture.
     * Must be paired with leo_EndTextureMode().
     */
    LEO_API void leo_BeginTextureMode(leo_RenderTexture2D target);

    /**
     * @brief End rendering to texture
     *
     * Restores rendering to previous target (screen or parent texture).
     */
    LEO_API void leo_EndTextureMode(void);

    /**
     * @brief Draw texture rectangle
     * @param tex Source texture
     * @param src Source rectangle (negative width/height flips)
     * @param position Screen position to draw at
     * @param tint Color tint to apply
     *
     * Draws portion of texture at screen position. Respects active camera transform.
     */
    LEO_API void leo_DrawTextureRec(leo_Texture2D tex, leo_Rectangle src, leo_Vector2 position, leo_Color tint);

    /**
     * @brief Draw texture with full transform control
     * @param tex Source texture
     * @param src Source rectangle (negative width/height flips sampling)
     * @param dest Destination rectangle (negative width/height flips geometry)
     * @param origin Pivot point for rotation within destination
     * @param rotation Rotation in degrees
     * @param tint Color tint to apply
     *
     * Advanced texture drawing with scaling, rotation, and pivot control.
     */
    LEO_API void leo_DrawTexturePro(leo_Texture2D tex, leo_Rectangle src, leo_Rectangle dest, leo_Vector2 origin,
                                    float rotation, leo_Color tint);

    /** @brief Logical resolution presentation modes */
    typedef enum
    {
        LEO_LOGICAL_PRESENTATION_DISABLED = 0, /**< No logical scaling (passthrough) */
        LEO_LOGICAL_PRESENTATION_STRETCH,      /**< Stretch to fill window */
        LEO_LOGICAL_PRESENTATION_LETTERBOX,    /**< Preserve aspect ratio with letterboxing */
        LEO_LOGICAL_PRESENTATION_OVERSCAN      /**< Preserve aspect ratio with cropping */
    } leo_LogicalPresentation;

    /** @brief Texture scaling modes */
    typedef enum
    {
        LEO_SCALE_NEAREST = 0, /**< Nearest neighbor (pixelated) */
        LEO_SCALE_LINEAR,      /**< Linear filtering (smooth) */
        LEO_SCALE_PIXELART     /**< Pixel-art optimized scaling */
    } leo_ScaleMode;

    /**
     * @brief Set logical resolution and scaling
     * @param width Logical width (0 to disable)
     * @param height Logical height (0 to disable)
     * @param presentation How to fit logical resolution to window
     * @param scale Default texture filtering mode
     * @return true on success, false on failure
     *
     * Enables virtual resolution that scales to window size.
     * All coordinates become logical pixels when enabled.
     */
    LEO_API bool leo_SetLogicalResolution(int width, int height, leo_LogicalPresentation presentation,
                                          leo_ScaleMode scale);

#ifdef __cplusplus
}
#endif
