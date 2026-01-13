#include "leo/texture_loader.h"
#include <SDL3/SDL.h>
#include <stdexcept>
#include <string>

#include <stb_image.h>

namespace engine
{

Texture::Texture() noexcept : handle(nullptr), width(0), height(0)
{
}

Texture::Texture(SDL_Texture *handle, int width, int height) noexcept : handle(handle), width(width), height(height)
{
}

Texture::~Texture()
{
    Reset();
}

Texture::Texture(Texture &&other) noexcept : handle(other.handle), width(other.width), height(other.height)
{
    other.handle = nullptr;
    other.width = 0;
    other.height = 0;
}

Texture &Texture::operator=(Texture &&other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    Reset();
    handle = other.handle;
    width = other.width;
    height = other.height;
    other.handle = nullptr;
    other.width = 0;
    other.height = 0;
    return *this;
}

void Texture::Reset() noexcept
{
    if (handle)
    {
        SDL_DestroyTexture(handle);
        handle = nullptr;
    }
    width = 0;
    height = 0;
}

TextureLoader::TextureLoader(VFS &vfs, SDL_Renderer *renderer) : vfs(vfs), renderer(renderer)
{
    if (!renderer)
    {
        throw std::runtime_error("TextureLoader requires a valid SDL_Renderer");
    }
}

Texture TextureLoader::Load(const char *vfs_path)
{
    if (!vfs_path || !*vfs_path)
    {
        throw std::runtime_error("TextureLoader::Load requires a non-empty path");
    }

    void *data = nullptr;
    size_t size = 0;
    vfs.ReadAll(vfs_path, &data, &size);

    if (!data || size == 0)
    {
        if (data)
        {
            SDL_free(data);
        }
        throw std::runtime_error("TextureLoader::Load received empty data buffer");
    }

    if (size > static_cast<size_t>(SDL_MAX_SINT32))
    {
        SDL_free(data);
        throw std::runtime_error("TextureLoader::Load image buffer too large");
    }

    int width = 0;
    int height = 0;
    int comp = 0;
    stbi_uc *pixels =
        stbi_load_from_memory(static_cast<const stbi_uc *>(data), static_cast<int>(size), &width, &height, &comp, 4);
    SDL_free(data);
    (void)comp;

    if (!pixels)
    {
        const char *reason = stbi_failure_reason();
        if (!reason)
        {
            reason = "unknown error";
        }
        throw std::runtime_error(std::string("TextureLoader::Load failed to decode image: ") + reason);
    }

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
    if (!texture)
    {
        stbi_image_free(pixels);
        throw std::runtime_error(std::string("TextureLoader::Load failed to create texture: ") + SDL_GetError());
    }

    const int pitch = width * 4;
    if (!SDL_UpdateTexture(texture, nullptr, pixels, pitch))
    {
        stbi_image_free(pixels);
        SDL_DestroyTexture(texture);
        throw std::runtime_error(std::string("TextureLoader::Load failed to upload texture: ") + SDL_GetError());
    }

    stbi_image_free(pixels);
    return Texture(texture, width, height);
}

} // namespace engine
