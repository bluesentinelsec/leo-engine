#include "leo/audio.h"
#include "leo/engine_config.h"
#include "leo/vfs.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <stdexcept>

namespace
{

engine::Config MakeConfig()
{
    return {.argv0 = "test",
            .resource_path = nullptr,
            .organization = "bluesentinelsec",
            .app_name = "leo-engine",
            .malloc_fn = SDL_malloc,
            .realloc_fn = SDL_realloc,
            .free_fn = SDL_free};
}

void SetNoDeviceEnv()
{
#ifdef _WIN32
    _putenv_s("LEO_AUDIO_NO_DEVICE", "1");
#else
    setenv("LEO_AUDIO_NO_DEVICE", "1", 1);
#endif
}

} // namespace

TEST_CASE("Sound loads from VFS and can be configured", "[audio]")
{
    SetNoDeviceEnv();

    engine::Config config = MakeConfig();
    engine::VFS vfs(config);

    engine::Sound sound = engine::Sound::LoadFromVfs(vfs, "sound/coin.wav");
    REQUIRE(sound.IsReady());

    sound.SetVolume(75.0f);
    sound.SetPitch(1.0f);
    sound.SetLooping(false);
    sound.Stop();
}

TEST_CASE("Music loads from VFS and can be configured", "[audio]")
{
    SetNoDeviceEnv();

    engine::Config config = MakeConfig();
    engine::VFS vfs(config);

    engine::Music music = engine::Music::LoadFromVfs(vfs, "music/music.wav");
    REQUIRE(music.IsReady());

    music.SetVolume(40.0f);
    music.SetPitch(1.0f);
    music.SetLooping(true);
    music.Stop();
}

TEST_CASE("Audio loaders throw when file is missing", "[audio]")
{
    SetNoDeviceEnv();

    engine::Config config = MakeConfig();
    engine::VFS vfs(config);

    REQUIRE_THROWS_AS(engine::Sound::LoadFromVfs(vfs, "sound/missing.wav"), std::runtime_error);
    REQUIRE_THROWS_AS(engine::Music::LoadFromVfs(vfs, "music/missing.wav"), std::runtime_error);
}
