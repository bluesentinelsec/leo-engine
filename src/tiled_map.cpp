#include "leo/tiled_map.h"

#include "leo/camera.h"
#include "leo/vfs.h"
#include <SDL3/SDL.h>
#include <tmxlite/Layer.hpp>
#include <tmxlite/LayerGroup.hpp>
#include <tmxlite/Map.hpp>
#include <tmxlite/TileLayer.hpp>
#include <tmxlite/Tileset.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace
{

std::string NormalizePath(const std::string &path)
{
    std::string normalized = path;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');

    std::vector<std::string> parts;
    size_t start = 0;
    while (start <= normalized.size())
    {
        size_t end = normalized.find('/', start);
        if (end == std::string::npos)
        {
            end = normalized.size();
        }
        std::string part = normalized.substr(start, end - start);
        if (!part.empty() && part != ".")
        {
            if (part == "..")
            {
                if (!parts.empty())
                {
                    parts.pop_back();
                }
            }
            else
            {
                parts.push_back(part);
            }
        }
        start = end + 1;
    }

    std::string result;
    for (size_t i = 0; i < parts.size(); ++i)
    {
        if (i > 0)
        {
            result.push_back('/');
        }
        result += parts[i];
    }
    return result;
}

std::string BaseName(const std::string &path)
{
    std::string normalized = NormalizePath(path);
    size_t pos = normalized.find_last_of('/');
    if (pos == std::string::npos)
    {
        return normalized;
    }
    return normalized.substr(pos + 1);
}

std::string DirName(const std::string &path)
{
    std::string normalized = NormalizePath(path);
    size_t pos = normalized.find_last_of('/');
    if (pos == std::string::npos)
    {
        return "";
    }
    return normalized.substr(0, pos);
}

SDL_Color ColorFromId(std::uint32_t id)
{
    std::uint32_t seed = id * 2654435761u;
    Uint8 r = static_cast<Uint8>(32 + (seed & 0x7F));
    Uint8 g = static_cast<Uint8>(32 + ((seed >> 8) & 0x7F));
    Uint8 b = static_cast<Uint8>(32 + ((seed >> 16) & 0x7F));
    return {r, g, b, 255};
}

engine::Texture CreateSolidTexture(SDL_Renderer *renderer, int width, int height, SDL_Color color)
{
    if (!renderer || width <= 0 || height <= 0)
    {
        throw std::runtime_error("CreateSolidTexture requires a valid renderer and positive size");
    }

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
    if (!texture)
    {
        throw std::runtime_error(std::string("CreateSolidTexture failed to create texture: ") + SDL_GetError());
    }

    const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA32);
    if (!format)
    {
        SDL_DestroyTexture(texture);
        throw std::runtime_error("CreateSolidTexture failed to get pixel format details");
    }

    Uint32 packed = SDL_MapRGBA(format, nullptr, color.r, color.g, color.b, color.a);

    std::vector<Uint32> pixels(static_cast<size_t>(width) * static_cast<size_t>(height), packed);
    const int pitch = width * static_cast<int>(sizeof(Uint32));
    if (!SDL_UpdateTexture(texture, nullptr, pixels.data(), pitch))
    {
        SDL_DestroyTexture(texture);
        throw std::runtime_error(std::string("CreateSolidTexture failed to upload texture: ") + SDL_GetError());
    }

    return engine::Texture(texture, width, height);
}

void AppendCandidate(std::vector<std::string> *out, const std::string &candidate)
{
    if (!out || candidate.empty())
    {
        return;
    }

    for (const auto &existing : *out)
    {
        if (existing == candidate)
        {
            return;
        }
    }

    out->push_back(candidate);
}

std::vector<std::string> BuildImageCandidates(const std::string &raw_path, const std::string &map_dir)
{
    std::vector<std::string> candidates;
    if (raw_path.empty())
    {
        return candidates;
    }

    std::string normalized = NormalizePath(raw_path);
    AppendCandidate(&candidates, normalized);

    if (!map_dir.empty())
    {
        AppendCandidate(&candidates, NormalizePath(map_dir + "/" + raw_path));
    }

    const std::string marker = "resources/";
    size_t pos = normalized.find(marker);
    if (pos != std::string::npos)
    {
        AppendCandidate(&candidates, normalized.substr(pos + marker.size()));
    }

    std::string base = BaseName(normalized);
    if (!base.empty())
    {
        AppendCandidate(&candidates, NormalizePath("resources/images/" + base));
    }

    return candidates;
}

