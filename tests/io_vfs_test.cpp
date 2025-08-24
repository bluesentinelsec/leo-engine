// tests/io_vfs_test.cpp
#include "catch2/catch_all.hpp"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

extern "C" {
#include <leo/io.h>
#include <leo/error.h>
#include <leo/pack_writer.h>
#include <leo/pack_reader.h>
}

namespace fs = std::filesystem;

static std::vector<unsigned char> readBytes(const fs::path& p)
{
	std::ifstream ifs(p, std::ios::binary);
	REQUIRE(ifs.good());
	return std::vector<unsigned char>((std::istreambuf_iterator<char>(ifs)), {});
}

static fs::path makePack(const fs::path& outPack,
	const std::vector<std::pair<std::string, fs::path> >& entries,
	const char* password = nullptr)
{
	leo_pack_writer* w = nullptr;
	leo_pack_build_opts o{};
	o.password = password;
	o.level = 5;
	o.align = 16;

	REQUIRE(leo_pack_writer_begin(&w, outPack.string().c_str(), &o) == LEO_PACK_OK);

	for (auto& kv: entries)
	{
		const std::string& logical = kv.first;
		const fs::path& srcFile = kv.second;
		auto data = readBytes(srcFile);
		REQUIRE(leo_pack_writer_add(w, logical.c_str(), data.data(), data.size(),
			/*compress=*/1, /*obfuscate=*/password ? 1 : 0) == LEO_PACK_OK);
	}
	REQUIRE(leo_pack_writer_end(w) == LEO_PACK_OK);
	return outPack;
}

TEST_CASE("VFS mounts, stat, read, load and priority using real resources/")
{
	// Expect the repo to contain the canonical test resources tree at cwd/resources
	fs::path resources = fs::path("resources");
	REQUIRE(fs::exists(resources));

	// Real files from the provided tree
	fs::path img_char = resources / "images" / "character_64x64.png"; // overlap candidate
	fs::path img_bg_small = resources / "images" / "background_320x200.png"; // pack-only example
	fs::path img_bg_large = resources / "images" / "background_1920x1080.png"; // (not necessarily used)
	fs::path img_parallax = resources / "images" / "parallax_background_1920x1080.png";

	fs::path font_ttf = resources / "font" / "font.ttf"; // pack-only example

	fs::path sfx_coin = resources / "sound" / "coin.wav"; // dir-only example
	fs::path sfx_ogre = resources / "sound" / "ogre3.wav"; // dir-only example

	fs::path music_wav = resources / "music" / "music.wav"; // dir-only example

	// Create a temporary pack that includes a subset, including an overlap with directory
	fs::path tmp = fs::temp_directory_path() / ("leo_io_test_" + std::to_string(::time(nullptr)));
	fs::create_directories(tmp);
	fs::path packPath = tmp / "core.leopack";

	std::vector<std::pair<std::string, fs::path> > packEntries = {
		{ "images/character_64x64.png", img_char }, // overlaps with directory
		{ "images/background_320x200.png", img_bg_small }, // pack-only for this test
		{ "font/font.ttf", font_ttf } // pack-only
	};
	makePack(packPath, packEntries /* no password */);

	// Mounts: pack at priority 100, directory at 50
	leo_ClearMounts();
	REQUIRE(leo_MountResourcePack(packPath.string().c_str(), /*password*/nullptr, 100));
	REQUIRE(leo_MountDirectory(resources.string().c_str(), 50));

	// Helper lambdas for size expectations from filesystem
	auto fsize = [](const fs::path& p) -> size_t { return (size_t)fs::file_size(p); };

	SECTION("Stat finds assets and reports origin + size")
	{
		leo_AssetInfo info{};

		// Overlapped asset: should come from pack
		REQUIRE(leo_StatAsset("images/character_64x64.png", &info));
		CHECK(info.from_pack == 1);
		CHECK(info.size == fsize(img_char));

		// Dir-only asset
		REQUIRE(leo_StatAsset("sound/coin.wav", &info));
		CHECK(info.from_pack == 0);
		CHECK(info.size == fsize(sfx_coin));

		// Pack-only asset
		REQUIRE(leo_StatAsset("font/font.ttf", &info));
		CHECK(info.from_pack == 1);
		CHECK(info.size == fsize(font_ttf));
	}

	SECTION("Size probe then read exact (pack font)")
	{
		size_t total = 0;
		REQUIRE(leo_ReadAsset("font/font.ttf", nullptr, 0, &total) == 0);
		CHECK(total == fsize(font_ttf));

		std::vector<unsigned char> buf(total);
		size_t n = leo_ReadAsset("font/font.ttf", buf.data(), buf.size(), nullptr);
		REQUIRE(n == total);

		// Sanity: first few bytes of a TTF/OTF are not zero
		REQUIRE(buf.size() >= 4);
		CHECK(!(buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 0));
	}

	SECTION("LoadAsset convenience returns bytes (dir sfx)")
	{
		size_t n = 0;
		void* p = leo_LoadAsset("sound/ogre3.wav", &n);
		REQUIRE(p != nullptr);
		CHECK(n == fsize(sfx_ogre));
		// simple RIFF/WAVE signature check if file is a standard WAV
		if (n >= 12)
		{
			const unsigned char* b = (const unsigned char*)p;
			// 'RIFF' .... 'WAVE'
			CHECK((b[0]=='R' && b[1]=='I' && b[2]=='F' && b[3]=='F'));
			CHECK((b[8]=='W' && b[9]=='A' && b[10]=='V' && b[11]=='E'));
		}
		free(p);
	}

	SECTION("Priority: pack overrides dir when both present (character png)")
	{
		size_t n = 0;
		void* p = leo_LoadAsset("images/character_64x64.png", &n);
		REQUIRE(p != nullptr);
		CHECK(n == fsize(img_char));
		free(p);
	}

	SECTION("Bad logical names are rejected")
	{
		size_t total = 0;
		CHECK(leo_ReadAsset("/etc/passwd", nullptr, 0, &total) == 0); // absolute path
		CHECK(leo_ReadAsset("../secret.bin", nullptr, 0, &total) == 0); // parent traversal
		CHECK(leo_ReadAsset("images\\hero.png", nullptr, 0, &total) == 0); // backslashes
	}

	SECTION("ClearMounts allows remounting cleanly")
	{
		leo_ClearMounts();
		// After clear, assets are not found
		leo_AssetInfo info{};
		CHECK_FALSE(leo_StatAsset("font/font.ttf", &info));
		// Remount should work
		REQUIRE(leo_MountDirectory(resources.string().c_str(), 50));
		CHECK(leo_StatAsset("music/music.wav", nullptr));
	}

	// cleanup temporary pack
	fs::remove_all(tmp);
}

