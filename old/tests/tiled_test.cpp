// tests/tiled_test.cpp
#include <catch2/catch_all.hpp>

#include <cstdio>
#include <cstring> // memcpy
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

extern "C"
{
#include <leo/base64.h>
#include <leo/error.h>
#include <leo/io.h>
#include <leo/pack_zlib.h>
#include <leo/tiled.h>
}

namespace fs = std::filesystem;

/* ---------------- Helpers ---------------- */

static fs::path make_tmp_dir(const char *tag)
{
    auto p = fs::temp_directory_path() / (std::string("leo_tiled_") + tag + "_" + std::to_string(::time(nullptr)));
    fs::create_directories(p);
    return p;
}

static fs::path write_text(const fs::path &dir, const std::string &name, const std::string &text)
{
    fs::path p = dir / name;
    std::ofstream o(p, std::ios::binary | std::ios::trunc);
    o << text;
    o.close();
    return p;
}

static const char *remap_to_images(const char *original, char *out, size_t cap, void *)
{
    // basename(original) -> images/<basename>
    const char *base = original;
    for (const char *q = original; *q; ++q)
        if (*q == '/' || *q == '\\')
            base = q + 1;
    std::snprintf(out, cap, "images/%s", base);
    return out;
}

/* ---------------- Tests ---------------- */

TEST_CASE("tiled: gid helper masks & flags")
{
    // craft: H|V|D on id 123
    uint32_t raw = LEO_TILED_FLIP_H | LEO_TILED_FLIP_V | LEO_TILED_FLIP_D | 123u;
    auto gi = leo_tiled_gid_info(raw);
    CHECK(gi.gid_raw == raw);
    CHECK(gi.id == 123u);
    CHECK(gi.flip_h == 1u);
    CHECK(gi.flip_v == 1u);
    CHECK(gi.flip_d == 1u);

    // no flags
    raw = 456u;
    gi = leo_tiled_gid_info(raw);
    CHECK(gi.id == 456u);
    CHECK_FALSE(gi.flip_h);
    CHECK_FALSE(gi.flip_v);
    CHECK_FALSE(gi.flip_d);

    // zero -> empty
    gi = leo_tiled_gid_info(0u);
    CHECK(gi.id == 0u);
}

