// engine.c — window, timing, 2D camera, render textures, drawing helpers

#include "leo/engine.h"
#include "leo/color.h"
#include "leo/error.h"
#include "leo/gamepad.h"
#include "leo/keyboard.h"
#include "leo/mouse.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_stdinc.h>
#include <math.h>

SDL_Window *globalWindow = NULL;
SDL_Renderer *globalRenderer = NULL;

#define LEO_RT_STACK_MAX 8
#ifndef LEO_RT_STACK_MAX
#define LEO_RT_STACK_MAX 8
#endif

// Consolidated internal engine state.
typedef struct EngineState
{
    /* frame/loop */
    int inFrame;                          /* are we between Begin/EndDrawing? */
    int quit;                             /* latched quit flag */
    Uint8 clearR, clearG, clearB, clearA; /* last clear color */

    /* timing */
    int targetFPS;
    double targetFrameSecs;

    Uint64 perfFreq;
    Uint64 startCounter;      /* when InitWindow succeeded */
    Uint64 frameStartCounter; /* set in BeginDrawing */

    float lastFrameTime;   /* seconds */
    int fpsCounter;        /* frames counted in the current 1s window */
    int currentFPS;        /* last computed FPS value */
    Uint64 fpsWindowStart; /* start counter for FPS window */

    /* camera */
    leo_Camera2D camStack[8];
    int camTop;
    bool cameraActive; /* true when inside BeginMode2D/EndMode2D */

    /* Current affine: screen <- world (row-major 2x3) */
    float m11, m12, tx;
    float m21, m22, ty;

    /* logical resolution */
    int hasLogical;
    int logicalW;
    int logicalH;

    /* default per-texture scale mode (applied on creation) */
    SDL_ScaleMode defaultScaleMode;

    /* render-target stack */
    SDL_Texture *rtStack[LEO_RT_STACK_MAX];
    int rtTop;

    /* window mode tracking */
    leo_WindowMode currentWindowMode;
} EngineState;

static EngineState s_state = {
    .inFrame = 0,
    .quit = 0,
    .clearR = 0,
    .clearG = 0,
    .clearB = 0,
    .clearA = 255,
    .targetFPS = 0,
    .targetFrameSecs = 0.0,
    .perfFreq = 0,
    .startCounter = 0,
    .frameStartCounter = 0,
    .lastFrameTime = 0.0f,
    .fpsCounter = 0,
    .currentFPS = 0,
    .fpsWindowStart = 0,
    .camTop = -1,
    .cameraActive = false,
    .m11 = 1.0f,
    .m12 = 0.0f,
    .tx = 0.0f,
    .m21 = 0.0f,
    .m22 = 1.0f,
    .ty = 0.0f,
    .hasLogical = 0,
    .logicalW = 0,
    .logicalH = 0,
    .defaultScaleMode = SDL_SCALEMODE_LINEAR,
    .rtTop = -1,
    .currentWindowMode = LEO_WINDOW_MODE_WINDOWED,
};

static inline float _deg2rad(float deg)
{
    return deg * 0.01745329251994329577f;
} // pi/180

static void _rebuildCameraMatrix(const leo_Camera2D *c)
{
    if (!c)
    {
        s_state.m11 = 1.0f;
        s_state.m12 = 0.0f;
        s_state.tx = 0.0f;
        s_state.m21 = 0.0f;
        s_state.m22 = 1.0f;
        s_state.ty = 0.0f;
        return;
    }
    const float z = (c->zoom <= 0.0f) ? 1.0f : c->zoom;
    const float rad = _deg2rad(c->rotation);
    const float cr = cosf(rad), sr = sinf(rad);

    const float a = z * cr;    // m11
    const float b = z * sr;    // m12
    const float c21 = -z * sr; // m21
    const float d = z * cr;    // m22

    const float Txx = c->offset.x - (a * c->target.x + b * c->target.y);
    const float Tyy = c->offset.y - (c21 * c->target.x + d * c->target.y);

    s_state.m11 = a;
    s_state.m12 = b;
    s_state.tx = Txx;
    s_state.m21 = c21;
    s_state.m22 = d;
    s_state.ty = Tyy;
}

