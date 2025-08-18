#pragma once

#include "leo/export.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* LeoWindow;
typedef void* LeoRenderer;

LEO_API bool leo_InitWindow(int width, int height, const char* title);

LEO_API void leo_CloseWindow();

LEO_API LeoWindow leo_GetWindow(void);

LEO_API LeoRenderer leo_GetRenderer(void);

LEO_API bool leo_SetFullscreen(bool enabled);

#ifdef __cplusplus
}
#endif
