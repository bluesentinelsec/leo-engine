#ifndef LEO_TEXTURE_LOADER_H
#define LEO_TEXTURE_LOADER_H

#include "leo/vfs.h"
#include <SDL3/SDL.h>

namespace engine
{

struct Texture
{
    SDL_Texture *handle;
    int width;
    int height;

    Texture() noexcept;
    Texture(SDL_Texture *handle, int width, int height) noexcept;
    ~Texture();

    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;

    Texture(Texture &&other) noexcept;
    Texture &operator=(Texture &&other) noexcept;

    void Reset() noexcept;
};

class TextureLoader
{
  public:
    TextureLoader(VFS &vfs, SDL_Renderer *renderer);
    Texture Load(const char *vfs_path);

  private:
    VFS &vfs;
    SDL_Renderer *renderer;
};

} // namespace engine

#endif // LEO_TEXTURE_LOADER_H
