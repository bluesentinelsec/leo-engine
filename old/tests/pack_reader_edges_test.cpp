#include <catch2/catch_all.hpp>
#include <filesystem>
#include <leo/pack_reader.h>
#include <leo/pack_writer.h>
#include <vector>
namespace fs = std::filesystem;

static fs::path out_path(const char *name)
{
    return fs::temp_directory_path() / name;
}

TEST_CASE("reader: extract by name, count/stat bounds, NOTFOUND")
{
    fs::path outP = out_path("byname_bounds.leopack");
    std::error_code ec;
    fs::remove(outP, ec);

    // Build a tiny pack with two entries
    leo_pack_writer *w = nullptr;
    leo_pack_build_opts opt{};
    REQUIRE(leo_pack_writer_begin(&w, outP.string().c_str(), &opt) == LEO_PACK_OK);
    const char a[] = "A";
    const char b[] = "B";
    REQUIRE(leo_pack_writer_add(w, "a.txt", a, sizeof(a) - 1, 0, 0) == LEO_PACK_OK);
    REQUIRE(leo_pack_writer_add(w, "b.txt", b, sizeof(b) - 1, 0, 0) == LEO_PACK_OK);
    REQUIRE(leo_pack_writer_end(w) == LEO_PACK_OK);

    leo_pack *p = nullptr;
    REQUIRE(leo_pack_open_file(&p, outP.string().c_str(), nullptr) == LEO_PACK_OK);
    REQUIRE(leo_pack_count(p) == 2);

    // NOTFOUND by name
    int idx = -1;
    REQUIRE(leo_pack_find(p, "nope", &idx) == LEO_PACK_E_NOTFOUND);

    // Out-of-bounds stat/extract
    leo_pack_stat st{};
    REQUIRE(leo_pack_stat_index(p, 99, &st) == LEO_PACK_E_NOTFOUND);
    char buf[8];
    size_t got = 0;
    REQUIRE(leo_pack_extract_index(p, -1, buf, sizeof buf, &got) == LEO_PACK_E_NOTFOUND);

    // Extract by name convenience API
    got = 0;
    REQUIRE(leo_pack_extract(p, "b.txt", buf, sizeof buf, &got) == LEO_PACK_OK);
    REQUIRE(got == 1);
    REQUIRE(buf[0] == 'B');

    leo_pack_close(p);
}