static void _buildCam3x2(const leo_Camera2D *c, float *m11, float *m12, float *m21, float *m22, float *tx, float *ty)
{
    const float z = (c->zoom <= 0.0f) ? 1.0f : c->zoom;
    const float rad = _deg2rad(c->rotation);
    const float cr = cosf(rad), sr = sinf(rad);
    const float a = z * cr, b = z * sr, c21 = -z * sr, d = z * cr;
    const float Txx = c->offset.x - (a * c->target.x + b * c->target.y);
    const float Tyy = c->offset.y - (c21 * c->target.x + d * c->target.y);
    if (m11)
        *m11 = a;
    if (m12)
        *m12 = b;
    if (m21)
        *m21 = c21;
    if (m22)
        *m22 = d;
    if (tx)
        *tx = Txx;
    if (ty)
        *ty = Tyy;
}

/* render-target stack is stored in s_state.rtStack / s_state.rtTop */
static void _rtPush(SDL_Texture *t)
{
    if (s_state.rtTop + 1 < LEO_RT_STACK_MAX)
        ++s_state.rtTop;
    s_state.rtStack[s_state.rtTop] = t;
}

static SDL_Texture *_rtPop(void)
{
    if (s_state.rtTop < 0)
        return NULL;
    SDL_Texture *t = s_state.rtStack[s_state.rtTop];
    --s_state.rtTop;
    return t;
}

// Window & loop
bool leo_InitWindow(int width, int height, const char *title)
{
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Initializing window: %dx%d '%s'", width, height, title);

    if (width <= 0 || height <= 0)
    {
        leo_SetError("Invalid window dimensions: width=%d, height=%d", width, height);
        return false;
    }
    if (!title)
    {
        leo_SetError("Invalid window title: null pointer");
        return false;
    }

    SDL_InitFlags flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS;
    if (!SDL_Init(flags))
    {
        leo_SetError("%s\n", SDL_GetError());
        return false;
    }

    Uint64 windowFlags = SDL_WINDOW_RESIZABLE;
    globalWindow = SDL_CreateWindow(title, width, height, windowFlags);
    if (!globalWindow)
    {
        leo_SetError("%s\n", SDL_GetError());
        return false;
    }

    globalRenderer = SDL_CreateRenderer(globalWindow, NULL);
    if (!globalRenderer)
    {
        leo_SetError("%s\n", SDL_GetError());
        SDL_DestroyWindow(globalWindow);
        globalWindow = NULL;
        return false;
    }

    s_state.inFrame = 0;
    s_state.quit = 0;
    s_state.clearR = 0;
    s_state.clearG = 0;
    s_state.clearB = 0;
    s_state.clearA = 255;

    s_state.perfFreq = SDL_GetPerformanceFrequency();
    s_state.startCounter = SDL_GetPerformanceCounter();
    s_state.frameStartCounter = s_state.startCounter;
    s_state.fpsWindowStart = s_state.startCounter;
    s_state.lastFrameTime = 0.0f;
    s_state.fpsCounter = 0;
    s_state.currentFPS = 0;
    s_state.targetFPS = 0;
    s_state.targetFrameSecs = 0.0;

    s_state.camTop = -1;
    s_state.cameraActive = false;
    _rebuildCameraMatrix(NULL);

    s_state.rtTop = -1;

    // logical resolution: start disabled
    s_state.hasLogical = 0;
    s_state.logicalW = 0;
    s_state.logicalH = 0;

    // default per-texture scale mode
    s_state.defaultScaleMode = SDL_SCALEMODE_LINEAR;

    // window mode: start in windowed mode
    s_state.currentWindowMode = LEO_WINDOW_MODE_WINDOWED;

    leo_InitMouse();
    leo_InitGamepads();

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Window initialization complete");
    return true;
}

