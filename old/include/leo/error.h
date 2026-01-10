#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "leo/export.h"

    LEO_API void leo_SetError(const char *fmt, ...);

    LEO_API const char *leo_GetError(void);

    LEO_API void leo_ClearError(void);

#ifdef __cplusplus
}
#endif
