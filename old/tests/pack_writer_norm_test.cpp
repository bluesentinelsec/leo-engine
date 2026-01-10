#include <catch2/catch_all.hpp>
#include <filesystem>
#include <leo/pack_writer.h>
namespace fs = std::filesystem;

static fs::path out_path(const char *name)
{
    return fs::temp_directory_path() / name;
}

TEST_CASE("writer: path normalization and duplicate detection")
{
    fs::path outP = out_path("norm_dupes.leopack");
    std::error_code ec;
    fs::remove(outP, ec);

    leo_pack_writer *w = nullptr;
    leo_pack_build_opts opt{};
    opt.align = 1;
    opt.level = 0;
    REQUIRE(leo_pack_writer_begin(&w, outP.string().c_str(), &opt) == LEO_PACK_OK);

    const char data[] = "x";
    // Normalizes backslashes and strips leading "./"
    REQUIRE(leo_pack_writer_add(w, "dir\\file.bin", data, 1, 0, 0) == LEO_PACK_OK);
    // "./dir/file.bin" normalizes to same canonical logical name -> duplicate
    REQUIRE(leo_pack_writer_add(w, "./dir/file.bin", data, 1, 0, 0) == LEO_PACK_E_STATE);

    REQUIRE(leo_pack_writer_end(w) == LEO_PACK_OK);
}
