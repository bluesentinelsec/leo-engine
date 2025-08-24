// =============================================
// tests/font_step5_vfs_tests.cpp
// Step 5: VFS-aware font loading (directory mounts)
// =============================================
#include <catch2/catch_all.hpp>

#include "leo/color.h"
#include "leo/engine.h"
#include "leo/error.h"
#include "leo/font.h"
#include "leo/io.h"

struct GfxEnvVFS
{
	GfxEnvVFS()
	{
		// Fresh VFS for each test file
		leo_ClearMounts();
		// Mount the repo's resources/ directory at high priority.
		// After this, "font/font.ttf" should resolve via VFS.
		REQUIRE(leo_MountDirectory("resources", /*priority=*/100));
		REQUIRE(leo_InitWindow(640, 360, "font-step5-vfs"));
	}

	~GfxEnvVFS()
	{
		leo_CloseWindow();
		leo_ClearMounts();
	}
};

TEST_CASE_METHOD(GfxEnvVFS, "VFS: logical font path resolves and loads", "[font][vfs][step5]")
{
	// Prove the asset exists via VFS first.
	leo_AssetInfo info{};
	REQUIRE(leo_StatAsset("font/font.ttf", &info));
	CHECK(info.size > 0);
	// For a directory mount, from_pack should be 0.
	CHECK(info.from_pack == 0);

	// Load via logical path (no direct filesystem prefix).
	auto font = leo_LoadFont("font/font.ttf", 24);
	REQUIRE(leo_IsFontReady(font));

	// Quick metrics sanity
	CHECK(font.baseSize == 24);
	CHECK(font.glyphCount > 0);
	CHECK(font.lineHeight > 0);

	// Measure/draw should work through the normal codepath
	auto m0 = leo_MeasureTextEx(font, "VFS OK", 24.0f, 0.0f);
	CHECK(m0.x > 0.0f);
	CHECK(m0.y > 0.0f);

	leo_BeginDrawing();
	leo_ClearBackground(12, 14, 20, 255);
	leo_DrawTextEx(font, "Loaded via VFS", leo_Vector2{ 16, 16 }, 24.0f, 0.0f, leo_Color{ 255, 255, 255, 255 });
	leo_EndDrawing();

	// No error expected
	const char* err = leo_GetError();
	if (err == nullptr)
	{
		CHECK(true);
	}
	else
	{
		CHECK(err[0] == '\0');
	}
	leo_UnloadFont(&font);
	CHECK_FALSE(leo_IsFontReady(font));
}

TEST_CASE_METHOD(GfxEnvVFS, "VFS: default-font path works with DrawText/DrawFPS", "[font][vfs][step5]")
{
	auto font = leo_LoadFont("font/font.ttf", 18);
	REQUIRE(leo_IsFontReady(font));
	leo_SetDefaultFont(font);

	leo_BeginDrawing();
	leo_ClearBackground(18, 20, 26, 255);
	leo_DrawText("VFS default font", 10, 10, 18, leo_Color{ 230, 240, 255, 255 });
	leo_DrawFPS(10, 36);
	leo_EndDrawing();

	const char* err = leo_GetError();
	if (err == nullptr)
	{
		CHECK(true);
	}
	else
	{
		CHECK(err[0] == '\0');
	}

	leo_UnloadFont(&font);
	leo_SetDefaultFont(leo_Font{ 0 });
}

TEST_CASE_METHOD(GfxEnvVFS, "VFS: mounting is required for logical path; fallback still supports FS paths",
	"[font][vfs][step5]")
{
	// First, logical path works because we mounted "resources" in the fixture.
	{
		auto f = leo_LoadFont("font/font.ttf", 16);
		REQUIRE(leo_IsFontReady(f));
		leo_UnloadFont(&f);
	}

	// Now clear mounts; the same logical path should fail,
	// but a direct filesystem path should still be loadable (fallback).
	leo_ClearMounts();

	auto missing = leo_LoadFont("font/font.ttf", 16);
	CHECK_FALSE(leo_IsFontReady(missing));
	const char* err1 = leo_GetError();
	REQUIRE(err1 != nullptr);
	CHECK(std::strlen(err1) > 0);
	leo_ClearError();

	auto fs = leo_LoadFont("resources/font/font.ttf", 16);
	REQUIRE(leo_IsFontReady(fs));
	leo_UnloadFont(&fs);
}
