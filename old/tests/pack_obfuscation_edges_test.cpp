#include <catch2/catch_all.hpp>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <leo/pack_format.h>
#include <leo/pack_reader.h>
#include <leo/pack_writer.h>

namespace fs = std::filesystem;
static fs::path out_path(const char *name)
{
    return fs::temp_directory_path() / name;
}

TEST_CASE("obfuscation: open without password then extract fails with BAD_PASSWORD")
{
    fs::path outP = out_path("obf_no_pwd.leopack");
    std::error_code ec;
    fs::remove(outP, ec);

    const char data[] = "secret";
    leo_pack_writer *w = nullptr;
    leo_pack_build_opts opt{};
    opt.password = "pw";
    REQUIRE(leo_pack_writer_begin(&w, outP.string().c_str(), &opt) == LEO_PACK_OK);
    REQUIRE(leo_pack_writer_add(w, "s.txt", data, sizeof(data) - 1, 0, 1) == LEO_PACK_OK);
    REQUIRE(leo_pack_writer_end(w) == LEO_PACK_OK);

    // Open without password
    leo_pack *p = nullptr;
    REQUIRE(leo_pack_open_file(&p, outP.string().c_str(), nullptr) == LEO_PACK_OK);

    int idx = -1;
    REQUIRE(leo_pack_find(p, "s.txt", &idx) == LEO_PACK_OK);
    char buf[16];
    size_t got = 0;
    REQUIRE(leo_pack_extract_index(p, idx, buf, sizeof buf, &got) == LEO_PACK_E_BAD_PASSWORD);

    leo_pack_close(p);
}