TEST_CASE("tiled: load resources/maps/map.json and inspect", "[vfs][tiled]")
{
    // Mount test resources directory (repo-provided)
    fs::path resources = fs::path("resources");
    REQUIRE(fs::exists(resources));
    leo_ClearMounts();
    REQUIRE(leo_MountDirectory(resources.string().c_str(), 50));

    // Options: enable compression and image remap
    leo_TiledLoadOptions opt{};
    opt.allow_compression = 1;
    opt.remap_image = remap_to_images;

    leo_TiledMap *map = leo_tiled_load("maps/map.json", &opt);
    REQUIRE(map != nullptr);

    // Header fields
    CHECK(map->width == 30);
    CHECK(map->height == 20);
    CHECK(map->tilewidth == 32);
    CHECK(map->tileheight == 32);
    REQUIRE(map->orientation != nullptr);
    REQUIRE(map->renderorder != nullptr);
    CHECK(std::string(map->orientation) == "orthogonal");
    CHECK(std::string(map->renderorder) == "right-down");

    // Tilesets
    REQUIRE(map->tileset_count == 1);
    const auto &ts = map->tilesets[0];
    CHECK(ts.first_gid == 1);
    CHECK(ts.columns == 60);
    CHECK(ts.tilecount == 3600);
    CHECK(ts.tilewidth == 16);
    CHECK(ts.tileheight == 16);
    REQUIRE(ts.image != nullptr);
    CHECK(std::string(ts.image).find("spritesheet") != std::string::npos);

    // Test tileset src helper
    {
        leo_Rectangle rect{};
        CHECK(leo_tiled_tileset_src(&ts, 1, &rect));
        CHECK(rect.x == 0.0f);
        CHECK(rect.y == 0.0f);
        CHECK(rect.width == 16.0f);
        CHECK(rect.height == 16.0f);

        CHECK(leo_tiled_tileset_src(&ts, 60, &rect));
        CHECK(static_cast<int>(rect.x) == 16 * 59);
        CHECK(static_cast<int>(rect.y) == 0);

        CHECK(leo_tiled_tileset_src(&ts, 61, &rect));
        CHECK(static_cast<int>(rect.x) == 0);
        CHECK(static_cast<int>(rect.y) == 16);
    }

    // Tile layer presence + a few values using provided map.json data
    const leo_TiledTileLayer *tl = leo_tiled_find_tile_layer(map, "Tile Layer 1");
    REQUIRE(tl != nullptr);
    CHECK(tl->width == map->width);
    CHECK(tl->height == map->height);
    REQUIRE(tl->gids_count == static_cast<size_t>(tl->width * tl->height));

    // First row sample per provided JSON:
    // row0 starts with 0, 0, 2613, 2614, 2614, ...
    CHECK(leo_tiled_get_gid(tl, 0, 0) == 0u);
    CHECK(leo_tiled_get_gid(tl, 1, 0) == 0u);
    CHECK(leo_tiled_get_gid(tl, 2, 0) == 2613u);
    CHECK(leo_tiled_get_gid(tl, 3, 0) == 2614u);
    CHECK(leo_tiled_get_gid(tl, 4, 0) == 2614u);

    // Object layer checks (a few objects & coords present)
    const leo_TiledObjectLayer *ol = leo_tiled_find_object_layer(map, "Object Layer 1");
    REQUIRE(ol != nullptr);
    REQUIRE(ol->object_count >= 6);
    {
        // Sample one object; verify rough size/position
        const auto &o1 = ol->objects[0];
        CHECK(o1.width == Catch::Approx(16.0));
        CHECK(o1.height == Catch::Approx(16.0));
        CHECK(o1.x == Catch::Approx(796.8076).margin(0.02)); // small tolerance
        CHECK(o1.y == Catch::Approx(292.6153).margin(0.02));
    }

    // Resolve gid -> tileset & src via convenience
    {
        const leo_TiledTileset *out_ts = nullptr;
        leo_Rectangle rect{};
        REQUIRE(leo_tiled_resolve_gid(map, 2613u, &out_ts, &rect));
        REQUIRE(out_ts == &ts);

        int index = 2613 - ts.first_gid; // 2612
        int col = index % ts.columns;    // 2612 % 60
        int row = index / ts.columns;    // 2612 / 60
        CHECK(static_cast<int>(rect.x) == col * ts.tilewidth);
        CHECK(static_cast<int>(rect.y) == row * ts.tileheight);
        CHECK(static_cast<int>(rect.width) == ts.tilewidth);
        CHECK(static_cast<int>(rect.height) == ts.tileheight);
    }

    leo_tiled_free(map);
    leo_ClearMounts();
}

