# Leo Engine

## Pardon our dust, this repository is under active development.

Leo Engine is a lightweight C game engine runtime built on SDL, offering a clean, Raylib-style API. It’s stand-alone with no dependency bloat from SDL extensions and is easy to bind from other languages. Focused on providing features needed to ship a game that are commonly missing in other frameworks (virtual filesystem for example). Leo Engine aims to deliver a fun, enjoyable developer experience.

---

# 🧰 Build & Include

```c
#include "leo/leo.h"   // your umbrella header
```

---

# 🚪 Window, Loop & Timing

```c
if (!leo_InitWindow(1280, 720, "Leo")) { /* fprintf(stderr,"%s\n", leo_GetError()); */ }

leo_SetTargetFPS(60);

while (!leo_WindowShouldClose()) {
    leo_BeginDrawing();
    leo_ClearBackground(30,30,30,255);

    // ... draw here ...

    leo_EndDrawing();
}

leo_CloseWindow();
```

* `leo_GetFrameTime()` → `float dt` seconds
* `leo_GetTime()` → seconds since `InitWindow`
* `leo_GetFPS()` → current FPS
* Fullscreen: `leo_SetFullscreen(true)`
* Screen size (logical if set): `leo_GetScreenWidth()`, `leo_GetScreenHeight()`

---

# 🧭 Logical (Virtual) Resolution

```c
// 320x180 “virtual” pixels, letterboxed; default per-texture scale = nearest
leo_SetLogicalResolution(320,180, LEO_LOGICAL_PRESENTATION_LETTERBOX, LEO_SCALE_NEAREST);

// Disable:
leo_SetLogicalResolution(0,0, LEO_LOGICAL_PRESENTATION_DISABLED, LEO_SCALE_LINEAR);
```

---

# 🎥 2D Camera

```c
leo_Camera2D cam = { .target={160,90}, .offset={160,90}, .rotation=0, .zoom=1 };
leo_BeginMode2D(cam);  /* draw world-space stuff */  leo_EndMode2D();

// Conversions:
leo_Vector2 s = leo_GetWorldToScreen2D((leo_Vector2){x,y}, cam);
leo_Vector2 w = leo_GetScreenToWorld2D((leo_Vector2){mx,my}, cam);
```

---

# 🖼️ Textures & Images (GPU-first)

```c
// VFS mounts (optional but recommended):
leo_MountResourcePack("resources.leopack", NULL, 100);
leo_MountDirectory("assets", 10);

// Load GPU texture (PNG/JPG):
leo_Texture2D tex = leo_LoadTexture("images/player.png");
if (!leo_IsTextureReady(tex)) { fprintf(stderr, "Leo: %s\n", leo_GetError()); }

// Draw a sub-rect:
leo_DrawTextureRec(
  tex,
  (leo_Rectangle){0,0,16,16},
  (leo_Vector2){100,80},
  (leo_Color){255,255,255,255}
);

// Clone GPU texture:
leo_Texture2D dup = leo_LoadTextureFromTexture(tex);

// From encoded memory (via VFS):
size_t n=0; void* png = leo_LoadAsset("images/ui/button.png", &n);
leo_Texture2D tmem = leo_LoadTextureFromMemory("png", (const unsigned char*)png, (int)n);

// From raw pixels:
leo_Texture2D raw = leo_LoadTextureFromPixels(rgba, w, h, w*4, LEO_TEXFORMAT_R8G8B8A8);

// Cleanup
leo_UnloadTexture(&raw);
leo_UnloadTexture(&tmem);
free(png);
leo_UnloadTexture(&dup);
leo_UnloadTexture(&tex);
```

**Render targets (offscreen):**

```c
leo_RenderTexture2D target = leo_LoadRenderTexture(320,180);

leo_BeginTextureMode(target);
  leo_ClearBackground(0,0,0,255);
  // ... draw scene ...
leo_EndTextureMode();

// Blit to screen:
leo_DrawTextureRec(target.texture, (leo_Rectangle){0,0,320,180}, (leo_Vector2){0,0}, (leo_Color){255,255,255,255});

leo_UnloadRenderTexture(target);
```

---

# 🔤 Fonts & Text

```c
leo_Font f = leo_LoadFont("fonts/Roboto-Regular.ttf", 32);
leo_SetDefaultFont(f);

leo_DrawText("Hello Leo!", 10, 10, 24, (leo_Color){255,255,0,255});
leo_DrawTextEx(f, "Score", (leo_Vector2){8,8}, 24.0f, 0.0f, (leo_Color){255,255,255,255});
leo_DrawTextPro(f, "Spin", (leo_Vector2){160,90}, (leo_Vector2){20,12}, 45.0f, 24.0f, 0.0f,
                (leo_Color){200,255,200,255});

// Measure
leo_Vector2 sz = leo_MeasureTextEx(f, "Hello", 24.0f, 0.0f);
int w = leo_MeasureText("Hello", 24);
int lh = leo_GetFontLineHeight(f, 24.0f);

leo_UnloadFont(&f);
```

---

# 🔊 Audio (miniaudio-backed SFX)

```c
// Optional explicit init: leo_InitAudio();  // auto-inits on first audio use

leo_Sound sfx = leo_LoadSound("audio/jump.wav");
if (leo_IsSoundReady(sfx)) {
    leo_PlaySound(&sfx, 0.8f, false);
    leo_SetSoundPitch(&sfx, 1.0f);
    leo_SetSoundPan(&sfx, 0.0f);
}

leo_PauseSound(&sfx);
leo_ResumeSound(&sfx);
bool playing = leo_IsSoundPlaying(&sfx);
leo_StopSound(&sfx);

leo_UnloadSound(&sfx);
// Optional: leo_ShutdownAudio();
```

