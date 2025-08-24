// =============================================
// tests/image_test.cpp
// =============================================
#include <catch2/catch_all.hpp>

#include "leo/engine.h"
#include "leo/error.h"
#include "leo/image.h"
#include "leo/io.h"

#include <fstream>
#include <vector>
#include <string>
#include <cstring>

static std::vector<unsigned char> read_file_bin(const std::string& path)
{
	std::ifstream f(path, std::ios::binary);
	REQUIRE(f.good()); // fail fast if test asset missing
	std::vector<unsigned char> buf((std::istreambuf_iterator<char>(f)), {});
	return buf;
}

struct ImageTestEnv
{
	ImageTestEnv()
	{
		// Tiny window + renderer so textures can be created
		REQUIRE(leo_InitWindow(64, 64, "img-tests") == true);
	}

	~ImageTestEnv()
	{
		leo_CloseWindow();
	}
};

TEST_CASE_METHOD(ImageTestEnv, "TexFormat bytes-per-pixel", "[image]")
{
	CHECK(leo_TexFormatBytesPerPixel(LEO_TEXFORMAT_UNDEFINED) == 0);
	CHECK(leo_TexFormatBytesPerPixel(LEO_TEXFORMAT_R8G8B8A8) == 4);
	CHECK(leo_TexFormatBytesPerPixel(LEO_TEXFORMAT_R8G8B8) == 3);
	CHECK(leo_TexFormatBytesPerPixel(LEO_TEXFORMAT_GRAY8) == 1);
	CHECK(leo_TexFormatBytesPerPixel(LEO_TEXFORMAT_GRAY8_ALPHA) == 2);
}

TEST_CASE_METHOD(ImageTestEnv, "LoadTexture from file (PNG)", "[image][file]")
{
	// Use a small image to keep GPU work minimal
	const char* path = "resources/images/character_64x64.png";

	leo_Texture2D tex = leo_LoadTexture(path);
	REQUIRE(leo_IsTextureReady(tex));
	CHECK(tex.width == 64);
	CHECK(tex.height == 64);

	leo_UnloadTexture(&tex);
	CHECK_FALSE(leo_IsTextureReady(tex)); // handle is freed in SDL; struct still has width/height
}

TEST_CASE_METHOD(ImageTestEnv, "LoadTexture via VFS (directory mount, logical path)", "[image][vfs]")
{
	// Mount the resources folder as a VFS directory with high priority.
	REQUIRE(leo_MountDirectory("resources", /*priority*/ 100));

	// NOTE: logical path relative to the mount ("resources/"), not a filesystem path.
	// This ensures we exercise the VFS branch, not the filesystem fallback.
	const char* logical = "images/character_64x64.png";

	leo_Texture2D tex = leo_LoadTexture(logical);
	REQUIRE(leo_IsTextureReady(tex));
	CHECK(tex.width == 64);
	CHECK(tex.height == 64);

	leo_UnloadTexture(&tex);
}

TEST_CASE_METHOD(ImageTestEnv, "LoadTextureFromMemory (PNG buffer)", "[image][memory]")
{
	const std::string path = "resources/images/background_320x200.png";
	auto buf = read_file_bin(path);
	REQUIRE_FALSE(buf.empty());

	leo_Texture2D tex = leo_LoadTextureFromMemory(".png",
		reinterpret_cast<const unsigned char*>(buf.data()),
		static_cast<int>(buf.size()));

	REQUIRE(leo_IsTextureReady(tex));
	CHECK(tex.width == 320);
	CHECK(tex.height == 200);

	leo_UnloadTexture(&tex);
}

TEST_CASE_METHOD(ImageTestEnv, "LoadTextureFromTexture (GPUxGPU copy)", "[image][copy]")
{
	const char* path = "resources/images/character_64x64.png";
	leo_Texture2D src = leo_LoadTexture(path);
	REQUIRE(leo_IsTextureReady(src));

	leo_Texture2D dup = leo_LoadTextureFromTexture(src);
	REQUIRE(leo_IsTextureReady(dup));
	CHECK(dup.width == src.width);
	CHECK(dup.height == src.height);

	leo_UnloadTexture(&dup);
	leo_UnloadTexture(&src);
}

TEST_CASE_METHOD(ImageTestEnv, "LoadTextureFromPixels RGBA8 (direct upload)", "[image][pixels]")
{
	const int W = 2, H = 2;
	// 2x2 checker RGBA
	unsigned char px[W * H * 4] = {
		255, 0, 0, 255, 0, 255, 0, 255,
		0, 0, 255, 255, 255, 255, 0, 255
	};
	leo_Texture2D tex = leo_LoadTextureFromPixels(px, W, H, 0, LEO_TEXFORMAT_R8G8B8A8);
	REQUIRE(leo_IsTextureReady(tex));
	CHECK(tex.width == W);
	CHECK(tex.height == H);
	leo_UnloadTexture(&tex);
}