TEST_CASE("tiled: CSV, base64, and base64+zlib layer decoding via VFS temp files")
{
    // Build three tiny 3x2 maps in a temp dir; mount and load each
    fs::path tmp = make_tmp_dir("enc");
    leo_ClearMounts();
    REQUIRE(leo_MountDirectory(tmp.string().c_str(), 200));

    // Minimal tileset (metadata-only; image not used by tests)
    const char *tileset_stub = R"(
        "tilesets":[{"firstgid":1,"name":"stub","tilewidth":16,"tileheight":16,
                     "columns":8,"tilecount":64,"image":"tiles.png","imagewidth":128,"imageheight":128}]
    )";

    // A known 3x2 payload: [1..6]
    auto payload_u32 = std::vector<uint32_t>{1, 2, 3, 4, 5, 6};

    // CSV-encoded map
    {
        std::string csv = "1,2,3\n4,5,6\n";
        std::string json_str = std::string("{\"type\":\"map\",\"width\":3,\"height\":2,") +
                               "\"tilewidth\":16,\"tileheight\":16," + tileset_stub + "," + "\"layers\":[{" +
                               "\"type\":\"tilelayer\",\"name\":\"L\",\"width\":3,\"height\":2," +
                               "\"encoding\":\"csv\",\"data\":\"" + csv + "\"}]}";
        write_text(tmp, "m_csv.tmj", json_str);

        leo_TiledLoadOptions opt{};
        opt.allow_compression = 1;
        leo_TiledMap *m = leo_tiled_load("m_csv.tmj", &opt);
        REQUIRE(m != nullptr);
        const auto *L = leo_tiled_find_tile_layer(m, "L");
        REQUIRE(L != nullptr);
        REQUIRE(L->gids_count == 6);
        for (int y = 0, k = 0; y < 2; ++y)
            for (int x = 0; x < 3; ++x, ++k)
                CHECK(leo_tiled_get_gid(L, x, y) == payload_u32[k]);

        leo_tiled_free(m);
    }

    // base64 (raw, uncompressed) map
    {
        // raw bytes of little-endian u32 [1..6]
        std::vector<unsigned char> raw(payload_u32.size() * sizeof(uint32_t));
        std::memcpy(raw.data(), payload_u32.data(), raw.size());

        char *b64 = nullptr;
        size_t b64len = 0;
        REQUIRE(leo_base64_encode_alloc(raw.data(), raw.size(), &b64, &b64len) == LEO_B64_OK);

        std::string json_str = std::string("{\"type\":\"map\",\"width\":3,\"height\":2,") +
                               "\"tilewidth\":16,\"tileheight\":16," + tileset_stub + "," + "\"layers\":[{" +
                               "\"type\":\"tilelayer\",\"name\":\"L\",\"width\":3,\"height\":2," +
                               "\"encoding\":\"base64\",\"data\":\"" + std::string(b64, b64len) + "\"}]}";
        free(b64);

        write_text(tmp, "m_b64.tmj", json_str);

        leo_TiledLoadOptions opt{};
        opt.allow_compression = 1;
        leo_TiledMap *m = leo_tiled_load("m_b64.tmj", &opt);
        REQUIRE(m != nullptr);
        const auto *L = leo_tiled_find_tile_layer(m, "L");
        REQUIRE(L != nullptr);
        for (int y = 0, k = 0; y < 2; ++y)
            for (int x = 0; x < 3; ++x, ++k)
                CHECK(leo_tiled_get_gid(L, x, y) == payload_u32[k]);

        leo_tiled_free(m);
    }

    // base64 + zlib map
    {
        std::vector<unsigned char> raw(payload_u32.size() * sizeof(uint32_t));
        std::memcpy(raw.data(), payload_u32.data(), raw.size());

        // compress with zlib wrapper
        size_t bound = leo_zlib_bound(raw.size());
        std::vector<unsigned char> comp(bound);
        size_t cap = comp.size();
        REQUIRE(leo_compress_zlib(raw.data(), raw.size(), comp.data(), &cap, 5) == LEO_PACK_OK);
        comp.resize(cap);

        char *b64 = nullptr;
        size_t b64len = 0;
        REQUIRE(leo_base64_encode_alloc(comp.data(), comp.size(), &b64, &b64len) == LEO_B64_OK);

        std::string json_str =
            std::string("{\"type\":\"map\",\"width\":3,\"height\":2,") + "\"tilewidth\":16,\"tileheight\":16," +
            tileset_stub + "," + "\"layers\":[{" + "\"type\":\"tilelayer\",\"name\":\"L\",\"width\":3,\"height\":2," +
            "\"encoding\":\"base64\",\"compression\":\"zlib\",\"data\":\"" + std::string(b64, b64len) + "\"}]}";
        free(b64);

        write_text(tmp, "m_b64_zlib.tmj", json_str);

        leo_TiledLoadOptions opt{};
        opt.allow_compression = 1;
        leo_TiledMap *m = leo_tiled_load("m_b64_zlib.tmj", &opt);
        REQUIRE(m != nullptr);
        const auto *L = leo_tiled_find_tile_layer(m, "L");
        REQUIRE(L != nullptr);
        for (int y = 0, k = 0; y < 2; ++y)
            for (int x = 0; x < 3; ++x, ++k)
                CHECK(leo_tiled_get_gid(L, x, y) == payload_u32[k]);

        leo_tiled_free(m);
    }

    leo_ClearMounts();
    fs::remove_all(tmp);
}