void leo_CloseWindow()
{
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Shutting down window and renderer");

    if (globalRenderer)
    {
        SDL_DestroyRenderer(globalRenderer);
        globalRenderer = NULL;
    }
    if (globalWindow)
    {
        SDL_DestroyWindow(globalWindow);
        globalWindow = NULL;
    }

    s_state.inFrame = 0;
    s_state.quit = 0;

    s_state.perfFreq = 0;
    s_state.startCounter = 0;
    s_state.frameStartCounter = 0;
    s_state.fpsWindowStart = 0;
    s_state.lastFrameTime = 0.0f;
    s_state.fpsCounter = 0;
    s_state.currentFPS = 0;
    s_state.targetFPS = 0;
    s_state.targetFrameSecs = 0.0;

    s_state.camTop = -1;
    s_state.cameraActive = false;
    _rebuildCameraMatrix(NULL);
    s_state.rtTop = -1;

    // logical resolution reset
    s_state.hasLogical = 0;
    s_state.logicalW = 0;
    s_state.logicalH = 0;

    leo_CleanupKeyboard();
    leo_ShutdownMouse();
    leo_ShutdownGamepads();
    SDL_Quit();
}

LeoWindow leo_GetWindow(void)
{
    return (LeoWindow)globalWindow;
}

LeoRenderer leo_GetRenderer(void)
{
    return (LeoRenderer)globalRenderer;
}

bool leo_SetFullscreen(bool enabled)
{
    if (!globalWindow)
        return false;

    Uint64 flags = SDL_GetWindowFlags(globalWindow);
    bool isFullscreen = (flags & SDL_WINDOW_FULLSCREEN) != 0;
    if (enabled == isFullscreen)
    {
        // Already in the desired state → do nothing
        return true;
    }

    if (enabled)
    {
        if (!SDL_SetWindowFullscreen(globalWindow, true))
        {
            leo_SetError("%s", SDL_GetError());
            return false;
        }
    }
    else
    {
        if (!SDL_SetWindowFullscreen(globalWindow, false))
        {
            leo_SetError("%s", SDL_GetError());
            return false;
        }
    }
    return true;
}

bool leo_SetWindowMode(leo_WindowMode mode)
{
    const char *mode_names[] = {"windowed", "fullscreen", "borderless"};
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Setting window mode: %s",
                (mode >= 0 && mode < 3) ? mode_names[mode] : "unknown");

    if (!globalWindow)
    {
        leo_SetError("leo_SetWindowMode called before leo_InitWindow");
        return false;
    }

    if (s_state.currentWindowMode == mode)
    {
        return true; // Already in desired mode
    }

    switch (mode)
    {
    case LEO_WINDOW_MODE_WINDOWED:
        if (!SDL_SetWindowFullscreen(globalWindow, false))
        {
            leo_SetError("%s", SDL_GetError());
            return false;
        }
        break;

    case LEO_WINDOW_MODE_BORDERLESS_FULLSCREEN:
        // Clear any exclusive fullscreen mode first
        if (!SDL_SetWindowFullscreenMode(globalWindow, NULL))
        {
            leo_SetError("%s", SDL_GetError());
            return false;
        }

        // For borderless fullscreen, we need to:
        // 1. Make window borderless
        // 2. Set it to desktop size
        // 3. Position it at (0,0)
        SDL_SetWindowBordered(globalWindow, false);

        // Get desktop display mode for sizing
        SDL_DisplayID displayID = SDL_GetDisplayForWindow(globalWindow);
        if (displayID == 0)
        {
            leo_SetError("Failed to get display for window: %s", SDL_GetError());
            return false;
        }

        const SDL_DisplayMode *desktopMode = SDL_GetDesktopDisplayMode(displayID);
        if (!desktopMode)
        {
            leo_SetError("Failed to get desktop display mode: %s", SDL_GetError());
            return false;
        }

        // Set window size to desktop size and position at (0,0)
        SDL_SetWindowSize(globalWindow, desktopMode->w, desktopMode->h);
        SDL_SetWindowPosition(globalWindow, 0, 0);

        // Raise window to top
        SDL_RaiseWindow(globalWindow);
        break;

    case LEO_WINDOW_MODE_FULLSCREEN_EXCLUSIVE: {
        // Get current display
        SDL_DisplayID displayID = SDL_GetDisplayForWindow(globalWindow);
        if (displayID == 0)
        {
            leo_SetError("Failed to get display for window: %s", SDL_GetError());
            return false;
        }

        // Get desktop display mode
        const SDL_DisplayMode *desktopMode = SDL_GetDesktopDisplayMode(displayID);
        if (!desktopMode)
        {
            leo_SetError("Failed to get desktop display mode: %s", SDL_GetError());
            return false;
        }

        // Set exclusive fullscreen mode
        if (!SDL_SetWindowFullscreenMode(globalWindow, desktopMode))
        {
            leo_SetError("%s", SDL_GetError());
            return false;
        }
        if (!SDL_SetWindowFullscreen(globalWindow, true))
        {
            leo_SetError("%s", SDL_GetError());
            return false;
        }
    }
    break;

    default:
        leo_SetError("Invalid window mode: %d", mode);
        return false;
    }

    s_state.currentWindowMode = mode;
    return true;
}

