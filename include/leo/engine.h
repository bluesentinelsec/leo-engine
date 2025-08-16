#pragma once

#include "leo/export.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


// deprecated
LEO_API int leo_sum(int a, int b);

bool leo_InitWindow(int width, int height, const char *title);

void leo_CloseWindow();


#ifdef __cplusplus
}
#endif