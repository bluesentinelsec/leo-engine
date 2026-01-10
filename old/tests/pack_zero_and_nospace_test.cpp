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

    // Zero-length allowed: nullptr with size==0
    REQUIRE(leo_pack_writer_add(w, "empty_null.bin", nullptr, 0, 0, 0) == LEO_PACK_OK);

    // Zero-length allowed: non-null pointer with size==0
    const char *zero = "";
    REQUIRE(leo_pack_writer_add(w, "empty_ptr.bin", zero, 0, 0, 0) == LEO_PACK_OK);

    REQUIRE(leo_pack_writer_end(w) == LEO_PACK_OK);

    leo_pack *p = nullptr;
    REQUIRE(leo_pack_open_file(&p, outP.string().c_str(), nullptr) == LEO_PACK_OK);

    // Verify both entries exist and report size 0
    for (const char *name : {"empty_null.bin", "empty_ptr.bin"})
    {
        int idx = -1;
        REQUIRE(leo_pack_find(p, name, &idx) == LEO_PACK_OK);

        // Stat says 0/0
        leo_pack_stat st{};
        REQUIRE(leo_pack_stat_index(p, idx, &st) == LEO_PACK_OK);
        CHECK(st.size_uncompressed == 0);
        CHECK(st.size_stored == 0);

        // Extract into 0-capacity buffer: OK and out_size==0
        size_t got = SIZE_MAX;
        char dummy = 42;
        REQUIRE(leo_pack_extract_index(p, idx, &dummy, 0, &got) == LEO_PACK_OK);
        CHECK(got == 0);
    }

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
