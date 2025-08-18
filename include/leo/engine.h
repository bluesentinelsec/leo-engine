#pragma once

#include "leo/export.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


// if false, use leo_GetError() to print reason
LEO_API bool leo_InitWindow(int width, int height, const char* title);

LEO_API void leo_CloseWindow();


#ifdef __cplusplus
}
#endif
