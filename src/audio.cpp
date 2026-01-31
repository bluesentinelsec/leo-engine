#include "leo/audio.h"
#include "leo/vfs.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_stdinc.h>
#include <miniaudio.h>
#include <stdexcept>
#include <string>

namespace
{

struct AudioHandle
{
    ma_sound sound;
    ma_decoder decoder;
    bool has_decoder;
    void *encoded;
    size_t encoded_size;
};

struct AudioEngineState
{
    ma_engine engine;
    bool initialized = false;

    ~AudioEngineState()
    {
        if (initialized)
        {
            ma_engine_uninit(&engine);
        }
    }
};

void *MiniaudioMalloc(size_t size, void *user_data)
{
    (void)user_data;
    return SDL_malloc(size);
}

void *MiniaudioRealloc(void *ptr, size_t size, void *user_data)
{
    (void)user_data;
    return SDL_realloc(ptr, size);
}

void MiniaudioFree(void *ptr, void *user_data)
{
    (void)user_data;
    SDL_free(ptr);
}

ma_allocation_callbacks MakeAllocationCallbacks()
{
    ma_allocation_callbacks callbacks = {};
    callbacks.pUserData = nullptr;
    callbacks.onMalloc = MiniaudioMalloc;
    callbacks.onRealloc = MiniaudioRealloc;
    callbacks.onFree = MiniaudioFree;
    return callbacks;
}

bool ShouldDisableAudioDevice()
{
    const char *value = SDL_getenv("LEO_AUDIO_NO_DEVICE");
    return value && *value && SDL_strcmp(value, "0") != 0;
}

ma_engine *GetAudioEngine()
{
    static AudioEngineState state;
    if (!state.initialized)
    {
        ma_engine_config config = ma_engine_config_init();
        config.allocationCallbacks = MakeAllocationCallbacks();
        if (ShouldDisableAudioDevice())
        {
            config.noDevice = MA_TRUE;
            config.channels = 2;
            config.sampleRate = 48000;
        }
        ma_result result = ma_engine_init(&config, &state.engine);
        if (result != MA_SUCCESS)
        {
            throw std::runtime_error("Audio engine init failed (miniaudio error " + std::to_string(result) + ")");
        }
        state.initialized = true;
    }
    return &state.engine;
}

AudioHandle *LoadAudioHandle(engine::VFS &vfs, const char *vfs_path, ma_uint32 flags, const char *context)
{
    if (!vfs_path || !*vfs_path)
    {
        throw std::runtime_error(std::string(context) + " requires a non-empty path");
    }

    void *data = nullptr;
    size_t size = 0;
    vfs.ReadAll(vfs_path, &data, &size);
    if (!data || size == 0)
    {
        if (data)
        {
            SDL_free(data);
        }
        throw std::runtime_error(std::string(context) + " received empty data buffer");
    }

    AudioHandle *handle = static_cast<AudioHandle *>(SDL_calloc(1, sizeof(AudioHandle)));
    if (!handle)
    {
        SDL_free(data);
        throw std::runtime_error(std::string(context) + " out of memory");
    }

    ma_result result = ma_decoder_init_memory(data, size, nullptr, &handle->decoder);
    if (result != MA_SUCCESS)
    {
        SDL_free(data);
        SDL_free(handle);
        throw std::runtime_error(std::string(context) + " failed to decode audio (miniaudio error " +
                                 std::to_string(result) + ")");
    }

    handle->has_decoder = true;
    handle->encoded = data;
    handle->encoded_size = size;

    ma_engine *engine = GetAudioEngine();
    result = ma_sound_init_from_data_source(engine, &handle->decoder, flags, nullptr, &handle->sound);
    if (result != MA_SUCCESS)
    {
        ma_decoder_uninit(&handle->decoder);
        SDL_free(handle->encoded);
        SDL_free(handle);
        throw std::runtime_error(std::string(context) + " failed to init sound (miniaudio error " +
                                 std::to_string(result) + ")");
    }

    return handle;
}

AudioHandle *RequireHandle(void *handle, const char *context, const char *type)
{
    if (!handle)
    {
        throw std::runtime_error(std::string(context) + " requires a loaded " + type);
    }
    return static_cast<AudioHandle *>(handle);
}

void ReleaseHandle(AudioHandle *handle)
{
    if (!handle)
    {
        return;
    }
    ma_sound_uninit(&handle->sound);
    if (handle->has_decoder)
    {
        ma_decoder_uninit(&handle->decoder);
    }
    if (handle->encoded)
    {
        SDL_free(handle->encoded);
    }
    SDL_free(handle);
}

void StartPlayback(AudioHandle *handle, bool *paused, const char *context)
{
    if (*paused)
    {
        ma_result result = ma_sound_start(&handle->sound);
        if (result != MA_SUCCESS)
        {
            throw std::runtime_error(std::string(context) + " failed to start (miniaudio error " +
                                     std::to_string(result) + ")");
        }
        *paused = false;
        return;
    }

    ma_sound_stop(&handle->sound);
    ma_sound_seek_to_pcm_frame(&handle->sound, 0);
    ma_result result = ma_sound_start(&handle->sound);
    if (result != MA_SUCCESS)
    {
        throw std::runtime_error(std::string(context) + " failed to start (miniaudio error " + std::to_string(result) +
                                 ")");
    }
}

void PausePlayback(AudioHandle *handle, bool *paused)
{
    ma_sound_stop(&handle->sound);
    *paused = true;
}

void StopPlayback(AudioHandle *handle, bool *paused)
{
    ma_sound_stop(&handle->sound);
    ma_sound_seek_to_pcm_frame(&handle->sound, 0);
    *paused = false;
}

void ApplyVolume(AudioHandle *handle, float volume)
{
    if (volume < 0.0f)
    {
        volume = 0.0f;
    }
    if (volume > 100.0f)
    {
        volume = 100.0f;
    }
    ma_sound_set_volume(&handle->sound, volume / 100.0f);
}

void ApplyPitch(AudioHandle *handle, float pitch)
{
    if (pitch < 0.01f)
    {
        pitch = 0.01f;
    }
    ma_sound_set_pitch(&handle->sound, pitch);
}

} // namespace

