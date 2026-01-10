#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

// Use CMake-generated export header if available, fallback to manual
#ifdef LEO_BUILDING_DLL
#include "leo/leo_export_generated.h"
#define LEO_API LEO_EXPORT
#else
// Fallback for manual builds or when linking statically
#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(LEO_STATIC_DEFINE) || defined(TESTING)
#define LEO_API
#else
#define LEO_API __declspec(dllimport)
#endif
#else
#define LEO_API __attribute__((visibility("default")))
#endif
#endif

#ifdef __cplusplus
}
#endif