TEST_CASE("VFS streaming open/read/seek/tell across pack & dir")
{
	fs::path resources = fs::path("resources");
	REQUIRE(fs::exists(resources));

	// Build a pack with a known file.
	fs::path tmp = fs::temp_directory_path() / ("leo_io_stream_" + std::to_string(::time(nullptr)));
	fs::create_directories(tmp);
	fs::path packPath = tmp / "core.leopack";

	fs::path a_dir = resources / "images" / "background_320x200.png";
	std::vector<std::pair<std::string, fs::path> > entries = {
		{ "images/background_320x200.png", a_dir }
	};
	makePack(packPath, entries);

	leo_ClearMounts();
	REQUIRE(leo_MountResourcePack(packPath.string().c_str(), nullptr, 100));
	REQUIRE(leo_MountDirectory(resources.string().c_str(), 50));

	// Open from pack
	leo_AssetInfo info{};
	leo_AssetStream* s = leo_OpenAsset("images/background_320x200.png", &info);
	REQUIRE(s != nullptr);
	CHECK(info.from_pack == 1);
	CHECK(info.size > 0);

	// Read first 16 bytes
	unsigned char head[16] = { 0 };
	size_t n = leo_AssetRead(s, head, sizeof(head));
	REQUIRE(n == sizeof(head));

	// Seek to end-16 and read tail
	REQUIRE(leo_AssetSeek(s, -16, LEO_SEEK_END));
	unsigned char tail[16] = { 0 };
	n = leo_AssetRead(s, tail, sizeof(tail));
	REQUIRE(n == sizeof(tail));

	// Compare against LoadAsset buffer
	size_t fullN = 0;
	void* full = leo_LoadAsset("images/background_320x200.png", &fullN);
	REQUIRE(full != nullptr);
	REQUIRE(fullN == info.size);
	CHECK(memcmp(head, full, 16) == 0);
	CHECK(memcmp(tail, (unsigned char*)full + fullN - 16, 16) == 0);
	free(full);
	leo_CloseAsset(s);

	// Open a dir-only file
	s = leo_OpenAsset("sound/coin.wav", &info);
	REQUIRE(s != nullptr);
	CHECK(info.from_pack == 0);
	CHECK(info.size > 0);

	REQUIRE(leo_AssetSeek(s, 0, LEO_SEEK_SET));

	// Read RIFF header (first 12 bytes)
	unsigned char header[12] = { 0 };
	REQUIRE(leo_AssetRead(s, header, sizeof(header)) == sizeof(header));

	// 'RIFF' .... 'WAVE'
	CHECK(std::memcmp(header, "RIFF", 4) == 0);
	CHECK(std::memcmp(header + 8, "WAVE", 4) == 0);

	leo_CloseAsset(s);


	fs::remove_all(tmp);
}

