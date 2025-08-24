#include <catch2/catch_all.hpp>
#include <cstring>
#include <leo/pack_reader.h>
#include <leo/pack_writer.h>
#include <string>
#include <vector>

// Small helper to build a pack file in the build dir
static std::string out_path(const char *name)
{
    return std::string(name);
}

TEST_CASE("pack: write/read basic/uncompressed")
{
    const char *out = "test_basic.leopack";
    const char *name = "hello.txt";
    const char payload[] = "hello world";

    leo_pack_writer *w = nullptr;
    leo_pack_build_opts opt{};
    opt.align = 16;
    opt.level = 5;

    REQUIRE(leo_pack_writer_begin(&w, out, &opt) == LEO_PACK_OK);
    REQUIRE(leo_pack_writer_add(w, name, payload, sizeof(payload) - 1, /*compress*/ 0, /*obfuscate*/ 0) == LEO_PACK_OK);
    REQUIRE(leo_pack_writer_end(w) == LEO_PACK_OK);

    leo_pack *p = nullptr;
    REQUIRE(leo_pack_open_file(&p, out, /*password*/ nullptr) == LEO_PACK_OK);

    int idx = -1;
    REQUIRE(leo_pack_find(p, name, &idx) == LEO_PACK_OK);

    leo_pack_stat st{};
    REQUIRE(leo_pack_stat_index(p, idx, &st) == LEO_PACK_OK);
    REQUIRE(st.size_uncompressed == sizeof(payload) - 1);

    std::vector<char> buf(st.size_uncompressed);
    size_t got = 0;
    REQUIRE(leo_pack_extract_index(p, idx, buf.data(), buf.size(), &got) == LEO_PACK_OK);
    REQUIRE(got == st.size_uncompressed);
    REQUIRE(std::string(buf.data(), got) == "hello world");

    leo_pack_close(p);
}

TEST_CASE("pack: compressed and obfuscated")
{
    // Highly compressible data
    std::string big(4096, '\0');
    for (size_t i = 0; i < big.size(); ++i)
        big[i] = char('A' + (i % 3));

    const char *out = "test_comp_obf.leopack";
    const char *name = "bin/lorem.bin";
    const char *password = "hunter2";

    leo_pack_writer *w = nullptr;
    leo_pack_build_opts opt{};
    opt.align = 16;
    opt.level = 7;
    opt.password = password;

    REQUIRE(leo_pack_writer_begin(&w, out, &opt) == LEO_PACK_OK);
    REQUIRE(leo_pack_writer_add(w, name, big.data(), big.size(), /*compress*/ 1, /*obfuscate*/ 1) == LEO_PACK_OK);
    REQUIRE(leo_pack_writer_end(w) == LEO_PACK_OK);

    // Wrong password should fail to extract obfuscated entry
    {
        leo_pack *p = nullptr;
        REQUIRE(leo_pack_open_file(&p, out, /*password*/ "wrong") == LEO_PACK_OK);

        int idx = -1;
        REQUIRE(leo_pack_find(p, name, &idx) == LEO_PACK_OK);

        std::vector<char> buf(big.size());
        size_t got = 0;
        auto r = leo_pack_extract_index(p, idx, buf.data(), buf.size(), &got);
        REQUIRE(r == LEO_PACK_E_BAD_PASSWORD);

        leo_pack_close(p);
    }

    // Correct password should extract OK and match
    {
        leo_pack *p = nullptr;
        REQUIRE(leo_pack_open_file(&p, out, /*password*/ password) == LEO_PACK_OK);

        int idx = -1;
        REQUIRE(leo_pack_find(p, name, &idx) == LEO_PACK_OK);

        leo_pack_stat st{};
        REQUIRE(leo_pack_stat_index(p, idx, &st) == LEO_PACK_OK);
        REQUIRE((st.flags & 0x1) != 0); // LEO_PE_COMPRESSED
        REQUIRE((st.flags & 0x2) != 0); // LEO_PE_OBFUSCATED

        std::vector<char> buf(st.size_uncompressed);
        size_t got = 0;
        REQUIRE(leo_pack_extract_index(p, idx, buf.data(), buf.size(), &got) == LEO_PACK_OK);
        REQUIRE(got == big.size());
        REQUIRE(std::memcmp(buf.data(), big.data(), big.size()) == 0);

        leo_pack_close(p);
    }
}

TEST_CASE("pack: small dest buffer yields NOSPACE")
{
    const char *out = "test_nospace.leopack";
    const char *name = "data.bin";
    const unsigned char data[8] = {1, 2, 3, 4, 5, 6, 7, 8};

    leo_pack_writer *w = nullptr;
    leo_pack_build_opts opt{};
    REQUIRE(leo_pack_writer_begin(&w, out, &opt) == LEO_PACK_OK);
    REQUIRE(leo_pack_writer_add(w, name, data, sizeof(data), 0, 0) == LEO_PACK_OK);
    REQUIRE(leo_pack_writer_end(w) == LEO_PACK_OK);

    leo_pack *p = nullptr;
    REQUIRE(leo_pack_open_file(&p, out, nullptr) == LEO_PACK_OK);

    int idx = -1;
    REQUIRE(leo_pack_find(p, name, &idx) == LEO_PACK_OK);

    char tiny[4];
    size_t got = 0;
    REQUIRE(leo_pack_extract_index(p, idx, tiny, sizeof(tiny), &got) == LEO_PACK_E_NOSPACE);

    leo_pack_close(p);
}