> **VFS note for all loaders:** they search **mounts by priority (high→low)**; if no match, they try the same string as a **direct filesystem path**.

---

# ⌨️ Keyboard

```c
leo_UpdateKeyboard(); // once per frame

if (leo_IsKeyPressed(SDLK_SPACE)) { /* edge */ }
if (leo_IsKeyDown(SDLK_LEFT))      { /* level */ }
if (leo_IsKeyReleased(SDLK_ESCAPE)){ /* edge  */ }

int key=0; while ((key = leo_GetKeyPressed())) { /* drain */ }
int ch =0; while ((ch  = leo_GetCharPressed())) { /* UTF-32 */ }

leo_SetExitKey(SDLK_ESCAPE);
```

---

# 🖱️ Mouse

```c
leo_UpdateMouse();

if (leo_IsMouseButtonPressed(LEO_MOUSE_BUTTON_LEFT)) { /* click */ }

leo_Vector2 mp = leo_GetMousePosition(); // logical coords if logical res is on
leo_Vector2 md = leo_GetMouseDelta();

leo_SetMousePosition(100,100);
leo_SetMouseOffset(0,0);
leo_SetMouseScale(1.0f,1.0f);

float wheel = leo_GetMouseWheelMove();
leo_Vector2 wheel2 = leo_GetMouseWheelMoveV();
```

---

# 🎮 Gamepads

```c
leo_UpdateGamepads();

if (leo_IsGamepadAvailable(0)) {
    const char* name = leo_GetGamepadName(0);
    if (leo_IsGamepadButtonPressed(0, LEO_GAMEPAD_BUTTON_FACE_DOWN)) { /* A/Cross */ }

    float lx = leo_GetGamepadAxisMovement(0, LEO_GAMEPAD_AXIS_LEFT_X);

    leo_SetGamepadStickThreshold(0.50f, 0.40f);
    if (leo_IsGamepadStickPressed(0, LEO_GAMEPAD_STICK_LEFT, LEO_GAMEPAD_DIR_RIGHT)) { /* ... */ }

    leo_SetGamepadVibration(0, 0.8f, 0.2f, 0.15f);
}
```

---

# 🔷 Primitive Drawing

```c
leo_DrawPixel(10,10, (leo_Color){255,0,0,255});
leo_DrawLine(20,20, 60,40, (leo_Color){255,255,255,255});
leo_DrawCircle(100,80, 24.0f, (leo_Color){0,200,255,255});
leo_DrawRectangle(140,60, 40,24, (leo_Color){80,200,80,255});
```

*Renders to the **current render target** (backbuffer or a render texture).*

---

# 🎒 VFS / Resource Mounts

```c
leo_ClearMounts();
leo_MountResourcePack("resources.leopack", "optional_password", 100); // higher = searched first
leo_MountDirectory("assets", 10);

// Existence / info:
leo_AssetInfo info;
bool ok = leo_StatAsset("images/player.png", &info);

// Read-all (malloc + copy), caller frees:
size_t n = 0;
void* bytes = leo_LoadAsset("shaders/post.fx", &n);

// Streaming:
leo_AssetStream* s = leo_OpenAsset("music/theme.ogg", &info);
uint8_t buf[4096];
size_t got = leo_AssetRead(s, buf, sizeof buf);
leo_AssetSeek(s, 0, LEO_SEEK_SET);
size_t total = leo_AssetSize(s);
bool fromPack = leo_AssetFromPack(s);
leo_CloseAsset(s);

// Text helper (adds trailing '\0'):
size_t len_no_nul = 0;
char* txt = leo_LoadTextAsset("config/game.json", &len_no_nul);
free(txt);
free(bytes);
```

**Lookup order:** mounts by **priority (high→low)** → if not found, **direct path**.

---

# 🧱 Collisions

```c
bool hit = leo_CheckCollisionRecs(a, b);
leo_Rectangle ov = leo_GetCollisionRec(a, b);
bool inside = leo_CheckCollisionPointCircle((leo_Vector2){mx,my}, (leo_Vector2){cx,cy}, r);
```

---

# 🧯 Errors

```c
if (!leo_IsTextureReady(tex)) {
    fprintf(stderr, "Leo error: %s\n", leo_GetError());
    leo_ClearError();
}
```

---

# 🧩 Minimal Example (image + text + sfx)

```c
#include "leo/leo.h"

int main(void) {
    if (!leo_InitWindow(640, 360, "Leo Quickstart")) return 1;
    leo_SetTargetFPS(60);

    // VFS
    leo_MountResourcePack("resources.leopack", NULL, 100);
    leo_MountDirectory("assets", 10);

    // Assets
    leo_Texture2D tex = leo_LoadTexture("images/player.png");
    leo_Font font = leo_LoadFont("fonts/Roboto-Regular.ttf", 32);
    leo_SetDefaultFont(font);
    leo_Sound blip = leo_LoadSound("audio/blip.wav");

    while (!leo_WindowShouldClose()) {
        leo_BeginDrawing();
        leo_ClearBackground(24,24,32,255);

        if (leo_IsKeyPressed(SDLK_SPACE)) leo_PlaySound(&blip, 0.8f, false);

        leo_DrawText("Hello Leo!", 16, 16, 24, (leo_Color){255,255,255,255});
        leo_DrawTextureRec(tex, (leo_Rectangle){0,0,32,32}, (leo_Vector2){100,100}, (leo_Color){255,255,255,255});

        leo_EndDrawing();
    }

    leo_UnloadSound(&blip);
    leo_UnloadFont(&font);
    leo_UnloadTexture(&tex);
    leo_CloseWindow();
    return 0;
}
```

Want this as a one-page Markdown or PDF you can ship with the repo? I can format/export it.

