#pragma once

#include "leo/color.h"
#include "leo/export.h"

LEO_API void leo_DrawPixel(int posX, int posY, leo_Color color);

LEO_API void leo_DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, leo_Color color);

LEO_API void leo_DrawCircle(int centerX, int centerY, float radius, leo_Color color);