leo_WindowMode leo_GetWindowMode(void)
{
    return s_state.currentWindowMode;
}

bool leo_WindowShouldClose(void)
{
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        switch (e.type)
        {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            s_state.quit = 1;
            break;
        default:
            break;
        }
        leo_HandleMouseEvent(&e);
        leo_HandleGamepadEvent(&e);
    }
    leo_UpdateKeyboard();
    leo_UpdateMouse();
    leo_UpdateGamepads();
    if (leo_IsExitKeyPressed())
        s_state.quit = 1;
    return s_state.quit != 0;
}

void leo_BeginDrawing(void)
{
    if (!globalRenderer)
        return;
    s_state.inFrame = 1;
    s_state.frameStartCounter = SDL_GetPerformanceCounter();
}

void leo_ClearBackground(int r, int g, int b, int a)
{
    if (!globalRenderer)
        return;

    if (r < 0)
        r = 0;
    if (r > 255)
        r = 255;
    if (g < 0)
        g = 0;
    if (g > 255)
        g = 255;
    if (b < 0)
        b = 0;
    if (b > 255)
        b = 255;
    if (a < 0)
        a = 0;
    if (a > 255)
        a = 255;

    s_state.clearR = (Uint8)r;
    s_state.clearG = (Uint8)g;
    s_state.clearB = (Uint8)b;
    s_state.clearA = (Uint8)a;

    SDL_SetRenderDrawColor(globalRenderer, s_state.clearR, s_state.clearG, s_state.clearB, s_state.clearA);
    SDL_RenderClear(globalRenderer);
}

void leo_EndDrawing(void)
{
    if (!globalRenderer)
        return;

    if (SDL_GetRenderTarget(globalRenderer) == NULL)
        SDL_RenderPresent(globalRenderer);

    Uint64 now = SDL_GetPerformanceCounter();
    double elapsed = s_state.perfFreq ? (double)(now - s_state.frameStartCounter) / (double)s_state.perfFreq : 0.0;

    if (s_state.targetFPS > 0 && s_state.targetFrameSecs > 0.0 && elapsed < s_state.targetFrameSecs)
    {
        double remaining = s_state.targetFrameSecs - elapsed;
        if (remaining > 0.0)
        {
            double ms = remaining * 1000.0;
            if (ms > 0.0)
                SDL_Delay((Uint32)(ms + 0.5));
        }
        now = SDL_GetPerformanceCounter();
        if (s_state.perfFreq)
            elapsed = (double)(now - s_state.frameStartCounter) / (double)s_state.perfFreq;
    }

    s_state.lastFrameTime = (float)elapsed;

    s_state.fpsCounter += 1;
    if (s_state.perfFreq && (now - s_state.fpsWindowStart) >= s_state.perfFreq)
    {
        s_state.currentFPS = s_state.fpsCounter;
        s_state.fpsCounter = 0;
        s_state.fpsWindowStart = now;
    }

    s_state.inFrame = 0;
}

void leo_SetTargetFPS(int fps)
{
    if (fps <= 0)
    {
        s_state.targetFPS = 0;
        s_state.targetFrameSecs = 0.0;
        return;
    }
    if (fps > 1000)
        fps = 1000;
    s_state.targetFPS = fps;
    s_state.targetFrameSecs = 1.0 / (double)fps;
}

float leo_GetFrameTime(void)
{
    return s_state.lastFrameTime;
}

double leo_GetTime(void)
{
    if (!s_state.perfFreq || !s_state.startCounter)
        return 0.0;
    Uint64 now = SDL_GetPerformanceCounter();
    return (double)(now - s_state.startCounter) / (double)s_state.perfFreq;
}

int leo_GetFPS(void)
{
    if (s_state.currentFPS == 0 && s_state.lastFrameTime > 0.0f)
    {
        int est = (int)(1.0f / s_state.lastFrameTime + 0.5f);
        if (est < 0)
            est = 0;
        return est;
    }
    return s_state.currentFPS;
}

