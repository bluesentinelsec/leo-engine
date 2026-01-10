#include "leo/leo.h"

int main(void) {
    if (!leo_InitWindow(640, 480, "Consumer Example")) return 1;
    
    while (!leo_WindowShouldClose()) {
        leo_BeginDrawing();
        leo_ClearBackground(30, 30, 30, 255);
        leo_DrawText("Hello from Leo!", 10, 10, 20, (leo_Color){255, 255, 255, 255});
        leo_EndDrawing();
    }
    
    leo_CloseWindow();
    return 0;
}
