// src/tiled.c
#include "leo/tiled.h"

#include "leo/base64.h"
#include "leo/csv.h"
#include "leo/error.h"
#include "leo/json.h"
#include "leo/pack_zlib.h"

#include <SDL3/SDL_stdinc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------- Internals ---------------- */

typedef struct leo__TiledInternals
{
    leo_JsonDoc *json; /* keep JSON strings alive */
    char **owned;      /* heap strings we allocated (remapped paths, joins, etc.) */
    int owned_count;
    int owned_cap;
} leo__TiledInternals;

static void *leo__malloc_zero(size_t n)
{
    void *p = SDL_malloc(n);
    if (p)
        SDL_memset(p, 0, n);
    return p;
}

static int leo__str_eq(const char *a, const char *b)
{
    if (!a || !b)
        return 0;
    return strcmp(a, b) == 0;
}

static int leo__ensure_owned_cap(leo__TiledInternals *in, int need)
{
    if (in->owned_cap >= need)
        return 1;
    int newcap = in->owned_cap ? in->owned_cap * 2 : 8;
    while (newcap < need)
        newcap *= 2;
    char **np = (char **)SDL_realloc(in->owned, (size_t)newcap * sizeof(char *));
    if (!np)
        return 0;
    in->owned = np;
    in->owned_cap = newcap;
    return 1;
}

static char *leo__own_dup(leo__TiledInternals *in, const char *s)
{
    if (!s)
        return NULL;
    size_t n = strlen(s);
    char *p = (char *)SDL_malloc(n + 1);
    if (!p)
        return NULL;
    SDL_memcpy(p, s, n + 1);
    if (!leo__ensure_owned_cap(in, in->owned_count + 1))
    {
        SDL_free(p);
        return NULL;
    }
    in->owned[in->owned_count++] = p;
    return p;
}

/* Very small absolute-path guesser (Unix-style '/', basic Windows drive ':') */
static int leo__path_is_absolute(const char *p)
{
    if (!p || !*p)
        return 0;
    if (p[0] == '/')
        return 1; /* Unix/mac */
    if ((p[0] && p[1] == ':'))
        return 1; /* "C:" style – crude but enough */
    return 0;
}

static char *leo__join_path(leo__TiledInternals *in, const char *base, const char *rel)
{
    if (!base || !rel)
        return NULL;
    size_t nb = strlen(base), nr = strlen(rel);
    /* ensure exactly one '/' between */
    int need_slash = 1;
    if (nb > 0 && (base[nb - 1] == '/' || base[nb - 1] == '\\'))
        need_slash = 0;
    while (*rel == '/')
        rel++; /* strip leading '/' to avoid double slash */

    size_t out_n = nb + (need_slash ? 1 : 0) + nr;
    char *out = (char *)SDL_malloc(out_n + 1);
    if (!out)
        return NULL;
    size_t k = 0;
    SDL_memcpy(out + k, base, nb);
    k += nb;
    if (need_slash)
    {
        out[k++] = '/';
    }
    SDL_memcpy(out + k, rel, nr);
    k += nr;
    out[k] = '\0';

    if (!leo__ensure_owned_cap(in, in->owned_count + 1))
    {
        SDL_free(out);
        return NULL;
    }
    in->owned[in->owned_count++] = out;
    return out;
}

