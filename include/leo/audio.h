// =============================================
// leo/audio.h — Simple audio (Raylib-style)
// Small sounds (SFX) are fully managed; music/streaming can be added next.
// =============================================
#pragma once

#include "leo/export.h"
#include <stdbool.h>

#include <miniaudio/miniaudio.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // ---------------------------------------------
    // Overview
    // ---------------------------------------------
    // - Small "Sound" assets (SFX-style). Future: "Music" (streamed).
    // - Engine (miniaudio) is initialized on first audio call; explicit
    //   init/shutdown are also provided if you want tighter control.
    // - On failure, loaders return a zero-initialized handle.
    // - Use leo_GetError() for details (from leo/error.h).
    // ---------------------------------------------

    // Forward-declared in C API: simple value-type handle.
    // Internals are opaque to users (similar to textures).
    typedef struct leo_Sound
    {
        void *_handle;  // internal: ma_sound*
        int channels;   // 1=mono, 2=stereo (advisory)
        int sampleRate; // advisory
                        // length in seconds is discoverable via query helpers later if needed
    } leo_Sound;

    // -------------------------------
    // Engine lifetime (optional)
    // -------------------------------
    // Audio auto-initializes on first call. These functions are optional.
    LEO_API bool leo_InitAudio(void);

    LEO_API void leo_ShutdownAudio(void);

    // -------------------------------
    // Sound (SFX-style; non-streamed)
    // -------------------------------
    LEO_API leo_Sound leo_LoadSound(const char *filePath);

    LEO_API void leo_UnloadSound(leo_Sound *sound);

    // Quick validity check
    LEO_API bool leo_IsSoundReady(leo_Sound sound);

    // Playback controls
    LEO_API bool leo_PlaySound(leo_Sound *sound, float volume, bool loop);

    LEO_API void leo_StopSound(leo_Sound *sound);

    LEO_API void leo_PauseSound(leo_Sound *sound);

    LEO_API void leo_ResumeSound(leo_Sound *sound);

    LEO_API bool leo_IsSoundPlaying(const leo_Sound *sound);

    // Volume/pitch/pan (Raylib-style ergonomics)
    LEO_API void leo_SetSoundVolume(leo_Sound *sound, float volume); // 0..1
    LEO_API void leo_SetSoundPitch(leo_Sound *sound, float pitch);   // 1.0 = normal
    LEO_API void leo_SetSoundPan(leo_Sound *sound, float pan);       // -1..+1

#ifdef __cplusplus
} // extern "C"
#endif