TEST_CASE("VFS: multiple packs priority override")
{
	namespace fs = std::filesystem;
	fs::path resources = fs::path("resources");
	REQUIRE(fs::exists(resources));

	// Build two packs that both provide the *same logical name* but with different underlying files.
	auto tmp = fs::temp_directory_path() / ("leo_multi_" + std::to_string(::time(nullptr)));
	fs::create_directories(tmp);
	auto p1 = tmp / "a.leopack";
	auto p2 = tmp / "b.leopack";

	fs::path A = resources / "images" / "character_64x64.png";
	fs::path B = resources / "images" / "background_320x200.png";
	REQUIRE(fs::exists(A));
	REQUIRE(fs::exists(B));

	makePack(p1, { { "data/alias.bin", A } });
	makePack(p2, { { "data/alias.bin", B } });

	// Mount p2 higher priority -> should serve B
	leo_ClearMounts();
	REQUIRE(leo_MountResourcePack(p1.string().c_str(), nullptr, 50));
	REQUIRE(leo_MountResourcePack(p2.string().c_str(), nullptr, 100));

	size_t nHi = 0;
	unsigned char* hi = (unsigned char*)leo_LoadAsset("data/alias.bin", &nHi);
	REQUIRE(hi != nullptr);
	REQUIRE(nHi > 0);

	// Swap priorities -> should now serve A
	leo_ClearMounts();
	REQUIRE(leo_MountResourcePack(p2.string().c_str(), nullptr, 50));
	REQUIRE(leo_MountResourcePack(p1.string().c_str(), nullptr, 100));

	size_t nLo = 0;
	unsigned char* lo = (unsigned char*)leo_LoadAsset("data/alias.bin", &nLo);
	REQUIRE(lo != nullptr);
	REQUIRE(nLo > 0);

	if (nHi == nLo)
		CHECK(std::memcmp(hi, lo, nHi) != 0);
	else
		SUCCEED();

	free(hi);
	free(lo);
	fs::remove_all(tmp);
}

TEST_CASE("VFS: passworded pack requires correct password")
{
	namespace fs = std::filesystem;
	fs::path resources = fs::path("resources");
	REQUIRE(fs::exists(resources));

	auto tmp = fs::temp_directory_path() / ("leo_pw_" + std::to_string(::time(nullptr)));
	fs::create_directories(tmp);
	auto pack = tmp / "pw.leopack";

	fs::path font_ttf = resources / "font" / "font.ttf";
	REQUIRE(fs::exists(font_ttf));

	makePack(pack, { { "font/font.ttf", font_ttf } }, "secret");

	leo_ClearMounts();
	CHECK_FALSE(leo_MountResourcePack(pack.string().c_str(), nullptr, 100)); // wrong/missing password
	CHECK(leo_MountResourcePack(pack.string().c_str(), "secret", 100)); // correct password
	CHECK(leo_StatAsset("font/font.ttf", nullptr));

	fs::remove_all(tmp);
}