engine::Texture LoadTextureWithFallback(engine::TextureLoader &loader, SDL_Renderer *renderer,
                                        const std::vector<std::string> &candidates, int fallback_w,
                                        int fallback_h, SDL_Color fallback_color, const std::string &label,
                                        std::unordered_set<std::string> *missing)
{
    for (const auto &candidate : candidates)
    {
        try
        {
            return loader.Load(candidate.c_str());
        }
        catch (const std::exception &)
        {
        }
    }

    if (missing && missing->insert(label).second)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "TiledMap: missing tile image '%s', using placeholder",
                    label.c_str());
    }

    return CreateSolidTexture(renderer, fallback_w, fallback_h, fallback_color);
}

SDL_FPoint ApplyCameraPoint(const leo::Camera::Camera2D *camera, SDL_FPoint point)
{
    if (!camera)
    {
        return point;
    }
    return leo::Camera::WorldToScreen(*camera, point);
}

float ApplyCameraScale(const leo::Camera::Camera2D *camera, float value)
{
    if (!camera)
    {
        return value;
    }
    return value * camera->zoom;
}

} // namespace

namespace engine
{

TiledMap::TiledMap() noexcept : map_width(0), map_height(0), tile_width(0), tile_height(0), ready(false)
{
}

TiledMap::~TiledMap()
{
    Reset();
}

TiledMap::TiledMap(TiledMap &&other) noexcept = default;
TiledMap &TiledMap::operator=(TiledMap &&other) noexcept = default;

TiledMap TiledMap::LoadFromVfs(VFS &vfs, SDL_Renderer *renderer, const char *vfs_path)
{
    if (!renderer)
    {
        throw std::runtime_error("TiledMap::LoadFromVfs requires a valid SDL_Renderer");
    }
    if (!vfs_path || !*vfs_path)
    {
        throw std::runtime_error("TiledMap::LoadFromVfs requires a non-empty path");
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
        throw std::runtime_error("TiledMap::LoadFromVfs received empty map data");
    }

    std::string map_data(static_cast<const char *>(data), size);
    SDL_free(data);

    std::string map_dir = DirName(vfs_path);
    tmx::Map map;
    if (!map.loadFromString(map_data, map_dir))
    {
        throw std::runtime_error("TiledMap::LoadFromVfs failed to parse map data");
    }

    if (map.getOrientation() != tmx::Orientation::Orthogonal)
    {
        throw std::runtime_error("TiledMap::LoadFromVfs only supports orthogonal maps");
    }

    if (map.isInfinite())
    {
        throw std::runtime_error("TiledMap::LoadFromVfs does not support infinite maps yet");
    }

    TiledMap result;
    result.map_width = static_cast<int>(map.getTileCount().x);
    result.map_height = static_cast<int>(map.getTileCount().y);
    result.tile_width = static_cast<int>(map.getTileSize().x);
    result.tile_height = static_cast<int>(map.getTileSize().y);

    if (result.map_width <= 0 || result.map_height <= 0 || result.tile_width <= 0 || result.tile_height <= 0)
    {
        throw std::runtime_error("TiledMap::LoadFromVfs map has invalid dimensions");
    }

    auto collect_layers = [&](const std::vector<tmx::Layer::Ptr> &layers_ref, auto &&self) -> void {
        for (const auto &layer : layers_ref)
        {
            if (!layer)
            {
                continue;
            }

            switch (layer->getType())
            {
            case tmx::Layer::Type::Tile: {
                const auto &tile_layer = layer->getLayerAs<tmx::TileLayer>();
                Layer out = {};
                out.name = layer->getName();
                out.width = result.map_width;
                out.height = result.map_height;
                out.visible = layer->getVisible();
                out.opacity = layer->getOpacity();
                out.offset_x = layer->getOffset().x;
                out.offset_y = layer->getOffset().y;

                const auto &tiles = tile_layer.getTiles();
                out.tiles.reserve(tiles.size());
                for (const auto &tile : tiles)
                {
                    Tile entry;
                    entry.gid = tile.ID;
                    entry.flip_flags = tile.flipFlags;
                    out.tiles.push_back(entry);
                }

                result.layers.push_back(std::move(out));
                break;
            }
            case tmx::Layer::Type::Group: {
                const auto &group = layer->getLayerAs<tmx::LayerGroup>();
                self(group.getLayers(), self);
                break;
            }
            default:
                break;
            }
        }
    };

    collect_layers(map.getLayers(), collect_layers);

    if (result.layers.empty())
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "TiledMap: map has no tile layers");
    }

    TextureLoader loader(vfs, renderer);
    std::unordered_set<std::string> missing_images;

    const auto &tilesets = map.getTilesets();
    for (const auto &tileset : tilesets)
    {
        const std::string &atlas_path = tileset.getImagePath();
        if (!atlas_path.empty())
        {
            std::vector<std::string> candidates = BuildImageCandidates(atlas_path, map_dir);
            SDL_Color fallback = ColorFromId(tileset.getFirstGID());
            Texture texture =
                LoadTextureWithFallback(loader, renderer, candidates, static_cast<int>(tileset.getTileSize().x),
                                        static_cast<int>(tileset.getTileSize().y), fallback, atlas_path,
                                        &missing_images);

            size_t texture_index = result.textures.size();
            result.textures.push_back(std::move(texture));

            int tile_w = static_cast<int>(tileset.getTileSize().x);
            int tile_h = static_cast<int>(tileset.getTileSize().y);
            int columns = static_cast<int>(tileset.getColumnCount());
            int spacing = static_cast<int>(tileset.getSpacing());
            int margin = static_cast<int>(tileset.getMargin());
            std::uint32_t tile_count = tileset.getTileCount();

            for (std::uint32_t tile_id = 0; tile_id < tile_count; ++tile_id)
            {
                int col = columns > 0 ? static_cast<int>(tile_id % static_cast<std::uint32_t>(columns)) : 0;
                int row = columns > 0 ? static_cast<int>(tile_id / static_cast<std::uint32_t>(columns)) : 0;
                float src_x = static_cast<float>(margin + col * (tile_w + spacing));
                float src_y = static_cast<float>(margin + row * (tile_h + spacing));

                TileDrawInfo info = {};
                info.texture_index = texture_index;
                info.src = {src_x, src_y, static_cast<float>(tile_w), static_cast<float>(tile_h)};
                info.draw_w = result.tile_width;
                info.draw_h = result.tile_height;
                result.tiles[tileset.getFirstGID() + tile_id] = info;
            }
        }
        else
        {
            const auto &tile_defs = tileset.getTiles();
            for (const auto &tile : tile_defs)
            {
                std::string image_path = tile.imagePath;
                std::vector<std::string> candidates = BuildImageCandidates(image_path, map_dir);
                SDL_Color fallback = ColorFromId(tileset.getFirstGID() + tile.ID);

                int fallback_w = static_cast<int>(tile.imageSize.x);
                int fallback_h = static_cast<int>(tile.imageSize.y);
                if (fallback_w <= 0 || fallback_h <= 0)
                {
                    fallback_w = result.tile_width;
                    fallback_h = result.tile_height;
                }

                Texture texture = LoadTextureWithFallback(loader, renderer, candidates, fallback_w, fallback_h, fallback,
                                                         image_path, &missing_images);

                size_t texture_index = result.textures.size();
                result.textures.push_back(std::move(texture));

                TileDrawInfo info = {};
                info.texture_index = texture_index;
                info.src = {0.0f, 0.0f, static_cast<float>(result.textures.back().width),
                            static_cast<float>(result.textures.back().height)};
                info.draw_w = result.tile_width;
                info.draw_h = result.tile_height;
                result.tiles[tileset.getFirstGID() + tile.ID] = info;
            }
        }
    }

    result.ready = true;
    return result;
}

