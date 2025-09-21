// =============================================
// leo/audio.c — miniaudio-backed implementation
// =============================================
#include "leo/audio.h"
#include "leo/error.h"
#include "leo/io.h"

#include <stdlib.h>
#include <string.h>

// ---------------------------------------------
// Global engine (lazy init)
// ---------------------------------------------
static ma_engine g_engine;
static bool g_engineInited = false;

static bool _audio_init_if_needed(void)
{
    if (g_engineInited)
        return true;

    ma_result r = ma_engine_init(NULL, &g_engine);
    if (r != MA_SUCCESS)
    {
        // Keep message short; ma has no strerror, encode code.
        leo_SetError("miniaudio: ma_engine_init failed (%d)", (int)r);
        return false;
    }
    g_engineInited = true;
    return true;
}

bool leo_InitAudio(void)
{
    return _audio_init_if_needed();
}

void leo_ShutdownAudio(void)
{
    if (g_engineInited)
    {
        ma_engine_uninit(&g_engine);
        g_engineInited = false;
    }
}

// ---------------------------------------------
// Small helpers
// ---------------------------------------------
typedef struct _LeoSoundImpl
{
    ma_sound snd;   /* actual sound object */
    ma_decoder dec; /* only used when created from memory */
    int hasDecoder; /* 1 if 'dec' is valid */
    void *encoded;  /* encoded file bytes when loaded via VFS */
    size_t encodedSize;
} _LeoSoundImpl;

static inline leo_Sound _zero_sound(void)
{
    leo_Sound s;
    s._handle = NULL;
    s.channels = 0;
    s.sampleRate = 0;
    return s;
}

bool leo_IsSoundReady(leo_Sound sound)
{
    return sound._handle != NULL;
}

static _LeoSoundImpl *_impl(leo_Sound *s)
{
    return (_LeoSoundImpl *)s->_handle;
}

static const _LeoSoundImpl *_impl_c(const leo_Sound *s)
{
    return (const _LeoSoundImpl *)s->_handle;
}

static ma_sound *_as_ma(leo_Sound *s)
{
    return s && s->_handle ? &(_impl(s)->snd) : NULL;
}

static const ma_sound *_as_ma_c(const leo_Sound *s)
{
    return s && s->_handle ? &(_impl_c(s)->snd) : NULL;
}

static void _fill_advisory_fields(leo_Sound *out, const ma_sound *snd)
{
    if (!out || !snd)
        return;
    ma_uint32 ch = 0, sr = 0;
    ma_sound_get_data_format((ma_sound *)snd, NULL, &ch, &sr, NULL, 0);
    out->channels = (int)ch;
    out->sampleRate = (int)sr;
}

// ---------------------------------------------
// Load/Unload
// ---------------------------------------------
leo_Sound leo_LoadSound(const char *filePath)
{
    if (!filePath || !*filePath)
    {
        leo_SetError("leo_LoadSound: invalid file path");
        return _zero_sound();
    }
    if (!_audio_init_if_needed())
    {
        return _zero_sound();
    }

    /* Wrapper holds snd + optional decoder + owned encoded bytes. */
    _LeoSoundImpl *impl = (_LeoSoundImpl *)calloc(1, sizeof(*impl));
    if (!impl)
    {
        leo_SetError("leo_LoadSound: OOM");
        return _zero_sound();
    }

    /* --------- Try VFS first (logical path) ---------- */
    size_t encSize = 0;
    void *encData = leo_LoadAsset(filePath, &encSize);
    if (encData && encSize > 0)
    {
        ma_result r;
        /* Build a decoder that reads from the encoded memory. */
        r = ma_decoder_init_memory(encData, encSize, NULL, &impl->dec);
        if (r == MA_SUCCESS)
        {
            /* Build a sound using the decoder as the data source. */
            r = ma_sound_init_from_data_source(&g_engine, (ma_data_source *)&impl->dec, 0 /* flags */, NULL /* group */,
                                               &impl->snd);
            if (r == MA_SUCCESS)
            {
                impl->hasDecoder = 1;
                impl->encoded = encData;
                impl->encodedSize = encSize;
                /* success path falls through */
            }
        }
        if (impl->hasDecoder == 0)
        {
            /* VFS path failed: free the encoded buffer and fall back to file. */
            free(encData);
        }
    }

    /* --------- Fallback: direct filesystem path ---------- */
    if (impl->hasDecoder == 0)
    {
        ma_result r = ma_sound_init_from_file(&g_engine, filePath, MA_SOUND_FLAG_STREAM, NULL, NULL, &impl->snd);
        if (r != MA_SUCCESS)
        {
            free(impl);
            leo_SetError("miniaudio: ma_sound_init_from_file failed (%d)", (int)r);
            return _zero_sound();
        }
    }

    leo_Sound s = _zero_sound();
    s._handle = (void *)impl;
    _fill_advisory_fields(&s, &impl->snd);
    return s;
}

