# Leo Engine API Cheat Sheet

A quick reference guide for the Leo Engine C API.

## Table of Contents
- [Window Management](#window-management)
- [Graphics & Drawing](#graphics--drawing)
- [Colors](#colors)
- [Timing](#timing)
- [Error Handling](#error-handling)
- [Basic Usage Example](#basic-usage-example)

---

## Window Management

### Core Window Functions
```c
// Initialize window and OpenGL context
bool leo_InitWindow(int width, int height, const char* title);

// Close window and unload OpenGL context
void leo_CloseWindow();

// Get window handle
LeoWindow leo_GetWindow(void);

// Get renderer handle
LeoRenderer leo_GetRenderer(void);

// Set fullscreen mode
bool leo_SetFullscreen(bool enabled);

// Check if window should close
bool leo_WindowShouldClose(void);
```

### Drawing Loop Functions
```c
// Clear background with color
void leo_ClearBackground(int r, int g, int b, int a);

// Begin drawing mode
void leo_BeginDrawing(void);

// End drawing mode and swap buffers
void leo_EndDrawing(void);
```

---

## Graphics & Drawing

### Basic Shapes
```c
// Draw a single pixel
void leo_DrawPixel(int posX, int posY, leo_Color color);

// Draw a line between two points
void leo_DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, leo_Color color);

// Draw a circle
void leo_DrawCircle(int centerX, int centerY, float radius, leo_Color color);

// Draw a rectangle
void leo_DrawRectangle(int posX, int posY, int width, int height, leo_Color color);
```

---

## Colors

### Color Structure
```c
typedef struct {
    Uint8 r;  // Red channel (0-255)
    Uint8 g;  // Green channel (0-255)
    Uint8 b;  // Blue channel (0-255)
    Uint8 a;  // Alpha channel (0-255, 0=transparent, 255=opaque)
} leo_Color;
```

### Common Color Values
```c
// Create colors (you can define these as constants)
leo_Color RED = {255, 0, 0, 255};
leo_Color GREEN = {0, 255, 0, 255};
leo_Color BLUE = {0, 0, 255, 255};
leo_Color WHITE = {255, 255, 255, 255};
leo_Color BLACK = {0, 0, 0, 255};
leo_Color TRANSPARENT = {0, 0, 0, 0};
```

---

## Timing

### FPS and Time Functions
```c
// Set target FPS (maximum)
void leo_SetTargetFPS(int fps);

// Get time in seconds for last frame drawn (delta time)
float leo_GetFrameTime(void);

// Get elapsed time in seconds since InitWindow()
double leo_GetTime(void);

// Get current FPS
int leo_GetFPS(void);
```

---

## Error Handling

### Error Management
```c
// Set error message
void leo_SetError(const char* fmt, ...);

// Get last error message
const char* leo_GetError(void);

// Clear error message
void leo_ClearError(void);
```

---

## Basic Usage Example

```c
#include "leo/engine.h"
#include "leo/graphics.h"
#include "leo/color.h"

int main() {
    // Initialize window
    if (!leo_InitWindow(800, 600, "Leo Engine Example")) {
        return -1;
    }
    
    // Set target FPS
    leo_SetTargetFPS(60);
    
    // Main game loop
    while (!leo_WindowShouldClose()) {
        // Begin drawing
        leo_BeginDrawing();
        
        // Clear background
        leo_ClearBackground(50, 50, 50, 255);
        
        // Draw some shapes
        leo_Color red = {255, 0, 0, 255};
        leo_Color blue = {0, 0, 255, 255};
        
        leo_DrawCircle(400, 300, 50, red);
        leo_DrawRectangle(100, 100, 200, 150, blue);
        
        // End drawing
        leo_EndDrawing();
    }
    
    // Cleanup
    leo_CloseWindow();
    return 0;
}
```

---

## Quick Reference

### Function Categories
- **Window**: `leo_InitWindow`, `leo_CloseWindow`, `leo_GetWindow`, `leo_GetRenderer`
- **Display**: `leo_SetFullscreen`, `leo_WindowShouldClose`
- **Drawing**: `leo_BeginDrawing`, `leo_EndDrawing`, `leo_ClearBackground`
- **Shapes**: `leo_DrawPixel`, `leo_DrawLine`, `leo_DrawCircle`, `leo_DrawRectangle`
- **Timing**: `leo_SetTargetFPS`, `leo_GetFrameTime`, `leo_GetTime`, `leo_GetFPS`
- **Errors**: `leo_SetError`, `leo_GetError`, `leo_ClearError`

### Common Patterns
1. **Initialize** → `leo_InitWindow()`
2. **Set FPS** → `leo_SetTargetFPS()`
3. **Game Loop** → `leo_BeginDrawing()` → Draw → `leo_EndDrawing()`
4. **Cleanup** → `leo_CloseWindow()`

---

*This cheat sheet covers the Leo Engine API version based on the current headers. For more detailed information, refer to the individual header files in the `include/leo/` directory.*
