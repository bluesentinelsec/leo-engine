/* ==========================================================
 * File: include/leo/tiled.h
 * Minimal, convenient Tiled (.tmj / .json) map loader for Leo
 * ========================================================== */
#pragma once

#include "leo/engine.h" /* leo_Rectangle, leo_Vector2 */
#include "leo/export.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* ----------------------------- */
    /* GID helpers (Tiled bit flags) */
    /* ----------------------------- */
    /* Per Tiled docs, the top 3 bits encode flipping. */
    enum
    {
        LEO_TILED_FLIP_H = 0x80000000u,
        LEO_TILED_FLIP_V = 0x40000000u,
        LEO_TILED_FLIP_D = 0x20000000u,
        LEO_TILED_GID_MASK = 0x1FFFFFFFu
    };

    typedef struct
    {
        uint32_t gid_raw; /* as read (may include flip bits) */
        uint32_t id;      /* gid_raw & LEO_TILED_GID_MASK (0 if empty) */
        unsigned flip_h : 1, flip_v : 1, flip_d : 1;
    } leo_TiledGidInfo;

    LEO_API leo_TiledGidInfo leo_tiled_gid_info(uint32_t gid_raw);

    /* ----------------------------- */
    /* Tiled properties (flat list)  */
    /* ----------------------------- */
    typedef enum
    {
        LEO_TILED_PROP_STRING = 0,
        LEO_TILED_PROP_INT,
        LEO_TILED_PROP_FLOAT,
        LEO_TILED_PROP_BOOL
    } leo_TiledPropType;

    typedef struct
    {
        const char *name; /* borrowed: points into doc-owned memory */
        leo_TiledPropType type;

        union {
            const char *s;
            int i;
            double f;
            int b;
        } val;
    } leo_TiledProperty;

    /* ----------------------------- */
    /* Tileset                       */
    /* ----------------------------- */
    typedef struct
    {
        int first_gid; /* global id base */
        int tilewidth, tileheight;
        int imagewidth, imageheight;
        int columns, tilecount;
        const char *name;  /* borrowed (doc-owned) */
        const char *image; /* borrowed; may be absolute; caller may remap */
    } leo_TiledTileset;

    /* Compute source rectangle for a given global id.
       Returns false if id not in this tileset. */
    LEO_API bool leo_tiled_tileset_src(const leo_TiledTileset *ts, uint32_t base_gid, leo_Rectangle *out_src);

    /* ----------------------------- */
    /* Layers                        */
    /* ----------------------------- */
    typedef enum
    {
        LEO_TILED_LAYER_TILE = 0,
        LEO_TILED_LAYER_OBJECT
    } leo_TiledLayerType;

    typedef struct
    {
        const char *name;  /* borrowed */
        int width, height; /* in tiles */
        /* width*height raw GIDs as read (may include flip bits).
           0 means empty tile. Owned by the map; freed on leo_tiled_free. */
        uint32_t *gids;
        size_t gids_count;
    } leo_TiledTileLayer;

    /* Minimal Tiled object subset commonly used in platformers/top-down. */
    typedef struct leo_TiledObject
    {
        const char *name;     /* borrowed (may be empty) */
        const char *type;     /* borrowed (may be empty) */
        double x, y;          /* pixels in Tiled space (top-left origin) */
        double width, height; /* pixels (0 for points) */
        uint32_t gid_raw;     /* optional for tile objects (else 0) */
        /* Flat properties (optional). */
        leo_TiledProperty *props;
        int prop_count;
    } leo_TiledObject;

    typedef struct
    {
        const char *name;         /* borrowed */
        leo_TiledObject *objects; /* owned by the map */
        int object_count;
    } leo_TiledObjectLayer;

    /* Discriminated layer */
    typedef struct
    {
        leo_TiledLayerType type;

        union {
            leo_TiledTileLayer tile;
            leo_TiledObjectLayer object;
        } as;
    } leo_TiledLayer;

    /* ----------------------------- */
    /* Map                           */
    /* ----------------------------- */
    typedef struct
    {
        int width, height;         /* in tiles */
        int tilewidth, tileheight; /* pixels */

        const char *orientation; /* borrowed (e.g., "orthogonal") */
        const char *renderorder; /* borrowed (e.g., "right-down") */

        /* Properties at map level (optional). */
        leo_TiledProperty *props;
        int prop_count;

        /* Tilesets (vector) */
        leo_TiledTileset *tilesets; /* owned by the map */
        int tileset_count;

        /* Layers (vector) */
        leo_TiledLayer *layers; /* owned by the map */
        int layer_count;

        /* Internals (opaque) */
        void *_doc; /* keeps backing strings alive */
    } leo_TiledMap;

    /* ----------------------------- */
    /* Loading options & lifecycle   */
    /* ----------------------------- */
    typedef struct
    {
        /* Optional base path prefix to rewrite tileset images into your VFS logical space.
           If non-NULL and tileset->image is relative, we produce "<image_base>/<image>".
           If absolute paths appear, they are copied as-is unless a remapper is provided. */
        const char *image_base;

        /* Optional: user remapper for tileset image paths. If provided, this wins over image_base.
           Should return a pointer to a persistent string (e.g., heap you own) OR write into out_buf. */
        const char *(*remap_image)(const char *original, char *out_buf, size_t out_cap, void *user);

        void *remap_user;

        /* Accept compressed tile layers (CSV, base64, base64+zlib/gzip). Default true. */
        int allow_compression; /* nonzero to allow. */
    } leo_TiledLoadOptions;

    /* Load a Tiled map (.json/.tmj) via Leo VFS.
       On error returns NULL and sets leo error string. */
    LEO_API leo_TiledMap *leo_tiled_load(const char *logical_path, const leo_TiledLoadOptions *opt);

    /* Destroy map and all owned arrays. */
    LEO_API void leo_tiled_free(leo_TiledMap *map);

    /* Convenience: fetch a tile layer by name (NULL if not found). */
    LEO_API const leo_TiledTileLayer *leo_tiled_find_tile_layer(const leo_TiledMap *map, const char *name);

    /* Convenience: fetch an object layer by name (NULL if not found). */
    LEO_API const leo_TiledObjectLayer *leo_tiled_find_object_layer(const leo_TiledMap *map, const char *name);

    /* Convenience: map a global id -> (tileset*, src rect). Returns false if not found. */
    LEO_API bool leo_tiled_resolve_gid(const leo_TiledMap *map, uint32_t base_gid, const leo_TiledTileset **out_ts,
                                       leo_Rectangle *out_src);

    /* Convenience: per-tile accessor (x,y in tiles; returns raw gid or 0). */
    LEO_API uint32_t leo_tiled_get_gid(const leo_TiledTileLayer *tl, int x, int y);

#ifdef __cplusplus
} /* extern "C" */
#endif
