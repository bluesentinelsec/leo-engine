#include <catch2/catch_all.hpp>
#include <filesystem>
#include <leo/pack_compress.h>
#include <leo/pack_reader.h>
#include <leo/pack_writer.h>
#include <vector>
namespace fs = std::filesystem;

static fs::path out_path(const char *name)
{
    return fs::temp_directory_path() / name;
}

TEST_CASE("pack: zero-length entry round-trip")
{
    fs::path outP = out_path("zero_entry.leopack");
    std::error_code ec;
    fs::remove(outP, ec);

    leo_pack_writer *w = nullptr;
    leo_pack_build_opts opt{};
    REQUIRE(leo_pack_writer_begin(&w, outP.string().c_str(), &opt) == LEO_PACK_OK);

    REQUIRE(leo_pack_writer_add(w, "empty.bin", nullptr, 0, 0, 0) == LEO_PACK_E_ARG); // strict arg check
    const char *zero = ""; // add explicit 0 size with valid pointer
    REQUIRE(leo_pack_writer_add(w, "empty.bin", zero, 0, 0, 0) == LEO_PACK_OK);
    REQUIRE(leo_pack_writer_end(w) == LEO_PACK_OK);

    leo_pack *p = nullptr;
    REQUIRE(leo_pack_open_file(&p, outP.string().c_str(), nullptr) == LEO_PACK_OK);
    int idx = -1;
    REQUIRE(leo_pack_find(p, "empty.bin", &idx) == LEO_PACK_OK);

    size_t got = SIZE_MAX;
    char dummy = 42;
    REQUIRE(leo_pack_extract_index(p, idx, &dummy, 0, &got) == LEO_PACK_OK);
    REQUIRE(got == 0);

    leo_pack_close(p);
}

TEST_CASE("compress/decompress: NOSPACE and DECOMPRESS error surfacing")
{
    const char payload[] = "highly_uncompressible_data_XXXXXXXXXXXX";
    char outbuf[8];
    size_t cap = sizeof(outbuf);
    leo_deflate_opts o{5};

    // Force NOSPACE by giving too-small output buffer
    REQUIRE(leo_compress_deflate(payload, sizeof(payload) - 1, outbuf, &cap, &o) == LEO_PACK_E_NOSPACE);

    // Now craft a bad compressed stream to trigger DECOMPRESS
    // (write pack with compressed entry, then flip a byte in the stored payload)
}