/* Parse ["properties"] array -> flat leo_TiledProperty list. */
static int leo__parse_properties(leo_JsonNode obj, leo_TiledProperty **out_props, int *out_count)
{
    *out_props = NULL;
    *out_count = 0;

    leo_JsonNode props = leo_json_obj_get(obj, "properties");
    if (leo_json_is_null(props) || !leo_json_is_array(props))
        return 1; /* ok, none */

    size_t n = leo_json_arr_size(props);
    if (n == 0)
        return 1;

    leo_TiledProperty *arr = (leo_TiledProperty *)SDL_calloc(n, sizeof(leo_TiledProperty));
    if (!arr)
        return 0;

    int count = 0;
    for (size_t i = 0; i < n; ++i)
    {
        leo_JsonNode p = leo_json_arr_get(props, i);
        if (!leo_json_is_object(p))
            continue;

        const char *nm = NULL;
        (void)leo_json_get_string(p, "name", &nm);

        const char *ty = NULL;
        (void)leo_json_get_string(p, "type", &ty);
        if (!ty)
            ty = "string";

        arr[count].name = nm;

        if (strcmp(ty, "string") == 0)
        {
            arr[count].type = LEO_TILED_PROP_STRING;
            arr[count].val.s = leo_json_as_string(leo_json_obj_get(p, "value"));
        }
        else if (strcmp(ty, "int") == 0 || strcmp(ty, "integer") == 0)
        {
            arr[count].type = LEO_TILED_PROP_INT;
            arr[count].val.i = leo_json_as_int(leo_json_obj_get(p, "value"));
        }
        else if (strcmp(ty, "float") == 0 || strcmp(ty, "double") == 0 || strcmp(ty, "number") == 0)
        {
            arr[count].type = LEO_TILED_PROP_FLOAT;
            arr[count].val.f = leo_json_as_double(leo_json_obj_get(p, "value"));
        }
        else if (strcmp(ty, "bool") == 0 || strcmp(ty, "boolean") == 0)
        {
            arr[count].type = LEO_TILED_PROP_BOOL;
            arr[count].val.b = leo_json_as_bool(leo_json_obj_get(p, "value")) ? 1 : 0;
        }
        else
        {
            /* default to string */
            arr[count].type = LEO_TILED_PROP_STRING;
            arr[count].val.s = leo_json_as_string(leo_json_obj_get(p, "value"));
        }
        ++count;
    }

    *out_props = arr;
    *out_count = count;
    return 1;
}

/* Decode tile layer "data" according to encoding/compression into a freshly malloc'd uint32_t[] of count=width*height.
 */