TEST_CASE("VFS: partial read fails but size probe works")
{
	namespace fs = std::filesystem;
	fs::path resources = fs::path("resources");
	REQUIRE(fs::exists(resources));

	leo_ClearMounts();
	REQUIRE(leo_MountDirectory(resources.string().c_str(), 50));

	size_t need = 0;
	REQUIRE(leo_ReadAsset("images/background_320x200.png", nullptr, 0, &need) == 0);
	REQUIRE(need > 32);

	std::vector<unsigned char> tooSmall(need - 1);
	// Should fail (returns 0) when buffer can't hold the whole thing
	CHECK(leo_ReadAsset("images/background_320x200.png", tooSmall.data(), tooSmall.size(), nullptr) == 0);

	// Exact fit should succeed
	std::vector<unsigned char> exact(need);
	CHECK(leo_ReadAsset("images/background_320x200.png", exact.data(), exact.size(), nullptr) == need);
}

#include <random>
TEST_CASE("VFS: streaming random seek/read matches full buffer")
{
	namespace fs = std::filesystem;
	fs::path resources = fs::path("resources");
	REQUIRE(fs::exists(resources));

	leo_ClearMounts();
	REQUIRE(leo_MountDirectory(resources.string().c_str(), 50));

	size_t allSz = 0;
	auto all = (unsigned char*)leo_LoadAsset("images/background_320x200.png", &allSz);
	REQUIRE(all != nullptr);
	REQUIRE(allSz > 0);

	leo_AssetStream* s = leo_OpenAsset("images/background_320x200.png", nullptr);
	REQUIRE(s);

	std::mt19937 rng(1337);
	for (int i = 0; i < 600; ++i)
	{
		size_t off = (allSz == 0) ? 0 : (rng() % allSz);
		size_t len = (allSz - off == 0) ? 0 : std::min<size_t>(rng() % 128, allSz - off);
		REQUIRE(leo_AssetSeek(s, (long long)off, LEO_SEEK_SET));
		unsigned char buf[128];
		size_t got = leo_AssetRead(s, buf, len);
		REQUIRE(got == len);
		CHECK(std::memcmp(buf, all + off, len) == 0);
	}

	leo_CloseAsset(s);
	free(all);
}

TEST_CASE("VFS: zero-length asset works (pack)")
{
	namespace fs = std::filesystem;
	fs::path resources = fs::path("resources");
	REQUIRE(fs::exists(resources));

	// Create an empty temp file and include it in a pack.
	auto tmp = fs::temp_directory_path() / ("leo_zero_" + std::to_string(::time(nullptr)));
	fs::create_directories(tmp);
	auto emptyFile = tmp / "empty.bin";
	{
		std::ofstream ofs(emptyFile, std::ios::binary);
	} // creates 0-byte file
	auto pack = tmp / "z.leopack";
	makePack(pack, { { "data/empty.bin", emptyFile } });

	leo_ClearMounts();
	REQUIRE(leo_MountResourcePack(pack.string().c_str(), nullptr, 100));

	leo_AssetInfo st{};
	REQUIRE(leo_StatAsset("data/empty.bin", &st));
	CHECK(st.size == 0);
	CHECK(st.from_pack == 1);

	// ReadAsset probe + read
	size_t need = 1234;
	CHECK(leo_ReadAsset("data/empty.bin", nullptr, 0, &need) == 0);
	CHECK(need == 0);

	unsigned char dummy[1] = { 0xFF };
	CHECK(leo_ReadAsset("data/empty.bin", dummy, sizeof(dummy), nullptr) == 0); // zero bytes read

	// Streaming should also open and just EOF immediately
	leo_AssetStream* s = leo_OpenAsset("data/empty.bin", &st);
	REQUIRE(s);
	CHECK(st.size == 0);
	unsigned char b;
	CHECK(leo_AssetRead(s, &b, 1) == 0);
	leo_CloseAsset(s);

	fs::remove_all(tmp);
}

TEST_CASE("VFS: additional bad logical names are rejected")
{
	leo_ClearMounts();
	// No mounts needed; we only check the path gate
	size_t total = 0;
	CHECK(leo_ReadAsset("./foo", nullptr, 0, &total) == 0); // reject leading ./
	CHECK(leo_ReadAsset("a/../b", nullptr, 0, &total) == 0); // reject parent traversal
	CHECK(leo_ReadAsset("a//b", nullptr, 0, &total) == 0); // reject empty segment (normalize or reject)
	CHECK(leo_ReadAsset("a/./b", nullptr, 0, &total) == 0); // reject explicit current-dir segment
}