TEST_CASE_METHOD(ImageTestEnv, "LoadTextureFromPixels RGB8 (expand to RGBA)", "[image][pixels]")
{
	const int W = 3, H = 1;
	unsigned char px[W * 3] = {
		10, 20, 30, 40, 50, 60, 70, 80, 90
	};
	leo_Texture2D tex = leo_LoadTextureFromPixels(px, W, H, 0, LEO_TEXFORMAT_R8G8B8);
	REQUIRE(leo_IsTextureReady(tex));
	CHECK(tex.width == W);
	CHECK(tex.height == H);
	leo_UnloadTexture(&tex);
}

TEST_CASE_METHOD(ImageTestEnv, "LoadTextureFromPixels GRAY8 (expand)", "[image][pixels]")
{
	const int W = 4, H = 1;
	unsigned char px[W] = { 0, 64, 128, 255 };
	leo_Texture2D tex = leo_LoadTextureFromPixels(px, W, H, 0, LEO_TEXFORMAT_GRAY8);
	REQUIRE(leo_IsTextureReady(tex));
	CHECK(tex.width == W);
	CHECK(tex.height == H);
	leo_UnloadTexture(&tex);
}

TEST_CASE_METHOD(ImageTestEnv, "LoadTextureFromPixels GRAY8_ALPHA (expand)", "[image][pixels]")
{
	const int W = 2, H = 2;
	unsigned char px[W * H * 2] = {
		0, 255, 255, 128,
		127, 64, 200, 0
	};
	leo_Texture2D tex = leo_LoadTextureFromPixels(px, W, H, 0, LEO_TEXFORMAT_GRAY8_ALPHA);
	REQUIRE(leo_IsTextureReady(tex));
	CHECK(tex.width == W);
	CHECK(tex.height == H);
	leo_UnloadTexture(&tex);
}

TEST_CASE_METHOD(ImageTestEnv, "Error paths: bad args", "[image][error]")
{
	// Bad filename
	{
		leo_Texture2D t = leo_LoadTexture(nullptr);
		CHECK_FALSE(leo_IsTextureReady(t));
		const char* err = leo_GetError();
		CHECK(err != nullptr);
		CHECK(std::strlen(err) > 0);
		leo_ClearError();
	}

	// Bad memory buffer
	{
		leo_Texture2D t = leo_LoadTextureFromMemory(".png", nullptr, 0);
		CHECK_FALSE(leo_IsTextureReady(t));
		const char* err = leo_GetError();
		CHECK(err != nullptr);
		CHECK(std::strlen(err) > 0);
		leo_ClearError();
	}

	// Bad pixels / unsupported format
	{
		unsigned char dummy = 0xFF;
		leo_Texture2D t = leo_LoadTextureFromPixels(&dummy, 0, 0, 0, LEO_TEXFORMAT_UNDEFINED);
		CHECK_FALSE(leo_IsTextureReady(t));
		const char* err = leo_GetError();
		CHECK(err != nullptr);
		CHECK(std::strlen(err) > 0);
		leo_ClearError();
	}

	// Copy from not-ready texture
	{
		leo_Texture2D zero = {};
		leo_Texture2D t = leo_LoadTextureFromTexture(zero);
		CHECK_FALSE(leo_IsTextureReady(t));
		const char* err = leo_GetError();
		CHECK(err != nullptr);
		CHECK(std::strlen(err) > 0);
		leo_ClearError();
	}
}

TEST_CASE_METHOD(ImageTestEnv, "LoadTexture from larger PNGs (sanity sizing)", "[image][file][sanity]")
{
	SECTION("background_1920x1080.png")
	{
		auto tex = leo_LoadTexture("resources/images/background_1920x1080.png");
		REQUIRE(leo_IsTextureReady(tex));
		CHECK(tex.width == 1920);
		CHECK(tex.height == 1080);
		leo_UnloadTexture(&tex);
	}
	SECTION("parallax_background_1920x1080.png")
	{
		auto tex = leo_LoadTexture("resources/images/parallax_background_1920x1080.png");
		REQUIRE(leo_IsTextureReady(tex));
		CHECK(tex.width == 1920);
		CHECK(tex.height == 1080);
		leo_UnloadTexture(&tex);
	}
}
