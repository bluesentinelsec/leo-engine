#include <SDL3/SDL.h>

#define STBI_MALLOC(sz)           SDL_malloc(sz)
#define STBI_REALLOC(p,newsz)     SDL_realloc(p,newsz)
#define STBI_FREE(p)              SDL_free(p)

#define STBTT_malloc(x,u)         ((void)(u),SDL_malloc(x))
#define STBTT_free(x,u)           ((void)(u),SDL_free(x))

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