bool TiledMap::IsReady() const noexcept
{
    return ready;
}

void TiledMap::Reset() noexcept
{
    layers.clear();
    textures.clear();
    tiles.clear();
    map_width = 0;
    map_height = 0;
    tile_width = 0;
    tile_height = 0;
    ready = false;
}

int TiledMap::GetWidth() const noexcept
{
    return map_width;
}

int TiledMap::GetHeight() const noexcept
{
    return map_height;
}

int TiledMap::GetTileWidth() const noexcept
{
    return tile_width;
}

int TiledMap::GetTileHeight() const noexcept
{
    return tile_height;
}

SDL_FRect TiledMap::GetPixelBounds() const noexcept
{
    return {0.0f, 0.0f, static_cast<float>(map_width * tile_width), static_cast<float>(map_height * tile_height)};
}

int TiledMap::GetLayerCount() const noexcept
{
    return static_cast<int>(layers.size());
}

const char *TiledMap::GetLayerName(int index) const noexcept
{
    if (index < 0 || index >= static_cast<int>(layers.size()))
    {
        return nullptr;
    }
    return layers[static_cast<size_t>(index)].name.c_str();
}

void TiledMap::Draw(SDL_Renderer *renderer, float x, float y, const ::leo::Camera::Camera2D *camera) const
{
    if (!renderer || !ready)
    {
        return;
    }

    for (size_t i = 0; i < layers.size(); ++i)
    {
        DrawLayer(renderer, static_cast<int>(i), x, y, camera);
    }
}