static int leo__load_tile_data(const leo_TiledLoadOptions *opt, leo_JsonNode layer_obj, int width, int height,
                               uint32_t **out, size_t *out_count)
{
    *out = NULL;
    *out_count = 0;
    size_t need = (size_t)width * (size_t)height;

    const char *encoding = NULL;
    (void)leo_json_get_string(layer_obj, "encoding", &encoding);

    if (!encoding)
    {
        /* Expect an array of numbers. */
        leo_JsonNode data = leo_json_obj_get(layer_obj, "data");
        if (!leo_json_is_array(data))
        {
            leo_SetError("tiled: tilelayer missing array 'data'");
            return 0;
        }
        size_t n = leo_json_arr_size(data);
        if (n != need)
        {
            leo_SetError("tiled: tilelayer data length %zu != expected %zu", n, need);
            return 0;
        }

        uint32_t *v = (uint32_t *)SDL_malloc(need * sizeof(uint32_t));
        if (!v)
        {
            leo_SetError("tiled: OOM for tile data");
            return 0;
        }
        for (size_t i = 0; i < n; ++i)
        {
            v[i] = (uint32_t)leo_json_as_int(leo_json_arr_get(data, i));
        }
        *out = v;
        *out_count = need;
        return 1;
    }

    if (strcmp(encoding, "csv") == 0)
    {
        const char *csv = leo_json_as_string(leo_json_obj_get(layer_obj, "data"));
        if (!csv)
        {
            leo_SetError("tiled: CSV-encoded layer missing string 'data'");
            return 0;
        }
        uint32_t *v = NULL;
        size_t cnt = 0;
        leo_csv_opts opts = {0};
        opts.delimiter = ',';
        opts.quote = '"';
        opts.trim_ws = 1;
        opts.allow_crlf = 1;
        if (leo_csv_parse_uint32_alloc(csv, strlen(csv), &v, &cnt, &opts) != LEO_CSV_OK)
        {
            leo_SetError("tiled: failed to parse CSV layer data");
            return 0;
        }
        if (cnt != need)
        {
            SDL_free(v);
            leo_SetError("tiled: CSV layer length %zu != expected %zu", cnt, need);
            return 0;
        }
        *out = v;
        *out_count = cnt;
        return 1;
    }

    if (strcmp(encoding, "base64") == 0)
    {
        const char *data_b64 = leo_json_as_string(leo_json_obj_get(layer_obj, "data"));
        if (!data_b64)
        {
            leo_SetError("tiled: base64-encoded layer missing string 'data'");
            return 0;
        }

        unsigned char *decoded = NULL;
        size_t dec_sz = 0;
        if (leo_base64_decode_alloc(data_b64, strlen(data_b64), &decoded, &dec_sz) != LEO_B64_OK)
        {
            leo_SetError("tiled: base64 decode failed");
            return 0;
        }

        const char *compression = leo_json_as_string(leo_json_obj_get(layer_obj, "compression"));
        unsigned char *raw = decoded;
        size_t raw_sz = dec_sz;
        unsigned char *decomp_buf = NULL;

        int allow_comp = 1;
        if (opt)
            allow_comp = opt->allow_compression ? 1 : 0;

        if (compression && *compression)
        {
            if (!allow_comp)
            {
                SDL_free(decoded);
                leo_SetError("tiled: compressed tile layer encountered but allow_compression=0");
                return 0;
            }
            if (strcmp(compression, "zlib") == 0)
            {
                size_t expect = need * sizeof(uint32_t);
                decomp_buf = (unsigned char *)SDL_malloc(expect);
                if (!decomp_buf)
                {
                    SDL_free(decoded);
                    leo_SetError("tiled: OOM for zlib output");
                    return 0;
                }
                size_t cap = expect;
                if (leo_decompress_zlib(decoded, dec_sz, decomp_buf, &cap) != LEO_PACK_OK)
                {
                    SDL_free(decoded);
                    SDL_free(decomp_buf);
                    leo_SetError("tiled: zlib decompression failed");
                    return 0;
                }
                raw = decomp_buf;
                raw_sz = cap;
            }
            else
            {
                /* Only zlib is needed for tests. */
                SDL_free(decoded);
                leo_SetError("tiled: unsupported compression '%s'", compression);
                return 0;
            }
        }

        size_t expect = need * sizeof(uint32_t);
        if (raw_sz != expect)
        {
            if (decomp_buf)
                SDL_free(decomp_buf);
            SDL_free(decoded);
            leo_SetError("tiled: base64 layer bytes %zu != expected %zu", raw_sz, expect);
            return 0;
        }

        uint32_t *v = (uint32_t *)SDL_malloc(expect);
        if (!v)
        {
            if (decomp_buf)
                SDL_free(decomp_buf);
            SDL_free(decoded);
            leo_SetError("tiled: OOM for tile data");
            return 0;
        }
        SDL_memcpy(v, raw, expect);

        if (decomp_buf)
            SDL_free(decomp_buf);
        SDL_free(decoded);

        *out = v;
        *out_count = need;
        return 1;
    }

    leo_SetError("tiled: unsupported encoding '%s'", encoding);
    return 0;
}

/* Parse object array into leo_TiledObject[] */
static int leo__parse_objects(leo_JsonNode layer_obj, leo_TiledObject **out_objs, int *out_count)
{
    *out_objs = NULL;
    *out_count = 0;

    leo_JsonNode arr = leo_json_obj_get(layer_obj, "objects");
    if (!leo_json_is_array(arr))
    {
        leo_SetError("tiled: objectgroup missing 'objects' array");
        return 0;
    }

    size_t n = leo_json_arr_size(arr);
    if (n == 0)
    {
        *out_objs = NULL;
        *out_count = 0;
        return 1;
    }

    leo_TiledObject *v = (leo_TiledObject *)SDL_calloc(n, sizeof(leo_TiledObject));
    if (!v)
    {
        leo_SetError("tiled: OOM for objects");
        return 0;
    }

    for (size_t i = 0; i < n; ++i)
    {
        leo_JsonNode o = leo_json_arr_get(arr, i);
        if (!leo_json_is_object(o))
            continue;

        (void)leo_json_get_string(o, "name", &v[i].name);
        (void)leo_json_get_string(o, "type", &v[i].type);
        v[i].x = leo_json_as_double(leo_json_obj_get(o, "x"));
        v[i].y = leo_json_as_double(leo_json_obj_get(o, "y"));
        v[i].width = leo_json_as_double(leo_json_obj_get(o, "width"));
        v[i].height = leo_json_as_double(leo_json_obj_get(o, "height"));
        v[i].gid_raw = (uint32_t)leo_json_as_int(leo_json_obj_get(o, "gid"));

        /* properties (optional) */
        if (!leo__parse_properties(o, &v[i].props, &v[i].prop_count))
        {
            /* clean already-parsed objects */
            for (size_t k = 0; k <= i; ++k)
                SDL_free(v[k].props);
            SDL_free(v);
            return 0;
        }
    }

    *out_objs = v;
    *out_count = (int)n;
    return 1;
}