// Helpers: map Leo enums to SDL3 types
static SDL_RendererLogicalPresentation _to_sdl_presentation(leo_LogicalPresentation p)
{
    switch (p)
    {
    case LEO_LOGICAL_PRESENTATION_STRETCH:
        return SDL_LOGICAL_PRESENTATION_STRETCH;
    case LEO_LOGICAL_PRESENTATION_LETTERBOX:
        return SDL_LOGICAL_PRESENTATION_LETTERBOX;
    case LEO_LOGICAL_PRESENTATION_OVERSCAN:
        return SDL_LOGICAL_PRESENTATION_OVERSCAN;
    case LEO_LOGICAL_PRESENTATION_DISABLED:
    default:
        return SDL_LOGICAL_PRESENTATION_DISABLED;
    }
}

static SDL_ScaleMode _to_sdl_scale(leo_ScaleMode m)
{
    switch (m)
    {
    case LEO_SCALE_NEAREST:
        return SDL_SCALEMODE_NEAREST;
    case LEO_SCALE_PIXELART:
        return SDL_SCALEMODE_NEAREST; /* SDL3 adds pixel-art scaler */
    case LEO_SCALE_LINEAR:
    default:
        return SDL_SCALEMODE_LINEAR;
    }
}

bool leo_SetLogicalResolution(int width, int height, leo_LogicalPresentation presentation, leo_ScaleMode scale)
{
    if (width <= 0 || height <= 0)
    {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Disabling logical resolution");
    }
    else
    {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Setting logical resolution: %dx%d", width, height);
    }

    if (!globalRenderer)
    {
        leo_SetError("leo_SetLogicalResolution called before leo_InitWindow");
        return false;
    }

    /* Record default per-texture scale mode we’ll apply to newly created textures. */
    s_state.defaultScaleMode = _to_sdl_scale(scale);

    if (width <= 0 || height <= 0)
    {
        /* Disable logical presentation */
        if (!SDL_SetRenderLogicalPresentation(globalRenderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED))
        {
            leo_SetError("%s", SDL_GetError());
            return false;
        }
        s_state.hasLogical = 0;
        s_state.logicalW = 0;
        s_state.logicalH = 0;
        return true;
    }

    SDL_RendererLogicalPresentation sp = _to_sdl_presentation(presentation);
    if (!SDL_SetRenderLogicalPresentation(globalRenderer, width, height, sp))
    {
        leo_SetError("%s", SDL_GetError());
        return false;
    }

    /* With logical presentation enabled, all SDL coordinates are now in logical pixels.
       Our camera math already works in “screen space” pixels, so this maps cleanly. */
    s_state.hasLogical = 1;
    s_state.logicalW = width;
    s_state.logicalH = height;
    return true;
}

int leo_GetScreenWidth(void)
{
    if (!globalWindow)
        return 0;
    /* Expose logical size when active for a raylib-like mental model */
    if (s_state.hasLogical && s_state.logicalW > 0)
        return s_state.logicalW;
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(globalWindow, &w, &h);
    return w;
}

int leo_GetScreenHeight(void)
{
    if (!globalWindow)
        return 0;
    if (s_state.hasLogical && s_state.logicalH > 0)
        return s_state.logicalH;
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(globalWindow, &w, &h);
    return h;
}

// Camera2D API
void leo_BeginMode2D(leo_Camera2D camera)
{
    const int cap = (int)(sizeof(s_state.camStack) / sizeof(s_state.camStack[0]));
    if (s_state.camTop + 1 < cap)
    {
        s_state.camTop++;
    } /* else overwrite top */

    if (camera.zoom <= 0.0f)
        camera.zoom = 1.0f;
    if (camera.rotation > 360.0f || camera.rotation < -360.0f)
    {
        int k = (int)(camera.rotation / 360.0f);
        camera.rotation -= 360.0f * (float)k;
    }
    s_state.camStack[s_state.camTop] = camera;
    s_state.cameraActive = true;
    _rebuildCameraMatrix(&s_state.camStack[s_state.camTop]);
}