namespace engine
{

Sound::Sound() noexcept : handle(nullptr), paused(false)
{
}

Sound::~Sound()
{
    Reset();
}

Sound::Sound(Sound &&other) noexcept : handle(other.handle), paused(other.paused)
{
    other.handle = nullptr;
    other.paused = false;
}

Sound &Sound::operator=(Sound &&other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    Reset();
    handle = other.handle;
    paused = other.paused;
    other.handle = nullptr;
    other.paused = false;
    return *this;
}

Sound Sound::LoadFromVfs(VFS &vfs, const char *vfs_path)
{
    Sound sound;
    sound.handle = LoadAudioHandle(vfs, vfs_path, MA_SOUND_FLAG_DECODE, "Sound::LoadFromVfs");
    sound.paused = false;
    return sound;
}

bool Sound::IsReady() const noexcept
{
    return handle != nullptr;
}

void Sound::Reset() noexcept
{
    if (handle)
    {
        ReleaseHandle(static_cast<AudioHandle *>(handle));
        handle = nullptr;
    }
    paused = false;
}

void Sound::Play()
{
    AudioHandle *impl = RequireHandle(handle, "Sound::Play", "Sound");
    StartPlayback(impl, &paused, "Sound::Play");
}

void Sound::Pause()
{
    AudioHandle *impl = RequireHandle(handle, "Sound::Pause", "Sound");
    PausePlayback(impl, &paused);
}

void Sound::Stop()
{
    AudioHandle *impl = RequireHandle(handle, "Sound::Stop", "Sound");
    StopPlayback(impl, &paused);
}

bool Sound::IsPlaying() const noexcept
{
    if (!handle)
    {
        return false;
    }
    return ma_sound_is_playing(&static_cast<AudioHandle *>(handle)->sound) == MA_TRUE;
}

void Sound::SetLooping(bool looping)
{
    AudioHandle *impl = RequireHandle(handle, "Sound::SetLooping", "Sound");
    ma_sound_set_looping(&impl->sound, looping ? MA_TRUE : MA_FALSE);
}

void Sound::SetVolume(float volume)
{
    AudioHandle *impl = RequireHandle(handle, "Sound::SetVolume", "Sound");
    ApplyVolume(impl, volume);
}

void Sound::SetPitch(float pitch)
{
    AudioHandle *impl = RequireHandle(handle, "Sound::SetPitch", "Sound");
    ApplyPitch(impl, pitch);
}

Music::Music() noexcept : handle(nullptr), paused(false)
{
}

Music::~Music()
{
    Reset();
}

Music::Music(Music &&other) noexcept : handle(other.handle), paused(other.paused)
{
    other.handle = nullptr;
    other.paused = false;
}

Music &Music::operator=(Music &&other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    Reset();
    handle = other.handle;
    paused = other.paused;
    other.handle = nullptr;
    other.paused = false;
    return *this;
}

Music Music::LoadFromVfs(VFS &vfs, const char *vfs_path)
{
    Music music;
    music.handle = LoadAudioHandle(vfs, vfs_path, MA_SOUND_FLAG_STREAM, "Music::LoadFromVfs");
    music.paused = false;
    return music;
}

bool Music::IsReady() const noexcept
{
    return handle != nullptr;
}

void Music::Reset() noexcept
{
    if (handle)
    {
        ReleaseHandle(static_cast<AudioHandle *>(handle));
        handle = nullptr;
    }
    paused = false;
}

void Music::Play()
{
    AudioHandle *impl = RequireHandle(handle, "Music::Play", "Music");
    StartPlayback(impl, &paused, "Music::Play");
}

void Music::Pause()
{
    AudioHandle *impl = RequireHandle(handle, "Music::Pause", "Music");
    PausePlayback(impl, &paused);
}

void Music::Stop()
{
    AudioHandle *impl = RequireHandle(handle, "Music::Stop", "Music");
    StopPlayback(impl, &paused);
}

bool Music::IsPlaying() const noexcept
{
    if (!handle)
    {
        return false;
    }
    return ma_sound_is_playing(&static_cast<AudioHandle *>(handle)->sound) == MA_TRUE;
}

void Music::SetLooping(bool looping)
{
    AudioHandle *impl = RequireHandle(handle, "Music::SetLooping", "Music");
    ma_sound_set_looping(&impl->sound, looping ? MA_TRUE : MA_FALSE);
}

void Music::SetVolume(float volume)
{
    AudioHandle *impl = RequireHandle(handle, "Music::SetVolume", "Music");
    ApplyVolume(impl, volume);
}

void Music::SetPitch(float pitch)
{
    AudioHandle *impl = RequireHandle(handle, "Music::SetPitch", "Music");
    ApplyPitch(impl, pitch);
}

} // namespace engine
