#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Core engine
#include "leo/engine.h"
#include "leo/error.h"
#include "leo/export.h"
#include "leo/signal.h"

// Graphics & rendering
#include "leo/graphics.h"
#include "leo/color.h"
#include "leo/image.h"
#include "leo/font.h"

// Input
#include "leo/keyboard.h"
#include "leo/mouse.h"
#include "leo/gamepad.h"

// Audio
#include "leo/audio.h"

// Gameplay helpers
#include "leo/collisions.h"

// I/O & VFS
#include "leo/io.h"

// Data utilities
#include "leo/base64.h"
#include "leo/json.h"
#include "leo/csv.h"

// Pack system
#include "leo/pack_compress.h"
#include "leo/pack_zlib.h"
#include "leo/pack_errors.h"
#include "leo/pack_format.h"
#include "leo/pack_obfuscate.h"
#include "leo/pack_reader.h"
#include "leo/pack_writer.h"
#include "leo/pack_util.h"

#ifdef __cplusplus
} // extern "C"
#endif
