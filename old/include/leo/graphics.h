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

    // Circle (filled)
    LEO_API void leo_DrawCircleFilled(int centerX, int centerY, float radius, leo_Color color);

    // Rectangle (outline)
    LEO_API void leo_DrawRectangleLines(int posX, int posY, int width, int height, leo_Color color);

    // Triangle (outline)
    LEO_API void leo_DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, leo_Color color);

    // Triangle (filled)
    LEO_API void leo_DrawTriangleFilled(int x1, int y1, int x2, int y2, int x3, int y3, leo_Color color);

    // Polygon (outline)
    LEO_API void leo_DrawPoly(int *points, int numPoints, leo_Color color);

    // Polygon (filled)
    LEO_API void leo_DrawPolyFilled(int *points, int numPoints, leo_Color color);

#ifdef __cplusplus
}
#endif
