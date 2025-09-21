#pragma once

#include "leo/export.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Get the base path for resources.
     * On macOS bundles, returns Contents/Resources directory.
     * Otherwise returns current working directory.
     *
     * @return Allocated string with base path. Caller must free().
     */
    LEO_API char *leo_GetResourceBasePath(void);

#ifdef __cplusplus
}
#endif