/* ---------------- Public API ---------------- */

bool leo_tiled_tileset_src(const leo_TiledTileset *ts, uint32_t base_gid, leo_Rectangle *out_src)
{
    if (!ts || !out_src)
        return false;
    int idx = (int)base_gid - ts->first_gid;
    if (idx < 0 || idx >= ts->tilecount)
        return false;
    int col = (ts->columns > 0) ? (idx % ts->columns) : 0;
    int row = (ts->columns > 0) ? (idx / ts->columns) : 0;
    out_src->x = (float)(col * ts->tilewidth);
    out_src->y = (float)(row * ts->tileheight);
    out_src->width = (float)ts->tilewidth;
    out_src->height = (float)ts->tileheight;
    return true;
}

/* GID helper implementation (external linkage) */
leo_TiledGidInfo leo_tiled_gid_info(uint32_t gid_raw)
{
    leo_TiledGidInfo i;
    i.gid_raw = gid_raw;
    i.flip_h = (gid_raw & LEO_TILED_FLIP_H) ? 1u : 0u;
    i.flip_v = (gid_raw & LEO_TILED_FLIP_V) ? 1u : 0u;
    i.flip_d = (gid_raw & LEO_TILED_FLIP_D) ? 1u : 0u;
    i.id = gid_raw & LEO_TILED_GID_MASK;
    return i;
}

