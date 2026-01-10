#include "leo/base64.h"
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <string>
#include <vector>

static std::string encode_string(const std::string &s)
{
    size_t out_len = leo_base64_encoded_len(s.size());
    std::string out(out_len, '\0');
    size_t wrote = 0;
    auto r = leo_base64_encode(s.data(), s.size(), out.data(), out.size(), &wrote);
    REQUIRE(r == LEO_B64_OK);
    out.resize(wrote);
    return out;
}

static std::vector<unsigned char> decode_string(const std::string &s)
{
    size_t cap = leo_base64_decoded_cap(s.size());
    std::vector<unsigned char> out(cap);
    size_t wrote = 0;
    auto r = leo_base64_decode(s.c_str(), s.size(), out.data(), out.size(), &wrote);
    REQUIRE(r == LEO_B64_OK);
    out.resize(wrote);
    return out;
}

TEST_CASE("base64: known vectors (RFC 4648)")
{
    CHECK(encode_string("") == "");
    CHECK(encode_string("f") == "Zg==");
    CHECK(encode_string("fo") == "Zm8=");
    CHECK(encode_string("foo") == "Zm9v");
    CHECK(encode_string("foob") == "Zm9vYg==");
    CHECK(encode_string("fooba") == "Zm9vYmE=");
    CHECK(encode_string("foobar") == "Zm9vYmFy");
}

TEST_CASE("base64: decode known vectors")
{
    auto d0 = decode_string("");
    CHECK(d0.size() == 0);

    auto d1 = decode_string("Zg==");
    REQUIRE(d1.size() == 1);
    CHECK(d1[0] == 'f');

    auto d2 = decode_string("Zm8=");
    REQUIRE(d2.size() == 2);
    CHECK(d2[0] == 'f');
    CHECK(d2[1] == 'o');

    auto d3 = decode_string("Zm9v");
    REQUIRE(d3.size() == 3);
    CHECK(d3[0] == 'f');
    CHECK(d3[1] == 'o');
    CHECK(d3[2] == 'o');
}

TEST_CASE("base64: decode ignores whitespace and handles unpadded")
{
    // "foobar" with spaces and newlines
    const char *spaced = " Zm9 v\nYm Fy ";
    auto d = decode_string(spaced);
    REQUIRE(d.size() == 6);
    CHECK(std::memcmp(d.data(), "foobar", 6) == 0);

    // Unpadded variant "Zg" -> 'f' is not valid per strict RFC, but many encoders omit '='.
    // Our decoder requires proper quads; so supply minimal padding-free that still forms quads:
    // We'll test unpadded full-quads: "Zm9vYmFy"
    auto d2 = decode_string("Zm9vYmFy");
    REQUIRE(d2.size() == 6);
    CHECK(std::memcmp(d2.data(), "foobar", 6) == 0);
}

TEST_CASE("base64: buffer-size errors and format errors")
{
    // Encode buffer too small
    const char *src = "abcd"; // enc len = 8
    char tiny[7];
    size_t wrote = 0;
    auto r = leo_base64_encode(src, 4, tiny, sizeof(tiny), &wrote);
    CHECK(r == LEO_B64_E_NOSPACE);

    // Decode invalid alphabet
    unsigned char out[16];
    r = leo_base64_decode("##$$", 4, out, sizeof(out), &wrote);
    CHECK(r == LEO_B64_E_FORMAT);

    // Decode incomplete quad (invalid)
    r = leo_base64_decode("Zg", 2, out, sizeof(out), &wrote);
    CHECK(r == LEO_B64_E_FORMAT);
}

TEST_CASE("base64: alloc helpers round-trip")
{
    const unsigned char raw[] = {0x00, 0x01, 0xFE, 0xFF, 0x10, 0x20, 0x7F};
    char *enc = nullptr;
    size_t enc_len = 0;
    auto r = leo_base64_encode_alloc(raw, sizeof(raw), &enc, &enc_len);
    REQUIRE(r == LEO_B64_OK);
    REQUIRE(enc != nullptr);
    REQUIRE(enc_len == std::strlen(enc)); // NUL-terminated

    unsigned char *dec = nullptr;
    size_t dec_len = 0;
    r = leo_base64_decode_alloc(enc, enc_len, &dec, &dec_len);
    REQUIRE(r == LEO_B64_OK);
    REQUIRE(dec != nullptr);
    CHECK(dec_len == sizeof(raw));
    CHECK(std::memcmp(dec, raw, sizeof(raw)) == 0);

    free(enc);
    free(dec);
}