void leo_UnloadSound(leo_Sound *sound)
{
    if (!sound || !sound->_handle)
        return;
    _LeoSoundImpl *impl = _impl(sound);
    if (impl)
    {
        ma_sound_uninit(&impl->snd);
        if (impl->hasDecoder)
        {
            ma_decoder_uninit(&impl->dec);
        }
        if (impl->encoded)
        {
            free(impl->encoded);
        }
        free(impl);
    }
    sound->_handle = NULL;
    sound->channels = 0;
    sound->sampleRate = 0;
}

// ---------------------------------------------
// Playback controls
// ---------------------------------------------
bool leo_PlaySound(leo_Sound *sound, float volume, bool loop)
{
    if (!sound || !sound->_handle)
    {
        leo_SetError("leo_PlaySound: invalid sound");
        return false;
    }
    if (!_audio_init_if_needed())
        return false;

    ma_sound *snd = _as_ma(sound);
    ma_result r;

    ma_sound_stop(snd); // restart from beginning for Raylib-like ergonomics
    ma_sound_set_looping(snd, loop ? MA_TRUE : MA_FALSE);

    // clamp volume 0..1
    if (volume < 0.0f)
        volume = 0.0f;
    if (volume > 1.0f)
        volume = 1.0f;
    ma_sound_set_volume(snd, volume);

    r = ma_sound_start(snd);
    if (r != MA_SUCCESS)
    {
        leo_SetError("miniaudio: ma_sound_start failed (%d)", (int)r);
        return false;
    }
    return true;
}

void leo_StopSound(leo_Sound *sound)
{
    if (!sound || !sound->_handle)
        return;
    ma_sound_stop(_as_ma(sound));
    ma_sound_seek_to_pcm_frame(_as_ma(sound), 0); // rewind for SFX feel
}

void leo_PauseSound(leo_Sound *sound)
{
    if (!sound || !sound->_handle)
        return;
    ma_sound_stop(_as_ma(sound));
}

void leo_ResumeSound(leo_Sound *sound)
{
    if (!sound || !sound->_handle)
        return;
    (void)ma_sound_start(_as_ma(sound));
}

bool leo_IsSoundPlaying(const leo_Sound *sound)
{
    if (!sound || !sound->_handle)
        return false;
    return ma_sound_is_playing(_as_ma_c(sound)) == MA_TRUE;
}

// ---------------------------------------------
// Tuning
// ---------------------------------------------
void leo_SetSoundVolume(leo_Sound *sound, float volume)
{
    if (!sound || !sound->_handle)
        return;
    if (volume < 0.0f)
        volume = 0.0f;
    if (volume > 1.0f)
        volume = 1.0f;
    ma_sound_set_volume(_as_ma(sound), volume);
}

void leo_SetSoundPitch(leo_Sound *sound, float pitch)
{
    if (!sound || !sound->_handle)
        return;
    if (pitch < 0.01f)
        pitch = 0.01f;
    ma_sound_set_pitch(_as_ma(sound), pitch);
}

void leo_SetSoundPan(leo_Sound *sound, float pan)
{
    if (!sound || !sound->_handle)
        return;
    // Clamp to -1..+1 for ergonomics.
    if (pan < -1.0f)
        pan = -1.0f;
    if (pan > +1.0f)
        pan = +1.0f;
    ma_sound_set_pan(_as_ma(sound), pan);
}