leo_TiledMap *leo_tiled_load(const char *logical_path, const leo_TiledLoadOptions *opt)
{
    const char *jerr = NULL;
    leo_JsonDoc *doc = leo_json_load(logical_path, &jerr);
    if (!doc)
    {
        leo_SetError("tiled: failed to load '%s': %s", logical_path ? logical_path : "(null)",
                     jerr ? jerr : "parse error");
        return NULL;
    }

    leo__TiledInternals *intern = (leo__TiledInternals *)leo__malloc_zero(sizeof(leo__TiledInternals));
    if (!intern)
    {
        leo_json_free(doc);
        leo_SetError("tiled: OOM");
        return NULL;
    }
    intern->json = doc;

    leo_TiledMap *map = (leo_TiledMap *)leo__malloc_zero(sizeof(leo_TiledMap));
    if (!map)
    {
        leo_json_free(doc);
        SDL_free(intern);
        leo_SetError("tiled: OOM");
        return NULL;
    }
    map->_doc = intern;

    leo_JsonNode root = leo_json_root(doc);
    if (!leo_json_is_object(root))
    {
        leo_SetError("tiled: root not object");
        goto fail;
    }

    /* header */
    if (!leo_json_get_int(root, "width", &map->width) || !leo_json_get_int(root, "height", &map->height) ||
        !leo_json_get_int(root, "tilewidth", &map->tilewidth) ||
        !leo_json_get_int(root, "tileheight", &map->tileheight))
    {
        leo_SetError("tiled: missing width/height/tilewidth/tileheight");
        goto fail;
    }
    (void)leo_json_get_string(root, "orientation", &map->orientation);
    (void)leo_json_get_string(root, "renderorder", &map->renderorder);

    /* map-level properties (optional) */
    if (!leo__parse_properties(root, &map->props, &map->prop_count))
        goto fail;

    /* tilesets */
    leo_JsonNode tilesets = leo_json_obj_get(root, "tilesets");
    if (leo_json_is_array(tilesets))
    {
        size_t n = leo_json_arr_size(tilesets);
        if (n > 0)
        {
            map->tilesets = (leo_TiledTileset *)SDL_calloc(n, sizeof(leo_TiledTileset));
            if (!map->tilesets)
            {
                leo_SetError("tiled: OOM tilesets");
                goto fail;
            }
            map->tileset_count = (int)n;

            for (size_t i = 0; i < n; ++i)
            {
                leo_JsonNode ts = leo_json_arr_get(tilesets, i);
                if (!leo_json_is_object(ts))
                    continue;

                (void)leo_json_get_int(ts, "firstgid", &map->tilesets[i].first_gid);
                (void)leo_json_get_int(ts, "tilewidth", &map->tilesets[i].tilewidth);
                (void)leo_json_get_int(ts, "tileheight", &map->tilesets[i].tileheight);
                (void)leo_json_get_int(ts, "imagewidth", &map->tilesets[i].imagewidth);
                (void)leo_json_get_int(ts, "imageheight", &map->tilesets[i].imageheight);
                (void)leo_json_get_int(ts, "columns", &map->tilesets[i].columns);
                (void)leo_json_get_int(ts, "tilecount", &map->tilesets[i].tilecount);
                (void)leo_json_get_string(ts, "name", &map->tilesets[i].name);

                const char *img = NULL;
                (void)leo_json_get_string(ts, "image", &img);

                /* Path remapping rules */
                if (img)
                {
                    const char *remapped = NULL;
                    char tmpbuf[1024];

                    if (opt && opt->remap_image)
                    {
                        const char *r = opt->remap_image(img, tmpbuf, sizeof(tmpbuf), opt->remap_user);
                        remapped = r ? r : img;
                        /* Make persistent copy */
                        char *owned = leo__own_dup(intern, remapped);
                        if (!owned)
                        {
                            leo_SetError("tiled: OOM for remapped image");
                            goto fail;
                        }
                        map->tilesets[i].image = owned;
                    }
                    else if (opt && opt->image_base && !leo__path_is_absolute(img))
                    {
                        char *joined = leo__join_path(intern, opt->image_base, img);
                        if (!joined)
                        {
                            leo_SetError("tiled: OOM for image_base join");
                            goto fail;
                        }
                        map->tilesets[i].image = joined;
                    }
                    else
                    {
                        /* Use original JSON-owned string */
                        map->tilesets[i].image = img;
                    }
                }
            }
        }
    }

    /* layers */
    leo_JsonNode layers = leo_json_obj_get(root, "layers");
    if (!leo_json_is_array(layers))
    {
        leo_SetError("tiled: missing layers array");
        goto fail;
    }

    size_t L = leo_json_arr_size(layers);
    if (L > 0)
    {
        map->layers = (leo_TiledLayer *)SDL_calloc(L, sizeof(leo_TiledLayer));
        if (!map->layers)
        {
            leo_SetError("tiled: OOM layers");
            goto fail;
        }
        map->layer_count = (int)L;

        for (size_t i = 0; i < L; ++i)
        {
            leo_JsonNode lay = leo_json_arr_get(layers, i);
            if (!leo_json_is_object(lay))
                continue;

            const char *type = leo_json_as_string(leo_json_obj_get(lay, "type"));
            const char *name = leo_json_as_string(leo_json_obj_get(lay, "name"));

            if (type && strcmp(type, "tilelayer") == 0)
            {
                map->layers[i].type = LEO_TILED_LAYER_TILE;
                map->layers[i].as.tile.name = name;
                (void)leo_json_get_int(lay, "width", &map->layers[i].as.tile.width);
                (void)leo_json_get_int(lay, "height", &map->layers[i].as.tile.height);

                uint32_t *gids = NULL;
                size_t cnt = 0;
                if (!leo__load_tile_data(opt, lay, map->layers[i].as.tile.width, map->layers[i].as.tile.height, &gids,
                                         &cnt))
                {
                    goto fail;
                }
                map->layers[i].as.tile.gids = gids;
                map->layers[i].as.tile.gids_count = cnt;
            }
            else if (type && (strcmp(type, "objectgroup") == 0))
            {
                map->layers[i].type = LEO_TILED_LAYER_OBJECT;
                map->layers[i].as.object.name = name;

                leo_TiledObject *objs = NULL;
                int oc = 0;
                if (!leo__parse_objects(lay, &objs, &oc))
                    goto fail;
                map->layers[i].as.object.objects = objs;
                map->layers[i].as.object.object_count = oc;
            }
            else
            {
                /* Ignore unsupported layer types (image, group, etc.) */
                /* Leave zeroed entry; tests don't rely on them. */
            }
        }
    }

    return map;

fail:
    leo_tiled_free(map);
    return NULL;
}