void TiledMap::DrawLayer(SDL_Renderer *renderer, int layer_index, float x, float y,
                         const ::leo::Camera::Camera2D *camera) const
{
    if (!renderer || !ready)
    {
        return;
    }

    if (layer_index < 0 || layer_index >= static_cast<int>(layers.size()))
    {
        return;
    }

    const Layer &layer = layers[static_cast<size_t>(layer_index)];
    if (!layer.visible || layer.opacity <= 0.0f)
    {
        return;
    }

    const float base_x = x + static_cast<float>(layer.offset_x);
    const float base_y = y + static_cast<float>(layer.offset_y);
    const float opacity = std::clamp(layer.opacity, 0.0f, 1.0f);

    bool warned_diagonal = false;

    for (int row = 0; row < layer.height; ++row)
    {
        for (int col = 0; col < layer.width; ++col)
        {
            size_t index = static_cast<size_t>(row) * static_cast<size_t>(layer.width) + static_cast<size_t>(col);
            if (index >= layer.tiles.size())
            {
                continue;
            }

            const Tile &tile = layer.tiles[index];
            if (tile.gid == 0)
            {
                continue;
            }

            auto it = tiles.find(tile.gid);
            if (it == tiles.end())
            {
                continue;
            }

            const TileDrawInfo &info = it->second;
            if (info.texture_index >= textures.size())
            {
                continue;
            }

            const Texture &texture = textures[info.texture_index];
            if (!texture.handle)
            {
                continue;
            }

            SDL_FRect dst = {base_x + static_cast<float>(col * tile_width),
                             base_y + static_cast<float>(row * tile_height), static_cast<float>(info.draw_w),
                             static_cast<float>(info.draw_h)};

            SDL_FPoint screen = ApplyCameraPoint(camera, {dst.x, dst.y});
            dst.x = screen.x;
            dst.y = screen.y;
            dst.w = ApplyCameraScale(camera, dst.w);
            dst.h = ApplyCameraScale(camera, dst.h);

            SDL_FlipMode flip = SDL_FLIP_NONE;
            if (tile.flip_flags & tmx::TileLayer::FlipFlag::Horizontal)
            {
                flip = static_cast<SDL_FlipMode>(flip | SDL_FLIP_HORIZONTAL);
            }
            if (tile.flip_flags & tmx::TileLayer::FlipFlag::Vertical)
            {
                flip = static_cast<SDL_FlipMode>(flip | SDL_FLIP_VERTICAL);
            }
            if (tile.flip_flags & tmx::TileLayer::FlipFlag::Diagonal)
            {
                if (!warned_diagonal)
                {
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "TiledMap: diagonal flip not supported");
                    warned_diagonal = true;
                }
            }

            if (opacity < 1.0f)
            {
                SDL_SetTextureAlphaMod(texture.handle, static_cast<Uint8>(opacity * 255.0f));
            }

            SDL_RenderTextureRotated(renderer, texture.handle, &info.src, &dst, 0.0, nullptr, flip);

            if (opacity < 1.0f)
            {
                SDL_SetTextureAlphaMod(texture.handle, 255);
            }
        }
    }
}

} // namespace engine
