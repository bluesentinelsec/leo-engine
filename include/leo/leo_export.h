#ifndef LEO_EXPORT_H
#define LEO_EXPORT_H

#ifdef LEO_STATIC_DEFINE
#define LEO_EXPORT
#define LEO_NO_EXPORT
#else
#ifndef LEO_EXPORT
#ifdef LEO_BUILDING_DLL
/* We are building this library */
#ifdef _WIN32
#define LEO_EXPORT __declspec(dllexport)
#else
#define LEO_EXPORT __attribute__((visibility("default")))
#endif
#else
/* We are using this library */
#ifdef _WIN32
#define LEO_EXPORT __declspec(dllimport)
#else
#define LEO_EXPORT __attribute__((visibility("default")))
#endif
#endif
#endif

#ifndef LEO_NO_EXPORT
#ifdef _WIN32
#define LEO_NO_EXPORT
#else
#define LEO_NO_EXPORT __attribute__((visibility("hidden")))
#endif
#endif
#endif

#endif /* LEO_EXPORT_H */
