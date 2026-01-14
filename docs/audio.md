# Audio (Caller Experience)

This document describes the caller-facing API for loading sound effects and
music from the VFS and controlling playback with a small SFML-like API.

## Goals

- Load audio from VFS paths with fail-fast error handling.
- Provide simple `Play`/`Pause`/`Stop` controls for sound effects and music.
- Keep the API small and predictable for use in `OnInit` and `OnUpdate`.

## API Surface

```cpp
namespace engine {

class Sound {
  public:
    Sound() noexcept;
    ~Sound();
    Sound(Sound&&) noexcept;
    Sound& operator=(Sound&&) noexcept;

    Sound(const Sound&) = delete;
    Sound& operator=(const Sound&) = delete;

    static Sound LoadFromVfs(VFS &vfs, const char *vfs_path);

    bool IsReady() const noexcept;
    void Reset() noexcept;

    void Play();
    void Pause();
    void Stop();
    bool IsPlaying() const noexcept;

    void SetLooping(bool looping);
    void SetVolume(float volume); // 0..100
    void SetPitch(float pitch);   // 1.0 = normal
};

class Music {
  public:
    Music() noexcept;
    ~Music();
    Music(Music&&) noexcept;
    Music& operator=(Music&&) noexcept;

    Music(const Music&) = delete;
    Music& operator=(const Music&) = delete;

    static Music LoadFromVfs(VFS &vfs, const char *vfs_path);

    bool IsReady() const noexcept;
    void Reset() noexcept;

    void Play();
    void Pause();
    void Stop();
    bool IsPlaying() const noexcept;

    void SetLooping(bool looping);
    void SetVolume(float volume); // 0..100
    void SetPitch(float pitch);   // 1.0 = normal
};

} // namespace engine
```

## Lifetime and Ownership

- `Sound` and `Music` are owning, move-only handles.
- Resources are released automatically when the objects go out of scope.
- Call `Reset()` if you want to release them early.

## Error Handling

- `Sound::LoadFromVfs` and `Music::LoadFromVfs` throw `std::runtime_error` on
  any failure (missing file, decode failure, or engine init failure).
- Playback and tuning calls throw if invoked on an unloaded object.

## Playback Semantics

- `Play()` restarts if the sound/music is already playing, and resumes if it
  was paused.
- `Pause()` stops playback without resetting the playback cursor.
- `Stop()` stops playback and rewinds to the start.
- Volume is a 0..100 range (SFML-style).

## VFS Path Rules

- `vfs_path` is always relative to the mounted VFS root and uses `/` separators.
- Loaders read from mounted resources, not the write dir.

## Tutorial

### 1) Load audio in `OnInit`

```cpp
#include "leo/engine_core.h"
#include "leo/audio.h"

struct GameState {
    engine::Sound coin;
    engine::Music music;
};

void OnInit(leo::Engine::Context &ctx, GameState &state)
{
    state.coin = engine::Sound::LoadFromVfs(*ctx.vfs, "sound/coin.wav");
    state.music = engine::Music::LoadFromVfs(*ctx.vfs, "music/music.wav");
    state.music.SetLooping(true);
    state.music.Play();
}
```

### 2) Trigger sound effects in `OnUpdate`

```cpp
void OnUpdate(leo::Engine::Context &ctx, GameState &state, const InputFrame &input)
{
    (void)ctx;
    (void)input;
    state.coin.Play();
}
```

### 3) Cleanup

`Sound` and `Music` are RAII types. Use `Reset()` if you need to release them
early:

```cpp
state.music.Reset();
state.coin.Reset();
```
