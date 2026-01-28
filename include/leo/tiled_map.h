#ifndef LEO_TILED_MAP_H
#define LEO_TILED_MAP_H

#include "leo/texture_loader.h"
#include <SDL3/SDL.h>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace leo
{
namespace Camera
{
struct Camera2D;
}
} // namespace leo

namespace engine
{

class VFS;

class TiledMap
{
  public:
    TiledMap() noexcept;
    ~TiledMap();
    TiledMap(TiledMap &&other) noexcept;
    TiledMap &operator=(TiledMap &&other) noexcept;

    TiledMap(const TiledMap &) = delete;
    TiledMap &operator=(const TiledMap &) = delete;

    static TiledMap LoadFromVfs(VFS &vfs, SDL_Renderer *renderer, const char *vfs_path);

    bool IsReady() const noexcept;
    void Reset() noexcept;

    int GetWidth() const noexcept;
    int GetHeight() const noexcept;
    int GetTileWidth() const noexcept;
    int GetTileHeight() const noexcept;
    SDL_FRect GetPixelBounds() const noexcept;
    int GetLayerCount() const noexcept;
    const char *GetLayerName(int index) const noexcept;

    void Draw(SDL_Renderer *renderer, float x = 0.0f, float y = 0.0f,
              const ::leo::Camera::Camera2D *camera = nullptr) const;
    void DrawLayer(SDL_Renderer *renderer, int layer_index, float x = 0.0f, float y = 0.0f,
                   const ::leo::Camera::Camera2D *camera = nullptr) const;

  private:
    struct Tile
    {
        std::uint32_t gid = 0;
        std::uint8_t flip_flags = 0;
    };

    struct Layer
    {
        std::string name;
        int width = 0;
        int height = 0;
        int offset_x = 0;
        int offset_y = 0;
        bool visible = true;
        float opacity = 1.0f;
        std::vector<Tile> tiles;
    };

    struct TileDrawInfo
    {
        size_t texture_index = 0;
        SDL_FRect src = {0.0f, 0.0f, 0.0f, 0.0f};
        int draw_w = 0;
        int draw_h = 0;
    };

    int map_width;
    int map_height;
    int tile_width;
    int tile_height;
    bool ready;

    std::vector<Layer> layers;
    std::vector<Texture> textures;
    std::unordered_map<std::uint32_t, TileDrawInfo> tiles;
};

} // namespace engine

#endif // LEO_TILED_MAP_H