void leo_tiled_free(leo_TiledMap *map)
{
    if (!map)
        return;
    leo__TiledInternals *intern = (leo__TiledInternals *)map->_doc;

    /* free layers */
    if (map->layers)
    {
        for (int i = 0; i < map->layer_count; ++i)
        {
            if (map->layers[i].type == LEO_TILED_LAYER_TILE)
            {
                SDL_free(map->layers[i].as.tile.gids);
            }
            else if (map->layers[i].type == LEO_TILED_LAYER_OBJECT)
            {
                leo_TiledObjectLayer *ol = &map->layers[i].as.object;
                if (ol->objects)
                {
                    for (int k = 0; k < ol->object_count; ++k)
                    {
                        SDL_free(ol->objects[k].props);
                    }
                }
                SDL_free(ol->objects);
            }
        }
        SDL_free(map->layers);
    }

    /* free tilesets: only our *owned* strings were tracked in intern->owned */
    SDL_free(map->tilesets);

    /* map-level props */
    SDL_free(map->props);

    if (intern)
    {
        /* Owned strings we duplicated (remapped paths, joins) */
        for (int i = 0; i < intern->owned_count; ++i)
            SDL_free(intern->owned[i]);
        SDL_free(intern->owned);

        if (intern->json)
            leo_json_free(intern->json);
        SDL_free(intern);
    }
    SDL_free(map);
}

const leo_TiledTileLayer *leo_tiled_find_tile_layer(const leo_TiledMap *map, const char *name)
{
    if (!map || !map->layers || !name)
        return NULL;
    for (int i = 0; i < map->layer_count; ++i)
    {
        if (map->layers[i].type != LEO_TILED_LAYER_TILE)
            continue;
        const char *nm = map->layers[i].as.tile.name;
        if (nm && strcmp(nm, name) == 0)
            return &map->layers[i].as.tile;
    }
    return NULL;
}

const leo_TiledObjectLayer *leo_tiled_find_object_layer(const leo_TiledMap *map, const char *name)
{
    if (!map || !map->layers || !name)
        return NULL;
    for (int i = 0; i < map->layer_count; ++i)
    {
        if (map->layers[i].type != LEO_TILED_LAYER_OBJECT)
            continue;
        const char *nm = map->layers[i].as.object.name;
        if (nm && strcmp(nm, name) == 0)
            return &map->layers[i].as.object;
    }
    return NULL;
}

bool leo_tiled_resolve_gid(const leo_TiledMap *map, uint32_t base_gid, const leo_TiledTileset **out_ts,
                           leo_Rectangle *out_src)
{
    if (out_ts)
        *out_ts = NULL;
    if (out_src)
        SDL_memset(out_src, 0, sizeof(*out_src));
    if (!map || !map->tilesets || map->tileset_count <= 0)
        return false;

    for (int i = 0; i < map->tileset_count; ++i)
    {
        const leo_TiledTileset *ts = &map->tilesets[i];
        int idx = (int)base_gid - ts->first_gid;
        if (idx < 0 || idx >= ts->tilecount)
            continue;

        if (out_ts)
            *out_ts = ts;
        if (out_src)
        {
            leo_tiled_tileset_src(ts, base_gid, out_src);
        }
        return true;
    }
    return false;
}

uint32_t leo_tiled_get_gid(const leo_TiledTileLayer *tl, int x, int y)
{
    if (!tl || !tl->gids || tl->width <= 0 || tl->height <= 0)
        return 0u;
    if (x < 0 || y < 0 || x >= tl->width || y >= tl->height)
        return 0u;
    size_t idx = (size_t)y * (size_t)tl->width + (size_t)x;
    if (idx >= tl->gids_count)
        return 0u;
    return tl->gids[idx];
}
