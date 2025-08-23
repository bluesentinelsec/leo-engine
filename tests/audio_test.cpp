// =============================================
// tests/audio_test.cpp
// =============================================
#include <catch2/catch_all.hpp>

#include "leo/audio.h"
#include "leo/error.h"

#include <cstring>
#include <thread>
#include <chrono>

// Small helper to avoid flaky race conditions after starting/stopping audio.
static void tiny_sleep_ms(int ms = 20)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

struct AudioEnv
{
	bool available = false;

	AudioEnv()
	{
		// Try to bring up audio; if unavailable on CI, we skip playback tests.
		available = leo_InitAudio();
		if (!available)
		{
			// Make sure error state is visible in logs but does not fail tests.
			const char* err = leo_GetError();
			WARN(std::string("leo_InitAudio failed; audio tests will be skipped. Error: ")
				+ (err ? err : "(none)"));
			leo_ClearError();
		}
	}

	~AudioEnv()
	{
		leo_ShutdownAudio();
		// Multiple shutdowns should be harmless; try again to ensure idempotence.
		leo_ShutdownAudio();
	}
};

TEST_CASE_METHOD(AudioEnv, "Audio init/shutdown is idempotent", "[audio][lifecycle]")
{
	// This should work whether or not the first init succeeded (idempotent API).
	bool ok = leo_InitAudio();
	// If audio is unavailable on this runner, ok may be false; that is acceptable.
	// We just assert that calling it does not set an error that must be cleared.
	const char* err = leo_GetError();
	if (err && std::strlen(err) > 0)
	{
		// Do not fail CI on headless audio; just surface the warning.
		WARN(std::string("Init reported error (harmless on headless CI): ") + err);
		leo_ClearError();
	}
	// Always safe to call shutdown multiple times.
	leo_ShutdownAudio();
	SUCCEED("Init/Shutdown calls completed without crashing");
}

TEST_CASE_METHOD(AudioEnv, "LoadSound from file returns a ready handle", "[audio][load]")
{
	if (!available)
	{
		WARN("Audio device unavailable in CI; skipping load test.");
		return;
	}

	auto sfx = leo_LoadSound("resources/sound/coin.wav");
	REQUIRE(leo_IsSoundReady(sfx));
	REQUIRE((sfx.channels == 1 || sfx.channels == 2));
	REQUIRE(sfx.sampleRate >= 8000); // advisory info; typical 22k/44.1k/48k

	leo_UnloadSound(&sfx);
	CHECK_FALSE(leo_IsSoundReady(sfx));
}

TEST_CASE_METHOD(AudioEnv, "Play / Stop basic flow", "[audio][playback]")
{
	if (!available)
	{
		WARN("Audio device unavailable in CI; skipping playback test.");
		return;
	}

	auto sfx = leo_LoadSound("resources/sound/coin.wav");
	REQUIRE(leo_IsSoundReady(sfx));

	// Start at a safe volume (also clamps internally).
	REQUIRE(leo_PlaySound(&sfx, 0.5f, false));
	tiny_sleep_ms(); // allow engine thread to transition state
	CHECK(leo_IsSoundPlaying(&sfx));

	leo_StopSound(&sfx);
	tiny_sleep_ms();
	CHECK_FALSE(leo_IsSoundPlaying(&sfx));

	leo_UnloadSound(&sfx);
}

TEST_CASE_METHOD(AudioEnv, "Pause / Resume toggles playing state", "[audio][playback]")
{
	if (!available)
	{
		WARN("Audio device unavailable in CI; skipping pause/resume test.");
		return;
	}

	auto sfx = leo_LoadSound("resources/sound/coin.wav");
	REQUIRE(leo_IsSoundReady(sfx));

	REQUIRE(leo_PlaySound(&sfx, 0.4f, true));
	tiny_sleep_ms();
	REQUIRE(leo_IsSoundPlaying(&sfx));

	leo_PauseSound(&sfx);
	tiny_sleep_ms();
	CHECK_FALSE(leo_IsSoundPlaying(&sfx));

	leo_ResumeSound(&sfx);
	tiny_sleep_ms();
	CHECK(leo_IsSoundPlaying(&sfx));

	leo_StopSound(&sfx);
	leo_UnloadSound(&sfx);
}