TEST_CASE("tiled: tileset src + resolve_gid rejects out-of-range ids")
{
    // Synthetic tileset
    leo_TiledTileset ts{};
    ts.first_gid = 100;
    ts.tilewidth = 8;
    ts.tileheight = 8;
    ts.columns = 4;
    ts.tilecount = 8;

    leo_Rectangle rect{};
    CHECK_FALSE(leo_tiled_tileset_src(&ts, 99, &rect));  // below range
    CHECK(leo_tiled_tileset_src(&ts, 100, &rect));       // first
    CHECK_FALSE(leo_tiled_tileset_src(&ts, 108, &rect)); // end is exclusive

    // resolve_gid needs a real map; create tiny one via temp file
    fs::path tmp = make_tmp_dir("range");
    leo_ClearMounts();
    REQUIRE(leo_MountDirectory(tmp.string().c_str(), 200));

    const std::string tiny_map_json = R"({
        "type":"map","width":1,"height":1,"tilewidth":8,"tileheight":8,
        "tilesets":[{"firstgid":100,"name":"s","tilewidth":8,"tileheight":8,"columns":4,"tilecount":8,
                     "image":"x.png","imagewidth":32,"imageheight":16}],
        "layers":[{"type":"tilelayer","name":"L","width":1,"height":1,"data":[100]}]
    })";
    write_text(tmp, "one.tmj", tiny_map_json);

    leo_TiledLoadOptions opt{};
    opt.allow_compression = 1;
    leo_TiledMap *m = leo_tiled_load("one.tmj", &opt);
    REQUIRE(m);

    const leo_TiledTileset *out_ts = nullptr;
    leo_Rectangle rr{}; // fresh local rect for resolve_gid checks
    CHECK(leo_tiled_resolve_gid(m, 100, &out_ts, &rr));
    CHECK(out_ts == &m->tilesets[0]);
    CHECK_FALSE(leo_tiled_resolve_gid(m, 99, &out_ts, &rr));
    CHECK_FALSE(leo_tiled_resolve_gid(m, 108, &out_ts, &rr));

    leo_tiled_free(m);
    leo_ClearMounts();
    fs::remove_all(tmp);
}

TEST_CASE("tiled: per-tile accessor bounds")
{
    // Make a 2x1 with data [10, 0]
    fs::path tmp = make_tmp_dir("bounds");
    leo_ClearMounts();
    REQUIRE(leo_MountDirectory(tmp.string().c_str(), 200));

    const std::string small_map_json = R"({
        "type":"map","width":2,"height":1,"tilewidth":16,"tileheight":16,
        "tilesets":[{"firstgid":1,"name":"s","tilewidth":16,"tileheight":16,"columns":1,"tilecount":1,
                     "image":"x.png","imagewidth":16,"imageheight":16}],
        "layers":[{"type":"tilelayer","name":"L","width":2,"height":1,"data":[10,0]}]
    })";
    write_text(tmp, "b.tmj", small_map_json);

    leo_TiledMap *m = leo_tiled_load("b.tmj", nullptr);
    REQUIRE(m);
    const auto *L = leo_tiled_find_tile_layer(m, "L");
    REQUIRE(L);

    CHECK(leo_tiled_get_gid(L, 0, 0) == 10u);
    CHECK(leo_tiled_get_gid(L, 1, 0) == 0u);
    // OOB should be safe (implementation may clamp/return 0) – we assert it returns 0.
    CHECK(leo_tiled_get_gid(L, -1, 0) == 0u);
    CHECK(leo_tiled_get_gid(L, 2, 0) == 0u);
    CHECK(leo_tiled_get_gid(L, 0, 1) == 0u);

    leo_tiled_free(m);
    leo_ClearMounts();
    fs::remove_all(tmp);
}