void leo_EndMode2D(void)
{
    if (s_state.camTop >= 0)
        s_state.camTop--;
    s_state.cameraActive = (s_state.camTop >= 0);
    _rebuildCameraMatrix(s_state.camTop >= 0 ? &s_state.camStack[s_state.camTop] : NULL);
}

bool leo_IsCameraActive(void)
{
    return s_state.cameraActive;
}

leo_Camera2D leo_GetCurrentCamera2D(void)
{
    if (s_state.camTop >= 0)
        return s_state.camStack[s_state.camTop];
    leo_Camera2D id;
    id.target.x = 0.0f;
    id.target.y = 0.0f;
    id.offset.x = 0.0f;
    id.offset.y = 0.0f;
    id.rotation = 0.0f;
    id.zoom = 1.0f;
    return id;
}

leo_Vector2 leo_GetWorldToScreen2D(leo_Vector2 p, leo_Camera2D cam)
{
    if (cam.zoom <= 0.0f)
        cam.zoom = 1.0f;
    float m11, m12, m21, m22, tx, ty;
    _buildCam3x2(&cam, &m11, &m12, &m21, &m22, &tx, &ty);
    leo_Vector2 out = {m11 * p.x + m12 * p.y + tx, m21 * p.x + m22 * p.y + ty};
    return out;
}

leo_Vector2 leo_GetScreenToWorld2D(leo_Vector2 p, leo_Camera2D cam)
{
    if (cam.zoom <= 0.0f)
        cam.zoom = 1.0f;
    float m11, m12, m21, m22, tx, ty;
    _buildCam3x2(&cam, &m11, &m12, &m21, &m22, &tx, &ty);
    const float sx = p.x - tx, sy = p.y - ty;
    const float det = m11 * m22 - m12 * m21;
    const float inv = (det != 0.0f) ? (1.0f / det) : 1.0f;
    leo_Vector2 out = {inv * (m22 * sx - m12 * sy), inv * (-m21 * sx + m11 * sy)};
    return out;
}

// RenderTexture API
leo_RenderTexture2D leo_LoadRenderTexture(int width, int height)
{
    leo_RenderTexture2D rt;
    SDL_memset(&rt, 0, sizeof(rt));
    if (!globalRenderer || width <= 0 || height <= 0)
        return rt;

    Uint32 fmt = SDL_PIXELFORMAT_RGBA32;
    SDL_Texture *tex = SDL_CreateTexture(globalRenderer, fmt, SDL_TEXTUREACCESS_TARGET, width, height);
    if (!tex)
        return rt;

    /* Blend and default scale mode for textures we create */
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(tex, s_state.defaultScaleMode);

    rt.width = width;
    rt.height = height;
    rt.texture.width = width;
    rt.texture.height = height;
    rt.texture._handle = tex;
    rt._rt_handle = tex;
    return rt;
}

void leo_UnloadRenderTexture(leo_RenderTexture2D target)
{
    if (target._rt_handle)
        SDL_DestroyTexture((SDL_Texture *)target._rt_handle);
}

void leo_BeginTextureMode(leo_RenderTexture2D target)
{
    if (!globalRenderer || !target._rt_handle)
        return;
    SDL_Texture *current = SDL_GetRenderTarget(globalRenderer);
    _rtPush(current);
    SDL_SetRenderTarget(globalRenderer, (SDL_Texture *)target._rt_handle);
}

void leo_EndTextureMode(void)
{
    if (!globalRenderer)
        return;
    SDL_Texture *prev = _rtPop();
    SDL_SetRenderTarget(globalRenderer, prev); // NULL means backbuffer
}

// Texture drawing
void leo_DrawTextureRec(leo_Texture2D tex, leo_Rectangle src, leo_Vector2 position, leo_Color tint)
{
    if (!globalRenderer || !tex._handle)
        return;

    SDL_FRect s = {src.x, src.y, src.width, src.height};
    // raylib-style flip: negative width/height flips & shifts origin
    if (s.w < 0.0f)
    {
        s.x += s.w;
        s.w = -s.w;
    }
    if (s.h < 0.0f)
    {
        s.y += s.h;
        s.h = -s.h;
    }

    // Apply camera transform if active
    leo_Vector2 screenPos = position;
    if (s_state.cameraActive)
    {
        screenPos = leo_GetWorldToScreen2D(position, leo_GetCurrentCamera2D());
    }

    SDL_FRect d = {screenPos.x, screenPos.y, s.w, s.h};

    SDL_Texture *t = (SDL_Texture *)tex._handle;

    // Apply tint
    SDL_SetTextureColorMod(t, tint.r, tint.g, tint.b);
    SDL_SetTextureAlphaMod(t, tint.a);

    // SDL3 blit
    SDL_RenderTexture(globalRenderer, t, &s, &d);

    // Restore neutral modulation (avoid accidental tinting later)
    SDL_SetTextureColorMod(t, 255, 255, 255);
    SDL_SetTextureAlphaMod(t, 255);
}