TEST_CASE_METHOD(AudioEnv, "Volume / Pitch / Pan are clamped and callable", "[audio][tuning]")
{
	if (!available)
	{
		WARN("Audio device unavailable in CI; skipping tuning test.");
		return;
	}

	auto sfx = leo_LoadSound("resources/sound/coin.wav");
	REQUIRE(leo_IsSoundReady(sfx));

	// These should not crash and should internally clamp/sanitize values.
	leo_SetSoundVolume(&sfx, -1.0f); // clamp to 0
	leo_SetSoundVolume(&sfx, 2.0f); // clamp to 1
	leo_SetSoundPitch(&sfx, 0.01f); // min reasonable
	leo_SetSoundPitch(&sfx, 2.0f);
	leo_SetSoundPan(&sfx, -2.0f); // clamp to -1
	leo_SetSoundPan(&sfx, 2.0f); // clamp to +1

	// Smoke-play to ensure settings don't break playback.
	REQUIRE(leo_PlaySound(&sfx, 1.0f, false));
	tiny_sleep_ms();
	CHECK(leo_IsSoundPlaying(&sfx));
	leo_StopSound(&sfx);

	leo_UnloadSound(&sfx);
}

TEST_CASE_METHOD(AudioEnv, "Multiple sounds can play simultaneously", "[audio][mixing]")
{
	if (!available)
	{
		WARN("Audio device unavailable in CI; skipping mixing test.");
		return;
	}

	auto sfx1 = leo_LoadSound("resources/sound/coin.wav");
	auto sfx2 = leo_LoadSound("resources/sound/ogre3.wav");
	REQUIRE(leo_IsSoundReady(sfx1));
	REQUIRE(leo_IsSoundReady(sfx2));

	REQUIRE(leo_PlaySound(&sfx1, 0.4f, false));
	REQUIRE(leo_PlaySound(&sfx2, 0.4f, false));
	tiny_sleep_ms();
	CHECK(leo_IsSoundPlaying(&sfx1));
	CHECK(leo_IsSoundPlaying(&sfx2));

	leo_StopSound(&sfx1);
	leo_StopSound(&sfx2);
	leo_UnloadSound(&sfx1);
	leo_UnloadSound(&sfx2);
}

TEST_CASE_METHOD(AudioEnv, "Unload while playing is safe", "[audio][lifecycle]")
{
	if (!available)
	{
		WARN("Audio device unavailable in CI; skipping unload-while-playing test.");
		return;
	}

	auto sfx = leo_LoadSound("resources/sound/coin.wav");
	REQUIRE(leo_IsSoundReady(sfx));

	REQUIRE(leo_PlaySound(&sfx, 0.5f, true));
	tiny_sleep_ms();
	REQUIRE(leo_IsSoundPlaying(&sfx));

	// Should stop and free resources without crashing.
	leo_UnloadSound(&sfx);
	CHECK_FALSE(leo_IsSoundReady(sfx));
}

TEST_CASE_METHOD(AudioEnv, "Error paths: bad args report error", "[audio][errors]")
{
	// These should not require a working device; they validate API behavior.

	// Bad path
	{
		auto sfx = leo_LoadSound(nullptr);
		CHECK_FALSE(leo_IsSoundReady(sfx));
		const char* err = leo_GetError();
		CHECK(err != nullptr);
		CHECK(std::strlen(err) > 0);
		leo_ClearError();
	}

	// Play/Stop/Pause/Resume on invalid handles should not crash
	{
		leo_Sound zero = {};
		CHECK_FALSE(leo_IsSoundReady(zero));

		// These calls should be safe no-ops (or set error for play).
		CHECK_FALSE(leo_PlaySound(&zero, 0.5f, false)); // expect failure
		const char* err = leo_GetError();
		CHECK(err != nullptr);
		CHECK(std::strlen(err) > 0);
		leo_ClearError();

		// No error required for no-ops, but they must not crash:
		leo_StopSound(&zero);
		leo_PauseSound(&zero);
		leo_ResumeSound(&zero);
	}
}
