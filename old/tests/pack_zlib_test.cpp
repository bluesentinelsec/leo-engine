#include "leo/pack_errors.h"
#include "leo/pack_zlib.h"

#include <catch2/catch_all.hpp>
#include <cstdint>
#include <cstring>
#include <vector>

// Simple deterministic filler (no <random> to avoid platform variance)
static void fill_pattern(std::vector<unsigned char> &buf, uint32_t seed)
{
    uint32_t x = seed ? seed : 0x12345678u;
    for (size_t i = 0; i < buf.size(); ++i)
    {
        // LCG-ish
        x = x * 1664525u + 1013904223u;
        buf[i] = (unsigned char)((x >> 24) ^ (x >> 16) ^ (x >> 8) ^ x);
    }
}

TEST_CASE("zlib: empty input round-trip")
{
    const unsigned char *src = reinterpret_cast<const unsigned char *>(""); // len = 0
    const size_t src_len = 0;

    size_t cap = leo_zlib_bound(src_len);
    std::vector<unsigned char> comp(cap ? cap : 64); // bound may be 64 for 0 input per our bound
    size_t out_sz = comp.size();
    auto r = leo_compress_zlib(src, src_len, comp.data(), &out_sz, /*level*/ 5);
    REQUIRE(r == LEO_PACK_OK);
    REQUIRE(out_sz > 0);
    comp.resize(out_sz);

    std::vector<unsigned char> plain(16, 0xCD); // any small buffer works
    size_t plain_sz = plain.size();
    r = leo_decompress_zlib(comp.data(), comp.size(), plain.data(), &plain_sz);
    REQUIRE(r == LEO_PACK_OK);
    CHECK(plain_sz == 0); // empty payload after inflate
}

TEST_CASE("zlib: small buffer round-trip")
{
    const char *text = "The quick brown fox jumps over the lazy dog.";
    const size_t src_len = std::strlen(text);

    size_t cap = leo_zlib_bound(src_len);
    std::vector<unsigned char> comp(cap);
    size_t out_sz = comp.size();
    auto r = leo_compress_zlib(text, src_len, comp.data(), &out_sz, /*level*/ 6);
    REQUIRE(r == LEO_PACK_OK);
    comp.resize(out_sz);

    std::vector<unsigned char> plain(src_len);
    size_t plain_sz = plain.size();
    r = leo_decompress_zlib(comp.data(), comp.size(), plain.data(), &plain_sz);
    REQUIRE(r == LEO_PACK_OK);
    REQUIRE(plain_sz == src_len);
    CHECK(std::memcmp(plain.data(), text, src_len) == 0);
}

TEST_CASE("zlib: large buffer round-trip")
{
    const size_t N = 128 * 1024; // 128 KiB
    std::vector<unsigned char> src(N);
    fill_pattern(src, 0xC0FFEEu);

    // Compress
    size_t cap = leo_zlib_bound(src.size());
    std::vector<unsigned char> comp(cap);
    size_t out_sz = comp.size();
    auto r = leo_compress_zlib(src.data(), src.size(), comp.data(), &out_sz, /*level*/ 7);
    REQUIRE(r == LEO_PACK_OK);
    REQUIRE(out_sz > 0);
    REQUIRE(out_sz <= comp.size());
    comp.resize(out_sz);

    // Decompress
    std::vector<unsigned char> plain(src.size(), 0);
    size_t plain_sz = plain.size();
    r = leo_decompress_zlib(comp.data(), comp.size(), plain.data(), &plain_sz);
    REQUIRE(r == LEO_PACK_OK);
    REQUIRE(plain_sz == src.size());
    CHECK(std::memcmp(plain.data(), src.data(), src.size()) == 0);
}

TEST_CASE("zlib: NOSPACE on compress when buffer smaller than bound")
{
    const unsigned char data[32] = {0};
    size_t need = leo_zlib_bound(sizeof(data));
    REQUIRE(need > 0);

    // Allocate slightly less than required to force NOSPACE.
    std::vector<unsigned char> comp(need - 1);
    size_t out_sz = comp.size();
    auto r = leo_compress_zlib(data, sizeof(data), comp.data(), &out_sz, /*level*/ 5);
    CHECK(r == LEO_PACK_E_NOSPACE);

    // Out size should remain the caller-provided capacity (function returns early)
    CHECK(out_sz == comp.size());
}

TEST_CASE("zlib: DECOMPRESS error on corrupted stream")
{
    const char *text = "corrupt me please";
    size_t src_len = std::strlen(text);

    // Compress valid stream
    size_t cap = leo_zlib_bound(src_len);
    std::vector<unsigned char> comp(cap);
    size_t out_sz = comp.size();
    auto r = leo_compress_zlib(text, src_len, comp.data(), &out_sz, /*level*/ 5);
    REQUIRE(r == LEO_PACK_OK);
    comp.resize(out_sz);

    // Corrupt a middle byte (avoid first 2 zlib header bytes)
    if (comp.size() > 6)
    {
        comp[5] ^= 0xFF;
    }

    std::vector<unsigned char> plain(256);
    size_t plain_sz = plain.size();
    r = leo_decompress_zlib(comp.data(), comp.size(), plain.data(), &plain_sz);
    CHECK(r == LEO_PACK_E_DECOMPRESS);
}
