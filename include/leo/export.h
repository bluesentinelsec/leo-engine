#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
  #define LEO_API __declspec(dllexport)
#else
#define LEO_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
}
#endif
