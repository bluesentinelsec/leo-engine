#ifndef LEO_ENGINE_CONFIG_H
#define LEO_ENGINE_CONFIG_H

#include <SDL3/SDL_stdinc.h>

namespace engine {

struct Config {
    const char* argv0;                    // argv[0] from main
    const char* resource_path;            // Path to mounted resource directory/archive
    const char* organization;             // "bluesentinelsec"
    const char* app_name;                 // "leo-engine"
    
    // Memory allocation functions (default to SDL3)
    void* (*malloc_fn)(size_t);           // Default: SDL_malloc
    void* (*realloc_fn)(void*, size_t);   // Default: SDL_realloc
    void  (*free_fn)(void*);              // Default: SDL_free
};

} // namespace engine

#endif // LEO_ENGINE_CONFIG_H
