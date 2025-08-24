// =============================================
// leo/graphics.h
// =============================================
#pragma once

#include "leo/color.h"
#include "leo/export.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // Draw a single pixel (world space)
    LEO_API void leo_DrawPixel(int posX, int posY, leo_Color color);

    // Draw a line segment (world space)
    LEO_API void leo_DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, leo_Color color);

    // Circle (outline)
    LEO_API void leo_DrawCircle(int centerX, int centerY, float radius, leo_Color color);

    // Rectangle (filled)
    LEO_API void leo_DrawRectangle(int posX, int posY, int width, int height, leo_Color color);

#ifdef __cplusplus
}
#endif
