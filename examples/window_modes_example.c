// ==========================================================
// File: examples/window_modes_example.c
// Example demonstrating the three window modes:
// - Windowed
// - Borderless Fullscreen  
// - Fullscreen Exclusive
// 
// Controls:
// - Press 1 for windowed mode
// - Press 2 for borderless fullscreen
// - Press 3 for fullscreen exclusive
// - Press ESC to quit
// ==========================================================

#include "leo/leo.h"
#include <stdio.h>

typedef struct {
    leo_WindowMode current_mode;
} ExampleState;

static bool setup(leo_GameContext *ctx) {
    ExampleState *state = (ExampleState *)ctx->user_data;
    state->current_mode = leo_GetWindowMode();
    
    printf("Window Modes Example\n");
    printf("Controls:\n");
    printf("  1 - Windowed mode\n");
    printf("  2 - Borderless fullscreen\n");
    printf("  3 - Fullscreen exclusive\n");
    printf("  ESC - Quit\n");
    printf("\nStarting in mode: %d\n", state->current_mode);
    
    return true;
}

static void update(leo_GameContext *ctx) {
    ExampleState *state = (ExampleState *)ctx->user_data;
    
    // Check for mode change requests
    if (leo_IsKeyPressed(LEO_KEY_1)) {
        if (leo_SetWindowMode(LEO_WINDOW_MODE_WINDOWED)) {
            state->current_mode = LEO_WINDOW_MODE_WINDOWED;
            printf("Switched to windowed mode\n");
        } else {
            printf("Failed to switch to windowed mode\n");
        }
    }
    
    if (leo_IsKeyPressed(LEO_KEY_2)) {
        if (leo_SetWindowMode(LEO_WINDOW_MODE_BORDERLESS_FULLSCREEN)) {
            state->current_mode = LEO_WINDOW_MODE_BORDERLESS_FULLSCREEN;
            printf("Switched to borderless fullscreen\n");
        } else {
            printf("Failed to switch to borderless fullscreen\n");
        }
    }
    
    if (leo_IsKeyPressed(LEO_KEY_3)) {
        if (leo_SetWindowMode(LEO_WINDOW_MODE_FULLSCREEN_EXCLUSIVE)) {
            state->current_mode = LEO_WINDOW_MODE_FULLSCREEN_EXCLUSIVE;
            printf("Switched to fullscreen exclusive\n");
        } else {
            printf("Failed to switch to fullscreen exclusive\n");
        }
    }
    
    if (leo_IsKeyPressed(LEO_KEY_ESCAPE)) {
        leo_GameQuit(ctx);
    }
}

static void render_ui(leo_GameContext *ctx) {
    ExampleState *state = (ExampleState *)ctx->user_data;
    
    // Draw current mode info
    const char *mode_names[] = {
        "Windowed",
        "Borderless Fullscreen", 
        "Fullscreen Exclusive"
    };
    
    char info_text[256];
    snprintf(info_text, sizeof(info_text), 
        "Current Mode: %s\n"
        "Screen Size: %dx%d\n"
        "FPS: %d\n\n"
        "Press 1/2/3 to change modes\n"
        "Press ESC to quit",
        mode_names[state->current_mode],
        leo_GetScreenWidth(),
        leo_GetScreenHeight(),
        leo_GetFPS()
    );
    
    // Simple text rendering at top-left
    leo_DrawText(info_text, 20, 20, 20, LEO_WHITE);
}

int main() {
    ExampleState state = {0};
    
    leo_GameConfig config = {
        .window_width = 800,
        .window_height = 600,
        .window_title = "Window Modes Example",
        .window_mode = LEO_WINDOW_MODE_WINDOWED,  // Start in windowed mode
        .target_fps = 60,
        .logical_width = 0,
        .logical_height = 0,
        .presentation = LEO_LOGICAL_PRESENTATION_DISABLED,
        .scale_mode = LEO_SCALE_LINEAR,
        .clear_color = {30, 30, 30, 255},  // Dark gray background
        .start_paused = false,
        .app_name = "Window Modes Example",
        .app_version = "1.0.0",
        .app_identifier = "com.leo-engine.window-modes-example",
        .user_data = &state
    };
    
    leo_GameCallbacks callbacks = {
        .on_setup = setup,
        .on_update = update,
        .on_render_ui = render_ui,
        .on_shutdown = NULL
    };
    
    return leo_GameRun(&config, &callbacks);
}
