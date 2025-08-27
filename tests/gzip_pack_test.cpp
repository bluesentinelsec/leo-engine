#include "catch2/catch_test_macros.hpp"
#include "leo/gzip_pack.h"
#include <cstring>
#include <string>
#include <vector>

TEST_CASE("gzip: round-trip", "[pack][gzip]")
{
    const std::string msg = "Tiled gzip payload — hello hello hello! 0123456789\n";

    // Compress
    std::vector<unsigned char> comp(leo_gzip_bound(msg.size()));
    size_t comp_cap = comp.size();
    auto rc = leo_compress_gzip(msg.data(), msg.size(), comp.data(), &comp_cap, 5);
    REQUIRE(rc == LEO_PACK_OK);
    comp.resize(comp_cap);

    // Decompress
    std::vector<unsigned char> out(msg.size() + 32);
    size_t out_cap = out.size();
    rc = leo_decompress_gzip(comp.data(), comp.size(), out.data(), &out_cap);
    REQUIRE(rc == LEO_PACK_OK);

    REQUIRE(out_cap == msg.size());
    REQUIRE(std::memcmp(out.data(), msg.data(), msg.size()) == 0);
}

TEST_CASE("gzip: nospace", "[pack][gzip]")
{
    const char *s = "short";
    unsigned char comp[4];
    size_t cap = sizeof(comp);
    auto rc = leo_compress_gzip(s, std::strlen(s), comp, &cap, 1);
    REQUIRE(rc == LEO_PACK_E_NOSPACE);
}
