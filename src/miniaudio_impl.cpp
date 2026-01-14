#include <SDL3/SDL.h>

#define MA_MALLOC(sz) SDL_malloc(sz)
#define MA_REALLOC(p, sz) SDL_realloc(p, sz)
#define MA_FREE(p) SDL_free(p)

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
