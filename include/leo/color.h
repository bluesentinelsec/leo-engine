#pragma once

#include <SDL3/SDL.h>

typedef struct
{
	Uint8 r; // Red channel
	Uint8 g; // Green channel
	Uint8 b; // Blue channel
	Uint8 a; // Alpha channel (opacity: 0 = transparent, 255 = opaque)
} leo_Color;