void leo_DrawTexturePro(leo_Texture2D tex, leo_Rectangle src, leo_Rectangle dest, leo_Vector2 origin, float rotation,
                        leo_Color tint)
{
    if (!globalRenderer || !tex._handle)
        return;

    // ---- Normalize source rect (handle negative width/height by flipping UVs) ----
    float sx = src.x;
    float sy = src.y;
    float sw = src.width;
    float sh = src.height;

    if (sw < 0.0f)
    {
        sx += sw;
        sw = -sw;
    }
    if (sh < 0.0f)
    {
        sy += sh;
        sh = -sh;
    }

    if (sw <= 0.0f || sh <= 0.0f)
        return;

    // ---- Compute UVs in [0,1] (swap for negative original sign) ----
    float u0 = sx / (float)tex.width;
    float v0 = sy / (float)tex.height;
    float u1 = (sx + sw) / (float)tex.width;
    float v1 = (sy + sh) / (float)tex.height;

    // If caller passed negative src.* originally, the swaps above already normalized sx/sy/sw/sh.
    // The UVs are now correct; no additional swap needed because we adjusted sx/sy.

    // ---- Build destination quad (top-left, top-right, bottom-right, bottom-left) in screen space ----
    // Allow negative dest.width/height (this flips geometry, mirroring around dest.x/y).
    const float dx = dest.x, dy = dest.y;
    const float dw = dest.width, dh = dest.height;

    // Pivot point (screen space): dest top-left + origin
    const float px = dx + origin.x;
    const float py = dy + origin.y;

    // Unrotated corners
    SDL_FPoint p[4] = {
        {dx, dy},           // TL
        {dx + dw, dy},      // TR
        {dx + dw, dy + dh}, // BR
        {dx, dy + dh}       // BL
    };

    // ---- Rotate around pivot (rotation is in degrees, counterclockwise) ----
    const float rad = rotation * 0.01745329251994329577f; // deg -> rad
    const float cr = cosf(rad), sr = sinf(rad);

    for (int i = 0; i < 4; ++i)
    {
        const float rx = p[i].x - px;
        const float ry = p[i].y - py;
        p[i].x = rx * cr - ry * sr + px;
        p[i].y = rx * sr + ry * cr + py;
    }

    // ---- Vertex colors (tint) ----
    const float s = 1.0f / 255.0f;
    const SDL_FColor fc = {tint.r * s, tint.g * s, tint.b * s, tint.a * s};

    // ---- Pack vertices with UVs ----
    SDL_Vertex v[4];
    v[0].position = (SDL_FPoint){p[0].x, p[0].y};
    v[0].color = fc;
    v[0].tex_coord = (SDL_FPoint){u0, v0}; // TL
    v[1].position = (SDL_FPoint){p[1].x, p[1].y};
    v[1].color = fc;
    v[1].tex_coord = (SDL_FPoint){u1, v0}; // TR
    v[2].position = (SDL_FPoint){p[2].x, p[2].y};
    v[2].color = fc;
    v[2].tex_coord = (SDL_FPoint){u1, v1}; // BR
    v[3].position = (SDL_FPoint){p[3].x, p[3].y};
    v[3].color = fc;
    v[3].tex_coord = (SDL_FPoint){u0, v1}; // BL

    static const int indices[6] = {0, 1, 2, 0, 2, 3};

    // ---- Draw as geometry with the source texture ----
    SDL_Texture *t = (SDL_Texture *)tex._handle;

    // Ensure blending is enabled for typical sprite draws (don’t error if it fails).
    SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);

    SDL_RenderGeometry(globalRenderer, t, v, 4, indices, 6);
}
//
