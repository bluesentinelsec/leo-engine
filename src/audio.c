// =============================================
// leo/audio.c — miniaudio-backed implementation
// =============================================
#include "leo/audio.h"
#include "leo/error.h"

#include <stdlib.h>
#include <string.h>

// ---------------------------------------------
// Global engine (lazy init)
// ---------------------------------------------
static ma_engine g_engine;
static bool g_engineInited = false;

static bool _audio_init_if_needed(void)
{
	if (g_engineInited) return true;

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

static ma_sound* _as_ma(leo_Sound* s)
{
	return (ma_sound*)s->_handle;
}

static const ma_sound* _as_ma_c(const leo_Sound* s)
{
	return (const ma_sound*)s->_handle;
}

static void _fill_advisory_fields(leo_Sound* out, const ma_sound* snd)
{
	if (!out || !snd) return;
	ma_uint32 ch = 0, sr = 0;
	ma_sound_get_data_format((ma_sound*)snd, NULL, &ch, &sr, NULL, 0);
	out->channels = (int)ch;
	out->sampleRate = (int)sr;
}

// ---------------------------------------------
// Load/Unload
// ---------------------------------------------
leo_Sound leo_LoadSound(const char* filePath)
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

	// Allocate the ma_sound on heap so our API stays value-returning.
	ma_sound* snd = (ma_sound*)malloc(sizeof(ma_sound));
	if (!snd)
	{
		leo_SetError("leo_LoadSound: OOM");
		return _zero_sound();
	}

	// For SFX we avoid streaming so we get Raylib-style low-latency reuse.
	// (miniaudio can still stream large assets; later we'll add leo_LoadMusic).
	ma_result r = ma_sound_init_from_file(&g_engine,
		filePath,
		0 /*flags: non-streamed*/,
		NULL, NULL,
		snd);
	if (r != MA_SUCCESS)
	{
		free(snd);
		leo_SetError("miniaudio: ma_sound_init_from_file failed (%d)", (int)r);
		return _zero_sound();
	}

	leo_Sound s = _zero_sound();
	s._handle = (void*)snd;
	_fill_advisory_fields(&s, snd);
	return s;
}

void leo_UnloadSound(leo_Sound* sound)
{
	if (!sound || !sound->_handle) return;
	ma_sound* snd = _as_ma(sound);
	ma_sound_uninit(snd);
	free(snd);
	sound->_handle = NULL;
	sound->channels = 0;
	sound->sampleRate = 0;
}

// ---------------------------------------------
// Playback controls
// ---------------------------------------------
bool leo_PlaySound(leo_Sound* sound, float volume, bool loop)
{
	if (!sound || !sound->_handle)
	{
		leo_SetError("leo_PlaySound: invalid sound");
		return false;
	}
	if (!_audio_init_if_needed()) return false;

	ma_sound* snd = _as_ma(sound);
	ma_result r;

	ma_sound_stop(snd); // restart from beginning for Raylib-like ergonomics
	ma_sound_set_looping(snd, loop ? MA_TRUE : MA_FALSE);

	// clamp volume 0..1
	if (volume < 0.0f) volume = 0.0f;
	if (volume > 1.0f) volume = 1.0f;
	ma_sound_set_volume(snd, volume);

	r = ma_sound_start(snd);
	if (r != MA_SUCCESS)
	{
		leo_SetError("miniaudio: ma_sound_start failed (%d)", (int)r);
		return false;
	}
	return true;
}

void leo_StopSound(leo_Sound* sound)
{
	if (!sound || !sound->_handle) return;
	ma_sound_stop(_as_ma(sound));
	ma_sound_seek_to_pcm_frame(_as_ma(sound), 0); // rewind for SFX feel
}

void leo_PauseSound(leo_Sound* sound)
{
	if (!sound || !sound->_handle) return;
	ma_sound_stop(_as_ma(sound));
}

void leo_ResumeSound(leo_Sound* sound)
{
	if (!sound || !sound->_handle) return;
	(void)ma_sound_start(_as_ma(sound));
}

bool leo_IsSoundPlaying(const leo_Sound* sound)
{
	if (!sound || !sound->_handle) return false;
	return ma_sound_is_playing(_as_ma_c(sound)) == MA_TRUE;
}

// ---------------------------------------------
// Tuning
// ---------------------------------------------
void leo_SetSoundVolume(leo_Sound* sound, float volume)
{
	if (!sound || !sound->_handle) return;
	if (volume < 0.0f) volume = 0.0f;
	if (volume > 1.0f) volume = 1.0f;
	ma_sound_set_volume(_as_ma(sound), volume);
}

void leo_SetSoundPitch(leo_Sound* sound, float pitch)
{
	if (!sound || !sound->_handle) return;
	if (pitch < 0.01f) pitch = 0.01f;
	ma_sound_set_pitch(_as_ma(sound), pitch);
}

void leo_SetSoundPan(leo_Sound* sound, float pan)
{
	if (!sound || !sound->_handle) return;
	// Clamp to -1..+1 for ergonomics.
	if (pan < -1.0f) pan = -1.0f;
	if (pan > +1.0f) pan = +1.0f;
	ma_sound_set_pan(_as_ma(sound), pan);
}
