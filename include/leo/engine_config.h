#ifndef LEO_ENGINE_CONFIG_H
#define LEO_ENGINE_CONFIG_H

#include <SDL3/SDL_stdinc.h>

namespace engine
{

enum class WindowMode
{
    Windowed = 0,
    Fullscreen,
    BorderlessFullscreen
};

struct Config
{
    const char *argv0;         // argv[0] from main
    const char *resource_path; // Path to mounted resource directory/archive
    const char *script_path;   // Lua script path inside VFS (optional)
    const char *organization;  // "bluesentinelsec"
    const char *app_name;      // "leo-engine"
    const char *window_title;  // Window title
    Sint32 window_width;       // Window width in pixels
    Sint32 window_height;      // Window height in pixels
    Sint32 logical_width;      // Logical render width
    Sint32 logical_height;     // Logical render height
    WindowMode window_mode;    // Window mode
    Sint32 tick_hz;            // Fixed update tick rate
    Uint32 NumFrameTicks;      // 0 = run until exit

    // Memory allocation functions (default to SDL3)
    void *(*malloc_fn)(size_t);          // Default: SDL_malloc
    void *(*realloc_fn)(void *, size_t); // Default: SDL_realloc
    void (*free_fn)(void *);             // Default: SDL_free
};

} // namespace engine

#endif // LEO_ENGINE_CONFIG_H
